#pragma once

#include <expected>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>

#include <llvm/Support/Error.h>

#include "stdbfjit/stdbfjit.hpp"
#include "utils.hpp"

std::optional<std::vector<char>> readfile(std::string_view filepath) {
  std::ifstream file(filepath.data());
  if (file.fail()) {
    return {std::nullopt};
  }
  file.seekg(0, std::ios::end);
  size_t fileLength = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buf(fileLength);
  file.read(buf.data(), fileLength);
  return buf;
}

void printCompileError(const std::vector<char> &source, CompileError &error) {
  std::cout << error.reason() << std::endl;

  for (auto c : source) {
    if (c != '\n') {
      std::cout << c;
    }
  }
  std::cout << '\n';
  for (size_t i = 0; i < error.getIndex() - 1; i++) {
    std::cout << ' ';
  }
  std::cout << "^ here" << std::endl;
}

std::expected<void, std::string> verifyLLVMModule(llvm::Module &m) {
  std::string verifiModuleError;
  llvm::raw_string_ostream rso{verifiModuleError};
  auto result = llvm::verifyModule(m, &rso);
  rso.flush();
  if (result) {
    return std::unexpected{rso.str()};
  } else {
    return {};
  }
}

inline std::string lltos(auto *inst) {
  if (inst != nullptr) {
    std::string s;
    llvm::raw_string_ostream rso(s);
    inst->print(rso);
    rso.flush();
    return s;
  } else {
    return "[[[nullptr]]]";
  }
}

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
