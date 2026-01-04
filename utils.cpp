#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <lld/Common/Driver.h>
#include <span>
#include <vector>

#include <llvm/Support/Error.h>

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

std::expected<std::filesystem::path, std::string>
findBrainFxxkRuntime(const std::string &ownProcAbsPath) {
  using namespace std::filesystem;
  std::filesystem::path p{ownProcAbsPath};
  auto rtPath = (p.parent_path() / ".." / "lib" / "bfrt.a").lexically_normal();

  if (!std::filesystem::exists(rtPath)) {
    return std::unexpected{
        std::format("brainf**k runtime does not exist: {}", rtPath.string())};
  }
  return rtPath;
};

std::expected<std::string, std::string>
runLLD(std::span<const char *> arg, std::span<lld::DriverDef> drivers) {
  std::string lldOutsStr{}, lldErrStr{};
  llvm::raw_string_ostream lldOut{lldOutsStr}, lldErr{lldErrStr};
  auto result = lld::lldMain(arg, lldOut, lldErr, drivers);

  lldOut.flush();
  lldErr.flush();

  if (result.retCode != 0) {
    return std::unexpected{lldErrStr};
  } else {
    return lldOutsStr;
  }
};
