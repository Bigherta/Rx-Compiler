# Test-only adapter: REIMU supplies printf, not the Rx println_i32 symbol.
    .text
    .globl println_i32
println_i32:
    mv a1, a0
    la a0, .Lscalar_newline_format
    tail printf

    .data
.Lscalar_newline_format:
    .string "%d\n"
