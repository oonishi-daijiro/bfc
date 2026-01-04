#pragma once

#include <iostream>
#include <optional>
#include <string>

#include <llvm/TargetParser/Host.h>

#include "bfc.hpp"

class CommandLineOptions {
  bool _runJIT = false;
  bool _emitLLVMIR = false;
  bool _onlyCompile = false;
  bool _verbose = false;
  size_t _memsize = BfCompilerOption::defaultMemSize;
  std::string _outputName = "a.out";

public:
  void setRunJIT(bool v) { _runJIT = v; }
  void setEmitLLVMIR(bool v) { _emitLLVMIR = v; }
  void setMemsize(size_t n) { _memsize = n; }
  void setOutputFileName(const std::string &f) { _outputName = f; }
  void setVerbose(bool v) { _verbose = v; }
  void setOnlyCompile(bool v) { _onlyCompile = v; }

  auto JITRun() const { return _runJIT; }
  auto emitIR() const { return _emitLLVMIR; }
  auto memsize() const { return _memsize; }
  auto verbose() const { return _verbose; }
  auto onlyCompile() const { return _onlyCompile; }

  const auto &outputFileName() const { return _outputName; }
};

template <size_t N>
inline std::optional<std::string> parseOptValue(const std::string &inp,
                                                const char (&optName)[N]) {
  if (inp.starts_with(std::format("-{}=", optName))) {
    return inp.substr(N + 1);
  } else {
    return {};
  }
}

CommandLineOptions parseCLIoption(int argc, char **argv);
