#include <v2d/support/Assert.hh>

#include <cstdio>
#include <cstdlib>

namespace v2d {

[[noreturn]] void fatal_error(const char *error, const char *note) {
    std::fprintf(stderr, "%s\n", error);
    if (note != nullptr) {
        std::fprintf(stderr, "=> %s\n", note);
    }
    std::abort();
}

} // namespace v2d
