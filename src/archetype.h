#pragma once
#include "core.h"
#include "comp_type_info.h"

#include <ranges>
#include <algorithm>
#include <vector>
#include <new>
#include <cassert>

namespace nid {
/**
 * @brief Sorts a component type list based on alignment and ID.
 *
 * This function takes a vector of component type infos
 * and sorts it based on the alignment and ID of the components.
 *
 * @param lst The vector of component type infos to be sorted.
 *
 * This function uses the `std::ranges::sort` algorithm with a custom comparator.
 * The comparator prioritizes the `alignment` field of the component type infos in descending order,
 * and in case of a tie, it prioritizes the `id` field in descending order.
 */
inline auto sort_component_list(CompTypeList& lst) -> void {
    std::ranges::sort(lst, [](const auto& lhs, const auto& rhs) {
        return lhs.alignment > rhs.alignment or
               (lhs.alignment == rhs.alignment and
                lhs.id > rhs.id);
    });
}

/**
 * @brief A custom iterator class for iterating over rows of components.
 *
 * This class defines an iterator for a range of components of type T. It provides
 * the necessary iterator traits and implements the standard iterator operations,
 * including increment, decrement, and random access.
 *
 * @tparam T The component type to iterate over. Must satisfy the Component concept.
 */
template<Component T>
class RowIterator {
    T* ptr{nullptr};

  public:
    using iterator_category = std::contiguous_iterator_tag; /**< The iterator category tag. */
    using value_type = T;                                   /**< The value type of the iterator. */
    using difference_type = std::ptrdiff_t;                 /**< The type used to represent the difference between iterators. */
    using pointer = T*;                                     /**< The pointer type of the iterator. */
    using reference = T&;                                   /**< The reference type of the iterator. */

    /**
     * @brief Default constructor. Initializes the pointer to nullptr.
     */
    RowIterator() = default;

    /**
     * @brief Default destructor.
     */
    ~RowIterator() = default;

    /**
     * @brief Constructs the iterator with a pointer to the component type.
     *
     * @param p Pointer to the component type.
     */
    explicit RowIterator(T* p) : ptr(p) {}

    /**
     * @brief Default copy constructor.
     */
    RowIterator(const RowIterator&) = default;

    /**
     * @brief Default copy assignment operator.
     *
     * @return A reference to the assigned RowIterator.
     */
    auto operator=(const RowIterator&) -> RowIterator& = default;

    /**
     * @brief Default move constructor.
     */
    RowIterator(RowIterator&&) = default;

    /**
     * @brief Default move assignment operator.
     *
     * @return A reference to the assigned RowIterator.
     */
    auto operator=(RowIterator&&) -> RowIterator& = default;

    /**
     * @brief Arrow operator for accessing the component pointer.
     *
     * @return Pointer to the component.
     */
    auto operator->() const -> pointer { return ptr; }

    /**
     * @brief Dereference operator for accessing the component reference.
     *
     * @return Reference to the component.
     */
    auto operator*() const -> reference { return *ptr; }

    /**
     * @brief Subscript operator for accessing the component at the given index.
     *
     * @param index The index of the component to access.
     * @return Reference to the component at the given index.
     */
    auto operator[](const difference_type index) const -> reference { return ptr[index]; }

    /**
     * @brief Pre-increment operator.
     *
     * @return A reference to the incremented iterator.
     */
    auto operator++() -> RowIterator& {
        ++ptr;
        return *this;
    }

    /**
     * @brief Post-increment operator.
     *
     * @return The iterator before increment.
     */
    auto operator++([[maybe_unused]] int) -> RowIterator {
        RowIterator temp(*this);
        ++ptr;
        return temp;
    }

    /**
     * @brief Pre-decrement operator.
     *
     * @return A reference to the decremented iterator.
     */
    auto operator--() -> RowIterator& {
        --ptr;
        return *this;
    }

    /**
     * @brief Post-decrement operator.
     *
     * @return The iterator before decrement.
     */
    auto operator--([[maybe_unused]] int) -> RowIterator {
        RowIterator temp(*this);
        --ptr;
        return temp;
    }

    /**
     * @brief Addition assignment operator.
     *
     * @param steps The number of steps to move the iterator forward.
     * @return A reference to the modified iterator.
     */
    auto operator+=(const difference_type steps) -> RowIterator& {
        ptr += steps;
        return *this;
    }

    /**
     * @brief Subtraction assignment operator.
     *
     * @param steps The number of steps to move the iterator backward.
     * @return A reference to the modified iterator.
     */
    auto operator-=(const difference_type steps) -> RowIterator& {
        ptr -= steps;
        return *this;
    }

    /**
     * @brief Three-way comparison operator.
     *
     * @param lhs The left-hand side iterator.
     * @param rhs The right-hand side iterator.
     * @return The result of the three-way comparison.
     */
    friend auto operator<=>(RowIterator lhs, RowIterator rhs) = default;

    /**
     * @brief Subtraction operator.
     *
     * @param lhs The left-hand side iterator.
     * @param rhs The right-hand side iterator.
     * @return The difference in steps between the two iterators.
     */
    friend auto operator-(RowIterator lhs, RowIterator rhs) -> difference_type { return lhs.ptr - rhs.ptr; }

    /**
     * @brief Addition operator.
     *
     * @param lhs The iterator.
     * @param rhs The number of steps to move forward.
     * @return The modified iterator.
     */
    friend auto operator+(RowIterator lhs, difference_type rhs) -> RowIterator {
        lhs += rhs;
        return lhs;
    }

    /**
     * @brief Subtraction operator.
     *
     * @param lhs The iterator.
     * @param rhs The number of steps to move backward.
     * @return The modified iterator.
     */
    friend auto operator-(RowIterator lhs, difference_type rhs) -> RowIterator {
        lhs -= rhs;
        return lhs;
    }

    /**
     * @brief Addition operator.
     *
     * @param lhs The number of steps to move forward.
     * @param rhs The iterator.
     * @return The modified iterator.
     */
    friend auto operator+(difference_type lhs, RowIterator rhs) -> RowIterator {
        rhs += lhs;
        return rhs;
    }
};

static_assert(std::contiguous_iterator<RowIterator<usize>>);
static_assert(std::contiguous_iterator<RowIterator<std::vector<f32>>>);

class Archetype {
    static constexpr usize start_capacity{10};
    std::vector<void*> rows;
    CompTypeList infos;
    usize capacity;
    usize size{0};

  public:
    explicit Archetype(CompTypeList comp_infos);
    ~Archetype();

    Archetype(const Archetype&) = delete;
    auto operator=(const Archetype&) -> Archetype& = delete;
    Archetype(Archetype&&) = delete;
    auto operator=(Archetype&&) -> Archetype& = delete;

    auto reserve(usize new_capacity) -> void;

    [[nodiscard]] auto remove(usize col) -> usize;

    template<typename T, Component... Ts>
        requires std::ranges::contiguous_range<T> and std::same_as<usize, std::ranges::range_value_t<T>>
    [[nodiscard]] auto emplace_back(T&& row_indices, Ts&&... args) -> usize {
        if (capacity == size) {
            reserve(capacity * 2);
        }

        auto func = [&]<Component Ty>(const usize index, Ty&& t) {
            new (static_cast<u8*>(rows[index]) + infos[index].size * size) std::decay_t<Ty>(std::forward<Ty>(t));
        };

        usize i{0};
        (..., func(row_indices[i++], std::forward<Ts>(args)));

        auto col = size++;
        return col;
    }

    template<Component T>
    [[nodiscard]] auto get_component(const usize col, const usize row) -> T& {
        assert(row < rows.size());
        assert(col < size);
        assert(infos[row].id == typeid(T).hash_code());
        return *static_cast<T*>(internal_get(col, row));
    }

    template<Component T>
    [[nodiscard]] auto begin(const usize row) -> RowIterator<T> {
        assert(row < rows.size() and infos[row].id == typeid(T).hash_code());
        return RowIterator<T>(static_cast<T*>(internal_get(0, row)));
    }

    template<Component T>
    [[nodiscard]] auto end(const usize row) -> RowIterator<T> {
        assert(row < rows.size() and infos[row].id == typeid(T).hash_code());
        return RowIterator<T>(static_cast<T*>(internal_get(size, row)));
    }

  private:
    [[nodiscard]] auto internal_get(const usize col, const usize row) const -> void* {
        return static_cast<u8*>(rows[row]) + infos[row].size * col;
    }
};
} // namespace nid
