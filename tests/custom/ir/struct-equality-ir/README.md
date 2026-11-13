# 结构体相等比较 IR 测试

```sh
make test FILTER=custom:ir:struct-equality-ir STAGE=ir
```

精选 12 项，覆盖 `==`/`!=`、空结构体、多层引用、非字母序字段、
bool、嵌套结构体、数组/Vec/Box/引用字段、结构体数组及 Vec。
也检查连续比较的组合结果。
递归用例包括 `Vec<Self>`、`Vec<Box<Self>>` 和相互递归结构体，
检查深层值和长度不等。

输入由合法 Rx 构造，不再强制设置 LLVM fixture 中的精确容量，
也不单独匹配 extractvalue 或 helper 定义。
运行流程、日志及迁移边界见[上级说明](../README.md)。
