#include "IRGenerator.hpp"
#include <iostream>
#include <variant>
#include <vector>

int main() {
  const Operand::SSA index("index");
  const auto isInteger = [](const Operand &operand, long long expected) {
    const auto *immediate = std::get_if<Operand::Immediate>(&operand.operand);
    if (!immediate) {
      return false;
    }
    const auto *value = std::get_if<long long>(&immediate->constValue);
    return value && *value == expected;
  };
  const auto isIndex = [&](const Operand &operand) {
    const auto *ssa = std::get_if<Operand::SSA>(&operand.operand);
    return ssa && ssa->SSAName == index.SSAName;
  };
  const std::vector<Operand> dynamicIndices{0, index};
  const std::vector<Operand> constantIndices{0, 1};
  const std::vector<Operand> singleIndex{index};
  const std::vector<Operand> negativeIndex{-1};
  const Operand implicitInteger = 17;
  if (dynamicIndices.size() != 2 || !isInteger(dynamicIndices[0], 0) ||
      !isIndex(dynamicIndices[1]) || constantIndices.size() != 2 ||
      !isInteger(constantIndices[0], 0) || !isInteger(constantIndices[1], 1) ||
      singleIndex.size() != 1 || !isIndex(singleIndex[0]) ||
      negativeIndex.size() != 1 || !isInteger(negativeIndex[0], -1) ||
      !isInteger(implicitInteger, 17)) {
    std::cerr << "Operand initializer-list compatibility failed\n";
    return 1;
  }
  std::cout << "Operand initializer-list compatibility passed\n";
}
