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

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "editor/OghamPopupBase.h"

using namespace godot;

/// <summary>
/// Toolwindow popup for editing one option — tag, mode+text, target, and
/// compact Conditions/Operations lists — commits on close. Direct port of
/// OghamOptionPopup.gd. Conditions/operations are plain JSON Dictionaries
/// (this popup edits the raw .ogham manifest, not GameplayTagCondition/
/// Operation Resources), matching the GDScript original's own design note.
/// </summary>
class OghamOptionPopup : public OghamPopupBase
{
    GDCLASS(OghamOptionPopup, OghamPopupBase);

private:
    static const char *const MODES[3];
    static const char *const COMPARISONS[11];
    static const char *const LOGIC_OPS[3];
    static const char *const ARITHMETICS[7];

    Dictionary option;
    Callable on_committed;
    PackedStringArray all_tags;
    String target_tag_;

    LineEdit *tag_edit_ = nullptr;
    OptionButton *mode_button_ = nullptr;
    LineEdit *text_edit_ = nullptr;
    Button *target_button_ = nullptr;
    VBoxContainer *conditions_box_ = nullptr;
    VBoxContainer *operations_box_ = nullptr;

    void _ensure_built();
    void _rebuild_conditions();
    void _rebuild_operations();
    void _on_add_condition();
    void _on_add_operation();
    void _open_target_picker();
    void _on_target_selected(String tag);
    void _do_close(bool commit);

    // -- bound row-field-edit helpers (Callable::bind() targets onto a
    // specific condition/operation Dictionary captured by value — Dictionary
    // is Godot's own implicitly-shared type, so mutating the bound copy
    // mutates the same Dictionary stored in option["conditions"/"operations"]) --
    // 'value' comes FIRST — it's the signal-supplied argument (text_changed's
    // new text, value_changed's new number, toggled's new bool); cond/field
    // are appended via .bind() and so arrive AFTER whatever the signal itself
    // delivers, not before.
    void _set_condition_field(Variant value, Dictionary cond, String field);
    void _set_operation_field(Variant value, Dictionary op, String field);
    void _remove_condition_at(int index);
    void _remove_operation_at(int index);
    // OptionButton's "item_selected" signal delivers an index, not the string
    // value itself — these look it up in the relevant const array first, then
    // delegate to the generic setters above. A plain .bind()/.unbind() on the
    // generic setters can't do this index->string translation.
    void _set_condition_comparison(int idx, Dictionary cond);
    void _set_condition_logic(int idx, Dictionary cond);
    void _set_operation_arithmetic(int idx, Dictionary op);

public:
    OghamOptionPopup() = default;

    void open(const Dictionary &opt, const PackedStringArray &tags, const Vector2i &screen_pos, const Callable &on_committed_cb);

protected:
    static void _bind_methods();
};
