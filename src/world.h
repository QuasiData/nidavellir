// ReSharper disable CppUseStructuredBinding
#pragma once
#include "archetype.h"
#include "comp_type_info.h"
#include "identifiers.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory_resource>
#include <tuple>
#include <vector>

#include <ankerl/unordered_dense.h>

namespace nid {
/**
 * @class World
 * @brief A World which is the heart of the ECS.
 *
 * INFO
 *
 * \code{.cpp}
 * struct Point {
 *     int x, y;
 * };
 *
 * struct Vector {
 *     int x, y;
 * };
 *
 * Point p{.x = 10, .y = 10};
 * Vector v{.x = 20, .y = 20};
 *
 * World world;
 * EntityId entity = world.spawn(p);
 * world.add(entity, v);
 * // The entity now has the components Point and Vector with values from the variables p and v.
 *
 * world.add(entity, Vector{.x = 100, .y = 100});
 * // The entity will now have the same type of components but the value of the Vector component will be overwritten.
 *
 * world.remove<Point>(entity);
 * // The entity now only has a Vector component.
 *
 * // Most functions on the world accept an arbitrary number of types
 * EntityId entity2 = world.spawn(int{1}, float{32.4f}, std::string("TestString"), std::vector<int>{1, 2, 3, 4, 5, 6});
 *
 * world.add(entity2, double{2.3}, std::unordered_map<int, int>{});
 * \endcode
 */
class World {
    struct ArchetypeRecord {
        Archetype archetype;
        std::vector<EntityId> entities;
        ArchetypeId id;
    };

    struct EntityRecord {
        ArchetypeId archetype;
        usize col;
    };

    struct RowRecord {
        usize row;
    };

    using ArchetypeMap = ankerl::unordered_dense::map<ArchetypeId, RowRecord>;

    ankerl::unordered_dense::map<ArchetypeId, ArchetypeRecord> archetype_map;
    ankerl::unordered_dense::map<EntityId, EntityRecord> entity_map;

    ankerl::unordered_dense::map<ComponentId, ArchetypeMap> component_map;
    ankerl::unordered_dense::map<CompTypeList, ArchetypeId, TypeHash> type_map;

    CompTypeList scratch_component_buffer;

    ArchetypeId next_archetype_id{0};
    EntityId next_entity_id{0};

  public:
    /**
     * @brief Default constructor for World.
     */
    World() = default;

    /**
     * @brief Destructor for World.
     */
    ~World() = default;

    /**
     * @brief Deleted copy constructor.
     */
    World(const World&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     */
    auto operator=(const World&) = delete;

    /**
     * @brief Deleted move constructor.
     */
    World(World&&) = delete;

    /**
     * @brief Deleted move assignment operator.
     */
    auto operator=(World&&) = delete;

    /**
     * @brief Despawns an entity from the world.
     *
     * This function removes the specified entity from the world, along with all its associated components.
     * The entity must exist in the world; attempting to despawn a non-existent entity will throw a `std::out_of_range` exception.
     *
     * @param entity The ID of the entity to despawn.
     *
     * \code{.cpp}
     * ComponentType1 comp1;
     * ComponentType2 comp2;
     *
     * World world;
     * EntityId entity = world.spawn(comp1, comp2);
     * world.despawn(entity);
     * \endcode
     */
    auto despawn(EntityId entity) -> void;

    /**
     * @brief Spawns a new entity with the given components.
     * @tparam Ts The types of the components.
     * @param pack The components to add to the new entity.
     * @return The ID of the newly spawned entity.
     *
     * \code{.cpp}
     * ComponentType1 comp1;
     * ComponentType2 comp2;
     *
     * World world;
     * // Spawn an entity with the components comp1 and comp2.
     * EntityId entity1 = world.spawn(comp1, comp2);
     *
     * ComponentType3 comp3;
     * ComponentType4 comp4;
     *
     * World world;
     * // Spawn an entity with the components comp3 and comp4.
     * EntityId entity2 = world.spawn(comp3, comp4);
     * \endcode
     */
    template<Component... Ts>
    auto spawn(Ts&&... pack) -> EntityId {
        CompTypeList comp_ts = {get_component_info<Ts>()...};
        sort_component_list(comp_ts);

        auto& arch_rec = find_or_create_archetype(comp_ts);
        const auto col = arch_rec.archetype.emplace_back(std::forward<Ts>(pack)...);

        const auto new_entity_id = next_entity_id++;
        arch_rec.entities.push_back(new_entity_id);
        entity_map.insert({new_entity_id, EntityRecord{.archetype = arch_rec.id, .col = col}});

        return new_entity_id;
    }

    /**
     * @brief Gets the components of the specified types for a given entity.
     *
     * This function retrieves the components of the specified types for the given entity.
     * If only one component type is requested, a single reference is returned.
     * If multiple component types are requested, a tuple of references is returned.
     * Throws a `std::out_of_range` exception if the entity does not exist or if the specified components are not present on the entity.
     *
     * @tparam Ts The types of the components to get.
     * @param entity The ID of the entity.
     * @return A reference or tuple of references to the requested components.
     *
     * \code{.cpp}
     * ComponentType1 comp1;
     * ComponentType2 comp2;
     *
     * World world;
     * EntityId entity = world.spawn(comp1, comp2);
     *
     * // Get a single component
     * auto& component1 = world.get<ComponentType1>(entity);
     *
     * // Get multiple components
     * auto& [component1, component2] = world.get<ComponentType1, ComponentType2>(entity);
     * \endcode
     */
    template<Component... Ts>
    [[nodiscard]] auto get(const EntityId entity) -> decltype(auto) {
        const auto [arch_id, col] = entity_map.at(entity);
        auto& [arch, _1, _2] = archetype_map.at(arch_id);

        auto tup = std::tie(arch.get_component<Ts>(col)...);
        static_assert(std::same_as<decltype(tup), std::tuple<Ts&...>>);

        if constexpr (sizeof...(Ts) == 1) {
            return std::get<0>(tup);
        } else {
            return tup;
        }
    }

    /**
     * @brief Adds the specified components to a given entity.
     *
     * This function adds the specified components to the given entity.
     * If the entity already has a component of one of the supplied types, the existing component will be overwritten with the new one provided in the pack.
     * Throws a `std::out_of_range` exception if the entity does not exist.
     *
     * @tparam Ts The types of the components to add.
     * @param entity The ID of the entity.
     * @param pack The components to add.
     *
     * \code{.cpp}
     * struct Point {
     *     int x, y;
     * };
     *
     * struct Vector {
     *     int x, y;
     * };
     *
     * Point p{.x = 10, .y = 10};
     * Vector v{.x = 20, .y = 20};
     *
     * World world;
     * EntityId entity = world.spawn(p);
     * world.add(entity, v);
     * // The entity now has the components Point and Vector with values from the variables p and v.
     *
     * world.add(entity, Vector{.x = 100, .y = 100});
     * // The entity will now have the same type of components but the value of the Vector component will be overwritten.
     * \endcode
     */
    template<Component... Ts>
    auto add(const EntityId entity, Ts&&... pack) -> void {
        static_assert(sizeof...(Ts) > 0);
        NIDAVELLIR_ASSERT(scratch_component_buffer.size() == 0, "The scratch buffer has not been cleared");

        auto& [src_id, src_col] = entity_map.at(entity);
        const auto& entity_types = archetype_map.at(src_id).archetype.type();

        constexpr usize stack_buffer_size = sizeof(CompTypeInfo) * 64;
        std::array<u8, stack_buffer_size> buffer{};
        std::pmr::monotonic_buffer_resource resource(buffer.data(), stack_buffer_size);
        std::pmr::vector<CompTypeInfo> in_pack_types{&resource};
        std::pmr::vector<CompTypeInfo> not_in_pack_types{&resource};

        std::array<CompTypeInfo, sizeof...(Ts)> pack_infos = {get_component_info<Ts>()...};
        auto in_pack = [&](const CompTypeInfo& info1) {
            return std::ranges::find_if(pack_infos, [info1](const CompTypeInfo& info2) { return info1.id == info2.id; }) != pack_infos.end();
        };

        for (const auto& info : entity_types) {
            if (in_pack(info)) {
                in_pack_types.push_back(info);
            } else {
                not_in_pack_types.push_back(info);
            }
        }

        scratch_component_buffer.reserve(sizeof...(Ts) + not_in_pack_types.size());
        std::ranges::copy(pack_infos, std::back_inserter(scratch_component_buffer));
        std::ranges::copy(not_in_pack_types, std::back_inserter(scratch_component_buffer));
        sort_component_list(scratch_component_buffer);

        auto& [target_arch, target_entities, target_id] = find_or_create_archetype(scratch_component_buffer);
        auto& [src_arch, src_entities, _] = archetype_map.at(src_id);

        if (src_id == target_id) {
            src_arch.update(src_col, std::forward<Ts>(pack)...);
        } else {
            target_arch.prepare_push(1);
            const usize target_col{target_arch.len()};

            for (const auto& info : not_in_pack_types) {
                void* src_ptr = src_arch.get_raw(src_col, src_arch.get_row(info.id));
                void* dst_ptr = target_arch.get_raw(target_col, target_arch.get_row(info.id));

                info.move_ctor_dtor(dst_ptr, src_ptr, 1);
            }

            for (const auto& info : in_pack_types) {
                void* src_ptr = src_arch.get_raw(src_col, src_arch.get_row(info.id));

                info.dtor(src_ptr, 1);
            }

            target_arch.create(target_col, std::forward<Ts>(pack)...);

            target_entities.push_back(entity);
            target_arch.increase_size(1);

            const usize src_last_col{src_arch.len() - 1};
            if (src_col < src_last_col) {
                src_arch.swap(src_col, src_last_col);
                std::swap(src_entities[src_col], src_entities[src_last_col]);
                entity_map.at(src_entities[src_col]).col = src_col;
            }

            src_entities.pop_back();
            src_arch.decrease_size(1);

            src_id = target_id;
            src_col = target_col;
        }

        scratch_component_buffer.clear();
    }

    /**
     * @brief Removes the specified components from a given entity.
     *
     * This function removes the specified components from the given entity.
     * Throws a `std::out_of_range` exception if the entity does not exist.
     * If the entity does not have all of the specified components, this function will cause undefined behavior.
     *
     * @tparam Ts The types of the components to remove.
     * @param entity The ID of the entity.
     *
     * \code{.cpp}
     * ComponentType1 comp1;
     * ComponentType2 comp2;
     *
     * World world;
     * EntityId entity = world.spawn(comp1, comp2);
     * // The entity has the components ComponentType1 and ComponentType2
     *
     * world.remove<ComponentType2>(entity);
     * // The entity only has the component ComponentType1
     * \endcode
     */
    template<Component... Ts>
    auto remove(const EntityId entity) -> void {
        static_assert(sizeof...(Ts) > 0);
        NIDAVELLIR_ASSERT(scratch_component_buffer.size() == 0, "The scratch buffer has not been cleared");

        auto& [src_id, src_col] = entity_map.at(entity);
        const auto& entity_types = archetype_map.at(src_id).archetype.type();

        std::array<CompTypeInfo, sizeof...(Ts)> pack_infos = {get_component_info<Ts>()...};
        auto not_in_pack = [&](const CompTypeInfo& info1) {
            return std::ranges::find_if(pack_infos, [info1](const CompTypeInfo& info2) { return info1.id == info2.id; }) == pack_infos.end();
        };

        scratch_component_buffer.reserve(sizeof...(Ts));

        for (const auto& info : entity_types) {
            if (not_in_pack(info)) {
                scratch_component_buffer.push_back(info);
            }
        }

#ifndef NDEBUG
        auto in_entity_type = [&](const CompTypeInfo& info1) {
            return std::ranges::find_if(entity_types, [info1](const CompTypeInfo& info2) { return info1.id == info2.id; }) != entity_types.end();
        };

        for (const auto& info : pack_infos) {
            NIDAVELLIR_ASSERT(in_entity_type(info), "Tried removing a component from an entity that did not have it");
        }
#endif

        sort_component_list(scratch_component_buffer);

        auto& [target_arch, target_entities, target_id] = find_or_create_archetype(scratch_component_buffer);
        auto& [src_arch, src_entities, _] = archetype_map.at(src_id);

        NIDAVELLIR_ASSERT(src_id != target_id, "When removing components there should be no way of ending up in the same archetype again");

        target_arch.prepare_push(1);
        const usize target_col{target_arch.len()};

        for (const auto& info : scratch_component_buffer) {
            void* src_ptr = src_arch.get_raw(src_col, src_arch.get_row(info.id));
            void* dst_ptr = target_arch.get_raw(target_col, target_arch.get_row(info.id));

            info.move_ctor_dtor(dst_ptr, src_ptr, 1);
        }

        for (const auto& info : pack_infos) {
            void* src_ptr = src_arch.get_raw(src_col, src_arch.get_row(info.id));

            info.dtor(src_ptr, 1);
        }

        target_entities.push_back(entity);
        target_arch.increase_size(1);

        const usize src_last_col{src_arch.len() - 1};
        if (src_col < src_last_col) {
            src_arch.swap(src_col, src_last_col);
            std::swap(src_entities[src_col], src_entities[src_last_col]);
            entity_map.at(src_entities[src_col]).col = src_col;
        }

        src_entities.pop_back();
        src_arch.decrease_size(1);

        src_id = target_id;
        src_col = target_col;

        scratch_component_buffer.clear();
    }

  private:
    /**
     * @brief Finds or creates an archetype for the given component type list.
     * @param comp_ts The component type list.
     * @return A reference to the archetype record.
     */
    auto find_or_create_archetype(const CompTypeList& comp_ts) -> ArchetypeRecord&;
};
} // namespace nid
