#include "cli.hpp"

CommandLineOptions parseCLIoption(int argc, char **argv) {
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
    } else if (op == "-compile-only") {
      opt.setOnlyCompile(true);
    } else if (op == "-verbose") {
      opt.setVerbose(true);
    } else if (auto value = parseOptValue(op, "memsize")) {
      size_t memsize = BfCompilerOption::defaultMemSize;
      try {
        memsize = std::stoull(*value);
      } catch (std::exception) {
        std::cout << "invalid format of -memsize option:" << *value
                  << "set to default size: " << memsize << std::endl;
      }
      opt.setMemsize(memsize);
    } else if (auto outFileName = parseOptValue(op, "o")) {
      opt.setOutputFileName(*outFileName);
    } else {
      std::cout << std::format("unknown option \"{}\"", op) << std::endl;
    }
  }

  return opt;
}
