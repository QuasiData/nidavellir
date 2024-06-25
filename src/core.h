#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace nid {
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using isize = std::conditional_t<sizeof(usize) == 8, i64, i32>;

auto nidavellir_assert(const char* expr_str, bool expr, const char* file, int line, const char* msg) -> void;

#ifndef NDEBUG
#define NIDAVELLIR_ASSERT(expr, msg) \
    nidavellir_assert(#expr, expr, __FILE__, __LINE__, msg)
#else
#define NIDAVELLIR_ASSERT(expr, msg) (void)0
#endif
} // namespace nid