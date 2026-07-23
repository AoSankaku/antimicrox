/* antimicrox Gamepad to KB+M event mapper
 * Copyright (C) 2026 AntiMicroX contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "mouseinputselection.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
JoyButton *button(int id) { return reinterpret_cast<JoyButton *>(static_cast<uintptr_t>(id)); }

InputDevice *device(int id) { return reinterpret_cast<InputDevice *>(static_cast<uintptr_t>(id)); }

bool expectButtons(const std::vector<JoyButton *> &actual, const std::vector<JoyButton *> &expected, const char *testName)
{
    if (actual == expected)
        return true;

    std::cerr << testName << " failed: expected " << expected.size() << " selected buttons, got " << actual.size()
              << '\n';
    return false;
}
} // namespace

int main()
{
    bool passed = true;

    {
        const std::vector<MouseInputCandidate> candidates = {
            {button(1), device(1), true},
            {button(2), device(2), true},
            {button(3), device(1), true},
        };

        passed &= expectButtons(selectMouseInputButtons(candidates), {button(1), button(2), button(3)},
                                "multiple explicitly enabled sources are combined");
    }

    {
        const std::vector<MouseInputCandidate> candidates = {
            {button(1), device(1), false},
            {button(2), device(2), true},
            {button(3), device(1), false},
        };

        passed &= expectButtons(selectMouseInputButtons(candidates), {button(2)},
                                "only explicitly enabled source is used");
    }

    {
        const std::vector<MouseInputCandidate> candidates = {
            {button(1), device(1), false},
            {button(2), device(2), false},
        };

        passed &= expectButtons(selectMouseInputButtons(candidates), {}, "no enabled source produces no movement");
    }

    {
        const std::vector<MouseInputCandidate> candidates = {
            {nullptr, device(1), true},
            {button(1), nullptr, true},
            {button(2), device(2), true},
        };

        passed &= expectButtons(selectMouseInputButtons(candidates), {button(2)},
                                "invalid candidates are ignored");
    }

    {
        const std::vector<MouseInputCandidate> candidates = {
            {button(1), device(1), true},
            {button(1), device(1), true},
            {button(2), device(2), true},
        };

        passed &= expectButtons(selectMouseInputButtons(candidates), {button(1), button(2)},
                                "the same pending button contributes only once");
    }

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
