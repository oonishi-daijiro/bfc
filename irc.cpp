#include "irc.hpp"
#include "utils.hpp"
#include <expected>
#include <iostream>
#include <lld/Common/Driver.h>
#include <string>
#include <type_traits>

std::expected<void, std::string>
compileIR2obj(const std::string &outPath,
              std::unique_ptr<llvm::Module> mainModule, bool verbose) {
  if (verbose) {
    std::cout << VERBOSE_LOG_PREFIX
              << "compiling LLVM IR to target:\"x86_64-w64-windows-gnu\""
              << std::endl;
  }

  llvm::Triple triple{"x86_64-w64-windows-gnu"};
  std::string lookupTargetError;

  auto target = llvm::TargetRegistry::lookupTarget(triple, lookupTargetError);
  if (!target) {
    return std::unexpected{lookupTargetError};
  }

  mainModule->setTargetTriple(triple);

  llvm::TargetOptions opt;
  auto CPU = "generic";
  auto feature = "";
  auto targetMachine = target->createTargetMachine(triple, CPU, feature, opt,
                                                   llvm::Reloc::Model::PIC_);
  mainModule->setDataLayout(targetMachine->createDataLayout());

  std::error_code streamErrorCode;
  llvm::raw_fd_ostream dest(outPath, streamErrorCode, llvm::sys::fs::OF_None);
  if (verbose) {
    std::cout << VERBOSE_LOG_PREFIX << "output object file into: " << outPath
              << std::endl;
  }

  if (streamErrorCode) {
    return std::unexpected{streamErrorCode.message()};
  }

  llvm::legacy::PassManager pm{};
  auto addPassError = targetMachine->addPassesToEmitFile(
      pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile);
  if (addPassError) {
    return std::unexpected{"failed to add passess to emit file"};
  }

  pm.run(*mainModule);
  dest.flush();
  return {};
}

std::expected<void, std::string>
linkExecutable(const std::string &runtimePath, const std::string &objFilePath,
               const std::string &executableName, bool verbose = false) {
  /*
    ld.lld path-to-bf-obj path-to-runtime -o path-to-executable -e start
  */

  auto execName = std::format("-o{}", executableName);
  const char *args[] = {"ld.lld", objFilePath.c_str(), runtimePath.c_str(),
                        execName.c_str(), "-estart"};

  if (verbose) {
    std::cout << VERBOSE_LOG_PREFIX << "running linker ld.lld:" << std::endl
              << '\t';
    for (auto arg : args) {
      std::cout << arg << ' ';
    }
    std::cout << std::endl;
  }

  lld::DriverDef driver[] = {{lld::MinGW, &lld::mingw::link}};

  auto result = runLLD(args, driver);
  if (verbose && result) {
    std::cout << VERBOSE_LOG_PREFIX << "linker result:" << *result << std::endl;
  }

  if (!result) {
    return std::unexpected{result.error()};
  }
  return {};
}
