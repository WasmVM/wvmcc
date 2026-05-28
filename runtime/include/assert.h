// M2-15: <assert.h>.
//
// `-DNDEBUG` turns assert into a no-op. Otherwise we call the failure
// handler with the source expression, file, line, and function name.
#ifndef _WVMCC_ASSERT_H
#define _WVMCC_ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
extern _Noreturn void __wvmcc_assert_fail(const char *expr, const char *file, int line, const char *func);
#define assert(expr) \
    ((void)((expr) || (__wvmcc_assert_fail(#expr, __FILE__, __LINE__, __func__), 0)))
#endif

#endif // _WVMCC_ASSERT_H
