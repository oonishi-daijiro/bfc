#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

class BFIR {
public:
  virtual ~BFIR() = default;
  virtual std::string toString() const = 0;

  template <typename T>
  std::optional<T *> isa()
    requires(!std::is_pointer_v<T> && !std::is_reference_v<T>)
  {
    auto ptr = dynamic_cast<T *>(this);
    if (ptr != nullptr) {
      return {ptr};
    } else {
      return std::nullopt;
    }
  }
};

class BFIRHandle {
  std::vector<BFIR *> vec;

public:
  BFIRHandle(std::vector<BFIR *> &&vec) : vec{std::move(vec)} {}
  BFIRHandle(const BFIRHandle &) = delete;
  BFIRHandle(BFIRHandle &&r) { this->vec = std::move(r.vec); }
  BFIRHandle &operator=(const BFIRHandle &) = delete;
  BFIRHandle &operator=(BFIRHandle &&r) {
    this->vec = std::move(r.vec);
    return *this;
  }

  ~BFIRHandle() {
    for (auto p : vec) {
      delete p;
    }
  }
  const auto begin() const { return vec.begin(); }
  const auto end() const { return vec.end(); }
};

class MovePtr : public BFIR {
  int64_t moveAmount = 0;

public:
  MovePtr() = delete;
  MovePtr(int64_t amount) : moveAmount{amount} {}
  ~MovePtr() = default;

  int amount() { return moveAmount; }
  virtual std::string toString() const override {
    return std::format("MovePtr: {}", moveAmount);
  }
};

class TransformPointee : public BFIR {
  int8_t transVal = 0;

public:
  TransformPointee() = delete;
  TransformPointee(int8_t to) : transVal{to} {};
  ~TransformPointee() = default;

  int8_t val() { return transVal; }
  virtual std::string toString() const override {
    return std::format("TransformPointee : {}", transVal);
  }
};

class In : public BFIR {
public:
  virtual ~In() = default;
  virtual std::string toString() const override { return std::format("Input"); }
};

class Out : public BFIR {
public:
  virtual ~Out() = default;
  virtual std::string toString() const override {
    return std::format("Output");
  }
};

class Loop : public BFIR {
  std::vector<BFIR *> oprs{};

public:
  Loop() {};
  Loop(const Loop &) = delete;
  Loop(Loop &&r) { this->oprs = std::move(r.oprs); }
  Loop &operator=(const Loop &) = delete;
  Loop &operator=(Loop &&r) {
    this->oprs = std::move(r.oprs);
    return *this;
  }

  virtual ~Loop() {
    for (auto p : oprs) {
      delete p;
    }
  }

  void add(BFIR *opr) { oprs.emplace_back(opr); }
  const std::vector<BFIR *> &operations() const { return oprs; }
  virtual std::string toString() const override {
    auto joined = oprs |
                  std::views::transform([](BFIR *v) { return v->toString(); }) |
                  std::views::join_with(std::string_view(" , ")) |
                  std::ranges::to<std::string>();
    return joined;
  }
};
