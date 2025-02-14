// ========================================================================
// Project: OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
// File:    cross_tu.cpp
// Brief:   Cross-translation unit test for the ara::core::Abort API.
// Details: This file defines a function that sets an abort handler unique to this file,
//          then returns that handler so that main.cpp can verify that inline variables
//          are shared across translation units.
// Note:    All comments use the double-slash style.
// ========================================================================

#include "ara/core/abort.h"
#include <iostream>

// Define a dummy handler unique to this translation unit.
void CrossTUHandler() noexcept {
    std::cout << "[CrossTUHandler] Called from cross_tu.cpp." << std::endl;
}

/* Function: SetHandlerFromOtherTU
 * Description: Sets the abort handler to CrossTUHandler and returns the installed handler.
 * This function is defined in cross_tu.cpp so that main.cpp can test that the global
 * abort handler state is shared across translation units.
 */
ara::core::AbortHandler SetHandlerFromOtherTU() noexcept
{
    std::cout << "Setting abort handler in cross_tu.cpp." << std::endl;
    return ara::core::SetAbortHandler(&CrossTUHandler);
}