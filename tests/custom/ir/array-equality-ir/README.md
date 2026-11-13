# 数组相等比较 IR 测试

```sh
make test FILTER=custom:ir:array-equality-ir
```

精选 14 项，覆盖：

- i32/u32/bool 的 `==`/`!=`、整数边界、首尾不等。
- 二维/三维数组的深层不等。
- 可变引用元素按目标值比较；不同地址的相同值应相等。
- 临时值、原对象保留、操作数从左至右各求值一次及提前返回。

输入改由 Rx 构造，直接比较输出，不再追加 LLVM fixture 或使用测试分配器。
运行流程、日志及迁移边界见[上级说明](../README.md)。

`operand_indices.cpp` 保留为 Operand 构造接口的历史单元测试资料，
不参与本目录的清单回归；`runtime.c` 也不参与当前执行。
