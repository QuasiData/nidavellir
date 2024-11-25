// ReSharper disable CppUseStructuredBinding
#include "archetype.h"

#include <cassert>

namespace nid {
Archetype::Archetype(CompTypeList comp_infos) : rows(comp_infos.size()), infos(std::move(comp_infos)), capacity(start_capacity) {
    for (usize row{0}; row < rows.size(); ++row) {
        rows[row] = operator new(infos[row].size * start_capacity, std::align_val_t{infos[row].alignment});
        comp_map.insert({infos[row].id, row});
    }
}

Archetype::~Archetype() {
    for (usize row{0}; row < rows.size(); ++row) {
        infos[row].dtor(rows[row], size);
        operator delete(rows[row], std::align_val_t{infos[row].alignment});
    }
}

Archetype::Archetype(Archetype&& other) noexcept
    : rows(std::move(other.rows)), infos(std::move(other.infos)), comp_map(std::move(other.comp_map)), capacity(other.capacity), size(other.size) {
    other.capacity = 0;
    other.size = 0;
    NIDAVELLIR_ASSERT(other.rows.empty(), "The rows of the other archetype should be empty after move");
    NIDAVELLIR_ASSERT(other.infos.empty(), "The infos of the other archetype should be empty after move");
}

auto Archetype::operator=(Archetype&& other) noexcept -> Archetype& {
    for (usize row{0}; row < rows.size(); ++row) {
        infos[row].dtor(rows[row], size);
        operator delete(rows[row], std::align_val_t{infos[row].alignment});
    }

    rows = std::move(other.rows);
    infos = std::move(other.infos);
    comp_map = std::move(other.comp_map);
    capacity = other.capacity;
    size = other.size;

    other.capacity = 0;
    other.size = 0;
    NIDAVELLIR_ASSERT(other.rows.empty(), "The rows of the other archetype should be empty after move");
    NIDAVELLIR_ASSERT(other.infos.empty(), "The infos of the other archetype should be empty after move");

    return *this;
}

auto Archetype::reserve(const usize new_capacity) -> void {
    NIDAVELLIR_ASSERT(new_capacity > capacity, "The reserve function is expected to be called with a larger capacity than the current one");
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

auto Archetype::prepare_push(const usize count) -> void {
    if (size + count > capacity) {
        if (size + count > 2 * capacity) {
            reserve(size + count);
        } else {
            grow();
        }
    }
}

auto Archetype::swap(const usize first, const usize second) noexcept -> void {
    NIDAVELLIR_ASSERT(first < size and second < size, "A swap can only be made between initialized columns");
    if (first == second) {
        return;
    }

    if (capacity == size) {
        grow();
    }

    for (usize row{0}; row < rows.size(); ++row) {
        void* end = get_raw(size, row);
        void* ptr_first = get_raw(first, row);
        void* ptr_second = get_raw(second, row);

        // Move construct first element at the back of buffer
        infos[row].move_ctor_dtor(end, ptr_first, 1);

        // Move assign from second to first
        infos[row].move_ctor_dtor(ptr_first, ptr_second, 1);

        // Move assign and destroy from end to second
        infos[row].move_ctor_dtor(ptr_second, end, 1);
    }
}

auto Archetype::remove(const usize col) -> usize {
    const auto last_col = --size;
    NIDAVELLIR_ASSERT(col <= last_col, "Only an initialized column can be removed");
    for (usize row{0}; row < rows.size(); ++row) {
        if (col == last_col) {
            void* last = get_raw(last_col, row);
            infos[row].dtor(last, 1);
        } else {
            void* dst = get_raw(col, row);
            void* src = get_raw(last_col, row);

            infos[row].move_assign_dtor(dst, src, 1);
        }
    }

    return last_col;
}

auto Archetype::partial_match(const std::span<CompTypeInfo> type_list) const -> bool {
    if (type_list.size() > infos.size()) {
        return false;
    }

    for (const auto& type : type_list) {
        bool found = false;
        for (const auto& info : infos) {
            if (type.id == info.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

auto Archetype::match(const std::span<CompTypeInfo> type_list) const -> bool {
    if (type_list.size() != infos.size()) {
        return false;
    }

    for (const auto& type : type_list) {
        bool found = false;
        for (const auto& info : infos) {
            if (type.id == info.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}
} // namespace nid
