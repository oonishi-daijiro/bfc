#pragma once

#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
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
findBrainfxxkStdLib(const std::string &ownProcAbsPath,
                    const std::string &target) {
  using namespace std::filesystem;
  std::filesystem::path p{ownProcAbsPath};
  auto stdlibPath =
      p.parent_path() / path{std::format("/lib/stdbf.{}.lib", target)};
  if (!std::filesystem::exists(stdlibPath)) {
    return std::unexpected{
        std::format("unable to find target stdlib for target {}", target)};
  }
  return p;
};
