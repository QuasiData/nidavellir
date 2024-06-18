#include "nidavellir.h"

#include <iostream>

namespace {
template<typename... Ts>
[[nodiscard]] auto get_infos() -> nid::CompTypeList {
    nid::CompTypeList lst;
    lst.reserve(sizeof...(Ts));

    (lst.emplace_back(nid::get_component_info<Ts>()), ...);
    return lst;
}
} // namespace

auto main() -> int {
    auto lst = get_infos<int, float, double>();

    for (auto i : lst) {
        std::cout << i.alignment << std::endl;
    }

    std::ranges::sort(lst, [](const auto& lhs, const auto& rhs) {
        return lhs.alignment > rhs.alignment or (lhs.alignment == rhs.alignment and lhs.id > rhs.id);
    });

    for (auto i : lst) {
        std::cout << i.alignment << std::endl;
    }

    return 0;
}