#pragma once

#include <expected>
#include <filesystem>
#include <iostream>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>

#include <lld/Common/Driver.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>

inline std::expected<void, std::string>
compileIR2obj(const std::string &outPath,
              std::unique_ptr<llvm::Module> mainModule,
              const std::string &targetTiple) {

  llvm::Triple triple{targetTiple};
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

LLD_HAS_DRIVER(coff)
// LLD_HAS_DRIVER(mingw)

inline std::expected<void, std::string>
linkExecutable(const std::string &targetTriple, const std::string &stdlibPath,
               const std::string &objFilePath,
               const std::string &executableName) {

  if (targetTriple == "x86_64-pc-windows-msvc") {
    /*
      link *compiled-bf-object-file *bf-stdlib /OUT:*outputname
      /SUBSYSTEM:CONSOLE \ /ENTRY:start kernel32.lib
    */

    auto execName = std::format("/OUT:{}", executableName);
    auto args = {
        "lld-link",       objFilePath.c_str(),  stdlibPath.c_str(),
        execName.c_str(), "/SUBSYSTEM:CONSOLE", "/ENTRY:start",
        "kernel32.lib",
    };

    std::string lldOutsStr{}, lldErrStr{};
    llvm::raw_string_ostream lldOut{lldOutsStr}, lldErr{lldErrStr};

    auto result =
        lld::lldMain(args, lldOut, lldErr, {{lld::WinLink, &lld::coff::link}});

    lldOut.flush();
    lldErr.flush();

    if (result.retCode != 0) {
      return std::unexpected{lldErrStr};
    }
    return {};
  } else if (targetTriple == "x86_64-pc-windows-gnu") {

  } else {
    return std::unexpected{
        "this compiler is supports only target x86_64-pc-windows-msvc"};
  }
}
