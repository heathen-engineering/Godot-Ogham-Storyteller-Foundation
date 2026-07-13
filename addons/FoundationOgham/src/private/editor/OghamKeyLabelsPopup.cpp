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

#include "editor/OghamKeyLabelsPopup.h"
#include "editor/OghamKeyLabelsNative.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>

using namespace godot;

void OghamKeyLabelsPopup::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    set_size(Vector2i(280, 240));
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

    Label *hint = memnew(Label);
    hint->set_text("Labels the contentKeys of every node by position (index 0, 1, 2, ...) — not tied to type or count.");
    hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    hint->add_theme_font_size_override("font_size", 11);
    hint->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
    vbox->add_child(hint);
    vbox->add_child(memnew(HSeparator));

    list_box_ = memnew(VBoxContainer);
    vbox->add_child(list_box_);

    vbox->add_child(memnew(HSeparator));

    HBoxContainer *new_row = memnew(HBoxContainer);
    new_name_edit_ = memnew(LineEdit);
    new_name_edit_->set_placeholder("New key label");
    new_name_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    new_name_edit_->connect("text_submitted", callable_mp(this, &OghamKeyLabelsPopup::_on_add).unbind(1));
    new_row->add_child(new_name_edit_);
    Button *add_btn = memnew(Button);
    add_btn->set_text("+");
    add_btn->connect("pressed", callable_mp(this, &OghamKeyLabelsPopup::_on_add));
    new_row->add_child(add_btn);
    vbox->add_child(new_row);

    OghamPopupBase *self_as_base = this;
    connect("popup_hide", callable_mp(self_as_base, &OghamPopupBase::close_and_free));
}

void OghamKeyLabelsPopup::open(const Vector2i &screen_pos, const Callable &on_changed_cb)
{
    _ensure_built();
    on_changed = on_changed_cb;
    closed_ = false;
    _rebuild_list();
    popup(Rect2i(screen_pos, get_size()));
}

void OghamKeyLabelsPopup::_rebuild_list()
{
    TypedArray<Node> children = list_box_->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    PackedStringArray labels = OghamKeyLabelsNative::get_labels();
    if (labels.is_empty())
    {
        Label *empty_label = memnew(Label);
        empty_label->set_text("No key labels defined yet.");
        empty_label->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
        list_box_->add_child(empty_label);
        return;
    }
    for (int i = 0; i < labels.size(); i++)
    {
        HBoxContainer *row = memnew(HBoxContainer);

        Label *index_label = memnew(Label);
        index_label->set_text(String::num_int64(i));
        index_label->set_custom_minimum_size(Vector2(20, 0));
        row->add_child(index_label);

        Button *up_btn = memnew(Button);
        up_btn->set_text(String::utf8("▲"));
        up_btn->set_disabled(i <= 0);
        up_btn->connect("pressed", callable_mp(this, &OghamKeyLabelsPopup::_move).bind(i, -1));
        row->add_child(up_btn);
        Button *down_btn = memnew(Button);
        down_btn->set_text(String::utf8("▼"));
        down_btn->set_disabled(i >= labels.size() - 1);
        down_btn->connect("pressed", callable_mp(this, &OghamKeyLabelsPopup::_move).bind(i, 1));
        row->add_child(down_btn);

        LineEdit *name_edit = memnew(LineEdit);
        name_edit->set_text(labels[i]);
        name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        name_edit->connect("text_submitted", callable_mp(this, &OghamKeyLabelsPopup::_rename).bind(i));
        name_edit->connect("focus_exited", callable_mp(this, &OghamKeyLabelsPopup::_rename).bind(name_edit->get_text(), i));
        row->add_child(name_edit);

        Button *delete_btn = memnew(Button);
        delete_btn->set_text("x");
        delete_btn->add_theme_color_override("font_color", Color(1.0, 0.45, 0.45));
        delete_btn->connect("pressed", callable_mp(this, &OghamKeyLabelsPopup::_delete).bind(i));
        row->add_child(delete_btn);

        list_box_->add_child(row);
    }
}

void OghamKeyLabelsPopup::_rename(String text, int index)
{
    PackedStringArray labels = OghamKeyLabelsNative::get_labels();
    if (index < 0 || index >= labels.size())
        return;
    labels.set(index, text);
    OghamKeyLabelsNative::set_labels(labels);
    if (on_changed.is_valid())
        on_changed.call();
}

void OghamKeyLabelsPopup::_move(int index, int delta)
{
    PackedStringArray labels = OghamKeyLabelsNative::get_labels();
    int target = index + delta;
    if (index < 0 || index >= labels.size() || target < 0 || target >= labels.size())
        return;
    String tmp = labels[index];
    labels.set(index, labels[target]);
    labels.set(target, tmp);
    OghamKeyLabelsNative::set_labels(labels);
    if (on_changed.is_valid())
        on_changed.call();
    _rebuild_list();
}

void OghamKeyLabelsPopup::_delete(int index)
{
    PackedStringArray labels = OghamKeyLabelsNative::get_labels();
    if (index < 0 || index >= labels.size())
        return;
    labels.remove_at(index);
    OghamKeyLabelsNative::set_labels(labels);
    if (on_changed.is_valid())
        on_changed.call();
    _rebuild_list();
}

void OghamKeyLabelsPopup::_on_add()
{
    String text = new_name_edit_->get_text().strip_edges();
    if (text.is_empty())
        return;
    PackedStringArray labels = OghamKeyLabelsNative::get_labels();
    labels.push_back(text);
    OghamKeyLabelsNative::set_labels(labels);
    new_name_edit_->set_text("");
    if (on_changed.is_valid())
        on_changed.call();
    _rebuild_list();
}

void OghamKeyLabelsPopup::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open", "screen_pos", "on_changed"), &OghamKeyLabelsPopup::open);
}
