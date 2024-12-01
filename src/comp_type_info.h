// ReSharper disable CppDFANullDereference
#pragma once
#include "core.h"
#include "identifiers.h"

#include <type_traits>
#include <vector>
#include <cassert>
#include <cstring>

#include "ankerl/unordered_dense.h"

namespace nid {
// clang-format off
/**
 * @brief Concept that defines the requirements for a component type.
 *
 * This concept ensures that a type `T` satisfies several properties, making it suitable
 * for use as a component in systems that rely on type-erased contiguous storage and
 * other similar mechanisms.
 *
 * @tparam T The type to check against the component requirements.
 */
template<typename T>
concept Component = std::is_move_assignable_v<std::decay_t<T>>
                    and std::is_nothrow_move_constructible_v<std::decay_t<T>>
                    and std::is_destructible_v<std::decay_t<T>>
                    and !std::is_pointer_v<std::decay_t<T>>;
// clang-format on

/**
 * @brief A structure containing type information and operations for a component.
 *
 * This structure holds metadata and function pointers for managing the lifecycle
 * of a component type. It includes functions for construction, destruction,
 * copy construction, copy assignment, move construction, move assignment, and
 * combined move and destruct operations.
 */
struct CompTypeInfo {
    /**
     * @brief The unique identifier for the component type.
     */
    ComponentId id;

    /**
     * @brief The alignment of the component type.
     */
    usize alignment;

    /**
     * @brief Function pointer for the default constructor.
     *
     * @param dst The destination where the components are constructed.
     * @param count The number of components to construct.
     */
    void (*ctor)(void* dst, usize count);

    /**
     * @brief Function pointer for the destructor.
     *
     * @param src The source from where the components are destructed.
     * @param count The number of components to destruct.
     */
    void (*dtor)(void* src, usize count);

    /**
     * @brief Function pointer for the copy constructor.
     *
     * @param dst The destination where the components are constructed.
     * @param src The source from where the components are copied.
     * @param count The number of components to copy construct.
     */
    void (*copy_ctor)(void* dst, void* src, usize count);

    /**
     * @brief Function pointer for the copy assignment operator.
     *
     * @param dst The destination where the components are assigned.
     * @param src The source from where the components are copied.
     * @param count The number of components to copy assign.
     */
    void (*copy_assign)(void* dst, void* src, usize count);

    /**
     * @brief Function pointer for the move constructor.
     *
     * @param dst The destination where the components are move constructed.
     * @param src The source from where the components are moved.
     * @param count The number of components to move construct.
     */
    void (*move_ctor)(void* dst, void* src, usize count);

    /**
     * @brief Function pointer for the move assignment operator.
     *
     * @param dst The destination where the components are move assigned.
     * @param src The source from where the components are moved.
     * @param count The number of components to move assign.
     */
    void (*move_assign)(void* dst, void* src, usize count);

    /**
     * @brief Function pointer for the combined move constructor and destructor.
     *
     * @param dst The destination where the components are move constructed.
     * @param src The source from where the components are moved and destructed.
     * @param count The number of components to move construct and destruct.
     */
    void (*move_ctor_dtor)(void* dst, void* src, usize count);

    /**
     * @brief Function pointer for the combined move assignment and destructor.
     *
     * @param dst The destination where the components are move assigned.
     * @param src The source from where the components are moved and destructed.
     * @param count The number of components to move assign and destruct.
     */
    void (*move_assign_dtor)(void* dst, void* src, usize count);

    /**
     * @brief The size of the component type in bytes.
     */
    usize size;

    [[nodiscard]] auto operator==(const CompTypeInfo& rhs) const noexcept -> bool {
        return id == rhs.id and move_assign == rhs.move_assign;
    }
};

/**
 * @brief A type alias for a list of component type information structures.
 *
 * This alias represents a vector of `CompTypeInfo` objects, which store metadata
 * and operations for managing the lifecycle of different component types.
 */
using CompTypeList = std::vector<CompTypeInfo>;

/**
 * @brief A hash function object for `TypeList`.
 *
 * This structure defines a hash function for `TypeList` objects. It computes
 * a hash value by iterating over the elements of the list and combining their
 * individual hash values.
 */
struct TypeHash {
    /**
     * @brief Computes the hash value for a given `TypeList`.
     *
     * This operator computes a hash value for a `TypeList` by iterating through its elements
     * and accumulating their individual hash values using `wyhash`.
     *
     * @param x The `TypeList` to hash.
     * @return The computed hash value of type `u64`.
     */
    auto operator()(const CompTypeList& x) const noexcept -> u64 {
        u64 h{0};
        for (usize i{0}; i < x.size(); ++i) {
            // TODO: Implement hash function instead of depending on library detail
            h += ankerl::unordered_dense::detail::wyhash::hash(x[i].id);
        }
        return h;
    }
};

/**
 * @brief Default constructor implementation for type `T`.
 *
 * Constructs `count` instances of type `T` at the location pointed to by `ptr`.
 *
 * @tparam T The type to be constructed.
 * @param ptr Pointer to the memory location where the instances will be constructed.
 * @param count The number of instances to construct.
 */
template<typename T>
auto ctor_impl(void* ptr, const usize count) -> void {
    NIDAVELLIR_ASSERT(ptr, "The pointer should always be valid");

    T* arr = static_cast<T*>(ptr);
    for (usize i{0}; i < count; ++i) {
        new (std::addressof(arr[i])) T();
    }
}

/**
 * @brief Destructor implementation for type `T`.
 *
 * Destroys `count` instances of type `T` at the location pointed to by `ptr`.
 *
 * @tparam T The type to be destructed.
 * @param ptr Pointer to the memory location where the instances will be destructed.
 * @param count The number of instances to destruct.
 */
template<typename T>
auto dtor_impl(void* ptr, const usize count) -> void {
    NIDAVELLIR_ASSERT(ptr, "The pointer should always be valid");

    if constexpr (!std::is_trivially_destructible_v<T>) {
        T* arr = static_cast<T*>(ptr);
        for (usize i{0}; i < count; ++i) {
            arr[i].~T();
        }
    }
}

/**
 * @brief Copy constructor implementation for type `T`.
 *
 * Constructs `count` instances of type `T` at the location pointed to by `dst` using the instances from `src`.
 *
 * @tparam T The type to be copy-constructed.
 * @param dst Pointer to the destination memory location.
 * @param src Pointer to the source memory location.
 * @param count The number of instances to copy construct.
 */
template<typename T>
auto copy_ctor_impl(void* dst, void* src, const usize count) -> void {
    NIDAVELLIR_ASSERT(dst && src, "The pointers should always be valid");

    T* src_arr = static_cast<T*>(src);
    T* dst_arr = static_cast<T*>(dst);

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i{0}; i < count; ++i) {
            new (std::addressof(dst_arr[i])) T(src_arr[i]);
        }
    }
}

/**
 * @brief Copy assignment implementation for type `T`.
 *
 * Assigns `count` instances of type `T` at the location pointed to by `dst` using the instances from `src`.
 *
 * @tparam T The type to be copy-assigned.
 * @param dst Pointer to the destination memory location.
 * @param src Pointer to the source memory location.
 * @param count The number of instances to copy assign.
 */
template<typename T>
auto copy_assgin_impl(void* dst, void* src, const usize count) -> void {
    NIDAVELLIR_ASSERT(dst && src, "The pointers should always be valid");

    T* src_arr = static_cast<T*>(src);
    T* dst_arr = static_cast<T*>(dst);

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i{0}; i < count; ++i) {
            dst_arr[i] = src_arr[i];
        }
    }
}

/**
 * @brief Move constructor implementation for type `T`.
 *
 * Constructs `count` instances of type `T` at the location pointed to by `dst` using the instances from `src`.
 *
 * @tparam T The type to be move-constructed.
 * @param dst Pointer to the destination memory location.
 * @param src Pointer to the source memory location.
 * @param count The number of instances to move construct.
 */
template<typename T>
auto move_ctor_impl(void* dst, void* src, const usize count) -> void {
    NIDAVELLIR_ASSERT(dst && src, "The pointers should always be valid");

    T* src_arr = static_cast<T*>(src);
    T* dst_arr = static_cast<T*>(dst);

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i{0}; i < count; ++i) {
            new (std::addressof(dst_arr[i])) T(std::move(src_arr[i]));
        }
    }
}

/**
 * @brief Move assignment implementation for type `T`.
 *
 * Assigns `count` instances of type `T` at the location pointed to by `dst` using the instances from `src`.
 *
 * @tparam T The type to be move-assigned.
 * @param dst Pointer to the destination memory location.
 * @param src Pointer to the source memory location.
 * @param count The number of instances to move assign.
 */
template<typename T>
auto move_assign_impl(void* dst, void* src, const usize count) -> void {
    NIDAVELLIR_ASSERT(dst && src, "The pointers should always be valid");

    T* src_arr = static_cast<T*>(src);
    T* dst_arr = static_cast<T*>(dst);

    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i{0}; i < count; ++i) {
            dst_arr[i] = std::move(src_arr[i]);
        }
    }
}

/**
 * @brief Combined move constructor and destructor implementation for type `T`.
 *
 * Constructs `count` instances of type `T` at the location pointed to by `dst` using the instances from `src`,
 * and then destroys the instances at `src`.
 *
 * @tparam T The type to be move-constructed and destructed.
 * @param dst Pointer to the destination memory location.
 * @param src Pointer to the source memory location.
 * @param count The number of instances to move construct and destruct.
 */
template<typename T>
auto move_ctor_dtor_impl(void* dst, void* src, const usize count) -> void {
    NIDAVELLIR_ASSERT(dst && src, "The pointers should always be valid");

    T* src_arr = static_cast<T*>(src);
    T* dst_arr = static_cast<T*>(dst);

    if constexpr (std::is_trivially_copyable_v<T> or requires(T) { typename T::is_relocatable; }) {
        std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i{0}; i < count; ++i) {
            new (std::addressof(dst_arr[i])) T(std::move(src_arr[i]));
            if constexpr (!std::is_trivially_destructible_v<T>) {
                src_arr[i].~T();
            }
        }
    }
}

/**
 * @brief Combined move assignment and destructor implementation for type `T`.
 *
 * Assigns `count` instances of type `T` at the location pointed to by `dst` using the instances from `src`,
 * and then destroys the instances at `src`.
 *
 * @tparam T The type to be move-assigned and destructed.
 * @param dst Pointer to the destination memory location.
 * @param src Pointer to the source memory location.
 * @param count The number of instances to move assign and destruct.
 */
template<typename T>
auto move_assign_dtor_impl(void* dst, void* src, const usize count) -> void {
    NIDAVELLIR_ASSERT(dst && src, "The pointers should always be valid");

    T* src_arr = static_cast<T*>(src);
    T* dst_arr = static_cast<T*>(dst);

    if constexpr (std::is_trivially_copyable_v<T> or requires(T) { typename T::is_relocatable; }) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (usize i{0}; i < count; ++i) {
                dst_arr[i].~T();
            }
        }

        std::memcpy(dst, src, sizeof(T) * count);
    } else {
        for (usize i{0}; i < count; ++i) {
            dst_arr[i] = std::move(src_arr[i]);
            if constexpr (!std::is_trivially_destructible_v<T>) {
                src_arr[i].~T();
            }
        }
    }
}

/**
 * @brief Computes the FNV-1a hash for a given string at compile time.
 *
 * This function uses the FNV-1a hash algorithm to generate a hash value for the provided
 * null-terminated string. It is a constexpr function, allowing it to be evaluated at compile time.
 *
 * @param str A pointer to a null-terminated string to be hashed.
 * @return The computed hash value as a `usize`.
 */
constexpr auto fnv1a_hash(const char* str) -> usize {
    usize hash = 0xcbf29ce484222325;
    while (*str != 0) {
        hash ^= static_cast<std::size_t>(*str);
        hash *= 0x100000001b3;
        ++str;
    }
    return hash;
}

/**
 * @brief Generates a unique ID for a given type at compile time.
 *
 * This function uses the FNV-1a hash algorithm to generate a unique ID based on the type's
 * signature. It utilizes compiler-specific macros to obtain the type signature, ensuring(with high probability anyway)
 * uniqueness across different types.
 *
 * @tparam T The type for which to generate a unique ID.
 * @return The computed unique ID as a `usize`.
 */
template<Component T>
constexpr auto type_id_impl() -> usize {
#if defined(_MSC_VER)
    return fnv1a_hash(__FUNCSIG__);
#elif defined(__GNUC__) || defined(__clang__)
    return fnv1a_hash(__PRETTY_FUNCTION__);
#else
    // Runtime fallback
    return fnv1a_hash(typeid(T).name());
#endif
}

/**
 * @brief Wrapper function to generate a unique ID for a given type.
 *
 * This function calls the `type_id_impl` function, passing the decayed type
 * to ensure that the unique ID is consistent across all type variations
 * (e.g., references, const, volatile).
 *
 * @tparam T The type for which to generate a unique ID.
 * @return The computed unique ID as a `usize`.
 */
template<Component T>
constexpr auto type_id() -> usize {
    return type_id_impl<std::decay_t<T>>();
}

/**
 * @brief Retrieves the component type information for type `T`.
 *
 * Constructs a `CompTypeInfo` object that contains type information and function pointers
 * for managing the lifecycle of the component type `T`.
 *
 * @tparam T The component type to get the type information for.
 * @return A `CompTypeInfo` object containing the type information and lifecycle functions for type `T`.
 */
template<Component T>
[[nodiscard]] constexpr auto get_component_info() -> CompTypeInfo {
    using Ty = std::decay_t<T>;

    auto info = CompTypeInfo{
        .id = type_id<Ty>(),
        .alignment = alignof(Ty),
        .ctor = &ctor_impl<Ty>,
        .dtor = &dtor_impl<Ty>,
        .copy_ctor = nullptr,
        .copy_assign = nullptr,
        .move_ctor = &move_ctor_impl<Ty>,
        .move_assign = &move_assign_impl<Ty>,
        .move_ctor_dtor = &move_ctor_dtor_impl<Ty>,
        .move_assign_dtor = &move_assign_dtor_impl<Ty>,
        .size = sizeof(Ty)};

    if constexpr (std::is_copy_constructible_v<Ty>) {
        info.copy_ctor = &copy_ctor_impl<Ty>;
    }

    if constexpr (std::is_copy_assignable_v<Ty>) {
        info.copy_assign = &copy_assgin_impl<Ty>;
    }

    return info;
}

/**
 * @brief Helper function to check for duplicate types in a parameter pack.
 *
 * This function checks if there are any duplicate types in the parameter pack `Ts...`.
 *
 * @tparam Ts The parameter pack to check for duplicates.
 * @return `true` if there are duplicate types, `false` otherwise.
 */
template<typename... Ts>
consteval auto pack_has_duplicates() -> bool {
    if constexpr (sizeof...(Ts) == 0) {
        return false;
    }

    auto count = []<typename T, typename... Rest>() {
        return ((std::is_same_v<T, Rest> ? 1 : 0) + ...);
    };

    return ((count.template operator()<Ts, Ts...>() > 1) || ...);
}

static_assert(!pack_has_duplicates<i32, f32, f64>());
static_assert(pack_has_duplicates<f32, f32>());
static_assert(pack_has_duplicates<f32, i32, f32, f64>());
} // namespace nid
