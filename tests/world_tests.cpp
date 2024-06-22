#include "identifiers.h"
#include "world.h"

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
    f32 x{0}, y{0};
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
            entities.emplace_back(world.spawn(t1, t2, t3, t4));
        }
    }
};
} // namespace

TEST_F(EmptyWorldTest, spawn_entity) {
    for (usize i{0}; i < num; ++i) {
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
    // Add checks that the components and everything is correct after
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
