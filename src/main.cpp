#include "ASTBuilder.hpp"
#include "ASTPrinter.hpp"
#include "RxLexer.h"
#include "RxParser.h"
#include "SemanticAnalyzer.hpp"
#include "SymbolCollector.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

int main(int argc, char **argv) {
  // Exit codes: 0 = accepted, 1 = source error, 2 = argument or file error.

  // Parse command-line arguments.
  bool printAst = false;
  const char *path = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--ast") {
      printAst = true;
    } else if (path == nullptr) {
      path = argv[i];
    } else {
      return 2;
    }
  }
  if (path == nullptr) {
    return 2;
  }

  // Read the source file.
  std::ifstream file(path);
  if (!file) {
    std::cerr << "File error: could not open source file `" << path
              << "` for reading\n";
    return 2;
  }
  antlr4::ANTLRInputStream input;
  input.load(file);

  // Run lexical and syntax analysis.
  RxLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  RxParser parser(&tokens);
  auto *tree = parser.crate();
  const std::size_t syntaxErrors =
      lexer.getNumberOfSyntaxErrors() + parser.getNumberOfSyntaxErrors();
  if (syntaxErrors != 0 || tree == nullptr) {
    std::cerr << syntaxErrors
              << (syntaxErrors == 1
                      ? " lexical or syntax error; compilation stopped\n"
                      : " lexical or syntax errors; compilation stopped\n");
    return 1;
  }

  // Build the AST and optionally print it.
  ASTBuilder builder;
  auto crate = builder.visitCrate(tree);
  if (!crate) {
    std::cerr << "AST error: failed to construct the syntax tree "
                 "from the parsed source\n";
    return 1;
  }

  if (printAst) {
    ASTPrinter printer;
    crate->accept(&printer);
  }

  // Collect symbols first; semantic analysis requires a valid symbol table.
  SymbolCollector collector;
  crate->accept(&collector);
  if (collector.error.hasError()) {
    collector.error.report(std::cerr);
    return 1;
  }

  SemanticAnalyzer analyzer(std::move(collector));

  // Evaluate constants before resolving array lengths.
  analyzer.checkConstant();
  if (analyzer.error.hasError()) {
    analyzer.error.report(std::cerr);
    return 1;
  }

  // Preprocess array lengths in their declaration contexts.
  analyzer.preprocessArrayLengths();
  if (analyzer.error.hasError()) {
    analyzer.error.report(std::cerr);
    return 1;
  }

  // Check the types.
  analyzer.checkItemType();
  if (analyzer.error.hasError()) {
    analyzer.error.report(std::cerr);
    return 1;
  }

  // Check structs after resolving `Self` to its enclosing struct name.
  analyzer.checkStruct();
  if (analyzer.error.hasError()) {
    analyzer.error.report(std::cerr);
    return 1;
  }

  // Run semantic checks.
  crate->accept(&analyzer);
  if (analyzer.error.hasError()) {
    analyzer.error.report(std::cerr);
    return 1;
  }

  return 0;
}
