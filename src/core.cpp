#include "core.h"

#include <iostream>

namespace nid {
auto nidavellir_assert(const char* expr_str, bool expr, const char* file, int line, const char* msg) -> void {
    if (!expr) {
        std::cerr << "Assert failed:\t" << msg << "\n"
                  << "Expected:\t" << expr_str << "\n"
                  << "Source:\t\t" << file << ", line " << line << "\n";
        abort();
    }
}
} // namespace nid