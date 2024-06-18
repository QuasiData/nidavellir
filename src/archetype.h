#pragma once
#include "core.h"
#include "identifiers.h"
#include "comp_type_info.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <limits>
#include <new>
#include <vector>
#include <ranges>

namespace nid {
class Archetype {
    std::vector<void*> rows;
    std::vector<CompTypeInfo> infos;
    static constexpr usize start_capacity{10};
    usize capacity;
    usize size{0};
    usize column_size_bytes{0};
    // TODO: Have one buffer, pointed to by the first row. Then let the other pointers point in to it.
    // Alignment will be handled by sorting the types by alignment

    [[nodiscard]] static inline auto check_align_order(const CompTypeList& lst) -> bool {
        usize prev_alignment = std::numeric_limits<usize>::max();
        return std::ranges::all_of(lst, [&prev_alignment](const CompTypeInfo& info) {
            auto cond = prev_alignment >= info.alignment;
            prev_alignment = info.alignment;
            return cond;
        });
    }

  public:
    explicit Archetype(CompTypeList comp_infos) : rows(comp_infos.size()), infos(std::move(comp_infos)), capacity(start_capacity) {
        assert(check_align_order(infos));

        for (const auto& comp_info : comp_infos) {
            column_size_bytes += comp_info.size;
        }

        if (!rows.empty()) {
            u8* buffer = static_cast<u8*>(::operator new(column_size_bytes * start_capacity));
            for (usize row{0}; row < rows.size(); ++row) {
                rows[row] = buffer;
                buffer += infos[row].size * start_capacity;
            }
        }
    }

    ~Archetype() {
        for (usize row{0}; row < rows.size(); ++row) {
            infos[row].dtor(rows[row], size);
        }

        if (!rows.empty()) {
            ::operator delete(rows[0]);
        }
    }

    template<typename T, Component... Ts>
        requires std::ranges::contiguous_range<T> and std::same_as<usize, std::ranges::range_value_t<T>>
    [[nodiscard]] auto emplace_back(T&& row_indices, Ts&&... args) -> usize {
        if (capacity == size) {
            // Reserve more space
            reserve(capacity * 2);
        }

        auto func = [&]<Component Ty>(const usize index, Ty&& t) {
            new (static_cast<u8*>(rows[index]) + (infos[index].size * size)) Ty(std::forward<Ty>(t));
        };

        usize i{0};
        (..., func(row_indices[i++], std::forward<Ts>(args)));

        auto col = size++;
        return col;
    }

    auto reserve(const usize new_capacity) -> void {
        std::vector<void*> new_rows(rows.size());

        if (!rows.empty()) {
            u8* buffer = static_cast<u8*>(::operator new(column_size_bytes * new_capacity));
            for (usize row{0}; row < new_rows.size(); ++row) {
                new_rows[row] = buffer;
                infos[row].move_ctor_dtor(buffer, rows[row], size);

                buffer += infos[row].size * start_capacity;
            }

            ::operator delete(rows[0]);
        }
    }
};
} // namespace nid