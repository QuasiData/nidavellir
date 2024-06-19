// ReSharper disable CppUseStructuredBinding
#include "archetype.h"
#include <cassert>

namespace nid {
namespace {
    auto check_align_order(const CompTypeList& lst) -> bool {
        usize prev_alignment = std::numeric_limits<usize>::max();
        return std::ranges::all_of(lst, [&prev_alignment](const CompTypeInfo& info) {
            const auto cond = prev_alignment >= info.alignment;
            prev_alignment = info.alignment;
            return cond;
        });
    }
} // namespace

Archetype::Archetype(CompTypeList comp_infos) : rows(comp_infos.size()), infos(std::move(comp_infos)), capacity(start_capacity) {
    assert(check_align_order(infos));

    for (const auto& comp_info : infos) {
        column_size_bytes += comp_info.size;
    }

    if (!rows.empty()) {
        auto* buffer = static_cast<u8*>(::operator new(column_size_bytes * start_capacity));
        for (usize row{0}; row < rows.size(); ++row) {
            rows[row] = buffer;
            buffer += infos[row].size * start_capacity;
        }
    }
}
Archetype::~Archetype() {
    for (usize row{0}; row < rows.size(); ++row) {
        infos[row].dtor(rows[row], size);
    }

    if (!rows.empty()) {
        ::operator delete(rows[0]);
    }
}
auto Archetype::reserve(const usize new_capacity) -> void {
    std::vector<void*> new_rows(rows.size());

    if (!rows.empty()) {
        auto* buffer = static_cast<u8*>(::operator new(column_size_bytes * new_capacity));
        for (usize row{0}; row < new_rows.size(); ++row) {
            new_rows[row] = buffer;
            infos[row].move_ctor_dtor(buffer, rows[row], size);

            buffer += infos[row].size * new_capacity;
        }

        ::operator delete(rows[0]);

        rows = std::move(new_rows);
        capacity = new_capacity;
    }
}

auto Archetype::remove(const usize col) -> usize {
    const auto last_col = --size;
    assert(col <= last_col);
    for (usize row{0}; row < rows.size(); ++row) {
        if (col == last_col) {
            void* last = internal_get(last_col, row);
            infos[row].dtor(last, 1);
        } else {
            void* dst = internal_get(col, row);
            void* src = internal_get(last_col, row);

            infos[row].move_assign_dtor(dst, src, 1);
        }
    }

    return last_col;
}
} // namespace nid