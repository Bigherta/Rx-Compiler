#include "RxLexer.h"
#include "RxParser.h"
#include <fstream>
#include <iostream>
#include "ASTBuilder.hpp"
#include "ASTPrinter.hpp"

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
  ASTBuilder builder;
  auto crate = builder.visitCrate(tree);
  ASTPrinter printer;
  crate->accept(&printer);
  return 0;
}
