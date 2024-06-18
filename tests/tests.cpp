#include "archetype.h"
#include "comp_type_info.h"
#include "nidavellir.h"

#include "gtest/gtest.h"
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <utility>
#include <vector>
#include <random>

using namespace nid;

namespace {
template<typename... Ts>
[[nodiscard]] auto get_infos() -> CompTypeList {
    CompTypeList lst;
    lst.reserve(sizeof...(Ts));

    (lst.emplace_back(nid::get_component_info<Ts>()), ...);
    return lst;
}

struct T1 {
    f32 x{0}, y{0};
};

struct T2 {
    f32 x{0}, y{0}, z{0}, w{0};
};

struct T3 {
    f32 x{0}, y{0};
    std::vector<f32> floats;
};

struct T4 {
    f32 x{0}, y{0};
    std::string message;
};
} // namespace

TEST(Archetype, Construct) {
    auto lst = get_infos<int, float, double>();
    sort_component_list(lst);

    EXPECT_NO_THROW(auto arch = Archetype(lst));
}

TEST(Archetype, emplace_back_once) {
    auto lst = get_infos<T1, T2, T3>();
    std::vector<usize> indices = {0, 1, 2};
    sort_component_list(lst, indices);

    auto arch = Archetype(lst);
    const auto col = arch.emplace_back(indices, T1{.x = 1, .y = 1}, T2{.x = 2, .y = 2, .z = 2, .w = 2}, T3{.x = 3, .y = 3, .floats = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}});
    const auto& [t1_x, t1_y] = arch.get_component<T1>(col, indices[0]);
    const auto& [t2_x, t2_y, t2_z, t2_w] = arch.get_component<T2>(col, indices[1]);
    const auto& [t3_x, t3_y, floats] = arch.get_component<T3>(col, indices[2]);

    EXPECT_EQ(1, t1_x);
    EXPECT_EQ(1, t1_y);

    EXPECT_EQ(2, t2_x);
    EXPECT_EQ(2, t2_y);
    EXPECT_EQ(2, t2_z);
    EXPECT_EQ(2, t2_w);

    EXPECT_EQ(3, t3_x);
    EXPECT_EQ(3, t3_y);
    const std::vector<f32> f = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(f, floats);
}

TEST(Archetype, emplace_back_and_reserve) {
    auto lst = get_infos<T1, T2, T3>();
    std::vector<usize> indices = {0, 1, 2};
    sort_component_list(lst, indices);

    constexpr usize num{512};
    auto arch = Archetype(lst);
    for (usize i{0}; i < num; ++i) {
        auto _ = arch.emplace_back(indices, T1{.x = 1, .y = 1}, T2{.x = 2, .y = 2, .z = 2, .w = 2}, T3{.x = 3, .y = 3, .floats = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}});
    }

    const auto& [t1_x, t1_y] = arch.get_component<T1>(num >> 2, indices[0]);
    const auto& [t2_x, t2_y, t2_z, t2_w] = arch.get_component<T2>(num >> 1, indices[1]);
    const auto& [t3_x, t3_y, floats] = arch.get_component<T3>(num >> 3, indices[2]);

    EXPECT_EQ(1, t1_x);
    EXPECT_EQ(1, t1_y);

    EXPECT_EQ(2, t2_x);
    EXPECT_EQ(2, t2_y);
    EXPECT_EQ(2, t2_z);
    EXPECT_EQ(2, t2_w);

    EXPECT_EQ(3, t3_x);
    EXPECT_EQ(3, t3_y);
    const std::vector<f32> f = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(f, floats);
}

TEST(Archetype, iterator) {
    auto lst = get_infos<T1, T2, T3, T4>();
    std::vector<usize> indices = {0, 1, 2, 3};
    sort_component_list(lst, indices);

    constexpr usize num{512};
    auto arch = Archetype(lst);
    for (usize i{0}; i < num; ++i) {
        auto _ = arch.emplace_back(indices,
            T1{.x = 1, .y = 1},
            T2{.x = 2, .y = 2, .z = 2, .w = 2},
            T3{.x = 3, .y = 3, .floats = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}},
            T4{.x = 4, .y = 5, .message = "TestMessage"});
    }
    std::random_device dev{};
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, num);

    const auto n = dist(rng);

    const std::vector t4s(arch.begin<T4>(indices[3]), arch.end<T4>(indices[3]));
    EXPECT_EQ(t4s[n].x, 4);
    EXPECT_EQ(t4s[n].y, 5);
    EXPECT_EQ(t4s[n].message, "TestMessage");
}

TEST(Archetype, remove) {
    auto lst = get_infos<T1, T2, T3, T4>();
    std::vector<usize> indices = {0, 1, 2, 3};
    sort_component_list(lst, indices);

    constexpr usize num{512};
    auto arch = Archetype(lst);
    for (usize i{0}; i < num - 1; ++i) {
        auto _ = arch.emplace_back(indices,
            T1{.x = 1, .y = 1},
            T2{.x = 2, .y = 2, .z = 2, .w = 2},
            T3{.x = 3, .y = 3, .floats = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}},
            T4{.x = 4, .y = 5, .message = "TestMessage"});
    }

    auto _ = arch.emplace_back(indices,
    T1{.x = 1, .y = 1},
    T2{.x = 2, .y = 2, .z = 2, .w = 2},
    T3{.x = 3, .y = 3, .floats = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}},
    T4{.x = 5, .y = 4, .message = "TheLastOne"});

    std::random_device dev{};
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, num - 1);

    const auto n = dist(rng);
    const auto l = arch.remove(n);

    const auto [x, y, message] = arch.get_component<T4>(n, indices[3]);

    EXPECT_EQ(x, 5);
    EXPECT_EQ(y, 4);
    EXPECT_EQ(message, "TheLastOne");
}
