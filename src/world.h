#pragma once
#include "archetype.h"
#include "comp_type_info.h"
#include "identifiers.h"
#include <ankerl/unordered_dense.h>

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

    template<Component... Ts>
    auto spawn(Ts&&... pack) -> EntityId {
        constexpr usize pack_size = sizeof...(Ts);

        CompTypeList comp_ts = {get_component_info<Ts>()...};
        std::array<usize, pack_size> orig_indices;
        std::ranges::iota(orig_indices, usize{0});
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

        auto func = [&](const ArchetypeId arch_id, const CompTypeList& comps) {
            for (const auto& [i, type] : std::views::enumerate(comps)) {
                if (const auto comp_it = component_map.find(type.id); comp_it != component_map.end()) {
                    comp_it->second.insert({arch_id, RowRecord{.row = orig_indices[i]}});
                } else {
                    auto result = component_map.insert({type.id, ArchetypeMap{}});
                    result.first->second.insert({arch_id, RowRecord{.row = orig_indices[i]}});
                }
            }
        };

        const auto new_entity_id = next_entity_id++;
        if (const auto arch_it = type_map.find(comp_ts); arch_it != type_map.end()) {
            auto& arch_record = archetype_map.at(arch_it->second);
            const auto col = arch_record.archetype.emplace_back(row_indices, std::forward<Ts>(pack)...);

            arch_record.entities.emplace_back(new_entity_id);
            entity_map.insert({new_entity_id, EntityRecord{.archetype = arch_it->second, .col = col}});

            func(arch_it->second, comp_ts);

            type_map.insert({std::move(comp_ts), arch_it->second});
        } else {
            const auto new_arch_id = next_archetype_id++;

            auto result = archetype_map.insert(
                {new_arch_id, ArchetypeRecord{.archetype = Archetype(comp_ts), .entities = {new_entity_id}}});
            const auto col = result.first->second.archetype.emplace_back(row_indices, std::forward<Ts>(pack)...);

            entity_map.insert({new_entity_id, EntityRecord{.archetype = new_arch_id, .col = col}});

            func(new_arch_id, comp_ts);

            type_map.insert({std::move(comp_ts), new_arch_id});
        }

        return new_entity_id;
    }

  private:
};
} // namespace nid
