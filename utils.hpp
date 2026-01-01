#pragma once

#include <expected>
#include <llvm/IR/Verifier.h>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CoreContainers.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h>
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

#include "bfc.hpp"

std::optional<std::vector<char>> readfile(std::string_view filepath);
std::string lltos(auto *inst);
void printCompileError(const std::vector<char> &source, CompileError &error);
std::expected<void, std::string> verifyLLVMModule(llvm::Module &m);
std::expected<int, std::string> JITRun(std::unique_ptr<llvm::Module> llModule);
