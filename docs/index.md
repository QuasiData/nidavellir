# Nidavellir

## Components
Components are any C++ type that satisfied the 'Component' concept. Which roughly means the component needs to be moveable and destructible.
```cpp
struct Point {
    float x, y;
};

struct Person {
    int age;
    std::string name;
};

struct Polygon {
    // is_relocatable is used to tell nidavellir that this
    // component can be bitwise-copied when relocated.
    // The stale copy will then be forgotten and the destructor will not be called.
    using is_relocatable = void;
    std::vector<Point> vertices;
};
```

## Worlds
Worlds store entites and components and allows spawning entities, adding and removing components etc.
```cpp
#include "nidavellir.h"

nid::World world;
```

## Entities
Entities are nothing more than unique ids used for storing components.
```cpp
#include "nidavellir.h"

struct Point {
    float x, y;
};

struct Person {
    int age;
    std::string name;
};

nid::World world;
auto entity = world.spawn(
    Point{.x = 10, .y = 10},
    Person{.age = 123, .name = "Odin"}
);

auto& [point, person] = world.get<Point, Person>(entity);
```

## Example
```cpp
#include "nidavellir.h"

struct Point {
    float x, y;
};

struct Person {
    int age;
    std::string name;
};

struct Polygon {
    using is_relocatable = void;
    std::vector<Point> vertices;
};

struct Hand {
    int fingers{5};
};

int main() {
    nid::World world;

    // Spawn an entity with a Point, Person and Polygon component.
    auto entity = world.spawn(
        Point{.x = 10, .y = 10},
        Person{.age = 2199, .name = "Brock"},
        Polygon{.vertices = {Point{.x = 2, .y = 2}}}
    );

    // Add a double and a Hand component to the entity.
    world.add(entity, double{3.2}, Hand{});

    // Get single or multiple components.
    auto& polygon = world.get<Polygon>(entity);
    auto& [point, person] = world.get<Point, Person>(entity);

    // Remove the Point, Hand and double components.
    world.remove<Point, Hand, double>(entity);

    // Destroy the entity and all its components.
    world.despawn(entity);

    return 0;
}


```
