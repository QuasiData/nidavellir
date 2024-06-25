#include "identifiers.h"
#include "world.h"

#include <stdexcept>

#include "gtest/gtest.h"

using namespace nid;

namespace {
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

static_assert(!std::is_trivially_copyable_v<NonTrivialCopy>);

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
    f64 x{0}, y{0};
    std::string message;
};

class EmptyWorldTest : public testing::Test {
  protected:
    World world;

    T1 t1{.x = 1, .y = 1};
    T2 t2{.x = 2, .y = 2, .z = 2, .w = 2};
    T3 t3{.x = 4, .y = 4, .floats = {1, 2}};
    T4 t4{.x = 6, .y = 6, .message = "TestMessage"};

    static constexpr usize num{32};

    EmptyWorldTest() = default;
};

class WorldTest : public testing::Test {
  protected:
    World world;
    std::vector<EntityId> entities;

    T1 t1{.x = 1, .y = 1};
    T2 t2{.x = 2, .y = 2, .z = 2, .w = 2};
    T3 t3{.x = 4, .y = 4, .floats = {1, 2}};
    T4 t4{.x = 6, .y = 6, .message = "TestMessage"};

    static constexpr usize num{32};

    WorldTest() {
        for (usize i{0}; i < num; ++i) {
            entities.emplace_back(world.spawn(t1));
            entities.emplace_back(world.spawn(t1, t2));
            entities.emplace_back(world.spawn(t1, t2, t3));
            entities.emplace_back(world.spawn(t1, t2, T3{.x = static_cast<f32>(i), .y = static_cast<f32>(i), .floats = {static_cast<f32>(i)}}, t4));
        }
    }
};
} // namespace

TEST_F(EmptyWorldTest, spawn_entity) {
    for (usize i{0}; i < num; ++i) {
        world.spawn();
        world.spawn(t1);
        world.spawn(t1, t2);
        world.spawn(t1, t2, t3);
        world.spawn(t1, t2, t3, t4);
    }
}

TEST_F(WorldTest, remove_entity) {
    auto ent1 = entities.at(16);
    EXPECT_NO_THROW(world.despawn(ent1));
    entities.erase(entities.begin() + 16);

    auto ent2 = entities.back();
    EXPECT_NO_THROW(world.despawn(ent2));
    entities.pop_back();

    auto ent3 = entities.front();
    EXPECT_NO_THROW(world.despawn(ent3));
    entities.erase(entities.begin());

    EXPECT_THROW(world.despawn(ent3), std::out_of_range);
    // TODO: Add checks that the components and everything is correct after
}

TEST_F(WorldTest, get_tup) {
    auto ent = world.spawn(t1, t2, t3, T4{.x = 20, .y = 30, .message = "GetTest"});
    auto [t_1, t_2, t_3, t_4] = world.get<T1, T2, T3, T4>(ent);
    EXPECT_EQ(t_4.x, 20);
    EXPECT_EQ(t_4.y, 30);
    EXPECT_EQ(t_4.message, "GetTest");

    t_4.message = "ChangedTestMessage";
    auto [i1, i2, i3, t_42] = world.get<T1, T2, T3, T4>(ent);
    EXPECT_EQ(t_42.message, "ChangedTestMessage");
}

TEST_F(WorldTest, get_tup_single) {
    auto ent = world.spawn(t1, t2, t3, T4{.x = 20, .y = 30, .message = "GetTest"});
    auto [t_1, t_2, t_3, t_4] = world.get<T1, T2, T3, T4>(ent);
    EXPECT_EQ(t_4.x, 20);
    EXPECT_EQ(t_4.y, 30);
    EXPECT_EQ(t_4.message, "GetTest");

    t_4.message = "ChangedTestMessage";
    auto& t_42 = world.get<T4>(ent);
    EXPECT_EQ(t_42.message, "ChangedTestMessage");
}

TEST_F(WorldTest, get_single_tup) {
    auto ent = world.spawn(t1, t2, t3, T4{.x = 20, .y = 30, .message = "GetTest"});
    auto& t_4 = world.get<T4>(ent);
    EXPECT_EQ(t_4.x, 20);
    EXPECT_EQ(t_4.y, 30);
    EXPECT_EQ(t_4.message, "GetTest");

    t_4.message = "ChangedTestMessage";
    auto [i1, i2, i3, t_42] = world.get<T1, T2, T3, T4>(ent);
    EXPECT_EQ(t_42.message, "ChangedTestMessage");
}

TEST_F(WorldTest, get_single_single) {
    auto ent = world.spawn(t1, t2, t3, T4{.x = 20, .y = 30, .message = "GetTest"});
    auto& t_4 = world.get<T4>(ent);
    EXPECT_EQ(t_4.x, 20);
    EXPECT_EQ(t_4.y, 30);
    EXPECT_EQ(t_4.message, "GetTest");

    t_4.message = "ChangedTestMessage";
    auto& t_42 = world.get<T4>(ent);
    EXPECT_EQ(t_42.message, "ChangedTestMessage");
}

TEST_F(WorldTest, get_comp_not_found) {
    auto ent = world.spawn(t2, t3, T4{.x = 20, .y = 30, .message = "GetTest"});
    EXPECT_THROW([[maybe_unused]] auto t_1 = world.get<T1>(ent), std::out_of_range);
}

TEST_F(WorldTest, add) {
    const auto ent = world.spawn(t1, t2, t3);
    world.add(ent, t4);
    const auto& t44 = world.get<T4>(ent);
    EXPECT_EQ(t44.message, t4.message);

    const auto ent2 = world.spawn();
    world.add(ent2, t1, t2, t3, t4);
    const auto& [t_1, t_2, t_3, t_4] = world.get<T1, T2, T3, T4>(ent2);
    EXPECT_EQ(t_1.x, t1.x);
    EXPECT_EQ(t_2.x, t2.x);
    EXPECT_EQ(t_3.floats, t3.floats);
    EXPECT_EQ(t_4.message, t4.message);
}

TEST_F(WorldTest, add_extend) {
    const auto ent2 = world.spawn();
    T1 test_1{.x = 1000, .y = 1000};
    T2 test_2{.x = 2000, .y = 2000, .z = 2000, .w = 2000};
    world.add(ent2, test_1, test_2);
    const auto& [t_1, t_2] = world.get<T1, T2>(ent2);
    EXPECT_EQ(t_1.x, test_1.x);
    EXPECT_EQ(t_2.x, test_2.x);
    T3 test_3{.x = 123, .y = 123, .floats = {1, 123, 321321}};
    T4 test_4{.x = 32, .y = 51, .message = "dsadagasdmkw"};
    world.add(ent2, test_3, test_4);
    const auto& [t_11, t_22, t_3, t_4] = world.get<T1, T2, T3, T4>(ent2);
    EXPECT_EQ(t_11.x, test_1.x);
    EXPECT_EQ(t_22.x, test_2.x);
    EXPECT_EQ(t_3.floats, test_3.floats);
    EXPECT_EQ(t_4.message, test_4.message);
}

TEST_F(WorldTest, add_overwrite_partial) {
    T1 test_1{.x = 1000, .y = 1000};
    T2 test_2{.x = 2000, .y = 2000, .z = 2000, .w = 2000};
    T3 test_3{.x = 123, .y = 123, .floats = {1, 123, 321321}};
    T4 test_4{.x = 32, .y = 51, .message = "dsadagasdmkw"};
    const auto ent2 = world.spawn(t1, t2);
    const auto& [t_11, t_21] = world.get<T1, T2>(ent2);
    EXPECT_EQ(t_11.x, t1.x);
    EXPECT_EQ(t_21.x, t2.x);

    world.add(ent2, test_1, test_2, test_3, test_4);
    const auto& [t_12, t_22, t_32, t_42] = world.get<T1, T2, T3, T4>(ent2);
    EXPECT_EQ(t_12.x, test_1.x);
    EXPECT_EQ(t_22.x, test_2.x);
    EXPECT_EQ(t_32.floats, test_3.floats);
    EXPECT_EQ(t_42.message, test_4.message);
}

TEST_F(WorldTest, add_overwrite_multiple) {
    T1 test_1{.x = 1000, .y = 1000};
    T2 test_2{.x = 2000, .y = 2000, .z = 2000, .w = 2000};
    T3 test_3{.x = 123, .y = 123, .floats = {1, 123, 321321}};
    T4 test_4{.x = 32, .y = 51, .message = "dsadagasdmkw"};

    for (usize i{0}; i < 100; ++i) {
        world.add(entities[3], test_1, test_2, test_3, test_4);
    }
}

TEST_F(WorldTest, add_unseen_comp) {
    for (usize i{0}; i < 100; ++i) {
        world.add(entities[3], std::string("A string"), std::vector<int>(2), 2, double{2.2});
    }
}

TEST_F(WorldTest, remove_and_add) {
    T1 test_1{.x = 1000, .y = 1000};
    T2 test_2{.x = 2000, .y = 2000, .z = 2000, .w = 2000};
    T3 test_3{.x = 123, .y = 123, .floats = {1, 123, 321321}};
    T4 test_4{.x = 32, .y = 51, .message = "dsadagasdmkw"};
    auto ent = world.spawn(test_1, test_2, test_3, test_4);
    world.remove<T3>(ent);

    world.add(ent, test_3);
    auto& t_33 = world.get<T3>(ent);
    EXPECT_EQ(t_33.floats, test_3.floats);
}

TEST_F(WorldTest, remove_multi_add) {
    T1 test_1{.x = 1000, .y = 1000};
    T2 test_2{.x = 2000, .y = 2000, .z = 2000, .w = 2000};
    T3 test_3{.x = 123, .y = 123, .floats = {1, 123, 321321}};
    T4 test_4{.x = 32, .y = 51, .message = "dsadagasdmkw"};
    auto ent = world.spawn(test_1, test_2, test_3, test_4);
    world.remove<T1, T2, T3, T4>(ent);

    EXPECT_NO_THROW(world.add(ent, test_1, test_2, test_3, test_4));
}