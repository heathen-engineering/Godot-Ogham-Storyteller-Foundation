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

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "editor/OghamPopupBase.h"

using namespace godot;

/// <summary>
/// Editor for the project-wide, position-based contentKey labels (see
/// OghamKeyLabelsNative.h) — an ordered list of plain name fields,
/// reorderable, deletable, with a trailing row to append a new slot. Direct
/// port of OghamKeyLabelsPopup.gd.
/// </summary>
class OghamKeyLabelsPopup : public OghamPopupBase
{
    GDCLASS(OghamKeyLabelsPopup, OghamPopupBase);

private:
    Callable on_changed;
    VBoxContainer *list_box_ = nullptr;
    LineEdit *new_name_edit_ = nullptr;

    void _ensure_built();
    void _rebuild_list();
    void _rename(String text, int index); // 'text' first — matches text_submitted's signal-supplied arg position; focus_exited's binding is ordered to match
    void _move(int index, int delta);
    void _delete(int index);
    void _on_add();

public:
    OghamKeyLabelsPopup() = default;

    void open(const Vector2i &screen_pos, const Callable &on_changed_cb);

protected:
    static void _bind_methods();
};
