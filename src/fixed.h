// Fixed-point math routines (signed 19.12)
// Largest positive value:  524287.999755859375
// Smallest positive value: 0.000244140625

#define FRAC_BITS      12
#define INT_MASK       0x7FFFF000 // 20 bits
#define FIXED_1        4096     // (1 << FRAC_BITS)
#define FIXED_255      1044480  // (255 << FRAC_BITS)
#define FIXED_HALF     2048     // (fixed_t)(0.5 * (float)(1L << FRAC_BITS) + 0.5)
#define FIXED_EPSILON  1

#define ROUND_FIXED_TO_INT(x) ((int)(x < 0 ? 0 : (x > FIXED_255) ? 255 : fixed_to_int(x + FIXED_HALF)))

typedef int32_t fixed_t;

static inline fixed_t int_to_fixed(int32_t x) {
  return x << FRAC_BITS;
}

static inline int32_t fixed_to_int(fixed_t x) {
  return x >> FRAC_BITS;
}

static inline fixed_t float_to_fixed(float x) {
  return ((fixed_t)((x) * (float)(1L << FRAC_BITS) + 0.5));
}

static inline float fixed_to_float(fixed_t x) {
  return ((float)((x) / (float)(1L << FRAC_BITS)));
}

#if defined(__GNUC__)
# if defined(__arm__)
static inline fixed_t fixed_mul(fixed_t x, fixed_t y) {
  fixed_t __hi, __lo, __result;
  __asm__ __volatile__("smull %0, %1, %3, %4\n\t"
                       "movs %0, %0, lsr %5\n\t" "adc %2, %0, %1, lsl %6":"=&r"(__lo), "=&r"(__hi), "=r"(__result)
                       :"%r"(x), "r"(y), "M"(FRAC_BITS), "M"(32 - (FRAC_BITS))
                       :"cc");
  return __result;
}
# elif defined(__i386__) || defined(__x86_64__)
// This improves fixed-point performance about 15-20% on x86
static inline fixed_t fixed_mul(fixed_t x, fixed_t y) {
  fixed_t __hi, __lo;
  __asm__ __volatile__("imull %3\n" "shrdl %4, %1, %0":"=a"(__lo), "=d"(__hi)
                       :"%a"(x), "rm"(y), "I"(FRAC_BITS)
                       :"cc");
  return __lo;
}
# elif defined(PADRE)           // Sparc ReadyNAS
static inline fixed_t fixed_mul(fixed_t x, fixed_t y) {
  fixed_t __hi, __lo, __result;
  __asm__ __volatile__(" nop\n"
                       " nop\n"
                       " smul %3, %4, %0\n"
                       " mov %%y, %1\n"
                       " srl %0, %5, %0\n"
                       " sll %1, %6, %1\n" " add %0, %1, %2\n":"=&r"(__lo), "=&r"(__hi), "=r"(__result)
                       :"%r"(x), "r"(y), "M"(FRAC_BITS), "M"(32 - (FRAC_BITS))
                       :"cc");
  return __result;
}
# else // Other gcc platform
static inline fixed_t fixed_mul(fixed_t x, fixed_t y) {
  return (fixed_t)(((int64_t)x * y) >> FRAC_BITS);
}
# endif
#elif defined(_MSC_VER)         // x86 Windows
static inline fixed_t fixed_mul(fixed_t x, fixed_t y) {
  enum {
    fracbits = FRAC_BITS
  };
  __asm {
	mov eax, x 
	imul y 
	shrd eax, edx, fracbits
  }
  // eax is returned automatically 
}
#else // Other non-gcc platform
static inline fixed_t fixed_mul(fixed_t x, fixed_t y) {
  return (fixed_t)(((int64_t)x * y) >> FRAC_BITS);
}
#endif

// XXX ARM version from http://me.henri.net/fp-div.html ?
static inline fixed_t fixed_div(fixed_t x, fixed_t y) {
  return (fixed_t)(((int64_t)x << FRAC_BITS) / y);
}

static inline fixed_t fixed_floor(fixed_t x) {
  return x & INT_MASK;
}
