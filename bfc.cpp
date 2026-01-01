#include <stack>

#include "bfc.hpp"

std::expected<BrainFxxkCompiler::module_ptr, CompileError>
BrainFxxkCompiler::compile(const std::vector<char> &source,
                           BfCompilerOption option) {
  auto i32ty = builder->getInt32Ty();
  auto i8ty = builder->getInt8Ty();
  auto i64ty = builder->getInt64Ty();
  auto voidty = builder->getVoidTy();
  auto ptrTy = llvm::PointerType::get(context, 0);

  auto putcharFnTy = llvm::FunctionType::get(voidty, {i8ty}, false);
  auto getcharFnTy = llvm::FunctionType::get(i8ty, {}, false);
  auto callocFnTy = llvm::FunctionType::get(ptrTy, {i64ty}, false);
  auto freeFnTy = llvm::FunctionType::get(voidty, {ptrTy}, false);

  auto entryFnTy = llvm::FunctionType::get(i32ty, false);
  auto arrayTy = llvm::ArrayType::get(i8ty, option.memsize);

  auto i8zero = builder->getInt8(0);
  auto i8one = builder->getInt8(1);

  auto i64zero = builder->getInt64(0);
  auto i64one = builder->getInt64(1);
  auto i64memsize = builder->getInt64(option.memsize);

  auto putcharFn = llvm::Function::Create(
      putcharFnTy, llvm::Function::ExternalLinkage, "bfputchar", mod.get());

  auto getcharFn = llvm::Function::Create(
      getcharFnTy, llvm::Function::ExternalLinkage, "bfgetchar", mod.get());

  auto callocFn = llvm::Function::Create(
      callocFnTy, llvm::Function::ExternalLinkage, "bfcalloc", mod.get());

  auto freeFn = llvm::Function::Create(
      freeFnTy, llvm::Function::ExternalLinkage, "bffree", mod.get());

  auto entryFn = llvm::Function::Create(
      entryFnTy, llvm::Function::ExternalLinkage, "entry", mod.get());

  auto entryBB = llvm::BasicBlock::Create(context, "", entryFn);

  builder->SetInsertPoint(entryBB);

  auto memory = builder->CreateCall(callocFn, {i64memsize});
  auto i64index = builder->CreateAlloca(i64ty, nullptr, "index");

  builder->CreateStore(i64zero, i64index);

  std::stack<std::tuple<llvm::BasicBlock *, llvm::BasicBlock *>> bbStack{};
  size_t index = 0;
  for (const auto c : source) {
    index++;
    switch (c) {
    default: {
      continue;
    }
    case '>': {
      auto ptrVal = builder->CreateLoad(i64ty, i64index);
      builder->CreateStore(builder->CreateAdd(ptrVal, i64one), i64index);
      continue;
    }
    case '<': {
      auto ptrVal = builder->CreateLoad(i64ty, i64index);
      builder->CreateStore(builder->CreateSub(ptrVal, i64one), i64index);
      continue;
    }

    case '+': {
      auto index = builder->CreateLoad(i64ty, i64index);
      auto pointingElmPtr =
          builder->CreateGEP(arrayTy, memory, {i64zero, index});
      auto pointee = builder->CreateLoad(i8ty, pointingElmPtr);
      builder->CreateStore(builder->CreateAdd(pointee, i8one), pointingElmPtr);
      continue;
    }

    case '-': {
      auto index = builder->CreateLoad(i64ty, i64index);
      auto pointingElmPtr =
          builder->CreateGEP(arrayTy, memory, {i64zero, index});
      auto pointee = builder->CreateLoad(i8ty, pointingElmPtr);
      builder->CreateStore(builder->CreateSub(pointee, i8one), pointingElmPtr);
      continue;
    }

    case '.': {
      auto index = builder->CreateLoad(i64ty, i64index);
      auto cur = builder->CreateGEP(arrayTy, memory, {i64zero, index});
      auto putVar = builder->CreateLoad(i8ty, cur);
      builder->CreateCall(putcharFn, {putVar});
      continue;
    }

    case ',': {
      auto index = builder->CreateLoad(i64ty, i64index);
      auto cur = builder->CreateGEP(arrayTy, memory, {i64zero, index});
      auto ret = builder->CreateCall(getcharFn, {});
      builder->CreateStore(ret, cur);
      continue;
    }

    case '[': {
      auto bb = llvm::BasicBlock::Create(context, "", entryFn);
      auto merge = llvm::BasicBlock::Create(context, "");

      auto index = builder->CreateLoad(i64ty, i64index);
      auto pointee = builder->CreateLoad(
          i8ty, builder->CreateGEP(arrayTy, memory, {i64zero, index}));
      auto cond = builder->CreateICmpEQ(pointee, i8zero);

      bbStack.push({bb, merge});
      builder->CreateCondBr(cond, merge, bb);
      builder->SetInsertPoint(bb);
      continue;
    }

    case ']': {
      if (bbStack.empty()) {
        return std::unexpected{
            CompileError{index, "no matching \"[\" for this bracket"}};
      }

      auto [body, merge] = bbStack.top();
      bbStack.pop();
      auto index = builder->CreateLoad(i32ty, i64index);
      auto pointee = builder->CreateLoad(
          i8ty, builder->CreateGEP(arrayTy, memory, {i64zero, index}));
      auto cond = builder->CreateICmpEQ(pointee, i8zero);
      auto parentFunc = builder->GetInsertBlock()->getParent();

      builder->CreateCondBr(cond, merge, body);
      merge->insertInto(parentFunc);
      builder->SetInsertPoint(merge);
      continue;
    } // case ']'
    } // switch
  } // end of for

  if (!bbStack.empty()) {
    return std::unexpected(CompileError{0, "no enough \"]\" for \"]\""});
  }

  builder->CreateCall(freeFn, {memory});
  builder->CreateRet(builder->getInt32(0));
  return {std::move(mod)};
}
