// ReSharper disable CppNoDiscardExpression
#include "archetype.h"
#include "comp_type_info.h"

#include "gtest/gtest.h"
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

template<typename... Ts>
[[nodiscard]] auto get_sorted_infos() -> CompTypeList {
    CompTypeList lst;
    lst.reserve(sizeof...(Ts));

    (lst.emplace_back(nid::get_component_info<Ts>()), ...);
    sort_component_list(lst);
    return lst;
}

struct TrivialCopy {
    usize copies{0};
    usize moves{0};

    TrivialCopy() = default;
    ~TrivialCopy() = default;

    TrivialCopy([[maybe_unused]] const TrivialCopy& other) : copies(other.copies) {
        ++copies;
    }

    auto operator=([[maybe_unused]] const TrivialCopy& other) -> TrivialCopy& {
        copies = other.copies;
        ++copies;
        return *this;
    }

    TrivialCopy([[maybe_unused]] TrivialCopy&& other) noexcept : moves(other.moves) {
        ++moves;
    }

    auto operator=([[maybe_unused]] TrivialCopy&& other) noexcept -> TrivialCopy& {
        moves = other.moves;
        ++moves;
        return *this;
    }
};

struct NonTrivialCopy {
    std::vector<usize> vec{10};
    usize copies{0};
    usize moves{0};

    NonTrivialCopy() = default;
    ~NonTrivialCopy() = default;

    NonTrivialCopy([[maybe_unused]] const NonTrivialCopy& other) : copies(other.copies) {
        ++copies;
    }

    auto operator=([[maybe_unused]] const NonTrivialCopy& other) -> NonTrivialCopy& {
        copies = other.copies;
        ++copies;
        return *this;
    }

    NonTrivialCopy([[maybe_unused]] NonTrivialCopy&& other) noexcept : moves(other.moves) {
        ++moves;
    }

    auto operator=([[maybe_unused]] NonTrivialCopy&& other) noexcept -> NonTrivialCopy& {
        moves = other.moves;
        ++moves;
        return *this;
    }
};

struct Relocatable {
    using is_relocatable = void;
    std::vector<usize> vec{10};
    usize copies{0};
    usize moves{0};

    Relocatable() = default;
    ~Relocatable() = default;

    Relocatable([[maybe_unused]] const Relocatable& other) : copies(other.copies) {
        ++copies;
    }

    auto operator=([[maybe_unused]] const Relocatable& other) -> Relocatable& {
        copies = other.copies;
        ++copies;
        return *this;
    }

    Relocatable([[maybe_unused]] Relocatable&& other) noexcept : moves(other.moves) {
        ++moves;
    }

    auto operator=([[maybe_unused]] Relocatable&& other) noexcept -> Relocatable& {
        moves = other.moves;
        ++moves;
        return *this;
    }
};

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

class ArchetypeTest : public testing::Test {
  protected:
    CompTypeList lst1 = get_sorted_infos<T1, T2>();
    std::vector<usize> row1;
    Archetype arch1;

    CompTypeList lst2 = get_sorted_infos<T2, T3>();
    std::vector<usize> row2;
    Archetype arch2;

    CompTypeList lst3 = get_sorted_infos<T3, T4>();
    std::vector<usize> row3;
    Archetype arch3;

    CompTypeList lste = get_sorted_infos<>();
    Archetype arche;

    CompTypeList lstcopy = get_sorted_infos<TrivialCopy, NonTrivialCopy, Relocatable>();
    std::vector<usize> row_copy;
    Archetype archcopy;

    T1 t1{.x = 1, .y = 1};
    T2 t2{.x = 2, .y = 2, .z = 2, .w = 2};
    T3 t3{.x = 4, .y = 4, .floats = {1, 2}};
    T4 t4{.x = 6, .y = 6, .message = "TestMessage"};

    static constexpr usize num{32};

    std::random_device dev;
    std::mt19937 rng{dev()};
    std::uniform_int_distribution<std::mt19937::result_type> dist{1, num - 2};

    ArchetypeTest()
        : arch1(lst1), arch2(lst2), arch3(lst3), arche(lste), archcopy(lstcopy) {

        const auto l1 = get_infos<T1, T2>();
        const auto l2 = get_infos<T2, T3>();
        const auto l3 = get_infos<T3, T4>();
        const auto lc = get_infos<TrivialCopy, NonTrivialCopy, Relocatable>();

        for (usize i{0}; i < l1.size(); ++i) {
            for (usize j{0}; j < lst1.size(); ++j) {
                if (l1[i].id == lst1[j].id) {
                    row1.emplace_back(j);
                    break;
                }
            }
        }

        for (usize i{0}; i < l2.size(); ++i) {
            for (usize j{0}; j < lst2.size(); ++j) {
                if (l2[i].id == lst2[j].id) {
                    row2.emplace_back(j);
                    break;
                }
            }
        }

        for (usize i{0}; i < l3.size(); ++i) {
            for (usize j{0}; j < lst3.size(); ++j) {
                if (l3[i].id == lst3[j].id) {
                    row3.emplace_back(j);
                    break;
                }
            }
        }

        for (usize i{0}; i < lc.size(); ++i) {
            for (usize j{0}; j < lstcopy.size(); ++j) {
                if (lc[i].id == lstcopy[j].id) {
                    row_copy.emplace_back(j);
                    break;
                }
            }
        }

        assert(l1[0].id == lst1[row1[0]].id);
        assert(l2[0].id == lst2[row2[0]].id);
        assert(l3[0].id == lst3[row3[0]].id);
        assert(lc[0].id == lstcopy[row_copy[0]].id);

        for (usize i{0}; i < num; ++i) {
            [[maybe_unused]] auto _1 = arch1.emplace_back(row1, t1, t2);
            [[maybe_unused]] auto _2 = arch2.emplace_back(row2, t2, t3);
            [[maybe_unused]] auto _3 = arch3.emplace_back(row3, t3, t4);
            [[maybe_unused]] auto _e = arche.emplace_back(std::vector<usize>{});
            [[maybe_unused]] auto _c = archcopy.emplace_back(row_copy, TrivialCopy{}, NonTrivialCopy{}, Relocatable{});
        }
    }
};
} // namespace

TEST_F(ArchetypeTest, get1) {
    const auto& [x, y] = arch1.get_component<T1>(0, row1[0]);
    const auto& [x2, y2, z2, w2] = arch1.get_component<T2>(dist(rng), row1[1]);
    const auto& [x3, y3, z3, w3] = arch1.get_component<T2>(num - 1, row1[1]);

    EXPECT_EQ(x, t1.x);
    EXPECT_EQ(y, t1.y);
    EXPECT_EQ(x2, t2.x);
    EXPECT_EQ(y2, t2.y);
    EXPECT_EQ(z2, t2.z);
    EXPECT_EQ(w2, t2.w);
    EXPECT_EQ(x3, t2.x);
    EXPECT_EQ(y3, t2.y);
    EXPECT_EQ(z3, t2.z);
    EXPECT_EQ(w3, t2.w);
}

TEST_F(ArchetypeTest, get2) {
    const auto& [x, y, z, w] = arch2.get_component<T2>(0, row2[0]);
    const auto& [x2, y2, floats2] = arch2.get_component<T3>(dist(rng), row2[1]);
    const auto& [x3, y3, floats3] = arch2.get_component<T3>(num - 1, row2[1]);

    EXPECT_EQ(x, t2.x);
    EXPECT_EQ(y, t2.y);
    EXPECT_EQ(z, t2.z);
    EXPECT_EQ(w, t2.w);
    EXPECT_EQ(x2, t3.x);
    EXPECT_EQ(y2, t3.y);
    EXPECT_EQ(floats2, t3.floats);
    EXPECT_EQ(x3, t3.x);
    EXPECT_EQ(y3, t3.y);
    EXPECT_EQ(floats3, t3.floats);
}

TEST_F(ArchetypeTest, get3) {
    const auto& [x, y, floats] = arch3.get_component<T3>(0, row3[0]);
    const auto& [x2, y2, message2] = arch3.get_component<T4>(dist(rng), row3[1]);
    const auto& [x3, y3, message3] = arch3.get_component<T4>(num - 1, row3[1]);

    EXPECT_EQ(x, t3.x);
    EXPECT_EQ(y, t3.y);
    EXPECT_EQ(floats, t3.floats);
    EXPECT_EQ(x2, t4.x);
    EXPECT_EQ(y2, t4.y);
    EXPECT_EQ(message2, t4.message);
    EXPECT_EQ(x3, t4.x);
    EXPECT_EQ(y3, t4.y);
    EXPECT_EQ(message3, t4.message);
}

TEST_F(ArchetypeTest, iterators1) {
    usize count{0};
    for (auto it = arch1.begin<T1>(row1[0]); it != arch1.end<T1>(row1[0]); ++it) {
        ++count;
        EXPECT_EQ(it->x, t1.x);
        EXPECT_EQ(it->y, t1.y);
    }
    EXPECT_EQ(count, num);
}

TEST_F(ArchetypeTest, iterators2) {
    usize count{0};
    for (auto it = arch2.begin<T3>(row2[1]); it != arch2.end<T3>(row2[1]); ++it) {
        ++count;
        EXPECT_EQ(it->x, t3.x);
        EXPECT_EQ(it->y, t3.y);
        EXPECT_EQ(it->floats, t3.floats);
    }
    EXPECT_EQ(count, num);
}

TEST_F(ArchetypeTest, iterators3) {
    usize count{0};
    for (auto it = arch3.begin<T4>(row3[1]); it != arch3.end<T4>(row3[1]); ++it) {
        ++count;
        EXPECT_EQ(it->x, t4.x);
        EXPECT_EQ(it->y, t4.y);
        EXPECT_EQ(it->message, t4.message);
    }
    EXPECT_EQ(count, num);
}

TEST_F(ArchetypeTest, remove1) {
    T1 l_t1{.x = 100, .y = 100};
    T2 l_t2{.x = 1000, .y = 1000, .z = 1000, .w = 1000};
    [[maybe_unused]] auto _1 = arch1.emplace_back(row1, l_t1, l_t2);
    [[maybe_unused]] auto _2 = arch1.remove(0);
    const auto& [x, y] = arch1.get_component<T1>(0, row1[0]);
    const auto& [x2, y2, z2, w2] = arch1.get_component<T2>(0, row1[1]);
    EXPECT_EQ(x, l_t1.x);
    EXPECT_EQ(y, l_t1.y);
    EXPECT_EQ(x2, l_t2.x);
    EXPECT_EQ(y2, l_t2.y);
    EXPECT_EQ(z2, l_t2.z);
    EXPECT_EQ(w2, l_t2.w);
}

TEST_F(ArchetypeTest, remove2) {
    T3 l_t3{.x = 100, .y = 100, .floats = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};
    [[maybe_unused]] auto _1 = arch2.emplace_back(row2, T2{.x = 1, .y = 1, .z = 1, .w = 1}, l_t3);
    [[maybe_unused]] auto _2 = arch2.remove(0);
    const auto& [x, y, floats] = arch2.get_component<T3>(0, row2[1]);
    EXPECT_EQ(x, l_t3.x);
    EXPECT_EQ(y, l_t3.y);
    EXPECT_EQ(floats, l_t3.floats);
}

TEST_F(ArchetypeTest, remove3) {
    const auto len = arche.len();
    [[maybe_unused]] auto _ = arche.remove(0);
    EXPECT_NE(len, arche.len());
}

TEST_F(ArchetypeTest, move_constructor) {
    EXPECT_EQ(arch1.len(), num);
    Archetype arch{std::move(arch1)};
    EXPECT_EQ(arch1.len(), 0);
    EXPECT_EQ(arch1.cap(), 0);
    EXPECT_EQ(arch.len(), num);
    const auto& [x, y] = arch.get_component<T1>(0, row1[0]);
    EXPECT_EQ(x, t1.x);
    EXPECT_EQ(y, t1.y);
}

TEST_F(ArchetypeTest, move_assign1) {
    EXPECT_EQ(arch1.len(), num);
    Archetype arch = std::move(arch1);
    EXPECT_EQ(arch1.len(), 0);
    EXPECT_EQ(arch1.cap(), 0);
    EXPECT_EQ(arch.len(), num);
    const auto& [x, y] = arch.get_component<T1>(0, row1[0]);
    EXPECT_EQ(x, t1.x);
    EXPECT_EQ(y, t1.y);
}

TEST_F(ArchetypeTest, move_assign2) {
    EXPECT_EQ(arch1.len(), num);
    arch2 = std::move(arch1);
    EXPECT_EQ(arch1.len(), 0);
    EXPECT_EQ(arch1.cap(), 0);
    EXPECT_EQ(arch2.len(), num);
    const auto& [x, y] = arch2.get_component<T1>(0, row1[0]);
    EXPECT_EQ(x, t1.x);
    EXPECT_EQ(y, t1.y);
}

TEST_F(ArchetypeTest, move_assign3) {
    EXPECT_NO_FATAL_FAILURE(Archetype arch = std::move(arche));
}

TEST_F(ArchetypeTest, move_assign4) {
    EXPECT_EQ(arch1.len(), num);
    arche = std::move(arch1);
    EXPECT_EQ(arch1.len(), 0);
    EXPECT_EQ(arch1.cap(), 0);
    EXPECT_EQ(arche.len(), num);
    const auto& [x, y] = arche.get_component<T1>(0, row1[0]);
    EXPECT_EQ(x, t1.x);
    EXPECT_EQ(y, t1.y);
}

TEST_F(ArchetypeTest, swap) {
    T3 l_t3{.x = 10, .y = 10, .floats = {1, 2, 3, 4}};
    T4 l_t4{.x = 20, .y = 20, .message = "SwapTest"};
    const auto col = arch3.emplace_back(row3, l_t3, l_t4);
    const auto rnd = dist(rng);
    arch3.swap(rnd, col);
    auto [x1, y1, floats] = arch3.get_component<T3>(rnd, row3[0]);
    EXPECT_EQ(x1, l_t3.x);
    EXPECT_EQ(y1, l_t3.y);
    EXPECT_EQ(floats, l_t3.floats);
    auto [x2, y2, message] = arch3.get_component<T4>(rnd, row3[1]);
    EXPECT_EQ(x2, l_t4.x);
    EXPECT_EQ(y2, l_t4.y);
    EXPECT_EQ(message, l_t4.message);
}

TEST_F(ArchetypeTest, remove_and_emplace) {
    EXPECT_EQ(arch3.len(), num);
    for (usize i{0}; i < num; ++i) {
        [[maybe_unused]] auto _ = arch3.remove(0);
    }
    EXPECT_EQ(arch3.len(), 0);
    for (usize i{0}; i < num; ++i) {
        [[maybe_unused]] auto _ = arch3.emplace_back(row3, t3, t4);
    }
    EXPECT_EQ(arch3.len(), num);
}

TEST_F(ArchetypeTest, swap_same_and_full) {
    T3 l_t3{.x = 10, .y = 10, .floats = {1, 2, 3, 4}};
    T4 l_t4{.x = 20, .y = 20, .message = "SwapTest"};
    const auto cap = arch3.cap();
    const auto len = arch3.len();
    for (usize i{0}; i < cap - len; ++i) {
        [[maybe_unused]] auto _ = arch3.emplace_back(row3, l_t3, l_t4);
    }
    EXPECT_EQ(arch3.cap(), arch3.len());
    arch3.swap(arch3.len() - 1, arch3.len() - 1);
    arch3.swap(arch3.len() - 1, 0);
}

TEST_F(ArchetypeTest, swap_empty) {
    arche.swap(0, arche.len() - 1);
}

TEST_F(ArchetypeTest, no_copies) {
    auto& t_c = archcopy.get_component<TrivialCopy>(0, row_copy[0]);
    auto& n_c = archcopy.get_component<NonTrivialCopy>(0, row_copy[1]);
    auto& r_c = archcopy.get_component<Relocatable>(0, row_copy[2]);
    EXPECT_EQ(t_c.copies, 0);
    EXPECT_EQ(t_c.moves, 1);

    EXPECT_EQ(n_c.copies, 0);
    EXPECT_GT(n_c.moves, 1);

    EXPECT_EQ(r_c.copies, 0);
    EXPECT_EQ(r_c.moves, 1);

    archcopy.swap(archcopy.len() - 1, 0);
    t_c = archcopy.get_component<TrivialCopy>(0, row_copy[0]);
    n_c = archcopy.get_component<NonTrivialCopy>(0, row_copy[1]);
    r_c = archcopy.get_component<Relocatable>(0, row_copy[2]);

    EXPECT_EQ(t_c.copies, 0);
    EXPECT_EQ(t_c.moves, 1);

    EXPECT_EQ(n_c.copies, 0);
    EXPECT_GT(n_c.moves, 1);

    EXPECT_EQ(r_c.copies, 0);
    EXPECT_EQ(r_c.moves, 1);
}
