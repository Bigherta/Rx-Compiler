# Rx 编译器

Rx 编译器课程项目仓库。仓库包含 Rx 语言的 ANTLR 文法与生成的 C++ Parser、Rust `rx` crate、官方测试框架以及 REIMU RV32IM 模拟器子模块。

## 项目结构

| 路径 | 说明 |
| --- | --- |
| `crates/rx/` | Rust `rx` crate 的源码。 |
| `grammar/` | Rx 语言的 ANTLR lexer/parser 文法。 |
| `src/Parser/` | 从文法生成的 C++ Lexer、Parser、Visitor 和 Listener 接口。 |
| `docs/parser.md` | Parser C++ 接口用法与错误处理说明。 |
| `tests/official/` | 官方测试用例（Git 子模块）；`tests/custom/` 可放置本地测试。 |
| `scripts/test.py` | 测试运行器；由根目录 `make test` 调用。 |
| `vendor/REIMU/` | REIMU 模拟器（Git 子模块）。 |
| `config.mk` | 测试运行器调用的构建、语义检查、IR、代码生成和执行命令。 |

## 获取子模块

克隆仓库后初始化子模块：

```sh
git submodule update --init --recursive
```

## 构建 Parser 库

项目使用 CMake 构建静态库 `rx_parser`，并通过 `vendor/antlr4-cpp` 提供 ANTLR C++ runtime：

```sh
cmake -S . -B build
cmake --build build
```

构建需要支持 C++23 的编译器和 CMake 3.15 或更新版本。使用方式、入口规则、parse tree 遍历以及错误处理详见 [`docs/parser.md`](docs/parser.md)。文法定义位于 [`grammar/`](grammar/)。

## 运行测试

测试命令在 [`config.mk`](config.mk) 中配置，根目录 Makefile 调用 Python 测试运行器。当前配置以 `rustc` 作为参考编译器；接入自己的编译器时，请按需修改 `BUILD`、`SEMANTIC`、`IR`、`CODEGEN` 和 `RUN` 命令。

运行测试前，需要准备 Python 3、xmake、Rust 工具链及 `riscv32im-unknown-none-elf` target；IR 测试还需要 `clang-22`。REIMU 首次构建命令如下：

```sh
rustup target add riscv32im-unknown-none-elf
xmake f -y -P vendor/REIMU -m release -o target/reimu
xmake -y -P vendor/REIMU
```

运行全部测试：

```sh
make test
```

常用筛选方式：

```sh
make test FILTER=official:semantic
make test FILTER=custom
make test STAGE=ir
make test VERBOSE=true
```

`FILTER` 使用冒号分隔目录层级，多个选择用逗号分隔；`STAGE` 可设为 `semantic`、`ir`、`codegen` 或 `optimization`。其他选项（如编译和运行超时）可在 [`Makefile`](Makefile) 中查看。

## Rx 语言参考

- [Rx 语言规范](https://acmclasscourse-2025.github.io/rx-compiler-specification/)
- `references/`：课程参考资料
