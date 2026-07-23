/* antimicrox Gamepad to KB+M event mapper
 * Copyright (C) 2026 AntiMicroX contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "mouseinputselection.h"

#include <algorithm>

std::vector<JoyButton *> selectMouseInputButtons(const std::vector<MouseInputCandidate> &candidates)
{
    std::vector<JoyButton *> selectedButtons;
    selectedButtons.reserve(candidates.size());
    for (const MouseInputCandidate &candidate : candidates)
    {
        if (candidate.button != nullptr && candidate.source != nullptr && candidate.enabled &&
            std::find(selectedButtons.cbegin(), selectedButtons.cend(), candidate.button) == selectedButtons.cend())
        {
            selectedButtons.push_back(candidate.button);
        }
    }

    return selectedButtons;
}
