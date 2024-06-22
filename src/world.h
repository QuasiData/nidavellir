#pragma once
#include "archetype.h"
#include "comp_type_info.h"
#include "identifiers.h"

#include <numeric>

#include <ankerl/unordered_dense.h>
#include <stdexcept>
#include <tuple>

namespace nid {
class World {
    struct ArchetypeRecord {
        Archetype archetype;
        std::vector<EntityId> entities;
    };

    struct EntityRecord {
        ArchetypeId archetype;
        usize col;
    };

    struct RowRecord {
        usize row;
    };

    using EntityMap = ankerl::unordered_dense::map<usize, EntityId>;
    using ArchetypeMap = ankerl::unordered_dense::map<ArchetypeId, RowRecord>;

    ankerl::unordered_dense::map<ArchetypeId, ArchetypeRecord> archetype_map;
    ankerl::unordered_dense::map<EntityId, EntityRecord> entity_map;

    ankerl::unordered_dense::map<ComponentId, ArchetypeMap> component_map;
    ankerl::unordered_dense::map<CompTypeList, ArchetypeId, TypeHash> type_map;

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
        if (const auto arch_it = type_map.find(comp_ts); arch_it != type_map.end()) {
            auto& [archetype, entities] = archetype_map.at(arch_it->second);
            const auto col = archetype.emplace_back(row_indices, std::forward<Ts>(pack)...);

            entities.emplace_back(new_entity_id);
            entity_map.insert({new_entity_id, EntityRecord{.archetype = arch_it->second, .col = col}});

            func(arch_it->second, comp_ts);

            type_map.insert({std::move(comp_ts), arch_it->second});
        } else {
            const auto new_arch_id = next_archetype_id++;
            auto [fst, _] = archetype_map.insert(
                {new_arch_id, ArchetypeRecord{.archetype = Archetype(comp_ts), .entities = {new_entity_id}}});
            const auto col = fst->second.archetype.emplace_back(row_indices, std::forward<Ts>(pack)...);

            entity_map.insert({new_entity_id, EntityRecord{.archetype = new_arch_id, .col = col}});

            func(new_arch_id, comp_ts);

            type_map.insert({std::move(comp_ts), new_arch_id});
        }

        return new_entity_id;
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif
    }

    template<Component... Ts>
    [[nodiscard]] auto get(const EntityId entity) -> decltype(auto) {
        const auto ent_rec = entity_map.at(entity);
        auto& arch = archetype_map.at(ent_rec.archetype);

        auto func = [&]<typename T>() -> T& {
            const auto row = component_map.at(typeid(std::decay_t<T>).hash_code()).at(ent_rec.archetype).row;
            return arch.archetype.get_component<T>(ent_rec.col, row);
        };

        auto tup = std::tie(func.template operator()<Ts>()...);
        static_assert(std::same_as<decltype(tup), std::tuple<Ts&...>>);

        if constexpr (sizeof...(Ts) == 1) {
            return std::get<0>(tup);
        } else {
            return tup;
        }
    }

  private:
};
} // namespace nid
