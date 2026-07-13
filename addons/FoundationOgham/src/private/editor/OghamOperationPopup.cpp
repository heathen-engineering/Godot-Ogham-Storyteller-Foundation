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

#include "editor/OghamOperationPopup.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

const char *const OghamOperationPopup::ARITHMETICS[7] = {"Set", "Add", "Subtract", "Multiply", "Divide", "Min", "Max"};

void OghamOperationPopup::_ensure_built()
{
    if (built_)
        return;
    built_ = true;
    set_size(Vector2i(280, 140));
    MarginContainer *margin = memnew(MarginContainer);
    margin->set_anchors_preset(Control::PRESET_FULL_RECT);
    margin->add_theme_constant_override("margin_left", 8);
    margin->add_theme_constant_override("margin_right", 8);
    margin->add_theme_constant_override("margin_top", 8);
    margin->add_theme_constant_override("margin_bottom", 8);
    add_child(margin);

    VBoxContainer *vbox = memnew(VBoxContainer);
    margin->add_child(vbox);

    tag_edit_ = memnew(LineEdit);
    tag_edit_->set_placeholder("tag");
    vbox->add_child(tag_edit_);

    HBoxContainer *row = memnew(HBoxContainer);
    arithmetic_button_ = memnew(OptionButton);
    for (const char *a : ARITHMETICS)
        arithmetic_button_->add_item(a);
    row->add_child(arithmetic_button_);
    value_spin_ = memnew(SpinBox);
    value_spin_->set_min(-2147483648.0);
    value_spin_->set_max(2147483647.0);
    value_spin_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    row->add_child(value_spin_);
    vbox->add_child(row);

    HBoxContainer *buttons = memnew(HBoxContainer);
    Button *save_btn = memnew(Button);
    save_btn->set_text("Save");
    save_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    save_btn->connect("pressed", callable_mp(this, &OghamOperationPopup::_do_close).bind(true));
    buttons->add_child(save_btn);
    Button *cancel_btn = memnew(Button);
    cancel_btn->set_text("Cancel");
    cancel_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    cancel_btn->connect("pressed", callable_mp(this, &OghamOperationPopup::_do_close).bind(false));
    buttons->add_child(cancel_btn);
    vbox->add_child(buttons);

    connect("popup_hide", callable_mp(this, &OghamOperationPopup::_do_close).bind(true));
}

void OghamOperationPopup::open(const Dictionary &op, const Vector2i &screen_pos, const Callable &on_committed_cb)
{
    _ensure_built();
    operation = op;
    on_committed = on_committed_cb;
    closed_ = false;

    tag_edit_->set_text(op.get("tag", ""));
    String arith = op.get("arithmetic", "Set");
    int idx = 0;
    for (int i = 0; i < 7; i++)
    {
        if (arith == ARITHMETICS[i])
        {
            idx = i;
            break;
        }
    }
    arithmetic_button_->select(idx);
    value_spin_->set_value(double(op.get("value", 0)));

    popup(Rect2i(screen_pos, get_size()));
}

void OghamOperationPopup::_do_close(bool commit)
{
    if (closed_)
        return;
    if (commit)
    {
        operation["tag"] = tag_edit_->get_text();
        operation["arithmetic"] = String(ARITHMETICS[arithmetic_button_->get_selected()]);
        operation["value"] = int(value_spin_->get_value());
        if (on_committed.is_valid())
            on_committed.call();
    }
    close_and_free();
}

void OghamOperationPopup::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open", "op", "screen_pos", "on_committed"), &OghamOperationPopup::open);
}
