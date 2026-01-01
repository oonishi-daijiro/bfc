#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>

#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#include "bfc.hpp"
#include "irc.hpp"
#include "utils.hpp"

int main(int argc, char **argv) {
  llvm::InitLLVM init{argc, argv};
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  if (argc < 2) {
    std::cout << "please input filename as commandline argument" << std::endl;
    return 0;
  }

  std::string sourceFileName{argv[1]};
  auto file = readfile(sourceFileName);
  if (!file) {
    std::println("invalid path of source file:{}", sourceFileName);
    return 0;
  }

  auto isEmitIR = false;
  auto isJitRun = false;
  std::string target = llvm::sys::getDefaultTargetTriple();
  auto memsize = BfCompilerOption::defaultMemSize;
  std::string stdLibDir = "./";

  if (argc > 2) {
    for (int i = 2; i < argc; i++) {
      const std::string op{argv[i]};
      if (op == "-emit-llvm") {
        isEmitIR = true;
      } else if (op == "-jit") {
        isJitRun = true;
      } else if (op.starts_with("-memsize=")) {
        std::string memsizeStr = op.substr(std::size("-memsize=") - 1);
        try {
          memsize = std::stoull(memsizeStr);
        } catch (std::exception) {
          std::println("please invalid value to -memsize option");
          memsize = BfCompilerOption::defaultMemSize;
        }
      } else if (op.starts_with("-target=")) {
        target = op.substr(std::size("-target=") - 1);
      } else if (op.starts_with("-stdbf=")) {

      } else {
        std::print("unknown option: \"{}\". ", op);
        std::println("this unknown option is ignored");
      }
    }
  }

  llvm::LLVMContext context{};
  BrainFxxkCompiler bfc{context};
  auto mainModule = bfc.compile(file.value(), {memsize});

  if (!mainModule) {
    printCompileError(file.value(), mainModule.error());
    return 0;
  }

  auto verifyResult = verifyLLVMModule(*mainModule->get());

  if (!verifyResult) {
    std::cout << "failed to compile brainfuck to LLVM IR" << std::endl;
    std::cout << verifyResult.error() << std::endl;
    return 0;
  }

  if (isEmitIR) {
    auto sourcePath = std::filesystem::path{sourceFileName};

    auto irOutDir = sourcePath.parent_path();
    auto irname = sourcePath.stem();
    auto irFilePath =
        std::format("{}./{}.ll", irOutDir.string(), irname.string());
    std::error_code errorCode;
    llvm::raw_fd_ostream outfile(irFilePath, errorCode, llvm::sys::fs::OF_Text);
    mainModule->get()->print(outfile, nullptr);
    if (errorCode) {
      std::println("failed to write LLVM IR to file \"{}\" code: {}",
                   irFilePath, errorCode.message());
      return 0;
    }
  }

  if (isJitRun) {
#ifdef ASAN_ENABLED
    std::cout << "cannot JIT compilation on this address sanitizer enabled "
                 "executable"
              << std::endl;
    return 0;
#else
    auto result = JITRun(std::move(mainModule.value()));
    if (!result) {
      std::cout << "failed to JIT compile: " << result.error() << std::endl;
      return 0;
    }
#endif

  } else {
    auto sourcePath = std::filesystem::path{sourceFileName};
    auto outPath = std::format("{}./{}.exe", sourcePath.parent_path().string(),
                               sourcePath.stem().string());
    auto bfObjPath =
        std::format("{}./{}.obj", sourcePath.parent_path().string(),
                    sourcePath.stem().string());
    auto compile2objResult =
        compileIR2obj(bfObjPath, std::move(mainModule.value()), target);
    if (!compile2objResult) {
      std::cout << "failed to compile object file: "
                << compile2objResult.error() << std::endl;
      return 0;
    }
    auto linkExecResult = linkExecutable(target, bfObjPath, outPath);
    if (!linkExecResult) {
      std::cout << "failed to link executable by using lld: "
                << linkExecResult.error() << std::endl;
      return 0;
    }
  }

  return 0;
}
