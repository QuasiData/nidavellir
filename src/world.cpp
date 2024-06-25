#include "world.h"
#include "comp_type_info.h"
#include "core.h"
#include "identifiers.h"

#include <stdexcept>

namespace nid {
auto World::despawn(const EntityId entity) -> void {
    const auto opt = entity_map.extract(entity);
    if (opt.has_value()) {
        const auto [id, col] = opt.value().second;
        auto& [arch, entities, _] = archetype_map.at(id);

        const auto moved_col = arch.remove(col);

        NIDAVELLIR_ASSERT(entities[col] == entity, "The entity corresponding to the column should be the one we are despawning");
        if (moved_col != col) {
            entity_map.at(entities[moved_col]).col = col;
            std::swap(entities[col], entities[moved_col]);
            entities.pop_back();
        }
    } else {
        throw std::out_of_range("The entity was not found");
    }
}

auto World::find_or_create_archetype(const CompTypeList& comp_ts) -> ArchetypeRecord& {
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

    if (const auto arch_it = type_map.find(comp_ts); arch_it != type_map.end()) {
        return archetype_map.at(arch_it->second);
    }

    const auto new_arch_id = next_archetype_id++;
    auto [fst, _] = archetype_map.insert(
        {new_arch_id, ArchetypeRecord{.archetype = Archetype(comp_ts), .entities = {}, .id = new_arch_id}});
    func(new_arch_id, comp_ts);
    type_map.insert({comp_ts, new_arch_id});
    return fst->second;
}
} // namespace nid
