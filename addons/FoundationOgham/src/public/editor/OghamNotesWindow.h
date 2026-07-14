/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "editor/OghamPopupBase.h"

using namespace godot;

/// <summary>
/// Small floating popup editing one node's Director Notes (VO export only —
/// see OghamScriptExporter). Direct port of Unity's OghamNotesWindow: commits
/// on Save or on losing focus, cancels on Escape/Cancel with no commit.
/// </summary>
class OghamNotesWindow : public OghamPopupBase
{
    GDCLASS(OghamNotesWindow, OghamPopupBase);

private:
    Callable on_commit;
    TextEdit *notes_edit_ = nullptr;

    void _ensure_built();
    void _commit();
    void _on_focus_exited();

public:
    OghamNotesWindow() = default;

    void open(const String &current_notes, const Vector2i &screen_pos, const Callable &on_commit_cb);

protected:
    static void _bind_methods();
};
