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

OJ 的 `BUILD` 配置启用 `RX_USE_SYSTEM_ANTLR=ON`，直接链接评测环境预装的 ANTLR C++ runtime，不编译 `vendor/antlr4-cpp`。配置时会打印库版本和路径；当前生成代码要求 runtime 为 **4.13.x**，缺少开发包或版本不兼容时会立即报错，不会退回耗时的源码构建。非标准安装位置可通过 `RX_ANTLR_INCLUDE_DIR` 和 `RX_ANTLR_LIBRARY` 指定。

本地默认仍使用随仓库携带的 runtime；也可显式设置 `-DRX_USE_SYSTEM_ANTLR=OFF`。`RX_FAST_BUILD=ON` 可降低编译器自身的构建耗时。

## 运行测试

测试命令在 [`config.mk`](config.mk) 中配置，根目录 Makefile 调用 Python 测试运行器。当前 `BUILD`、`SEMANTIC` 使用本项目的 C++ 编译器和系统 ANTLR runtime；`IR`、`CODEGEN` 仍是参考 `rustc` 命令，尚未接入自己的后端。本地若使用上述 `build` 目录中的随仓库 runtime 构建产物，可运行 `make test BUILD= SEMANTIC='./build/src/rx_compiler {source}' FILTER=official:semantic STAGE=semantic`。

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
