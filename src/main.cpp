#include "ANTLRInputStream.h"
#include "RxLexer.h"
#include "RxParser.h"
#include "support/CPPUtils.h"
#include "tree/Trees.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void printParseTree(antlr4::tree::ParseTree *tree, antlr4::Parser &parser,
                    const std::string &prefix = "") {
  std::cout << antlrcpp::escapeWhitespace(
                   antlr4::tree::Trees::getNodeText(tree, &parser), false)
            << '\n';

  for (std::size_t i = 0; i < tree->children.size(); ++i) {
    const bool isLast = i + 1 == tree->children.size();
    std::cout << prefix << (isLast ? "└── " : "├── ");
    printParseTree(tree->children[i], parser,
                   prefix + (isLast ? "    " : "│   "));
  }
}

} // namespace

int main() {
  antlr4::ANTLRInputStream input;
  std::ifstream file("test.rx");
  if (!file) {
    std::cerr << "Failed to open test.rx\n";
    return 1;
  }
  input.load(file);
  RxLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  RxParser parser(&tokens);
  auto *tree = parser.crate();
  printParseTree(tree, parser);
  return 0;
}
