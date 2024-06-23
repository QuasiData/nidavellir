// ReSharper disable CppUseStructuredBinding
#pragma once
#include "archetype.h"
#include "comp_type_info.h"
#include "identifiers.h"

#include <numeric>
#include <ranges>
#include <tuple>

#include <ankerl/unordered_dense.h>
#include <type_traits>

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

    CompTypeList scratch_comp_ts;

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

    auto swap(ArchetypeRecord& arch_rec, usize col1, usize col2) -> void;

    auto find_or_create_archetype(const CompTypeList& comp_ts) -> ArchetypeRecord&;

    template<Component... Ts>
    auto spawn(Ts&&... pack) -> EntityId {
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        constexpr usize pack_size = sizeof...(Ts);

        /*
        Create a list of component infos and corresponding indices
        Then sort them together
        Do a linear search to create a list where the entry at index 'i'
        is the index that the 'i:th' component has after the sort
        */
        CompTypeList comp_ts = {get_component_info<Ts>()...};
        std::array<usize, pack_size> orig_indices;
        std::iota(orig_indices.begin(), orig_indices.end(), usize{0});
        sort_component_list(comp_ts, orig_indices);

        std::array<usize, pack_size> row_indices;
        for (usize i{0}; i < pack_size; ++i) {
            for (usize j{0}; j < pack_size; ++j) {
                if (i == orig_indices[j]) {
                    row_indices[i] = j;
                    break;
                }
            }
        }

        /*
        If component already registered then add the new RowRecord
        Else create an ArchetypeMap and insert the RowRecord
        */
        auto func = [&](const ArchetypeId arch_id, const CompTypeList& comps) {
            for (usize i{0}; i < comps.size(); ++i) {
                if (const auto comp_it = component_map.find(comps[i].id); comp_it != component_map.end()) {
                    comp_it->second.insert({arch_id, RowRecord{.row = i}});
                } else {
                    auto [fst, _] = component_map.insert({comps[i].id, ArchetypeMap{}});
                    fst->second.insert({arch_id, RowRecord{.row = i}});
                }
            }
        };

        /*
        Create and entity.
        If the archetype corresponding to this set of component is found
            Get the archetype and a new column to it, add any components that didnt exist already

        Else
            Create an Archetype and add a new column, add any components that didnt exist already
        */
        const auto new_entity_id = next_entity_id++;
        auto& arch_rec = find_or_create_archetype(comp_ts);
        const auto col = arch_rec.archetype.emplace_back(row_indices, std::forward<Ts>(pack)...);
        arch_rec.entities.push_back(new_entity_id);
        entity_map.insert({new_entity_id, EntityRecord{.archetype = arch_rec.id, .col = col}});
        func(arch_rec.id, comp_ts);
        type_map.insert({std::move(comp_ts), arch_rec.id});

        return new_entity_id;
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif
    }

    template<Component... Ts>
    [[nodiscard]] auto get(const EntityId entity) -> decltype(auto) {
        const auto [arch_id, col] = entity_map.at(entity);
        auto& [arch, entities, _] = archetype_map.at(arch_id);

        auto func = [&]<typename T>() -> T& {
            const auto row = arch.get_row(type_id<std::decay_t<T>>());
            return arch.get_component<T>(col, row);
        };

        auto tup = std::tie(func.template operator()<Ts>()...);
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
        const auto [src_arch_id, col] = entity_map.at(entity);
        auto& [src_arch, src_entities, _] = archetype_map.at(src_arch_id);
        const auto& src_arch_comps = src_arch.type();

        scratch_comp_ts.insert(scratch_comp_ts.begin(), {get_component_info<Ts>()...});

        constexpr usize stack_buffer_size = sizeof(CompTypeInfo) * 30;
        std::array<u8, stack_buffer_size> buffer{};
        std::pmr::monotonic_buffer_resource resource(buffer.data(), stack_buffer_size);
        std::pmr::vector<CompTypeInfo> in_pack_types{&resource};
        std::pmr::vector<CompTypeInfo> not_in_pack_types{&resource};

        auto in_pack = [&](const CompTypeInfo& info1) {
            return std::ranges::find_if(scratch_comp_ts, [info1](const CompTypeInfo& info2) { return info1.id == info2.id; }) != scratch_comp_ts.end();
        };

        for (const auto& item : src_arch_comps) {
            if (in_pack(item)) {
                in_pack_types.push_back(item);
            } else {
                not_in_pack_types.push_back(item);
            }
        }

        std::ranges::copy(not_in_pack_types, std::back_inserter(scratch_comp_ts));
        sort_component_list(scratch_comp_ts);

        auto& [arch, entities, arch_id] = find_or_create_archetype(scratch_comp_ts);

        usize target_col = 0;
        if (src_arch_id == arch_id) {
            target_col = col;
        } else {
            target_col = arch.len();
            arch.prepare_push(1);
        }

        for (const auto& info : not_in_pack_types) {
            void* src_ptr = src_arch.get(col, src_arch.get_row(info.id));
            void* dst_ptr = arch.get(target_col, arch.get_row(info.id));

            info.move_ctor_dtor(dst_ptr, src_ptr, 1);
        }

        for (const auto& info : in_pack_types) {
            void* src_ptr = src_arch.get(col, src_arch.get_row(info.id));

            info.dtor(src_ptr, 1);
        }

        auto func = [&]<Component T>(T&& t) {
            void* dst_ptr = arch.get(target_col, arch.get_row(type_id<std::decay_t<T>>()));
            new (dst_ptr) std::decay_t<T>(std::forward<T>(t));
        };

        (func(pack), ...);

        if (src_arch_id != arch_id) {
            entity_map.at(entity) = EntityRecord{.archetype = arch_id, .col = target_col};
            entities.push_back(entity);
            arch.increase_size(1);

            if (col < src_entities.size() - 1) {
                std::swap(src_entities[col], src_entities.back());
                entity_map.at(src_entities[col]).col = col;
            }
            src_entities.pop_back();
            src_arch.decrease_size(1);
        }

        scratch_comp_ts.clear();
    }
};
} // namespace nid
