#include "core.h"
#include "nidavellir.h"
#include <iostream>

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

    /*
     */
    for (usize i{0}; i < num; ++i) {
        entities.emplace_back(world.spawn(t1));
        entities.emplace_back(world.spawn(t1, t2));
        entities.emplace_back(world.spawn(t1, t2, t3));
        entities.emplace_back(world.spawn(t1, t2, t3, t4));
    }

    EntityId id{};
    for (usize i{0}; i < 100; ++i) {
        id = world.spawn();
        world.add(id, t1);

        id = world.spawn();
        world.add(id, t1, t2);

        id = world.spawn(t1);
        world.add(id, t2);

        id = world.spawn(t1);
        world.add(id, t1, t2);

        id = world.spawn(t1, t2);
        world.add(id, t1);

        id = world.spawn(t1, t2);
        world.add(id, t1, t2);
    }

    world.query<T1, T2>().run([](usize len, T1* t1, T2* t2) {
        std::cout << len << "\n";
        for (usize i{0}; i < len; ++i) {
            // std::cout << t1[0].x << "\n";
            // std::cout << t2[0].z << "\n";
        }
    });

    return 0;
}
