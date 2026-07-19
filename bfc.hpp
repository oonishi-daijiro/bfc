#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <vector>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>


#include "bfir.hpp"

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

using module_ptr = std::unique_ptr<llvm::Module>;
using builder_ptr = std::shared_ptr<llvm::IRBuilder<>>;

struct Runtime {

public:
  struct Function {
    llvm::Function *free;
    llvm::Function *calloc;
    llvm::Function *putchar;
    llvm::Function *getchar;
    llvm::Function *entry;
  };

  struct Type {

    llvm::FunctionType *free;
    llvm::FunctionType *calloc;
    llvm::FunctionType *putchar;
    llvm::FunctionType *getchar;
    llvm::FunctionType *entry;

    llvm::Type *i32;
    llvm::Type *i8;
    llvm::Type *i64;
    llvm::Type *vo1d;
    llvm::Type *ptr;
    llvm::Type *mem;
  };

  struct Constant {
    llvm::Constant *i8zero;
    llvm::Constant *i8one;
    llvm::Constant *i64zero;
    llvm::Constant *i64one;
    llvm::Constant *i64memsize;
  };

  struct Memory {
    llvm::Value *ptr;
    llvm::Value *index;
  };

  Function functions;
  Type types;
  Constant constants;
  Memory memory;

  Runtime(llvm::LLVMContext &ctx, llvm::Module &module, builder_ptr builder,
          const BfCompilerOption &option);
};

class BrainFxxkCompiler {

  llvm::LLVMContext &context;
  module_ptr mod;
  builder_ptr builder;
  BfCompilerOption option;
  Runtime rt;

  void compile(BFIR *ir);
  void compile(MovePtr *ir);
  void compile(TransformPointee *ir);
  void compile(In *ir);
  void compile(Out *ir);
  void compile(Loop *ir);

public:
  BrainFxxkCompiler(llvm::LLVMContext &ctx, BfCompilerOption option)
      : context{ctx}, mod{std::make_unique<llvm::Module>("main", context)},
        builder{std::make_shared<llvm::IRBuilder<>>(context)}, option{option},
        rt{context, *mod, builder, option} {}

  std::expected<BFIRHandle, CompileError> compile(const std::vector<char> &);
  std::expected<module_ptr, CompileError> compile(BFIRHandle &&ir);

  // std::expected<module_ptr, CompileError>
  // compile(const std::vector<char> &source, BfCompilerOption option = {});
};
