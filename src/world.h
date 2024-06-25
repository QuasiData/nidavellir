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
    World() = default;
    ~World() = default;

    World(const World&) = delete;
    auto operator=(const World&) = delete;
    World(World&&) = delete;
    auto operator=(World&&) = delete;

    auto despawn(EntityId entity) -> void;

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
    auto find_or_create_archetype(const CompTypeList& comp_ts) -> ArchetypeRecord&;
};
} // namespace nid
