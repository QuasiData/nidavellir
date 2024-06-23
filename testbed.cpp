#include "nidavellir.h"

using namespace nid;

namespace {
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

auto main() -> int {
    std::vector<EntityId> entities;

    T1 t1{.x = 1, .y = 1};
    T2 t2{.x = 2, .y = 2, .z = 2, .w = 2};
    T3 t3{.x = 4, .y = 4, .floats = {1, 2}};
    T4 t4{.x = 6, .y = 6, .message = "TestMessage"};

    static constexpr usize num{32};
    World world;

    for (usize i{0}; i < num; ++i) {
        entities.emplace_back(world.spawn(t1));
        entities.emplace_back(world.spawn(t1, t2));
        entities.emplace_back(world.spawn(t1, t2, t3));
        entities.emplace_back(world.spawn(t1, t2, t3, t4));
    }

    T1 t11{.x = 1, .y = 1};
    T2 t21{.x = 2, .y = 2, .z = 2, .w = 2};
    T3 t31{.x = 3, .y = 3, .floats = {1, 2, 3}};
    T4 t41{.x = 4, .y = 4, .message = "1234"};

    for (usize i{0}; i < 100000000; ++i) {
        world.add(entities[3], t11, t21, t31, t41);
    }

    return 0;
}
