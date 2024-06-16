#pragma once
#include "core.h"
#include "identifiers.h"
#include "comp_type_info.h"

#include <vector>

namespace nid {
class Archetype {
    static constexpr usize start_capacity{10};
    std::vector<void*> rows;

    // TODO: Have one buffer, pointed to by the first row. Then let the other pointers point in to it.
    // Alignment will be handled by sorting the types by alignment
};
} // namespace nid