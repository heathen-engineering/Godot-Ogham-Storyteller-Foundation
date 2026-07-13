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

#include "editor/OghamLabelPickerPopup.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>

using namespace godot;

void OghamLabelPickerPopup::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    set_size(Vector2i(300, 200));
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_anchors_preset(Control::PRESET_FULL_RECT);
    add_child(scroll);

    MarginContainer *margin = memnew(MarginContainer);
    margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    margin->add_theme_constant_override("margin_left", 8);
    margin->add_theme_constant_override("margin_right", 8);
    margin->add_theme_constant_override("margin_top", 8);
    margin->add_theme_constant_override("margin_bottom", 8);
    scroll->add_child(margin);

    VBoxContainer *vbox = memnew(VBoxContainer);
    margin->add_child(vbox);

    list_box_ = memnew(VBoxContainer);
    vbox->add_child(list_box_);

    vbox->add_child(memnew(HSeparator));

    HBoxContainer *new_row = memnew(HBoxContainer);
    new_name_edit_ = memnew(LineEdit);
    new_name_edit_->set_placeholder("New label name");
    new_name_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    new_row->add_child(new_name_edit_);
    new_color_btn_ = memnew(ColorPickerButton);
    new_color_btn_->set_custom_minimum_size(Vector2(28, 0));
    new_color_btn_->set_pick_color(Color(0.4, 0.6, 0.9));
    new_row->add_child(new_color_btn_);
    Button *add_btn = memnew(Button);
    add_btn->set_text("+");
    add_btn->connect("pressed", callable_mp(this, &OghamLabelPickerPopup::_on_add_label));
    new_row->add_child(add_btn);
    vbox->add_child(new_row);

    OghamPopupBase *self_as_base = this;
    connect("popup_hide", callable_mp(self_as_base, &OghamPopupBase::close_and_free));
}

void OghamLabelPickerPopup::open(const Ref<OghamDocument> &document, const String &entry_tag, const Vector2i &screen_pos, const Callable &on_changed_cb)
{
    _ensure_built();
    doc = document;
    tag = entry_tag;
    on_changed = on_changed_cb;
    closed_ = false;
    _rebuild_list();
    popup(Rect2i(screen_pos, get_size()));
}

void OghamLabelPickerPopup::_rebuild_list()
{
    TypedArray<Node> children = list_box_->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    Array labels = doc->get_labels();
    Array assigned = doc->get_assigned_label_ids(tag);
    if (labels.is_empty())
    {
        Label *empty_label = memnew(Label);
        empty_label->set_text("No labels defined yet.");
        empty_label->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
        list_box_->add_child(empty_label);
        return;
    }
    for (int i = 0; i < labels.size(); i++)
    {
        Dictionary def = labels[i];
        int id = int(def.get("id", -1));
        String name = def.get("name", "");
        Color color = Color::html(def.get("color", "#6699CC"));
        HBoxContainer *row = memnew(HBoxContainer);

        CheckBox *check = memnew(CheckBox);
        check->set_pressed(assigned.has(id));
        check->connect("toggled", callable_mp(this, &OghamLabelPickerPopup::_on_assign_toggled).bind(id));
        row->add_child(check);

        ColorPickerButton *color_btn = memnew(ColorPickerButton);
        color_btn->set_custom_minimum_size(Vector2(24, 0));
        color_btn->set_pick_color(color);
        color_btn->connect("color_changed", callable_mp(this, &OghamLabelPickerPopup::_on_recolor).bind(id, name));
        row->add_child(color_btn);

        LineEdit *name_edit = memnew(LineEdit);
        name_edit->set_text(name);
        name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        name_edit->connect("text_submitted", callable_mp(this, &OghamLabelPickerPopup::_on_rename).bind(id, color));
        name_edit->connect("focus_exited", callable_mp(this, &OghamLabelPickerPopup::_on_rename).bind(name_edit->get_text(), id, color).unbind(0));
        row->add_child(name_edit);

        Button *delete_btn = memnew(Button);
        delete_btn->set_text("x");
        delete_btn->add_theme_color_override("font_color", Color(1.0, 0.45, 0.45));
        delete_btn->connect("pressed", callable_mp(this, &OghamLabelPickerPopup::_on_delete_label).bind(id));
        row->add_child(delete_btn);

        list_box_->add_child(row);
    }
}

void OghamLabelPickerPopup::_on_assign_toggled(bool pressed, int id)
{
    doc->set_label_assigned(tag, id, pressed);
    if (on_changed.is_valid())
        on_changed.call();
}

void OghamLabelPickerPopup::_on_recolor(Color c, int id, String name)
{
    doc->update_label(id, name, c);
    if (on_changed.is_valid())
        on_changed.call();
}

void OghamLabelPickerPopup::_on_rename(String text, int id, Color color)
{
    doc->update_label(id, text, color);
    if (on_changed.is_valid())
        on_changed.call();
}

void OghamLabelPickerPopup::_on_delete_label(int id)
{
    doc->remove_label(id);
    if (on_changed.is_valid())
        on_changed.call();
    _rebuild_list();
}

void OghamLabelPickerPopup::_on_add_label()
{
    String name = new_name_edit_->get_text().strip_edges();
    if (name.is_empty())
        return;
    doc->add_label(name, new_color_btn_->get_pick_color());
    new_name_edit_->set_text("");
    if (on_changed.is_valid())
        on_changed.call();
    _rebuild_list();
}

void OghamLabelPickerPopup::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open", "document", "entry_tag", "screen_pos", "on_changed"), &OghamLabelPickerPopup::open);
}
