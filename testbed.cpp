#include "nidavellir.h"

#include <concepts>
#include <iostream>

struct Kaka {
    // using is_relocatable = void;
};

template<typename T>
auto func() -> void {
    if constexpr (requires(T) { typename T::is_relocatable; }) {
        std::cout << "Kaka" << "\n";
    } else {
        std::cout << "Mu" << "\n";
    }
}

auto main() -> int {
    func<Kaka>();
    return 0;
}