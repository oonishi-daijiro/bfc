#include <cstddef>
#include <cstdio>
#include <format>
#include <llvm/IR/Module.h>
#include <stack>
#include <vector>

#include "bfc.hpp"

Runtime::Runtime(llvm::LLVMContext &ctx, llvm::Module &module,
                 builder_ptr builder, const BfCompilerOption &option)
    : functions{}, types{}, constants{} {
  types.i32 = builder->getInt32Ty();
  types.i8 = builder->getInt8Ty();
  types.i64 = builder->getInt64Ty();
  types.vo1d = builder->getVoidTy();
  types.ptr = llvm::PointerType::get(ctx, 0);
  types.mem = llvm::ArrayType::get(types.i8, option.memsize);

  types.putchar = llvm::FunctionType::get(types.vo1d, {types.i8}, false);
  types.getchar = llvm::FunctionType::get(types.i8, {}, false);
  types.calloc = llvm::FunctionType::get(types.ptr, {types.i64}, false);
  types.free = llvm::FunctionType::get(types.vo1d, {types.ptr}, false);

  auto entryFnTy = llvm::FunctionType::get(types.i32, false);

  constants.i8zero = builder->getInt8(0);
  constants.i8one = builder->getInt8(1);
  constants.i64zero = builder->getInt64(0);
  constants.i64one = builder->getInt64(1);
  constants.i64memsize = builder->getInt64(option.memsize);

  functions.putchar = llvm::Function::Create(
      types.putchar, llvm::Function::ExternalLinkage, "bfputchar", &module);

  functions.getchar = llvm::Function::Create(
      types.getchar, llvm::Function::ExternalLinkage, "bfgetchar", &module);
  functions.calloc = llvm::Function::Create(
      types.calloc, llvm::Function::ExternalLinkage, "bfcalloc", &module);

  functions.free = llvm::Function::Create(
      types.free, llvm::Function::ExternalLinkage, "bffree", &module);

  functions.entry = llvm::Function::Create(
      entryFnTy, llvm::Function::ExternalLinkage, "entry", &module);
};

std::expected<module_ptr, CompileError>
BrainFxxkCompiler::compile(BFIRHandle &&ir) {
  auto entryBB = llvm::BasicBlock::Create(context, "", rt.functions.entry);
  builder->SetInsertPoint(entryBB);

  rt.memory.ptr =
      builder->CreateCall(rt.functions.calloc, {rt.constants.i64memsize});
  rt.memory.index = builder->CreateAlloca(rt.types.i64, nullptr, "index");

  for (auto i : ir) {
    compile(i);
  }

  builder->CreateCall(rt.functions.free, {rt.memory.ptr});
  builder->CreateRet(builder->getInt32(0));
  return {std::move(mod)};
};

void BrainFxxkCompiler::compile(BFIR *ir) {
  if (auto mp = ir->isa<MovePtr>()) {
    compile(mp.value());
  } else if (auto tp = ir->isa<TransformPointee>()) {
    compile(tp.value());
  } else if (auto in = ir->isa<In>()) {
    compile(in.value());
  } else if (auto out = ir->isa<Out>()) {
    compile(out.value());
  } else if (auto lp = ir->isa<Loop>()) {
    compile(lp.value());
  }
}

void BrainFxxkCompiler::compile(MovePtr *ir) {
  auto ptrVal = builder->CreateLoad(rt.types.i64, rt.memory.index);
  auto moveAmount = builder->getInt64(ir->amount());
  builder->CreateStore(builder->CreateAdd(ptrVal, moveAmount), rt.memory.index);
}

void BrainFxxkCompiler::compile(TransformPointee *ir) {
  auto index = builder->CreateLoad(rt.types.i64, rt.memory.ptr);
  auto pointingElmPtr = builder->CreateGEP(rt.types.mem, rt.memory.ptr,
                                           {rt.constants.i64zero, index});
  auto pointee = builder->CreateLoad(rt.types.i8, pointingElmPtr);
  auto val = builder->getInt8(ir->val());
  builder->CreateStore(builder->CreateAdd(pointee, val), pointingElmPtr);
}

void BrainFxxkCompiler::compile(In *ir) {
  auto index = builder->CreateLoad(rt.types.i64, rt.memory.index);
  auto cur = builder->CreateGEP(rt.types.mem, rt.memory.ptr,
                                {rt.constants.i64zero, index});
  auto putVar = builder->CreateLoad(rt.types.i8, cur);
  builder->CreateCall(rt.functions.putchar, {putVar});
}

void BrainFxxkCompiler::compile(Out *ir) {
  auto index = builder->CreateLoad(rt.types.i64, rt.memory.index);
  auto cur = builder->CreateGEP(rt.types.mem, rt.memory.ptr,
                                {rt.constants.i64zero, index});
  auto ret = builder->CreateCall(rt.functions.getchar, {});
  builder->CreateStore(ret, cur);
}

void BrainFxxkCompiler::compile(Loop *ir) {
  auto bb = llvm::BasicBlock::Create(context, "", rt.functions.entry);
  auto merge = llvm::BasicBlock::Create(context, "");

  {
    auto index = builder->CreateLoad(rt.types.i64, rt.memory.index);
    auto pointee = builder->CreateLoad(
        rt.types.i8, builder->CreateGEP(rt.types.mem, rt.memory.ptr,
                                        {rt.constants.i64zero, index}));
    auto cond = builder->CreateICmpEQ(pointee, rt.constants.i8zero);
    builder->CreateCondBr(cond, merge, bb);
    builder->SetInsertPoint(bb);
  }

  for (auto i : ir->operations()) {
    compile(i);
  }

  {
    auto index = builder->CreateLoad(rt.types.i64, rt.memory.index);
    auto pointee = builder->CreateLoad(
        rt.types.i8, builder->CreateGEP(rt.types.mem, rt.memory.ptr,
                                        {rt.constants.i64zero, index}));
    auto cond = builder->CreateICmpEQ(pointee, rt.constants.i8zero);
    auto parentFunc = builder->GetInsertBlock()->getParent();

    builder->CreateCondBr(cond, merge, bb);
    merge->insertInto(parentFunc);
    builder->SetInsertPoint(merge);
  }
}

std::expected<BFIRHandle, CompileError>
BrainFxxkCompiler::compile(const std::vector<char> &src) {
  std::vector<BFIR *> vec{};
  std::stack<Loop *> lps;
  size_t index = 0;

  auto emplaceOperation = [&](auto ir) {
    if (lps.empty()) {
      lps.top()->add(ir);
    } else {
      vec.emplace_back(ir);
    };
  };

  for (auto c : src) {
    index++;

    switch (c) {
    case '>': {
      emplaceOperation(new MovePtr(1));
      continue;
    }
    case '<': {
      emplaceOperation(new MovePtr(-1));
      continue;
    }

    case '+': {
      emplaceOperation(new TransformPointee(1));
      continue;
    }

    case '-': {
      emplaceOperation(new TransformPointee(-1));
      continue;
    }

    case '.': {
      emplaceOperation(new Out());
      continue;
    }

    case ',': {
      emplaceOperation(new In());
      continue;
    }

    case '[': {
      lps.push(new Loop());
      continue;
    }

    case ']': {
      if (lps.empty()) {
        return std::unexpected{
            CompileError{index, "no matching \"[\" for this bracket"}};
      }
      auto top = lps.top();
      lps.pop();
      emplaceOperation(top);
      continue;
    }

    default:
      continue;
    }
  }

  if (!lps.empty()) {
    return std::unexpected(CompileError{0, "no match \"]\" for \"]\""});
  }

  return vec;
}
