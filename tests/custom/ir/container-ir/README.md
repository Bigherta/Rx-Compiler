# Box 与 Vec 基础内建函数 IR 测试

```sh
make test FILTER=custom:ir:container-ir STAGE=ir
```

精选 10 项，覆盖 Box 保存 bool、数组/嵌套结构体、可变引用和空 Vec；
参数仅求值一次；嵌套 Box、含 Box 的结构体及共享引用的克隆独立性；
Vec 的 len、is_empty 和按值传参。

非空 Vec 改用 Rx 的 `new/push` 构造。所有用例使用产品运行时在 RV32/REIMU
执行，不再使用宿主机的额外空间分配器，也不直接断言分配次数、大小或对齐。
运行流程、日志及迁移边界见[上级说明](../README.md)。
