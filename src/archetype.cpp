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

    for (usize row{0}; row < rows.size(); ++row) {
        rows[row] = operator new(infos[row].size * start_capacity, std::align_val_t{infos[row].alignment});
    }
}

Archetype::~Archetype() {
    for (usize row{0}; row < rows.size(); ++row) {
        infos[row].dtor(rows[row], size);
        operator delete(rows[row], std::align_val_t{infos[row].alignment});
    }
}

Archetype::Archetype(Archetype&& other) noexcept
    : rows(std::move(other.rows)), infos(std::move(other.infos)), capacity(other.capacity), size(other.size) {
    other.capacity = 0;
    other.size = 0;
    assert(other.rows.empty());
    assert(other.infos.empty());
}

auto Archetype::operator=(Archetype&& other) noexcept -> Archetype& {
    for (usize row{0}; row < rows.size(); ++row) {
        infos[row].dtor(rows[row], size);
        operator delete(rows[row], std::align_val_t{infos[row].alignment});
    }

    rows = std::move(other.rows);
    infos = std::move(other.infos);
    capacity = other.capacity;
    size = other.size;

    other.capacity = 0;
    other.size = 0;
    assert(other.rows.empty());
    assert(other.infos.empty());

    return *this;
}

auto Archetype::reserve(const usize new_capacity) -> void {
    assert(new_capacity > capacity);
    std::vector<void*> new_rows(rows.size());

    for (usize row{0}; row < new_rows.size(); ++row) {
        new_rows[row] = operator new(infos[row].size * new_capacity, std::align_val_t{infos[row].alignment});
        infos[row].move_ctor_dtor(new_rows[row], rows[row], size);
        operator delete(rows[row], std::align_val_t{infos[row].alignment});
    }

    rows = std::move(new_rows);
    capacity = new_capacity;
}
auto Archetype::grow() -> void {
    reserve(capacity * 2);
}

auto Archetype::swap(const usize first, const usize second) -> void {
    if (first == second) {
        return;
    }

    if (capacity == size) {
        grow();
    }

    for (usize row{0}; row < rows.size(); ++row) {
        void* end = internal_get(size, row);
        void* ptr_first = internal_get(first, row);
        void* ptr_second = internal_get(second, row);

        // Move construct first element at the back of buffer
        infos[row].move_ctor(end, ptr_first, 1);

        // Move assign from second to first
        infos[row].move_assign(ptr_first, ptr_second, 1);

        // Move assign and destroy from end to second
        infos[row].move_assign_dtor(ptr_second, end, 1);
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