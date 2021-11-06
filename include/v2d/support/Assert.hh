#pragma once

#ifndef NDEBUG
#define V2D_ASSERTIONS
#define V2D_ASSERTIONS_PEDANTIC
#endif

#define V2D_STRINGIFY_A(x) #x
#define V2D_STRINGIFY(x) V2D_STRINGIFY_A(x)
#define V2D_ENSURE(expr, ...)                                                                                          \
    static_cast<bool>(expr)                                                                                            \
        ? static_cast<void>(0)                                                                                         \
        : v2d::fatal_error("Assertion '" #expr "' failed at " __FILE__ ":" V2D_STRINGIFY(__LINE__), ##__VA_ARGS__)

#ifdef V2D_ASSERTIONS
#define V2D_ASSERT(expr, ...) V2D_ENSURE(expr, ##__VA_ARGS__)
#else
#define V2D_ASSERT(expr, ...)
#endif
#ifdef V2D_ASSERTIONS_PEDANTIC
#define V2D_ASSERT_PEDANTIC(expr, ...) V2D_ENSURE(expr, ##__VA_ARGS__)
#else
#define V2D_ASSERT_PEDANTIC(expr, ...)
#endif

#define V2D_ASSERT_NOT_REACHED(...) V2D_ASSERT(false, ##__VA_ARGS__)
#define V2D_ENSURE_NOT_REACHED(...) V2D_ENSURE(false, ##__VA_ARGS__)

namespace v2d {

[[noreturn]] void fatal_error(const char *error, const char *note = nullptr);

} // namespace v2d
