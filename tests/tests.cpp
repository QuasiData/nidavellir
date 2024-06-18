#include "archetype.h"
#include "comp_type_info.h"
#include "nidavellir.h"

#include "gtest/gtest.h"
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <utility>
#include <vector>

namespace {
template<typename... Ts>
[[nodiscard]] auto get_infos() -> nid::CompTypeList {
    nid::CompTypeList lst;
    lst.reserve(sizeof...(Ts));

    (lst.emplace_back(nid::get_component_info<Ts>()), ...);
    return lst;
}

auto sort_comp_lst(nid::CompTypeList& lst) -> void {
    std::ranges::sort(lst, [](const auto& lhs, const auto& rhs) {
        return lhs.alignment > rhs.alignment or (lhs.alignment == rhs.alignment and lhs.id > rhs.id);
    });
}

template<typename T>
auto sort_comp_lst(nid::CompTypeList& lst, T& indices) -> void {
    std::views::
    std::ranges::sort(zip, [](const auto& lhs, const auto& rhs) {
        return std::get<0>(lhs).alignment > std::get<0>(rhs).alignment or (std::get<0>(lhs).alignment == std::get<0>(rhs).alignment and std::get<0>(lhs).id > std::get<0>(rhs).id);
    });
}
} // namespace

TEST(Archetype, Construct) {
    auto lst = get_infos<int, float, double>();
    sort_comp_lst(lst);

    EXPECT_NO_THROW(auto arch = nid::Archetype(lst));
}

TEST(Archetype, emplace_back_once) {
    auto lst = get_infos<int, float, double>();
    std::vector<nid::usize> indices = {0, 1, 2};
    sort_comp_lst(lst, indices);

    auto arch = nid::Archetype(lst);
    arch.emplace_back(indices, 1, 2.3f, 2.3);
}
