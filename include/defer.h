#pragma once

#if defined(__clang__)
#define _DEFER_CONCAT(a, b) a##b
#define _DEFER_NAME(a, b) _DEFER_CONCAT(a, b)

static inline void _defer_cleanup(void (^*block)(void)) {
  if (*block)
    (*block)();
}

#define defer                                                                  \
  __attribute__((unused)) void (^_DEFER_NAME(_defer_var_, __COUNTER__))(void)  \
      __attribute__((cleanup(_defer_cleanup))) = ^

#elif defined(__GNUC__)

#define defer _DEFER(__COUNTER__)
#define _DEFER(N) __DEFER(N)
#define __DEFER(N) ___DEFER(__DEFER_FUNC_##N, __DEFER_VAR_##N)

#define ___DEFER(F, V)                                                         \
  auto void F(void *);                                                         \
  __attribute__((cleanup(F))) int V __attribute__((unused));                   \
  auto void F(void *_dummy_ptr)

#else

// Runtime error for unsupported compilers
#define defer assert(!"unsupported compiler");

#endif
