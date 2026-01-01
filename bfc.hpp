#pragma once

#include <cstddef>
#include <expected>
#include <llvm/IR/Value.h>
#include <memory>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

struct CompileError {
private:
  size_t index;
  std::string reasonStr;

public:
  CompileError(size_t index, const std::string &reason)
      : index{index}, reasonStr(reason) {}
  const size_t getIndex() { return index; }
  const std::string &reason() { return reasonStr; }
};

struct BfCompilerOption {
  static constexpr inline size_t defaultMemSize = 30000;
  size_t memsize = defaultMemSize;
};

class BrainFxxkCompiler {
  using module_ptr = std::unique_ptr<llvm::Module>;
  using builder_ptr = std::unique_ptr<llvm::IRBuilder<>>;

  llvm::LLVMContext &context;
  module_ptr mod;
  builder_ptr builder;

public:
  BrainFxxkCompiler(llvm::LLVMContext &ctx) : context{ctx} {
    mod = std::make_unique<llvm::Module>("main", context);
    builder = std::make_unique<llvm::IRBuilder<>>(context);
  }

  std::expected<module_ptr, CompileError>
  compile(const std::vector<char> &source, BfCompilerOption option = {});
};
