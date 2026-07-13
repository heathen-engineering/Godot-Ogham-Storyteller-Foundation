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

#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// <summary>
/// One-row compact editor for OghamContentKey — Type | Mode | (key_or_value,
/// shown as a Lexicon key picker when Localised, else a plain text field for
/// Text-type content; asset_path + asset_sub_name for other types in
/// Literal/Invariant mode). Direct C++ port of OghamContentKeyCompactEditor.gd.
///
/// Reuses FoundationLexicon's LexiconKeyPickerProperty when that addon is
/// enabled — a soft dependency, same as the GDScript original: no compile-time
/// header/link dependency on FoundationLexicon. Instantiated dynamically via
/// ResourceLoader + Script::call("new") (LexiconKeyPickerProperty is still a
/// GDScript class as of this port).
///
/// CAVEAT for whoever ports LexiconKeyPickerProperty to C++ later: this file
/// calls key_picker_->call("_update_property") in _update_property() below,
/// exactly mirroring the GDScript original's direct _key_picker._update_property()
/// call. That works today because GDScript methods (even underscore-prefixed
/// ones matching an engine virtual's name) are always Object::call()-able —
/// confirmed empirically that this is NOT true for a GDExtension C++ class's
/// own override of a virtual like _update_property(): BIND_VIRTUAL_METHOD
/// registers it purely as an internal engine-invoked hook, not as a callable
/// script method, so .call("_update_property") on a C++-backed picker would
/// fail with "Nonexistent function". Object::call()/connect() dispatch by
/// plain (non-virtual) method name, e.g. set_object_and_property() a few
/// lines below, is unaffected and safe either way.
/// </summary>
class OghamContentKeyCompactEditor : public EditorProperty
{
    GDCLASS(OghamContentKeyCompactEditor, EditorProperty);

private:
    static const char *const TYPES[5];
    static const char *const MODES[3];
    static const int TYPE_TEXT = 0;

    OptionButton *type_button_ = nullptr;
    OptionButton *mode_button_ = nullptr;
    Control *key_picker_ = nullptr; // dynamically-typed LexiconKeyPickerProperty, or null if Lexicon isn't enabled
    LineEdit *literal_text_edit_ = nullptr;
    LineEdit *asset_path_edit_ = nullptr;
    LineEdit *asset_sub_edit_ = nullptr;
    bool updating_ = false;
    bool built_ = false; // see OghamPopupBase's built_ doc comment — same constructor-timing hazard applies here

    void _ensure_built();
    void _on_type_selected(int index);
    void _on_mode_selected(int index);
    void _on_literal_text_submitted(String text);
    void _on_asset_path_submitted(String text);
    void _on_asset_sub_submitted(String text);
    // focus_exited carries no signal args (unlike text_submitted, which
    // supplies the text directly) — these read the LineEdit's current text
    // at the moment focus is lost and forward it, mirroring the GDScript
    // original's `func(): _on_x_submitted(_x_edit.text)` closures. A bound
    // Callable arg would be wrong here: bind() captures its value at
    // connect() time (this method's construction), not at signal-fire time,
    // so it would always resubmit an empty string.
    void _on_literal_text_focus_exited();
    void _on_asset_path_focus_exited();
    void _on_asset_sub_focus_exited();
    void _relay_property_changed(StringName property, Variant value, StringName field, bool changing);

public:
    OghamContentKeyCompactEditor() = default;

    virtual void _update_property() override;

protected:
    static void _bind_methods() {}
};
