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

std::expected<void, std::string>
compileIR2obj(const std::string &outPath,
              std::unique_ptr<llvm::Module> mainModule, bool verbose);

LLD_HAS_DRIVER(mingw)

std::expected<void, std::string>
linkExecutable(const std::string &stdlibPath, const std::string &objFilePath,
               const std::string &executableName, bool verbose);
