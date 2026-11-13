# 结构体与数组 Clone IR 测试

```sh
make test FILTER=custom:ir:clone-ir STAGE=ir
```

精选 10 项，覆盖仅派生 Clone 的结构体、整数/bool 字段、嵌套结构体、
数组字段、结构体数组、零长度字段、共享/多层引用及原值独立性。
也覆盖接收者仅求值一次，以及同名固有方法
不替代字段的派生 Clone。

聚合结果由 Rx 入口逐字段输出，不再断言 helper 调用图、具体指令顺序或复用次数。
运行流程、日志及迁移边界见[上级说明](../README.md)。
