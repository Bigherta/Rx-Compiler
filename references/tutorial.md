# Yx Tutorial 解读：从最小语言理解完整编译器流程

> 原文：https://github.com/ZYHowell/Yx/blob/main/Tutorial.md（`Yx: a simplified smallest grammar`）
> 本文是对该 Tutorial 的中文讲解 + 批注版，原文为空骨架，这里按“为什么要这么做”逐段解释，并标注映射到本仓库 `rx-compiler` 时要注意的点。
>
> Yx 是 Mx 语言的极简子集。作者用它演示：**前端（词法/语法/AST/语义）→ 中端（IR/优化）→ 后端（指令选择/寄存器分配）** 的最小闭环。

---

## 1. Yx 语言定义：为什么要先做减法

### 1.1 Type（类型）

原文：

- 默认只有 `int` 和 `bool`（但没有 `bool` 变量，即 `bool` 只作为比较表达式的中间类型存在）；
- 支持 `struct` 定义，写法同 C++，但初学时先忽略，直到 [Intro to type](#6-类型扩展-struct-与两遍扫描) 再看。

**解读：**
这是刻意把类型系统压到最小：变量只有 `int`，`bool` 只出现在 `a == b` 的结果里。这样语义检查阶段几乎不用做类型推导，只需记住“比较表达式是 bool，其余是 int”。等理解了主流程，再把 `struct` 加回来，体会“类型表 / 全局作用域”为什么必要。

对 Rx 的启示：Rx 比 Yx 大（数组/类/函数/字符串等），但可以照抄这个学习顺序——先让 `int + if + return` 跑通，再逐个加特性。

### 1.2 Arithmetic operation（运算）

原文只允许 `+ - == !=`。

**解读：**
四则运算里只留加减，乘除取模暂时砍掉；逻辑运算只留判等/判不等。这是为了：

1. IR 只需一种 `binary` 指令格式；
2. 指令选择时只需处理 `sub + seqz/snez` 就能实现 `==/!=`，不用处理短路、乘除等复杂情况。

### 1.3 Statement（语句）

原文语句只有 4 种：

1. 变量定义 `varDef`
2. 表达式语句
3. `if-else`
4. `return`

没有 `for/while/func-call`。连 `program` 都直接规定为 `int main() suite EOF`。

**解读：**
控制流只剩 `if-else + return`，CFG（控制流图）只有 `branch + jump` 两种边，IR 的基本块划分最简单。这是理解 CFG 的最小代价路径。

---

## 2. 朴素前端实现（Naïve Front-End）

总顺序：`program - statement - expression - primary`，Mx 则在 `program` 和 `statement` 之间插入函数/类定义。

### 2.1 写 grammar（`.g4` 文件）

原文核心片段：

```antlr
program: 'int main()' suite EOF;
varDef : Int Identifier ('=' expression)? ';';
suite : '{' statement* '}';
statement
    : suite                                             #block
    | varDef                                            #vardefStmt
    | If '(' expression ')' trueStmt=statement
        (Else falseStmt=statement)?                     #ifStmt
    | Return expression? ';'                            #returnStmt
    | expression ';'                                    #pureExprStmt
    | ';'                                               #emptyStmt
    ;
expression
    : primary                                           #atomExpr
    | expression op=('+' | '-') expression              #binaryExpr
    | expression op=('==' | '!=' ) expression           #binaryExpr
    | <assoc=right> expression '=' expression           #assignExpr
    ;
primary
    : '(' expression ')'
    | Identifier
    | literal
    ;
```

另有一大段 Token 定义（`Int/If/Else/Return`、括号、比较符、位运算、`Identifier`、`DecimalInteger`、空白/换行/注释 `-> skip`）。

两处 ANTLR 技巧的解读：

- `#block / #ifStmt ...`：给同一规则的不同分支起标签。ANTLR 会为每个标签生成独立的 `XxxContext` 类（如 `IfStmtContext`），Visitor 里就可以写 `visitIfStmt` 而不用 `instanceof` 判断。这是“语法分支 → AST 节点类型”一一对应的关键。
- `<assoc=right>`：声明右结合。`a = b = c` 应解析为 `a = (b = c)`，不加则默认左结合会错。`+ - ==` 保持默认左结合即可。

对 Rx：看本仓库 `grammar/Lexer.g4, Parser.g4`，做法完全一样，只是规则更多。建议照抄 Yx 的命名习惯：`#` 标签名即 AST 节点名。

### 2.2 编译器中的 Parse 部分

流程：ANTLR 生成 `YxLexer.java / YxParser.java`，`main` 里：

```java
YxLexer lexer = new YxLexer(CharStreams.fromStream(input));
lexer.removeErrorListeners();
lexer.addErrorListener(new YxErrorListener());
YxParser parser = new YxParser(new CommonTokenStream(lexer));
parser.removeErrorListeners();
parser.addErrorListener(new YxErrorListener());
ParseTree parseTreeRoot = parser.program();
```

解读：

1. 先 `removeErrorListeners` 去掉 ANTLR 默认往 stderr 打印的监听器，换成自己的 `YxErrorListener`，目的是把语法错误转成带 `position(line, col)` 的异常，统一错误处理；
2. `YxErrorListener extends BaseErrorListener`，重写 `syntaxError(...)` 直接 `throw new syntaxError(...)`。这样上层 `try/catch` 捕获后转 `RuntimeException`，测试框架只需看退出码（对应本仓库 `config.mk` 里 `SEMANTIC` 约定 exit 0/1）。

### 2.3 设计 AST

原文强调：parse tree 已是树形，为什么还要 AST？因为 parse tree 节点是语法导向的（有很多中间符号、括号、冗余层），AST 节点是语义导向的（要存类型、值、作用域信息）。

原文层级：

```
ASTNode
  RootNode
  StmtNode
    varDefStmtNode / returnStmtNode / blockStmtNode / exprStmtNode / ifStmtNode
  ExprNode
    assignExprNode / binaryExprNode / constExprNode / cmpExprNode / varExprNode
```

以 `ifStmtNode` 为例：存 `condition, thenStmt, elseStmt, pos`，并实现 `accept(ASTVisitor)`。Visitor 返回值可以是任意类型（不一定 `void`），方便后阶段复用（如语义检查返回 `Type`，IR 构建返回 `Register`）。

### 2.4 AST Builder

AST Builder 是跑在 parse tree 上的 Visitor，把 `XxxContext` 转成 `XxxNode`。原文示例：

```java
@Override public ASTNode visitVarDef(YxParser.VarDefContext ctx) {
  String name = ctx.Identifier().toString();
  ExprNode expr = null;
  if (ctx.expression() != null) expr = (ExprNode)visit(ctx.expression());
  return new varDefStmtNode(name, expr, new position(ctx));
}
```

解读：琐碎但必须——每个 `visitX` 只做“解包 Context + new Node”。建议写完 g4 就立刻把 Builder 写完并单测打印 AST，否则后阶段 debug 会很痛苦。

### 2.5 Scope（作用域）

Yx 全局只需记变量名（只有 `int/bool`，不用记类型表）。每个 `Scope` 存 `members + parentScope`：

```java
defineVariable(name, pos) // 重定义抛 semanticError
containsVariable(name, lookUpon) // lookUpon=true 则向父作用域查
```

**批注（原文笔误）：** 原文示例中 `containsVariable` 递归调用写成了 `parentScope.containsMember(name, true)`，实际应为 `containsVariable`。照抄会编译不过，注意改。

### 2.6 Semantic（语义检查）

Yx 只需查三件事：

1. 类型是否匹配 → 给每个 `ExprNode` 加 `type` 属性。Yx 里除 `cmpExprNode` 是 `bool` 外全是 `int`，所以很简单；Mx/C++ 则要在此阶段定类型/做推导。
2. 左值检查 → 给每个 `ExprNode` 加 `isAssignable`，防止 `1 = a` 这类“右值在左”。
3. 未定义 / 重定义变量。

原文关键细节——**顺序很重要**：

```java
visit(varDefStmtNode it) {
  if (it.init != null) {
    it.init.accept(this);            // 先检查初值
    if (!it.init.type.isInt) throw ...;
  }
  currentScope.defineVariable(it.name, it.pos); // 后定义
}
```

如果先 `define` 再检查，就查不出 `int x = x + 1;`（右边 `x` 应视为未定义）。这是语义检查的经典坑。

### 2.7 类型扩展：struct 与两遍扫描

新增语法：

```antlr
classDef : 'struct' Identifier '{' varDef* '}'';';
program: classDef* mainFn;
varDef : type Identifier ('=' expression)? ';';
type : Int | Identifier;
```

难点例子：

```c
struct a { b x; };
struct b { a x; };
```

`a` 和 `b` 互相引用，一遍扫描时无论先检查谁，对方类型都还未知（前向引用问题）。Yx 里因为全是引用语义（指针），这种循环定义是合法的，不能直接报错。

解法：**加一遍 `SymbolCollector`**。第一遍只收集所有 struct 名进 `GlobalScope`（`Scope` 子类，加 `Map<String,Type>`），不检查成员合法性；第二遍再检查成员类型、重定义、成员不允许默认初始化等。同时把 `Scope` 从 `HashSet<String>` 升级为 `Map<String,Type>`，并提供 `getType(name, lookUpon)`，`varExpr/assignExpr` 的 `type` 从 scope 查得。

解读：这就是编译器里“声明 vs 定义”分离的思想，Rx 的类/函数同样需要（先收类名/函数签名，再查函数体）。

---

## 3. Codegen（代码生成）：经由 IR 再到汇编

原文路线图：

```
code -> AST -> IR -> IR ... -> IR -> assembly
       frontend   optimization      backend
```

为什么不从 AST 直译汇编？原文给了经典例子：

```
x = a + b + c + d;
y = a + b + c + e;
```

`a+b+c` 被算了两次。在 AST 上要发现它，需要两两比较子树（同构检测），代价高；若先把复杂表达式拆成**单操作、至多双操作数**的三地址码，再做公共子表达式消除就很自然。而且汇编本身就是双操作数形式，IR 与汇编更接近。

### 3.1 IR 设计

Yx 的图 IR（graph IR）只有 4 种：

```
x = u op v        # op = + / - / == / !=
br u label1, label2
j label
ret u
```

- `x,u,v` 都是“寄存器”（变量/常量/中间结果 `apb` 都视为寄存器，32-bit，bool 也占 32-bit）；
- struct 变量即存指针的寄存器；成员访问 = 基址 + offset 再 `load/store`；`malloc/free` 则加 IR 调用；
- `if` 先算出 bool 到寄存器，再 `br` 分真假分支，分支末尾 `j` 跳到汇合点；
- **Block 定义**：从一个 label 开始，到第一个 `br/j` 结束。这是 CFG 的基本单位，后续几乎所有优化都基于 CFG，正如很多优化基于二元运算。

### 3.2 IR Builder

先定义 IR 类：`Register/Constant/Block/语句`，label 直接用 `Block` 表示。

给每个 `ExprNode` 加 `val`（对应寄存器/常量），示例：

```java
visit(cmpExprNode it) {
  it.lhs.accept(this); it.rhs.accept(this);
  register value = new register();
  binary.opType op = (it.opCode == eq ? eq : ne);
  currentBlock.push_back(new binary(value, it.lhs.val, it.rhs.val, op));
  it.val = value;
}
visit(varExprNode it) {
  it.val = currentScope.getEntity(it.name, true);
}
```

Block 维护（`if`）：

```java
visit(ifStmtNode it) {
  it.condition.accept(this);
  block trueBranch = new block(), falseBranch = new block();
  currentBlock.push_back(new branch(it.condition.val, trueBranch, falseBranch));
  block destination = new block();
  currentBlock = trueBranch;
  it.thenStmt.accept(this);
  currentBlock.push_back(new jump(destination));
  currentBlock = falseBranch;
  it.elseStmt.accept(this); // 注意：无 else 时需处理空分支
  currentBlock.push_back(new jump(destination));
  currentBlock = destination;
}
```

### 3.3 IR Printer / Pass 概念

为验证 IR 正确性，先写 IR Printer。原文顺带引入 **Pass** 概念：对 `program/function/block` 做一次遍历/改写即一个 Pass。优化 = 一个个 Pass；IR Printer 是最典型的只读 Pass。Yx 为简单起见只有“整程序 Pass”一种。

对 Rx：本仓库 `IR` 阶段要求输出 LLVM `.ll` 再用 Clang 降到 RV32IM，可以把“打印 LLVM”视为你的第一个 Pass。

### 3.4 Prune（剪枝）与其它 IR

- **Prune**：在 IRBuilder 里顺手做常量折叠/无用跳转剪枝等，减轻后阶段压力。
- **其它 IR**：Yx 的是图 IR；还有贴近 AST 的树 IR；实际编译器常有多层 IR 逐层 lowering（每 lowering 一次丢一些高层信息，但换来某些分析更易做）。“设计自己的 IR”是允许个性化的部分。

### 3.5 Instruction selection（指令选择）

假设 IR 已优化好，转汇编第一步是选指令。做法与 IR 建类完全类似，只是更自由（可用 `mv/li/ret` 等伪指令增强可读性），同时要处理 ISA 差异：RISC-V 没有 `seq/sne`，需译为 `sub + seqz/snez`。

此阶段仍用**虚拟寄存器**（假设无限多），32 个物理寄存器的限制留到下一步。树 IR 在此阶段还需多做“树形 → 基本块图”的拍平，图 IR 则几乎一一对应，这是选图 IR 的好处。

### 3.6 Asm Printer

虚拟寄存器阶段就先实现汇编打印，便于调试指令选择，与 IR Printer 同理。

### 3.7 Register Allocation（寄存器分配，最终步）

背景：RISC-V 寄存器分 `caller-save`（调用后不保证保留，活区间至多在两次 call 之间）与 `callee-save`（调用前后保持一致，活区间可跨整个函数，但用之前要 spill/恢复）。原文技巧：给每个 callee-save 物理寄存器配一个虚拟寄存器，函数首尾加 `move`，没用到则靠着色 + peephole 删掉，省去“统计用了哪些再补 spill”的麻烦。

Yx 只讲**朴素分配**：所有虚拟寄存器全放栈上，用时 `lw` 到 `t0/t1/t2`（caller-save，索引 5-7），改完再 `sw` 回去。每条指令前后插 load/store 即可。

```java
class Ld extends Inst { PhyReg rd, addr; Imm offset; } // lw rd, offset(addr)
loadVirtualReg(r, rd)  -> Ld(rd, sp, r.index * 4)
storeVirtualReg(r)     -> St(t2, sp, r.index * 4)      // 全 4B，只用 lw/sw
```

实现要点：

1. Java 没有好用的侵入式链表，自己写 `insert_before/insert_after` 维护 `headInst/tailInst/prev/next`，方便插指令；
2. 栈大小维护：朴素算法下栈大小 = 虚拟寄存器数 × 4；一旦有函数调用（参数过多需栈传参）或上更好的分配算法，栈大小要在分配后才知道，注意回填 `Imm offset`。

---

## 4. 一句话总结 + 如何用到 rx-compiler

| Yx 阶段 | 做什么 | 对应 Rx 落点 |
|---|---|---|
| Grammar + Parse | g4 → ANTLR → parse tree + ErrorListener | `grammar/*.g4`，`SEMANTIC` 退出码 0/1 |
| AST + Builder | parse tree → AST（加 `type/val/pos`） | 先让最小子集可打印 AST |
| Scope + Semantic | 作用域链 + 三查（类型/左值/定义）+ SymbolCollector 两遍 | 类/函数前向引用同样两遍 |
| IR + Builder + Printer | 三地址 + CFG Block + Pass 打印 | `IR` 输出 `.ll`，用 Clang 验证 |
| 指令选择 + 分配 | 虚拟寄存器 → RV32IM（`seq` 展开）→ 朴素 spill (`t0-t2` + `lw/sw`) | `CODEGEN` 输出给 REIMU 跑，`RUN` 对拍 |

> 下一步建议：照 Yx 顺序在 `rx-compiler` 里先跑通 `int main + varDef + if/return + +-==` 的端到端（semantic → IR → codegen → REIMU），再逐个加 `struct/class、数组、函数调用、循环`，每加一个就回看本节对应小节。
