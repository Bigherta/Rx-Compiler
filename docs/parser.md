# Parser 接口说明

`src/Parser/` 提供由 ANTLR 4.13.2 从 `grammar/Lexer.g4` 和 `grammar/Parser.g4` 生成的 C++ 词法分析器、语法分析器以及 parse tree 遍历接口。它们是生成式 API，没有额外的手写封装。

## 文件与用途

| 文件 | 用途 |
| --- | --- |
| `RxLexer.h/.cpp` | `RxLexer` 词法分析器，将输入字符流转换为 token。 |
| `RxParser.h/.cpp` | `RxParser` 语法分析器；包含各语法规则对应的 `*Context` 类型及入口方法。 |
| `RxParserVisitor.h/.cpp` | `RxParserVisitor` 抽象 visitor 接口，可按需访问 parse tree 节点。 |
| `RxParserBaseVisitor.h/.cpp` | 默认递归访问子节点的 visitor 基类，适合继承后只重写关心的规则。 |
| `RxParserListener.h/.cpp` | `RxParserListener` 抽象 listener 接口，提供规则进入和退出回调。 |
| `RxParserBaseListener.h/.cpp` | 所有回调均为空实现的 listener 基类，适合按需重写。 |

所有生成的类位于全局命名空间。使用 `RxParser` 时，也需要包含 ANTLR C++ runtime 的头文件；通过本仓库的 CMake target `rx_parser` 链接 runtime。

## 解析入口与基本用法

根规则是 `crate`，用于解析一个完整源文件（零个或多个 item，随后必须到达 EOF）。其他规则也可直接作为入口调用，例如 `expression()` 或 `typeRef()`，具体可用方法见 `RxParser.h`。

```cpp
#include "antlr4-runtime.h"
#include "RxLexer.h"
#include "RxParser.h"

#include <string>

int main() {
  const std::string source = "fn main() { 1 + 2 }";
  antlr4::ANTLRInputStream input(source);
  RxLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  RxParser parser(&tokens);

  RxParser::CrateContext *tree = parser.crate();
  // tree 是根 parse-tree 节点；错误处理见下文。
}
```

常见的 `crate` 子节点包括：

- `item`：顶层条目分派节点；
- `useDeclaration`：`use` 声明；
- `functionDefinition`：函数定义；
- `structDefinition`：结构体定义；
- `constantItem`：常量项；
- `inherentImpl`：固有实现块。

完整规则和语法范围以 `grammar/Parser.g4` 为准。`RxParser.h` 中每个规则都有对应的上下文类（例如 `FunctionDefinitionContext`），并提供该规则的解析方法。上下文类也提供访问 token、子上下文、起止位置等 ANTLR 标准接口；可直接查看生成的 `RxParser.h` 了解具体访问器。

## 遍历 parse tree

### Visitor

`RxParserVisitor` 为每条 parser 规则定义 `visit<规则名>` 方法，返回类型为 `std::any`。实现该纯虚接口需要处理所有方法；通常从 `RxParserBaseVisitor` 派生，只重写感兴趣的规则，其余节点默认递归访问子节点。

```cpp
#include "RxParserBaseVisitor.h"

class FunctionVisitor : public RxParserBaseVisitor {
public:
  std::any visitFunctionDefinition(
      RxParser::FunctionDefinitionContext *ctx) override {
    // ctx->identifier()、ctx->blockExpression() 等访问器来自语法规则。
    return visitChildren(ctx);
  }
};

// 在解析后调用：
// FunctionVisitor visitor;
// visitor.visit(tree);
```

### Listener

`RxParserListener` 对每条规则提供 `enter<规则名>` 和 `exit<规则名>` 回调。可继承 `RxParserBaseListener` 并按需重写回调，再用 ANTLR 的 `antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree)` 遍历。Listener 适用于进入/退出节点时执行动作；Visitor 则适合显式递归并组合返回值。

## Lexer token 与诊断

`RxLexer` 中定义了 token 类型常量（`RxLexer.h`），`RxParser` 中也包含同一组 token 常量。主要类别包括关键字、标识符、生命周期、整数字面量、运算符和分隔符。空白符、行注释和块注释被放到 `HIDDEN` channel；如需保留它们，可从 token stream 的隐藏 channel 查询。

词法器会为部分非法输入生成显式 token，而不是静默丢弃：

- `INVALID_LIFETIME`、`INVALID_CHARACTER_LITERAL`、`INVALID_NUMBER`；
- `UNTERMINATED_BLOCK_COMMENT`：未闭合的块注释；
- `ERROR_CHAR`：其他无法识别的字符。

解析可能通过 ANTLR 默认错误恢复生成 parse tree，即使过程中报告了语法错误。因此调用者应注册 lexer 和 parser 的 error listener 并检查语法错误数量；如需严格校验词法输入，也应检查上述非法 token，不能仅凭 `crate()` 返回了上下文就认定输入有效。ANTLR 默认错误 listener 会向标准错误输出诊断信息。

## 生成代码与语法来源

`src/Parser/` 是生成产物，语法源文件在 `grammar/`。重新生成后，规则名、token 编号、上下文访问器以及 visitor/listener 方法都可能变化。实现应依赖语法规则名和公开上下文 API，避免依赖内部状态或硬编码 token 数值。ANTLR C++ runtime 由仓库的 CMake 工程通过 `vendor/antlr4-cpp` 提供；`rx_parser` target 会公开 parser 头文件目录并链接 runtime。
