#pragma once

#include <iostream>
#include <optional>
#include <string>

#include <llvm/TargetParser/Host.h>

#include "bfc.hpp"

class CommandLineOptions {
  bool _runJIT = false;
  bool _emitLLVMIR = false;
  size_t _memsize = BfCompilerOption::defaultMemSize;
  std::string _target = llvm::sys::getDefaultTargetTriple();
  std::string _outputName = "a.out";

public:
  void setRunJIT(bool v) { _runJIT = v; }
  void setEmitLLVMIR(bool v) { _emitLLVMIR = v; }
  void setMemsize(size_t n) { _memsize = n; }
  void setTarget(const std::string &t) { _target = t; }
  void setOutputFileName(const std::string &f) { _outputName = f; }

  auto JITRun() const { return _runJIT; }
  auto emitIR() const { return _emitLLVMIR; }
  auto memsize() const { return _memsize; }
  const auto &target() const { return _target; }
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

inline CommandLineOptions parseCLIoption(int argc, char **argv) {
  std::vector<std::string> arg{};

  for (int i = 2; i < argc; i++) {
    arg.emplace_back(argv[i]);
  }

  CommandLineOptions opt{};

  for (auto op : arg) {
    if (op == "-emit-llvm") {
      opt.setEmitLLVMIR(true);
    } else if (op == "-jit") {
      opt.setRunJIT(true);
    } else if (auto value = parseOptValue(op, "memsize")) {
      size_t memsize = BfCompilerOption::defaultMemSize;
      try {
        memsize = std::stoull(*value);
      } catch (std::exception) {
        std::cout << "invalid format of -memsize option:" << *value
                  << "set to default size: " << memsize << std::endl;
      }
      opt.setMemsize(memsize);
    } else if (auto target = parseOptValue(op, "target")) {
      opt.setTarget(*target);
    } else if (auto outFileName = parseOptValue(op, "o")) {
      opt.setOutputFileName(*outFileName);
    } else {
      std::cout << std::format("unknown option \"{}\"", op) << std::endl;
    }
  }

  return opt;
}
