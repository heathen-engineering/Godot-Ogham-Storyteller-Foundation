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

#include "editor/OghamContentKeyPopup.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

const char *const OghamContentKeyPopup::TYPES[5] = {"Text", "Image", "Sprite", "Audio", "PackedScene"};
const char *const OghamContentKeyPopup::MODES[3] = {"Literal", "Invariant", "Localised"};

void OghamContentKeyPopup::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    set_size(Vector2i(420, 320));
    MarginContainer *margin = memnew(MarginContainer);
    margin->set_anchors_preset(Control::PRESET_FULL_RECT);
    margin->add_theme_constant_override("margin_left", 8);
    margin->add_theme_constant_override("margin_right", 8);
    margin->add_theme_constant_override("margin_top", 8);
    margin->add_theme_constant_override("margin_bottom", 8);
    add_child(margin);

    VBoxContainer *vbox = memnew(VBoxContainer);
    margin->add_child(vbox);

    HBoxContainer *header = memnew(HBoxContainer);
    type_button_ = memnew(OptionButton);
    for (const char *t : TYPES)
        type_button_->add_item(t);
    type_button_->connect("item_selected", callable_mp(this, &OghamContentKeyPopup::_on_type_changed));
    header->add_child(type_button_);
    mode_button_ = memnew(OptionButton);
    for (const char *m : MODES)
        mode_button_->add_item(m);
    header->add_child(mode_button_);
    vbox->add_child(header);

    text_edit_ = memnew(TextEdit);
    text_edit_->set_custom_minimum_size(Vector2(0, 180));
    text_edit_->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    vbox->add_child(text_edit_);

    key_edit_ = memnew(LineEdit);
    key_edit_->set_placeholder("key (literal text, or Lexicon key when Localised)");
    vbox->add_child(key_edit_);

    HBoxContainer *path_row = memnew(HBoxContainer);
    path_row_ = path_row;
    path_edit_ = memnew(LineEdit);
    path_edit_->set_placeholder("res://path/to/asset");
    path_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    path_row->add_child(path_edit_);
    Button *browse_btn = memnew(Button);
    browse_btn->set_text("...");
    file_dialog_ = memnew(FileDialog);
    file_dialog_->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    file_dialog_->set_access(FileDialog::ACCESS_RESOURCES);
    file_dialog_->connect("file_selected", callable_mp(path_edit_, &LineEdit::set_text));
    Window *file_dialog_as_window = file_dialog_;
    browse_btn->connect("pressed", callable_mp(file_dialog_as_window, &Window::popup_centered_ratio).bind(0.6f));
    path_row->add_child(browse_btn);
    vbox->add_child(path_row);
    add_child(file_dialog_);

    HBoxContainer *buttons = memnew(HBoxContainer);
    Button *save_btn = memnew(Button);
    save_btn->set_text("Save");
    save_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    save_btn->connect("pressed", callable_mp(this, &OghamContentKeyPopup::_do_close).bind(true));
    buttons->add_child(save_btn);
    Button *cancel_btn = memnew(Button);
    cancel_btn->set_text("Cancel");
    cancel_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    cancel_btn->connect("pressed", callable_mp(this, &OghamContentKeyPopup::_do_close).bind(false));
    buttons->add_child(cancel_btn);
    vbox->add_child(buttons);

    connect("popup_hide", callable_mp(this, &OghamContentKeyPopup::_do_close).bind(true));
}

void OghamContentKeyPopup::open(const Dictionary &key, const Vector2i &screen_pos, const Callable &on_committed_cb)
{
    _ensure_built();
    content_key = key;
    on_committed = on_committed_cb;
    closed_ = false;

    String type = key.get("type", "Text");
    int type_idx = 0;
    for (int i = 0; i < 5; i++)
        if (type == TYPES[i]) { type_idx = i; break; }
    type_button_->select(type_idx);

    String mode = key.get("mode", "Literal");
    int mode_idx = 0;
    for (int i = 0; i < 3; i++)
        if (mode == MODES[i]) { mode_idx = i; break; }
    mode_button_->select(mode_idx);

    text_edit_->set_text(key.get("key", ""));
    key_edit_->set_text(key.get("key", ""));
    path_edit_->set_text(key.get("path", ""));
    _update_visibility();

    popup(Rect2i(screen_pos, get_size()));
}

void OghamContentKeyPopup::_on_type_changed(int)
{
    _update_visibility();
}

void OghamContentKeyPopup::_update_visibility()
{
    bool is_text = String(TYPES[type_button_->get_selected()]) == "Text";
    text_edit_->set_visible(is_text);
    key_edit_->set_visible(!is_text);
    path_row_->set_visible(!is_text);
}

void OghamContentKeyPopup::_do_close(bool commit)
{
    if (closed_)
        return;
    if (commit)
    {
        String type = TYPES[type_button_->get_selected()];
        content_key["type"] = type;
        content_key["mode"] = String(MODES[mode_button_->get_selected()]);
        if (type == "Text")
        {
            content_key["key"] = text_edit_->get_text();
        }
        else
        {
            content_key["key"] = key_edit_->get_text();
            content_key["path"] = path_edit_->get_text();
        }
        if (on_committed.is_valid())
            on_committed.call();
    }
    close_and_free();
}

void OghamContentKeyPopup::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open", "key", "screen_pos", "on_committed"), &OghamContentKeyPopup::open);
}
