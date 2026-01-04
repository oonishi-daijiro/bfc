#include "jit.hpp"

std::expected<int, std::string> JITRun(std::unique_ptr<llvm::Module> llModule) {
  auto jit = llvm::orc::LLJITBuilder().create();
  if (!jit) {
    return std::unexpected{llvm::toString(std::move(jit.takeError()))};
  }

  auto &jitDynamicLib = jit->get()->getMainJITDylib();

  llvm::orc::SymbolMap symbols;

  auto putcharName = jit->get()->mangleAndIntern("bfputchar");
  auto getcharName = jit->get()->mangleAndIntern("bfgetchar");
  auto callocName = jit->get()->mangleAndIntern("bfcalloc");
  auto freeName = jit->get()->mangleAndIntern("bffree");

  auto stdbfjitPutcharPtr = &stdbfjit::bfputchar;
  auto stdbfjitGetcharPtr = &stdbfjit::bfgetchar;
  auto stdbfjitCallocPtr = &stdbfjit::bfcalloc;
  auto stdbfjitFreePtr = &stdbfjit::bffree;

  auto putcharDef = llvm::orc::ExecutorSymbolDef(
      llvm::orc::ExecutorAddr::fromPtr(stdbfjitPutcharPtr),
      llvm::JITSymbolFlags::Exported);

  auto getcharDef = llvm::orc::ExecutorSymbolDef(
      llvm::orc::ExecutorAddr::fromPtr(stdbfjitGetcharPtr),
      llvm::JITSymbolFlags::Exported);

  auto callocDeg = llvm::orc::ExecutorSymbolDef(
      llvm::orc::ExecutorAddr::fromPtr(stdbfjitCallocPtr),
      llvm::JITSymbolFlags::Exported);
  auto freeDef = llvm::orc::ExecutorSymbolDef(
      llvm::orc::ExecutorAddr::fromPtr(stdbfjitFreePtr),
      llvm::JITSymbolFlags::Exported);

  symbols[putcharName] = putcharDef;
  symbols[getcharName] = getcharDef;
  symbols[callocName] = callocDeg;
  symbols[freeName] = freeDef;

  auto defineationResult =
      jitDynamicLib.define(llvm::orc::absoluteSymbols(symbols));

  if (defineationResult) {
    return std::unexpected{"failed to define symbols to JIT library"};
  }

  auto tsm = llvm::orc::ThreadSafeModule(std::move(llModule),
                                         std::make_unique<llvm::LLVMContext>());

  auto addModuleRes = jit->get()->addIRModule(std::move(tsm));
  if (addModuleRes) {
    return std::unexpected{"failed to add IR modules"};
  }

  auto entry = jit->get()->lookup("entry");
  if (!entry) {
    return std::unexpected{"failed to lookup entry function"};
  }

  auto entryFnPtr = entry->toPtr<int (*)()>();
  return entryFnPtr();
}
