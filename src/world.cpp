#include "world.h"
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
} // namespace nid
