#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
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
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>

#include "bfc.hpp"
#include "cli.hpp"
#include "irc.hpp"
#include "jit.hpp"
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
    std::cout << "failed to open brainf**k file: " << sourceFileName
              << std::endl;
    return 0;
  }

  auto option = parseCLIoption(argc, argv);

  llvm::LLVMContext context{};
  BrainFxxkCompiler bfc{context};
  auto mainModule = bfc.compile(file.value(), {option.memsize()});

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

  if (option.emitIR()) {
    auto sourcePath = std::filesystem::path{sourceFileName};
    auto irFilePath = sourcePath.replace_extension("ll").string();

    std::error_code errorCode;
    llvm::raw_fd_ostream outfile(irFilePath, errorCode, llvm::sys::fs::OF_Text);
    mainModule->get()->print(outfile, nullptr);

    if (errorCode) {
      std::cout << "failed to write LLVM IR to file " << irFilePath
                << "code: " << errorCode.message() << std::endl;
      return 0;
    }
  }

  if (option.JITRun()) {
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
    auto outPath = sourcePath.replace_extension("exe").string();
    auto bfObjPath = sourcePath.replace_extension("obj").string();

    auto compile2objResult = compileIR2obj(
        bfObjPath, std::move(mainModule.value()), option.target());

    if (!compile2objResult) {
      std::cout << "failed to compile object file: "
                << compile2objResult.error() << std::endl;
      return 0;
    }

    auto stdbf = findBrainfxxkStdLib(argv[0], option.target());
    if (!stdbf) {
      std::cout << "failed to find standart brainfxxk library: "
                << stdbf.error() << std::endl;
      return 0;
    }
    std::cout << stdbf->string() << std::endl;
    auto linkExecResult =
        linkExecutable(option.target(), stdbf->string(), bfObjPath, outPath);
    if (!linkExecResult) {
      std::cout << "failed to link executable by using lld: "
                << linkExecResult.error() << std::endl;
      return 0;
    }
  }

  return 0;
}
