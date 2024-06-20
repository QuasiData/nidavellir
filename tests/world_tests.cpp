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

class WorldTest : public testing::Test {
  protected:
    World world;

    T1 t1{.x = 1, .y = 1};
    T2 t2{.x = 2, .y = 2, .z = 2, .w = 2};
    T3 t3{.x = 4, .y = 4, .floats = {1, 2}};
    T4 t4{.x = 6, .y = 6, .message = "TestMessage"};

    static constexpr usize num{32};

    WorldTest() = default;
};
} // namespace

TEST_F(WorldTest, spawn_entity) {
    for (usize i{0}; i < num; ++i) {
        world.spawn(t1);
        world.spawn(t1, t2);
        world.spawn(t1, t2, t3);
        world.spawn(t1, t2, t3, t4);
    }
}
