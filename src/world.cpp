#include "world.h"
#include "comp_type_info.h"
#include "identifiers.h"

namespace nid {
auto World::despawn(const EntityId entity) -> void {
    // Get entity, now grap the archetype record
    const auto& ent_rec = entity_map.at(entity);
    auto& arch = archetype_map.at(ent_rec.archetype);

    // Remove the entity from the archetype
    const auto moved_col = arch.archetype.remove(ent_rec.col);

    // If moved_col is not the entity we are removing we need to change the maps for it
    assert(arch.entities[ent_rec.col] == entity);
    if (moved_col != ent_rec.col) {
        entity_map.at(arch.entities[moved_col]).col = ent_rec.col;
        std::swap(arch.entities[ent_rec.col], arch.entities[moved_col]);
        arch.entities.pop_back(); // We can use pop back because moved_col will be the last column
    }

    entity_map.erase(entity);
}

auto World::swap(ArchetypeRecord& arch_rec, const usize col1, const usize col2) -> void {
    if (col1 == col2) {
        return;
    }

    entity_map.at(arch_rec.entities[col1]).col = col2;
    entity_map.at(arch_rec.entities[col2]).col = col1;
    std::swap(arch_rec.entities[col1], arch_rec.entities[col2]);
}

auto World::find_or_create_archetype(const CompTypeList& comp_ts) -> ArchetypeRecord& {
    if (const auto arch_it = type_map.find(comp_ts); arch_it != type_map.end()) {
        return archetype_map.at(arch_it->second);
    } else {
        const auto new_arch_id = next_archetype_id++;
        auto [fst, _] = archetype_map.insert(
            {new_arch_id, ArchetypeRecord{.archetype = Archetype(comp_ts), .entities = {}, .id = new_arch_id}});
        return fst->second;
    }
}

} // namespace nid
