/* Test-only driver: generated main calls println_i32(0); run the LLVM fixture
 * instead and print its result. This is not the compiler's I/O runtime.
 */
extern int printf(const char *format, ...);
extern unsigned __array_equality_test(void);

void println_i32(int unused) {
  (void)unused;
  printf("%u\n", __array_equality_test());
}
