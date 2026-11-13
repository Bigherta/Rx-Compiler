/* Reference I/O and program-lifetime heap allocation for RV32/ILP32. */
typedef int int32_t;
typedef unsigned int uint32_t;

/* Compile-time size checks: a negative array length makes compilation fail. */
typedef char rx_int32_is_four_bytes[(sizeof(int32_t) == 4) ? 1 : -1];
typedef char rx_uint32_is_four_bytes[(sizeof(uint32_t) == 4) ? 1 : -1];

extern int printf(const char *format, ...);
extern int scanf(const char *format, ...);
extern void *malloc(uint32_t size);

/*
 * Valid course allocations have nonzero size and alignment 1 or 4. REIMU's
 * malloc result satisfies both alignments. Objects are not individually freed.
 */
void *__rx_alloc(uint32_t size, uint32_t align) {
  (void)align;
  return malloc(size);
}

int32_t get_i32(void) {
  int32_t value;
  (void)scanf("%d", &value);
  return value;
}

void print_i32(int32_t value) { (void)printf("%d", value); }

void println_i32(int32_t value) { (void)printf("%d\n", value); }
