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

#include "editor/OghamOptionPopup.h"

#include "editor/OghamTargetPickerPopup.h"

#include <algorithm>

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>

using namespace godot;

const char *const OghamOptionPopup::MODES[3] = {"Literal", "Invariant", "Localised"};
const char *const OghamOptionPopup::COMPARISONS[11] = {"Exists", "NotExists", "Equal", "NotEqual", "Less", "LessEqual", "Greater", "GreaterEqual", "IsMemberOf", "IsParentOf", "IsExactly"};
const char *const OghamOptionPopup::LOGIC_OPS[3] = {"And", "Or", "Xor"};
const char *const OghamOptionPopup::ARITHMETICS[7] = {"Set", "Add", "Subtract", "Multiply", "Divide", "Min", "Max"};

namespace
{
    int find_index(const char *const *arr, int count, const String &value)
    {
        for (int i = 0; i < count; i++)
            if (value == arr[i])
                return i;
        return -1;
    }
}

void OghamOptionPopup::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    set_size(Vector2i(420, 480));
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

    tag_edit_ = memnew(LineEdit);
    vbox->add_child(tag_edit_);

    HBoxContainer *mode_row = memnew(HBoxContainer);
    mode_button_ = memnew(OptionButton);
    for (const char *m : MODES)
        mode_button_->add_item(m);
    mode_row->add_child(mode_button_);
    text_edit_ = memnew(LineEdit);
    text_edit_->set_placeholder("display text");
    text_edit_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    mode_row->add_child(text_edit_);
    vbox->add_child(mode_row);

    HBoxContainer *target_row = memnew(HBoxContainer);
    target_button_ = memnew(Button);
    target_button_->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    target_button_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    target_button_->connect("pressed", callable_mp(this, &OghamOptionPopup::_open_target_picker));
    target_row->add_child(target_button_);
    Button *target_clear = memnew(Button);
    target_clear->set_text("Clear");
    target_clear->connect("pressed", callable_mp(this, &OghamOptionPopup::_on_target_selected).bind(String()));
    target_row->add_child(target_clear);
    vbox->add_child(target_row);
    Label *target_hint = memnew(Label);
    target_hint->set_text("Leave as (none) to close the conversation.");
    target_hint->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
    vbox->add_child(target_hint);

    vbox->add_child(memnew(HSeparator));
    HBoxContainer *cond_header = memnew(HBoxContainer);
    Label *cond_label = memnew(Label);
    cond_label->set_text("Conditions");
    cond_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    cond_header->add_child(cond_label);
    Button *cond_add = memnew(Button);
    cond_add->set_text("+");
    cond_add->connect("pressed", callable_mp(this, &OghamOptionPopup::_on_add_condition));
    cond_header->add_child(cond_add);
    vbox->add_child(cond_header);
    conditions_box_ = memnew(VBoxContainer);
    vbox->add_child(conditions_box_);

    vbox->add_child(memnew(HSeparator));
    HBoxContainer *op_header = memnew(HBoxContainer);
    Label *op_label = memnew(Label);
    op_label->set_text("Operations");
    op_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    op_header->add_child(op_label);
    Button *op_add = memnew(Button);
    op_add->set_text("+");
    op_add->connect("pressed", callable_mp(this, &OghamOptionPopup::_on_add_operation));
    op_header->add_child(op_add);
    vbox->add_child(op_header);
    operations_box_ = memnew(VBoxContainer);
    vbox->add_child(operations_box_);

    vbox->add_child(memnew(HSeparator));
    HBoxContainer *buttons = memnew(HBoxContainer);
    Button *save_btn = memnew(Button);
    save_btn->set_text("Save");
    save_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    save_btn->connect("pressed", callable_mp(this, &OghamOptionPopup::_do_close).bind(true));
    buttons->add_child(save_btn);
    Button *cancel_btn = memnew(Button);
    cancel_btn->set_text("Cancel");
    cancel_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    cancel_btn->connect("pressed", callable_mp(this, &OghamOptionPopup::_do_close).bind(false));
    buttons->add_child(cancel_btn);
    vbox->add_child(buttons);

    connect("popup_hide", callable_mp(this, &OghamOptionPopup::_do_close).bind(true));
}

void OghamOptionPopup::open(const Dictionary &opt, const PackedStringArray &tags, const Vector2i &screen_pos, const Callable &on_committed_cb)
{
    _ensure_built();
    option = opt;
    all_tags = tags;
    on_committed = on_committed_cb;
    closed_ = false;

    tag_edit_->set_text(opt.get("tag", ""));
    int mode_idx = find_index(MODES, 3, String(opt.get("mode", "Literal")));
    mode_button_->select(std::max(mode_idx, 0));
    text_edit_->set_text(opt.get("textKey", ""));

    target_tag_ = String(opt.get("targetTag", ""));
    target_button_->set_text(target_tag_.is_empty() ? String("(none)") : target_tag_);

    _rebuild_conditions();
    _rebuild_operations();

    popup(Rect2i(screen_pos, get_size()));
}

void OghamOptionPopup::_set_condition_field(Variant value, Dictionary cond, String field)
{
    cond[field] = value;
}

void OghamOptionPopup::_set_operation_field(Variant value, Dictionary op, String field)
{
    op[field] = value;
}

void OghamOptionPopup::_set_condition_comparison(int idx, Dictionary cond)
{
    if (idx >= 0 && idx < 11)
        cond["comparison"] = String(COMPARISONS[idx]);
}

void OghamOptionPopup::_set_condition_logic(int idx, Dictionary cond)
{
    if (idx >= 0 && idx < 3)
        cond["logicOp"] = String(LOGIC_OPS[idx]);
}

void OghamOptionPopup::_set_operation_arithmetic(int idx, Dictionary op)
{
    if (idx >= 0 && idx < 7)
        op["arithmetic"] = String(ARITHMETICS[idx]);
}

void OghamOptionPopup::_remove_condition_at(int index)
{
    Array conditions = option.get("conditions", Array());
    if (index >= 0 && index < conditions.size())
        conditions.remove_at(index);
    _rebuild_conditions();
}

void OghamOptionPopup::_remove_operation_at(int index)
{
    Array operations = option.get("operations", Array());
    if (index >= 0 && index < operations.size())
        operations.remove_at(index);
    _rebuild_operations();
}

void OghamOptionPopup::_rebuild_conditions()
{
    TypedArray<Node> children = conditions_box_->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    Array conditions = option.get("conditions", Array());
    for (int i = 0; i < conditions.size(); i++)
    {
        Dictionary cond = conditions[i];
        HBoxContainer *row = memnew(HBoxContainer);

        LineEdit *tag_edit = memnew(LineEdit);
        tag_edit->set_text(cond.get("tag", ""));
        tag_edit->set_custom_minimum_size(Vector2(120, 0));
        tag_edit->connect("text_changed", callable_mp(this, &OghamOptionPopup::_set_condition_field).bind(cond, String("tag")));
        row->add_child(tag_edit);

        OptionButton *comparison = memnew(OptionButton);
        for (const char *c : COMPARISONS)
            comparison->add_item(c);
        comparison->select(std::max(find_index(COMPARISONS, 11, String(cond.get("comparison", "Exists"))), 0));
        comparison->connect("item_selected", callable_mp(this, &OghamOptionPopup::_set_condition_comparison).bind(cond));
        row->add_child(comparison);

        SpinBox *value_spin = memnew(SpinBox);
        value_spin->set_min(-2147483648.0);
        value_spin->set_max(2147483647.0);
        value_spin->set_custom_minimum_size(Vector2(70, 0));
        value_spin->set_value(double(cond.get("compareValue", 1)));
        value_spin->connect("value_changed", callable_mp(this, &OghamOptionPopup::_set_condition_field).bind(cond, String("compareValue")));
        row->add_child(value_spin);

        CheckBox *exact = memnew(CheckBox);
        exact->set_text("Exact");
        exact->set_pressed(bool(cond.get("exactMatch", true)));
        exact->connect("toggled", callable_mp(this, &OghamOptionPopup::_set_condition_field).bind(cond, String("exactMatch")));
        row->add_child(exact);

        OptionButton *logic = memnew(OptionButton);
        for (const char *l : LOGIC_OPS)
            logic->add_item(l);
        logic->select(std::max(find_index(LOGIC_OPS, 3, String(cond.get("logicOp", "And"))), 0));
        logic->connect("item_selected", callable_mp(this, &OghamOptionPopup::_set_condition_logic).bind(cond));
        row->add_child(logic);

        Button *delete_btn = memnew(Button);
        delete_btn->set_text("x");
        delete_btn->connect("pressed", callable_mp(this, &OghamOptionPopup::_remove_condition_at).bind(i));
        row->add_child(delete_btn);

        conditions_box_->add_child(row);
    }
}

void OghamOptionPopup::_on_add_condition()
{
    if (!option.has("conditions"))
        option["conditions"] = Array();
    Array conditions = option["conditions"];
    Dictionary cond;
    cond["tag"] = "";
    cond["comparison"] = "Exists";
    cond["compareValue"] = 1;
    cond["exactMatch"] = true;
    cond["logicOp"] = "And";
    conditions.push_back(cond);
    _rebuild_conditions();
}

void OghamOptionPopup::_rebuild_operations()
{
    TypedArray<Node> children = operations_box_->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    Array operations = option.get("operations", Array());
    for (int i = 0; i < operations.size(); i++)
    {
        Dictionary op = operations[i];
        HBoxContainer *row = memnew(HBoxContainer);

        LineEdit *tag_edit = memnew(LineEdit);
        tag_edit->set_text(op.get("tag", ""));
        tag_edit->set_custom_minimum_size(Vector2(140, 0));
        tag_edit->connect("text_changed", callable_mp(this, &OghamOptionPopup::_set_operation_field).bind(op, String("tag")));
        row->add_child(tag_edit);

        OptionButton *arithmetic = memnew(OptionButton);
        for (const char *a : ARITHMETICS)
            arithmetic->add_item(a);
        arithmetic->select(std::max(find_index(ARITHMETICS, 7, String(op.get("arithmetic", "Set"))), 0));
        arithmetic->connect("item_selected", callable_mp(this, &OghamOptionPopup::_set_operation_arithmetic).bind(op));
        row->add_child(arithmetic);

        SpinBox *value_spin = memnew(SpinBox);
        value_spin->set_min(-2147483648.0);
        value_spin->set_max(2147483647.0);
        value_spin->set_custom_minimum_size(Vector2(70, 0));
        value_spin->set_value(double(op.get("value", 1)));
        value_spin->connect("value_changed", callable_mp(this, &OghamOptionPopup::_set_operation_field).bind(op, String("value")));
        row->add_child(value_spin);

        Button *delete_btn = memnew(Button);
        delete_btn->set_text("x");
        delete_btn->connect("pressed", callable_mp(this, &OghamOptionPopup::_remove_operation_at).bind(i));
        row->add_child(delete_btn);

        operations_box_->add_child(row);
    }
}

void OghamOptionPopup::_on_add_operation()
{
    if (!option.has("operations"))
        option["operations"] = Array();
    Array operations = option["operations"];
    Dictionary op;
    op["tag"] = "";
    op["arithmetic"] = "Set";
    op["value"] = 1;
    operations.push_back(op);
    _rebuild_operations();
}

void OghamOptionPopup::_open_target_picker()
{
    OghamTargetPickerPopup *picker = memnew(OghamTargetPickerPopup);
    add_child(picker);
    Vector2 below = target_button_->get_screen_position() + Vector2(0, target_button_->get_size().y);
    picker->open(all_tags, target_tag_, Vector2i(below), callable_mp(this, &OghamOptionPopup::_on_target_selected));
}

void OghamOptionPopup::_on_target_selected(String tag)
{
    target_tag_ = tag;
    target_button_->set_text(tag.is_empty() ? String("(none)") : tag);
}

void OghamOptionPopup::_do_close(bool commit)
{
    if (closed_)
        return;
    if (commit)
    {
        option["tag"] = tag_edit_->get_text();
        option["mode"] = String(MODES[mode_button_->get_selected()]);
        option["textKey"] = text_edit_->get_text();
        option["targetTag"] = target_tag_;
        if (on_committed.is_valid())
            on_committed.call();
    }
    close_and_free();
}

void OghamOptionPopup::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open", "opt", "tags", "screen_pos", "on_committed"), &OghamOptionPopup::open);
}
