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

#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "editor/OghamDocument.h"
#include "editor/OghamPopupBase.h"

using namespace godot;

/// <summary>
/// Trello-style label picker — assign/unassign this node's labels via
/// checkbox, rename/recolor a label definition inline, or delete it entirely.
/// Direct port of OghamLabelPickerPopup.gd; 'doc' is the file's live
/// OghamDocument (labels are global to the file, not per-entry) — see
/// OghamDocument.h's update_label/set_label_assigned/add_label/remove_label.
/// </summary>
class OghamLabelPickerPopup : public OghamPopupBase
{
    GDCLASS(OghamLabelPickerPopup, OghamPopupBase);

private:
    Ref<OghamDocument> doc;
    String tag;
    Callable on_changed;

    VBoxContainer *list_box_ = nullptr;
    LineEdit *new_name_edit_ = nullptr;
    ColorPickerButton *new_color_btn_ = nullptr;

    void _ensure_built();
    void _rebuild_list();
    void _on_assign_toggled(bool pressed, int id);
    void _on_recolor(Color c, int id, String name);
    void _on_rename(String text, int id, Color color);
    void _on_delete_label(int id);
    void _on_add_label();

public:
    OghamLabelPickerPopup() = default;

    void open(const Ref<OghamDocument> &document, const String &entry_tag, const Vector2i &screen_pos, const Callable &on_changed_cb);

protected:
    static void _bind_methods();
};
