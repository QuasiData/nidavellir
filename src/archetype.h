#pragma once
#include "core.h"
#include "comp_type_info.h"

#include <ranges>
#include <algorithm>
#include <vector>
#include <cassert>

namespace nid {
/**
 * @brief Sorts a component type list based on alignment and ID, with additional zipped ranges.
 *
 * This function takes a vector of component type infos and additional contiguous ranges,
 * zips them together, and sorts the zipped ranges based on the alignment and ID of the components.
 *
 * @tparam Ts A parameter pack of contiguous ranges.
 * @param lst The vector of component type infos to be sorted.
 * @param range Additional ranges to be zipped with the component list.
 *
 * This function uses the `std::views::zip` to combine the component type infos and
 * the additional ranges. The resulting zipped range is sorted using a custom
 * comparator. The comparator prioritizes the `alignment` field of the component type infos
 * in descending order, and in case of a tie, it prioritizes the `id` field in descending order.
 */
template<std::ranges::contiguous_range... Ts>
auto sort_component_list(CompTypeList& lst, Ts&&... range) -> void {
    auto zip = std::views::zip(lst, std::forward<Ts>(range)...);
    std::ranges::sort(zip, [](const auto& lhs, const auto& rhs) {
        return std::get<0>(lhs).alignment > std::get<0>(rhs).alignment or
               (std::get<0>(lhs).alignment == std::get<0>(rhs).alignment and
                std::get<0>(lhs).id > std::get<0>(rhs).id);
    });
}

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

/**
 * @class Archetype
 * @brief Manages a collection of components arranged in a contiguous memory layout.
 */
class Archetype {
    static constexpr usize start_capacity{10}; ///< Initial capacity for components.
    std::vector<void*> rows;                   ///< Storage for component data.
    CompTypeList infos;                        ///< List of component type information.
    usize capacity;                            ///< Current capacity of the archetype.
    usize size{0};                             ///< Number of components currently stored.

  public:
    /**
     * @brief Constructs an Archetype with the given component type list.
     * @param comp_infos List of component type information.
     */
    explicit Archetype(CompTypeList comp_infos);

    /**
     * @brief Destructor for Archetype.
     */
    ~Archetype();

    Archetype(const Archetype&) = delete;
    auto operator=(const Archetype&) -> Archetype& = delete;

    /**
     * @brief Move constructor for Archetype.
     * @param other Archetype to be moved.
     */
    Archetype(Archetype&& other) noexcept;

    /**
     * @brief Move assignment operator for Archetype.
     * @param other Archetype to be moved.
     * @return Reference to the assigned Archetype.
     */
    auto operator=(Archetype&& other) noexcept -> Archetype&;

    /**
     * @brief Gets the capacity of the Archetype.
     * @return Current capacity.
     */
    [[nodiscard]] auto cap() const noexcept -> usize { return capacity; }

    /**
     * @brief Gets the number of columns in the Archetype.
     * @return Current number of columns.
     */
    [[nodiscard]] auto len() const noexcept -> usize { return size; }

    /**
     * @brief Reserves additional capacity for the Archetype.
     * @param new_capacity The new capacity to reserve.
     */
    auto reserve(usize new_capacity) -> void;

    /**
     * @brief Grows the Archetype by some amount.
     */
    auto grow() -> void;

    /**
     * @brief Swaps the components at two specified columns.
     * @param first Index of the first column.
     * @param second Index of the second column.
     */
    auto swap(usize first, usize second) -> void;

    /**
     * @brief Removes the component at the specified column.
     * @param col Index of the column to remove.
     * @return Index to the column that was last before removal(the len after removal).
     */
    [[nodiscard]] auto remove(usize col) -> usize;

    /**
     * @brief Adds a new column to the Archetype.
     *
     * 'row_indices' should be a container which in each position
     * contains the row within which to the place the corresponding component.
     *
     * @tparam T Type of the index container.
     * @tparam Ts Types of the components.
     * @param row_indices Container of row indices.
     * @param args Components to be added.
     * @return Index of the added column.
     */
    template<typename T, Component... Ts>
        requires std::ranges::contiguous_range<T> and std::same_as<usize, std::ranges::range_value_t<T>>
    [[nodiscard]] auto emplace_back(T&& row_indices, Ts&&... args) -> usize {
        if (capacity == size) {
            grow();
        }

        if constexpr (sizeof...(Ts) > 0) {
            auto func = [&]<Component Ty>(const usize index, Ty&& t) {
                new (static_cast<u8*>(rows[index]) + infos[index].size * size) std::decay_t<Ty>(std::forward<Ty>(t));
            };

            usize i{0};
            (..., func(std::forward<T>(row_indices)[i++], std::forward<Ts>(args)));
        }

        const auto col = size++;
        return col;
    }

    /**
     * @brief Gets a reference to a component at the specified position.
     * @tparam T Type of the component.
     * @param col Column index of the component.
     * @param row Row index of the component.
     * @return Reference to the component.
     */
    template<Component T>
    [[nodiscard]] auto get_component(const usize col, const usize row) -> T& {
        assert(row < rows.size());
        assert(col < size);
        assert(infos[row].id == typeid(T).hash_code());
        return *static_cast<T*>(internal_get(col, row));
    }

    /**
     * @brief Gets an iterator to the beginning of the specified row.
     * @tparam T Type of the component.
     * @param row Row index.
     * @return Iterator to the beginning of the row.
     */
    template<Component T>
    [[nodiscard]] auto begin(const usize row) -> RowIterator<T> {
        assert(row < rows.size() and infos[row].id == typeid(T).hash_code());
        return RowIterator<T>(static_cast<T*>(internal_get(0, row)));
    }

    /**
     * @brief Gets an iterator to the end of the specified row.
     * @tparam T Type of the component.
     * @param row Row index.
     * @return Iterator to the end of the row.
     */
    template<Component T>
    [[nodiscard]] auto end(const usize row) -> RowIterator<T> {
        assert(row < rows.size() and infos[row].id == typeid(T).hash_code());
        return RowIterator<T>(static_cast<T*>(internal_get(size, row)));
    }

  private:
    /**
     * @brief Gets a pointer to the memory at the specified position.
     * @param col Column index of the component.
     * @param row Row index of the component.
     * @return Pointer to the memory.
     */
    [[nodiscard]] auto internal_get(const usize col, const usize row) const -> void* {
        return static_cast<u8*>(rows[row]) + infos[row].size * col;
    }
};
} // namespace nid
