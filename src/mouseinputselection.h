/* antimicrox Gamepad to KB+M event mapper
 * Copyright (C) 2026 AntiMicroX contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef MOUSEINPUTSELECTION_H
#define MOUSEINPUTSELECTION_H

#include <vector>

class InputDevice;
class JoyButton;

struct MouseInputCandidate
{
    JoyButton *button;
    InputDevice *source;
    bool enabled;
};

std::vector<JoyButton *> selectMouseInputButtons(const std::vector<MouseInputCandidate> &candidates);

#endif // MOUSEINPUTSELECTION_H
