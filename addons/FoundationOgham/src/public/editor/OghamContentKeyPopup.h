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

#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "editor/OghamPopupBase.h"

using namespace godot;

/// <summary>
/// Toolwindow popup for editing one content key — Type/Mode header row plus a
/// type-specific editor area (plain multi-line text for Text, key+asset path
/// for everything else). Direct port of OghamContentKeyPopup.gd.
/// </summary>
class OghamContentKeyPopup : public OghamPopupBase
{
    GDCLASS(OghamContentKeyPopup, OghamPopupBase);

private:
    static const char *const TYPES[5];
    static const char *const MODES[3];

    Dictionary content_key;
    Callable on_committed;

    OptionButton *type_button_ = nullptr;
    OptionButton *mode_button_ = nullptr;
    TextEdit *text_edit_ = nullptr;
    LineEdit *key_edit_ = nullptr;
    LineEdit *path_edit_ = nullptr;
    Control *path_row_ = nullptr;
    FileDialog *file_dialog_ = nullptr;

    void _ensure_built();
    void _update_visibility();
    void _on_type_changed(int index);
    void _do_close(bool commit);

public:
    OghamContentKeyPopup() = default;

    void open(const Dictionary &key, const Vector2i &screen_pos, const Callable &on_committed_cb);

protected:
    static void _bind_methods();
};
