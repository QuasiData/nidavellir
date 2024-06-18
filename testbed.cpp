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

    return 0;
}
