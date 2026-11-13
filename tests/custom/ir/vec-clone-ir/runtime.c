/* Test-only allocator and driver; not the compiler's runtime implementation.
 * Avoid system headers so clang can compile this for bare-metal RV32 as well.
 */
typedef __SIZE_TYPE__ test_size_t;
typedef __UINTPTR_TYPE__ test_uintptr_t;
extern void *malloc(test_size_t size);
extern void *memset(void *pointer, int value, test_size_t size);
extern int printf(const char *format, ...);
extern unsigned __vec_clone_test(void);

enum { MAX_ALLOCATIONS = 256, GUARD_SIZE = 16, MAX_BYTES = 1048576 };
struct Allocation {
  unsigned char *data;
  unsigned requested, alignment, payload;
};
static struct Allocation allocations[MAX_ALLOCATIONS];
static unsigned count, invalid;

void *__rx_alloc(unsigned size, unsigned alignment) {
  if (count == MAX_ALLOCATIONS || size > MAX_BYTES ||
      (alignment != 1 && alignment != 4)) {
    invalid = 1;
    return malloc(64);
  }
  /* Native pointers may be wider than RV32. RV32 allocations have exact bounds;
   * native over-allocation does not change the recorded target size/alignment.
   * A zero-byte request is permitted for empty vectors, with readable guards.
   */
  unsigned payload = sizeof(void *) == 4 ? size : size * 4 + 64;
  unsigned char *raw = malloc((test_size_t)payload + 2 * GUARD_SIZE);
  if (!raw) {
    invalid = 1;
    return raw;
  }
  memset(raw, 0xd7, (test_size_t)payload + 2 * GUARD_SIZE);
  unsigned char *data = raw + GUARD_SIZE;
  memset(data, 0xa5, payload);
  allocations[count].data = data;
  allocations[count].requested = size;
  allocations[count].alignment = alignment;
  allocations[count].payload = payload;
  ++count;
  if ((test_uintptr_t)data % alignment)
    invalid = 1;
  return data;
}

unsigned __test_storage(void *data, unsigned capacity, unsigned stride,
                        unsigned alignment) {
  if (!capacity)
    return 1;
  if (!stride || capacity > 0xffffffffu / stride)
    return 0;
  for (unsigned i = 0; i < count; ++i) {
    if (allocations[i].data == data)
      return allocations[i].requested >= capacity * stride &&
             allocations[i].alignment == alignment;
  }
  return 0;
}

unsigned __test_alloc_count(void) { return count; }

unsigned __test_guards(void) {
  if (invalid)
    return 0;
  for (unsigned i = 0; i < count; ++i) {
    for (int offset = 0; offset < GUARD_SIZE; ++offset) {
      if (allocations[i].data[offset - GUARD_SIZE] != 0xd7 ||
          allocations[i].data[allocations[i].payload + offset] != 0xd7)
        return 0;
    }
  }
  return 1;
}

/* Generated main calls println_i32(0). This test-only adapter invokes the LLVM
 * fixture instead, then prints its diagnostic code: 1 means success.
 */
void println_i32(int unused) {
  (void)unused;
  printf("%u\n", __vec_clone_test());
}
