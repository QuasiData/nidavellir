#pragma once
#include "core.h"
#include "comp_type_info.h"
#include "identifiers.h"

#include <algorithm>
#include <vector>
#include <cassert>
#include <span>

#include <ankerl/unordered_dense.h>

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
inline auto sort_component_list(std::span<CompTypeInfo> lst) -> void {
    std::ranges::sort(lst, [](const auto& lhs, const auto& rhs) {
        return lhs.id > rhs.id;
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
    static constexpr usize start_capacity{10};                 ///< Initial capacity for components.
    std::vector<void*> rows;                                   ///< Storage for component data.
    CompTypeList infos;                                        ///< List of component type information.
    ankerl::unordered_dense::map<ComponentId, usize> comp_map; ///< Map from component id to row.
    usize capacity;                                            ///< Current capacity of the archetype.
    usize size{0};                                             ///< Number of components currently stored.

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
     * @brief Prepares the Archetype to push new components by ensuring sufficient capacity.
     *
     * This function ensures that the Archetype has enough capacity to add the specified number of new components.
     * If the current capacity is insufficient, it will grow the capacity to accommodate the new components.
     *
     * @param count The number of new components to be added.
     */
    auto prepare_push(usize count) -> void;

    /**
     * @brief Increases the current size of the Archetype by the specified count.
     *
     * This function increments the size of the Archetype by the given count, which represents the number of new columns added.
     *
     * @param count The number of columns to add to the current size.
     */
    auto increase_size(const usize count) -> void { size += count; }

    /**
     * @brief Decrease the current size of the Archetype by the specified count.
     *
     * This function decrements the size of the Archetype by the given count, which represents the number of columns that are removed.
     *
     * @param count The number of columns to remove from the current size.
     */
    auto decrease_size(const usize count) -> void {
        NIDAVELLIR_ASSERT(count <= size and size > 0, "The size needs to be larger than 0 before a decrease and needs to be non-negative after");
        size -= count;
    }

    /**
     * @brief Swaps the components at two specified columns.
     * @param first Index of the first column.
     * @param second Index of the second column.
     */
    auto swap(usize first, usize second) noexcept -> void;

    /**
     * @brief Removes the component at the specified column.
     * @param col Index of the column to remove.
     * @return Index to the column that was last before removal(the len after removal).
     */
    [[nodiscard]] auto remove(usize col) -> usize;

    /**
     * @brief Retrieves the component type list.
     * @return A constant reference to the CompTypeList containing component information.
     */
    [[nodiscard]] auto type() const -> std::span<const CompTypeInfo> { return infos; }

    /**
     * @brief Gets the row index associated with the specified component ID.
     * @param id The component ID for which to get the row index.
     * @return The row index associated with the specified component ID.
     * @throws std::out_of_range if the component ID is not found in the map.
     */
    [[nodiscard]] auto get_row(const ComponentId id) const -> usize { return comp_map.at(id); }

    /**
     * @brief Checks if there is a partial match with the given type list.
     *
     * This function determines whether the provided type list partially matches
     * the types within the archetype.
     *
     * @param type_list A span representing the list of types to be checked.
     * @return true if there is a partial match, false otherwise.
     */
    [[nodiscard]] auto partial_match(std::span<CompTypeInfo> type_list) const -> bool;

    /**
     * @brief Checks if the given type list matches the types within the archetype.
     *
     * @param type_list A span representing the list of types to be matched.
     * @return true if its is a match, false otherwise.
     */
    [[nodiscard]] auto match(std::span<CompTypeInfo> type_list) const -> bool;

    /**
     * @brief Adds a new column to the Archetype.
     *
     * This function adds a new column to the archetype, initializing it with the provided components.
     * If the current capacity is reached, the function will grow the underlying storage before adding
     * the new column. The components are constructed in place at the appropriate locations.
     *
     * @tparam Ts Types of the components.
     * @param pack Components to be added.
     * @return Index of the added column.
     */
    template<Component... Ts>
    [[nodiscard]] auto emplace_back(Ts&&... pack) -> usize {
        if (capacity == size) {
            grow();
        }

        if constexpr (sizeof...(Ts) > 0) {
            auto func = [&]<Component Ty>(const usize index, Ty&& t) {
                new (static_cast<u8*>(rows[index]) + infos[index].size * size) std::decay_t<Ty>(std::forward<Ty>(t));
            };

            (..., func(get_row(type_id<Ts>()), std::forward<Ts>(pack)));
        }

        const auto col = size++;
        return col;
    }

    /**
     * @brief Updates components in an existing column of the Archetype.
     *
     * This function updates the specified column with new components, destroying the existing components
     * in the column before constructing the new ones in place. The column index must be valid (less than the current size).
     * The provided components (`pack`) may only cover part of the column, not necessarily all the rows.
     *
     * @tparam Ts Types of the components to update.
     * @param col The column to be updated.
     * @param pack New components to update the column with.
     */
    template<Component... Ts>
    [[nodiscard]] auto update(const usize col, Ts&&... pack) {
        NIDAVELLIR_ASSERT(col < size, "An update can only happen in an initialized column");
        if constexpr (sizeof...(Ts) > 0) {
            auto func = [&]<Component Ty>(const usize index, Ty&& t) {
                void* dst = static_cast<u8*>(rows[index]) + infos[index].size * col;
                infos[index].dtor(dst, 1);
                new (dst) std::decay_t<Ty>(std::forward<Ty>(t));
            };

            (..., func(get_row(type_id<Ts>()), std::forward<Ts>(pack)));
        }
    }

    /**
     * @brief Creates components in an existing column of the Archetype.
     *
     * This function constructs new components in the specified column at the appropriate locations.
     * The column index must be valid (less than the current capacity). The provided components (`pack`)
     * may only cover part of the column, not necessarily all the rows.
     *
     * @tparam Ts Types of the components to create.
     * @param col The column where the components will be created.
     * @param pack New components to create in the specified column.
     */
    template<Component... Ts>
    [[nodiscard]] auto create(const usize col, Ts&&... pack) {
        NIDAVELLIR_ASSERT(col < capacity, "A create can only happen in an initialized column");
        if constexpr (sizeof...(Ts) > 0) {
            auto func = [&]<Component Ty>(const usize index, Ty&& t) {
                new (static_cast<u8*>(rows[index]) + infos[index].size * col) std::decay_t<Ty>(std::forward<Ty>(t));
            };

            (..., func(get_row(type_id<Ts>()), std::forward<Ts>(pack)));
        }
    }

    /**
     * @brief Gets a reference to a component at the specified position.
     * @tparam T Type of the component.
     * @param col Column index of the component.
     * @return Reference to the component.
     */
    template<Component T>
    [[nodiscard]] auto get_component(const usize col) -> T& {
        NIDAVELLIR_ASSERT(col < size, "Can only get components from initialized columns");
        return *static_cast<T*>(get_raw(col, comp_map.at(type_id<T>())));
    }

    /**
     * @brief Gets an iterator to the beginning of the specified components row.
     * @tparam T Type of the component.
     * @return Iterator to the beginning of the row of component type T.
     */
    template<Component T>
    [[nodiscard]] auto begin() -> RowIterator<T> {
        return RowIterator<T>(static_cast<T*>(get_raw(0, comp_map.at(type_id<T>()))));
    }

    /**
     * @brief Gets an iterator to the end of the specified components row.
     * @tparam T Type of the component.
     * @return Iterator to the end of the row of component type T.
     */
    template<Component T>
    [[nodiscard]] auto end() -> RowIterator<T> {
        return RowIterator<T>(static_cast<T*>(get_raw(size, comp_map.at(type_id<T>()))));
    }

    /**
     * @brief Gets a pointer to the memory at the specified position.
     * @param col Column index of the component.
     * @param row Row index of the component.
     * @return Pointer to the memory.
     */
    [[nodiscard]] auto get_raw(const usize col, const usize row) const -> void* {
        NIDAVELLIR_ASSERT(row < rows.size(), "Row should never be less than the amount of rows in the archetype");
        return static_cast<u8*>(rows[row]) + infos[row].size * col;
    }
};
} // namespace nid
