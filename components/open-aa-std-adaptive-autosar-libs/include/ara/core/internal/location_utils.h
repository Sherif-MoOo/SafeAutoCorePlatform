/***********************************************************************************************************************
 *  PROJECT
 *  --------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  --------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       location_utils.h
 *  \brief      Internal utilities for compile-time extraction of filename/line information.
 *
 *  \details    This file defines helper functions and macros for path-stripping and location capturing.
 *              It provides a fully constexpr implementation that strips directory paths from __FILE__ at compile time.
 *  \note       This helper could be replace with std::source_location if you're using (CXX_STANDARD 20)
 ***********************************************************************************************************************/

#ifndef ARA_CORE_INTERNAL_LOCATION_UTILS_H_
#define ARA_CORE_INTERNAL_LOCATION_UTILS_H_

#include <cstddef>      // For std::size_t
#include <string_view>  // For std::string_view

namespace ara {
namespace core {
namespace internal {

/*!
 * \brief   A constexpr function template to strip directory paths from a string literal (e.g. __FILE__).
 * \tparam  N  The size of the string literal (including the null terminator).
 * \param   fullPath  A reference to a string literal representing the full path.
 * \return  A std::string_view pointing to the filename part (after the last '/' or '\\').
 *
 * \details 
 *  - Iterates backwards over the string literal until a path separator is found.
 *  - If no separator is found, the entire string is returned.
 *  - Because it accepts a string literal reference, the operation is fully evaluated at compile time.
 */
template <std::size_t N>
constexpr std::string_view StripFilePath(const char (&fullPath)[N]) noexcept {
    std::string_view path{ fullPath, N - 1 }; // Exclude the null terminator
    for (std::size_t i = path.size(); i > 0; --i) {
        if (path[i - 1] == '/' || path[i - 1] == '\\') {
            return path.substr(i);
        }
    }
    return path;
}

}  // namespace internal
}  // namespace core
}  // namespace ara

/*!
 * \brief   Helper macros to stringify tokens.
 */
#define ARA_CORE_LOC_STRINGIFY_(x) #x
#define ARA_CORE_LOC_STRINGIFY(x)  ARA_CORE_LOC_STRINGIFY_(x)

/*!
 * \brief   Provides a compile-time evaluated, path-stripped version of __FILE__.
 *
 * \return  A std::string_view pointing to the filename without directory paths.
 */
#define ARA_CORE_INTERNAL_FILE (::ara::core::internal::StripFilePath(__FILE__))

/*!
 * \brief   Provides a compile-time evaluated, path-stripped version of __FILE__ with line number.
 *
 * \details 
 *  - Uses compile-time literal concatenation to form a string of the format "file.cpp:123".
 *  - The StripFilePath function removes any leading directory information.
 *
 * \return  A std::string_view containing the stripped filename and line number.
 */
#define ARA_CORE_INTERNAL_FILELINE (::ara::core::internal::StripFilePath(__FILE__ ":" ARA_CORE_LOC_STRINGIFY(__LINE__)))

#endif // ARA_CORE_INTERNAL_LOCATION_UTILS_H_
 