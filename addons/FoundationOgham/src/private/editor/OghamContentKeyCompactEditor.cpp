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

#include "editor/OghamContentKeyCompactEditor.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

const char *const OghamContentKeyCompactEditor::TYPES[5] = {"Text", "Image", "Sprite", "Audio", "PackedScene"};
const char *const OghamContentKeyCompactEditor::MODES[3] = {"Localised", "Literal", "Invariant"};

void OghamContentKeyCompactEditor::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    HBoxContainer *row = memnew(HBoxContainer);

    type_button_ = memnew(OptionButton);
    for (const char *t : TYPES)
        type_button_->add_item(String(t));
    type_button_->connect("item_selected", callable_mp(this, &OghamContentKeyCompactEditor::_on_type_selected));
    row->add_child(type_button_);

    mode_button_ = memnew(OptionButton);
    for (const char *m : MODES)
        mode_button_->add_item(String(m));
    mode_button_->connect("item_selected", callable_mp(this, &OghamContentKeyCompactEditor::_on_mode_selected));
    row->add_child(mode_button_);

    literal_text_edit_ = memnew(LineEdit);
    literal_text_edit_->set_placeholder("Text content...");
    literal_text_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    literal_text_edit_->connect("text_submitted", callable_mp(this, &OghamContentKeyCompactEditor::_on_literal_text_submitted));
    literal_text_edit_->connect("focus_exited", callable_mp(this, &OghamContentKeyCompactEditor::_on_literal_text_focus_exited));
    row->add_child(literal_text_edit_);

    asset_path_edit_ = memnew(LineEdit);
    asset_path_edit_->set_placeholder("res://path/to/asset");
    asset_path_edit_->set_custom_minimum_size(Vector2(200, 0));
    asset_path_edit_->connect("text_submitted", callable_mp(this, &OghamContentKeyCompactEditor::_on_asset_path_submitted));
    asset_path_edit_->connect("focus_exited", callable_mp(this, &OghamContentKeyCompactEditor::_on_asset_path_focus_exited));
    row->add_child(asset_path_edit_);

    asset_sub_edit_ = memnew(LineEdit);
    asset_sub_edit_->set_placeholder("sub-resource");
    asset_sub_edit_->set_custom_minimum_size(Vector2(100, 0));
    asset_sub_edit_->connect("text_submitted", callable_mp(this, &OghamContentKeyCompactEditor::_on_asset_sub_submitted));
    asset_sub_edit_->connect("focus_exited", callable_mp(this, &OghamContentKeyCompactEditor::_on_asset_sub_focus_exited));
    row->add_child(asset_sub_edit_);

    // LexiconKeyPickerProperty is only available if FoundationLexicon's plugin is
    // enabled — no compile-time dependency on that addon. Instantiated
    // dynamically via ResourceLoader + Script::call("new") (the standard
    // GDExtension idiom for instantiating an arbitrary, possibly-GDScript-
    // backed class by path); if the file doesn't exist, Localised mode just
    // falls back to the plain literal text field, same as the GDScript
    // original's ResourceLoader.exists() guard.
    const String lexicon_picker_path = "res://addons/FoundationLexicon/editor/LexiconKeyPickerProperty.gd";
    if (ResourceLoader::get_singleton()->exists(lexicon_picker_path))
    {
        Ref<Script> picker_script = ResourceLoader::get_singleton()->load(lexicon_picker_path);
        if (picker_script.is_valid())
        {
            Variant instance = picker_script->call("new");
            key_picker_ = Object::cast_to<Control>(instance);
            if (key_picker_ != nullptr)
            {
                key_picker_->set_custom_minimum_size(Vector2(200, 0));
                key_picker_->connect(
                    "property_changed",
                    callable_mp(this, &OghamContentKeyCompactEditor::_relay_property_changed));
                row->add_child(key_picker_);
            }
        }
    }

    add_child(row);
    add_focusable(type_button_);
}

void OghamContentKeyCompactEditor::_update_property()
{
    _ensure_built();

    updating_ = true;
    Object *obj = get_edited_object();

    int type_val = int(obj->get("type"));
    int mode_val = int(obj->get("mode"));
    type_button_->select(type_val);
    mode_button_->select(mode_val);

    bool is_localised = mode_val == 0;
    bool is_text = type_val == TYPE_TEXT;

    literal_text_edit_->set_visible(is_text && !is_localised);
    if (key_picker_ != nullptr)
        key_picker_->set_visible(is_localised);
    asset_path_edit_->set_visible(!is_text && !is_localised);
    asset_sub_edit_->set_visible(!is_text && !is_localised);

    if (is_localised && key_picker_ != nullptr)
    {
        key_picker_->call("set_object_and_property", obj, StringName("key_or_value"));
        key_picker_->call("_update_property");
    }
    else if (is_text)
    {
        literal_text_edit_->set_text(String(obj->get("key_or_value")));
    }
    else
    {
        asset_path_edit_->set_text(String(obj->get("asset_path")));
        asset_sub_edit_->set_text(String(obj->get("asset_sub_name")));
    }

    updating_ = false;
}

void OghamContentKeyCompactEditor::_on_type_selected(int index)
{
    if (updating_)
        return;
    emit_changed("type", index);
}

void OghamContentKeyCompactEditor::_on_mode_selected(int index)
{
    if (updating_)
        return;
    emit_changed("mode", index);
}

void OghamContentKeyCompactEditor::_on_literal_text_submitted(String text)
{
    if (updating_)
        return;
    emit_changed("key_or_value", text);
}

void OghamContentKeyCompactEditor::_on_asset_path_submitted(String text)
{
    if (updating_)
        return;
    emit_changed("asset_path", text);
}

void OghamContentKeyCompactEditor::_on_asset_sub_submitted(String text)
{
    if (updating_)
        return;
    emit_changed("asset_sub_name", text);
}

void OghamContentKeyCompactEditor::_on_literal_text_focus_exited()
{
    _on_literal_text_submitted(literal_text_edit_->get_text());
}

void OghamContentKeyCompactEditor::_on_asset_path_focus_exited()
{
    _on_asset_path_submitted(asset_path_edit_->get_text());
}

void OghamContentKeyCompactEditor::_on_asset_sub_focus_exited()
{
    _on_asset_sub_submitted(asset_sub_edit_->get_text());
}

void OghamContentKeyCompactEditor::_relay_property_changed(StringName property, Variant value, StringName field, bool changing)
{
    if (updating_)
        return;
    if (property == StringName("key_or_value"))
        emit_changed("key_or_value", value, field, changing);
}
