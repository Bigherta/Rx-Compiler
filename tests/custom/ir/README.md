# 自定义 IR 测试

所有 IR 回归统一由根目录 `make test` 管理，不再提供独立 Python 测试脚本。
各分类目录保存静态 `.rx`、可选 `.in`、预期 `.out` 和 `manifest.json`。

## 运行

在仓库根目录执行：

```sh
make test FILTER=custom:ir
make test FILTER=custom:ir STAGE=ir
make test FILTER=custom:ir:array-ir STAGE=ir
make test FILTER=custom:ir:vec-clone-ir VERBOSE=true
```

本目录恰好保留 **200 个 IR 清单用例、200 个 `.rx` 文件**。
语义拒绝测试复用已有前端测试集，不在这里重复铺开。
`FILTER` 选择目录前缀，不支持文件名。单个源码可用
`./target/oj-build/src/rx_compiler --check path/to/case.rx` 检查语义。

需要 CMake、GCC 14、Python 3、`clang-22`、xmake 和已构建的 REIMU。
REIMU 首次构建：

```sh
xmake f -y -P vendor/REIMU -m release -o target/reimu
xmake -y -P vendor/REIMU
```

默认 `BUILD` 构建本项目编译器，并将已提交的 `vendor/runtime/runtime.s` 复制为共享运行时。
该汇编由产品 `runtime.c` 生成；修改 C 文件后运行 `make runtime` 并提交更新。
不要直接设置 `BUILD=` 来跳过它，否则每次运行新建的运行时文件为空。
命令模板在 `config.mk`；日志、原始 IR、汇编和实际输出位于 `target/tests/run-*/`。

## 覆盖与迁移边界

从原 18 个专项脚本中精选重点用例，删除相似常量、重复类型组合与冗余分支枚举，
保留求值顺序、短路/提前返回、嵌套控制流、聚合读写、递归比较、深克隆和 Vec 扩容。
预期结果来自原脚本的模型/断言，不是用待测编译器生成的输出。
标量与控制流的参数表保存在 `.in` 中，保留边界值与布尔组合；
Vec 修改还保留原固定种子的随机参考模型。
聚合结果由 Rx 测试入口逐字段/元素输出；克隆用例还验证修改后的原值。

| 类别 | 用例数 |
| --- | ---: |
| 基础入口与提前返回 | 2 |
| 标量 / Unit | 14 / 5 |
| 数组构造与下标 / 数组比较 | 20 / 14 |
| 结构体构造与字段 / 结构体比较 | 10 / 12 |
| 数组与结构体 Clone / Vec Clone | 10 / 10 |
| Box/Vec 基础 / Vec 比较 / Vec 修改 | 10 / 9 / 10 |
| 方法调用 | 14 |
| if / if 合流 | 6 / 12 |
| 短路逻辑 / 发散传播与求值顺序 | 10 / 10 |
| loop / while | 12 / 10 |
| **合计** | **200** |

合法程序使用同一条流程：本项目编译器 → LLVM IR → Clang `-O0` RV32IM 汇编 →
REIMU → 逐字节比较标准输出。崩溃、超时、无效 IR、
执行失败和输出差异都不能算作通过。不会调用参考 `rustc` 替代自有后端。

这是端到端行为回归，不等价于原脚本的所有白盒断言：不再逐条检查
`insertvalue`/GEP/PHI 数量、helper 调用图、精确容量、指针身份、
分配次数/字节数/对齐或分配器哨兵，也不再执行宿主机 `-O0/-O2` 组合。
Vec 输入改由合法 Rx `new/push` 构造，原用例名中的容量/布局描述保留用于追溯，
不表示能通过 Rx 强制构造或断言原 LLVM fixture 的精确内部状态。
有意发散的函数只生成代码，不从 `main` 调用；不新增对 Rx UB 的要求。

遗留 `runtime.c`/`runtime.s` 和 `operand_indices.cpp` 是旧测试的辅助资料，
不参与当前清单执行，不能替换产品运行时。

新增测试直接添加源码、独立预期输出和清单条目，不再创建专项运行脚本。
通用运行器和汇编处理工具位于 `scripts/`；官方测试仍位于 `tests/official/`。
