/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  Author: Sherif Mohamed
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/internal/span_storage.h
 *  \brief      Internal storage for span types.
 *  \details    This file provides the implementation details for span storage,
 *              including the span_storage_base class and subspan_extent calculation.
 ***********************************************************************************************************************/

#ifndef ARA_CORE_INTERNAL_STORAGE_SPAN_STORAGE_H_
#define ARA_CORE_INTERNAL_STORAGE_SPAN_STORAGE_H_

#include "ara/core/internal/utility.h"  // For utility functions and traits

namespace ara::core::detail {

/***********************************************************************************************************************
 *  SPAN STORAGE BASE CLASS - PRIMARY TEMPLATE FOR STATIC EXTENT
 ***********************************************************************************************************************/
/*!
 * \brief  Storage optimization for compile-time known extent
 * \tparam ElementType  Type of elements in the span
 * \tparam Extent       Number of elements (compile-time constant)
 */
template<typename ElementType, std::size_t Extent>
struct span_storage_base {
public:
    ElementType* data_{nullptr};

protected:
    constexpr span_storage_base() noexcept = default;
    constexpr span_storage_base(const span_storage_base&) noexcept = default;
    constexpr span_storage_base(span_storage_base&&) noexcept = default;
    constexpr auto operator=(const span_storage_base&) noexcept -> span_storage_base& = default;
    constexpr auto operator=(span_storage_base&&) noexcept -> span_storage_base& = default;
    ~span_storage_base() = default;

    explicit constexpr span_storage_base(ElementType* data, std::size_t /*unused*/) noexcept 
        : data_{data} {
            static_assert(static_size != ara::core::dynamic_extent, 
                          "\n[Error] Cannot use dynamic extent with static span storage.\n");
    }    
    
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { 
        return static_size;
    }
    
private:
    static constexpr std::size_t static_size = Extent;
};

/***********************************************************************************************************************
 *  SPAN STORAGE BASE CLASS - SPECIALIZATION FOR DYNAMIC EXTENT
 ***********************************************************************************************************************/
template<typename ElementType>
struct span_storage_base<ElementType, ara::core::dynamic_extent> {
public:
    /*Data members shall be public for standard layout*/
    ElementType* data_{nullptr};
    std::size_t size_{0};
    
protected:
    constexpr span_storage_base() noexcept = default;
    constexpr span_storage_base(const span_storage_base&) noexcept = default;
    constexpr span_storage_base(span_storage_base&&) noexcept = default;
    constexpr auto operator=(const span_storage_base&) noexcept -> span_storage_base& = default;
    constexpr auto operator=(span_storage_base&&) noexcept -> span_storage_base& = default;
    ~span_storage_base() = default;
    
    explicit constexpr span_storage_base(ElementType* data, std::size_t size) noexcept 
        : data_{data}, size_{size} {}
    
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { 
        return size_; 
    }

};

/***********************************************************************************************************************
 *  COMPILE-TIME EXTENT CALCULATIONS
 ***********************************************************************************************************************/
/*!
 * \brief  Calculate subspan extent at compile time
 */
template<std::size_t Extent, std::size_t Offset, std::size_t Count>
struct subspan_extent {
    static constexpr std::size_t value = 
        Count != ara::core::dynamic_extent ? Count :
        (Extent != ara::core::dynamic_extent ? Extent - Offset : ara::core::dynamic_extent);
};

} // namespace ara::core::detail

#endif // ARA_CORE_INTERNAL_STORAGE_SPAN_STORAGE_H_