#pragma once

#include <expected>
#include <filesystem>
#include <lld/Common/Driver.h>
#include <llvm/IR/Verifier.h>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include "bfc.hpp"

std::optional<std::vector<char>> readfile(std::string_view filepath);
std::string lltos(auto *inst);
void printCompileError(const std::vector<char> &source, CompileError &error);
std::expected<void, std::string> verifyLLVMModule(llvm::Module &m);

std::expected<std::filesystem::path, std::string>
findBrainFxxkRuntime(const std::string &target);

std::expected<std::string, std::string>
runLLD(std::span<const char *> arg, std::span<lld::DriverDef> drivers);
#define VERBOSE_LOG_PREFIX "[bfc] "
