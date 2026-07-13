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

#include "editor/OghamGraphNode.h"
#include "editor/OghamKeyLabelsNative.h"

#include <algorithm>
#include <cmath>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

std::unordered_map<std::string, Ref<ImageTexture>> OghamGraphNode::icon_cache_;

namespace
{
    std::string to_std(const String &s) { return std::string(s.utf8().get_data()); }

    const std::unordered_map<std::string, std::string> ARITHMETIC_SYMBOLS = {
        {"Set", "="}, {"Add", "+="}, {"Subtract", "-="}, {"Multiply", "*="}, {"Divide", "/="}};

    const std::unordered_map<std::string, std::string> CONTENT_TYPE_ICONS = {
        {"Text", "Font"}, {"Image", "ImageTexture"}, {"Sprite", "Sprite2D"},
        {"Audio", "AudioStreamPlayer"}, {"PackedScene", "PackedScene"}, {"Prefab", "PackedScene"}};
}

void OghamGraphNode::setup(const Dictionary &entry, const Dictionary &expanded, bool incoming_connection, const Dictionary &annotations)
{
    entry_data = entry;
    section_expanded = expanded;
    has_incoming_connection = incoming_connection;
    label_defs = annotations.get("label_defs", Dictionary());
    assigned_label_ids = annotations.get("assigned_label_ids", Array());
    border_color = annotations.get("border_color", Color(0, 0, 0, 0));
    tab_flags = annotations.get("tab_flags", Dictionary());
    rebuild();
}

void OghamGraphNode::_gui_input(const Ref<InputEvent> &event)
{
    Ref<InputEventMouseButton> mb = event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_RIGHT && mb->is_pressed())
    {
        emit_signal("context_menu_requested", this);
        accept_event();
    }
}

int OghamGraphNode::option_port_index(int option_index) const
{
    if (option_index < 0 || option_index >= int(option_port_indices_.size()))
        return -1;
    return option_port_indices_[option_index];
}

int OghamGraphNode::option_index_for_port(int port) const
{
    for (size_t i = 0; i < option_port_indices_.size(); i++)
    {
        if (option_port_indices_[i] == port)
            return int(i);
    }
    return -1;
}

void OghamGraphNode::rebuild()
{
    HBoxContainer *titlebar = get_titlebar_hbox();
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child == titlebar)
            continue;
        remove_child(child);
        child->queue_free();
    }
    clear_all_slots();
    option_port_indices_.clear();

    bool is_fork = String(entry_data.get("nodeMode", "")) == "Fork";

    Color title_bg = is_fork ? Color(0.45, 0.45, 0.45) : header_color;
    Ref<StyleBoxFlat> titlebar_style;
    titlebar_style.instantiate();
    titlebar_style->set_bg_color(title_bg);
    titlebar_style->set_corner_radius_all(4);
    add_theme_stylebox_override("titlebar", titlebar_style);
    Ref<StyleBoxFlat> titlebar_selected_style = titlebar_style->duplicate();
    titlebar_selected_style->set_bg_color(title_bg.lightened(0.25));
    add_theme_stylebox_override("titlebar_selected", titlebar_selected_style);
    add_theme_color_override("title_color", _adaptive_text_color(title_bg));

    add_theme_constant_override("port_h_offset", 10);

    _apply_border();
    _rebuild_titlebar(is_fork, title_bg);

    int row_index = 0;
    row_index = _add_label_strip(row_index);
    row_index = _add_entry_operations_section(row_index);
    row_index = _add_fields_section(row_index);
    row_index = _add_options_section(row_index);
}

void OghamGraphNode::_apply_border()
{
    remove_theme_stylebox_override("panel");
    remove_theme_stylebox_override("panel_selected");
    if (border_color.a <= 0.0f)
        return;
    Ref<StyleBox> base = get_theme_stylebox("panel");
    Ref<StyleBoxFlat> flat_base = base;
    Ref<StyleBoxFlat> bordered;
    if (flat_base.is_valid())
    {
        Ref<Resource> dup = flat_base->duplicate();
        bordered = dup;
    }
    else
    {
        bordered.instantiate();
    }
    bordered->set_border_color(border_color);
    bordered->set_border_width_all(2);
    add_theme_stylebox_override("panel", bordered);
    add_theme_stylebox_override("panel_selected", bordered->duplicate());
}

int OghamGraphNode::_add_label_strip(int row_index)
{
    if (assigned_label_ids.is_empty())
        return row_index;
    HBoxContainer *row = memnew(HBoxContainer);
    row->add_theme_constant_override("separation", 4);
    for (int i = 0; i < assigned_label_ids.size(); i++)
    {
        Variant id = assigned_label_ids[i];
        if (!label_defs.has(id))
            continue;
        Dictionary def = label_defs[id];
        Color color = def.get("color", Color(0.4, 0.6, 0.9));
        PanelContainer *pill = memnew(PanelContainer);
        Ref<StyleBoxFlat> style;
        style.instantiate();
        style->set_bg_color(color);
        style->set_content_margin(SIDE_LEFT, 6);
        style->set_content_margin(SIDE_RIGHT, 6);
        style->set_content_margin(SIDE_TOP, 0);
        style->set_content_margin(SIDE_BOTTOM, 0);
        style->set_corner_radius_all(8);
        pill->add_theme_stylebox_override("panel", style);
        Label *lbl = memnew(Label);
        lbl->set_text(def.get("name", ""));
        lbl->add_theme_font_size_override("font_size", 10);
        lbl->add_theme_color_override("font_color", _adaptive_text_color(color));
        pill->add_child(lbl);
        row->add_child(pill);
    }
    add_child(row);
    set_slot(row_index, false, 0, Color(1, 1, 1), false, 0, Color(1, 1, 1));
    return row_index + 1;
}

void OghamGraphNode::_on_tag_label_pressed(Button *tag_label, LineEdit *tag_edit)
{
    tag_label->set_visible(false);
    tag_edit->set_visible(true);
    tag_edit->grab_focus();
    tag_edit->select_all();
}

void OghamGraphNode::_on_tag_edit_committed(LineEdit *tag_edit, Button *tag_label)
{
    tag_edit->set_visible(false);
    tag_label->set_visible(true);
    emit_signal("tag_renamed", this, tag_edit->get_text());
}

void OghamGraphNode::_rebuild_titlebar(bool is_fork, const Color &title_bg)
{
    set_title("");
    HBoxContainer *hbox = get_titlebar_hbox();
    TypedArray<Node> children = hbox->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        Node *child = Object::cast_to<Node>(children[i]);
        hbox->remove_child(child);
        child->queue_free();
    }

    Color text_color = _adaptive_text_color(title_bg);
    String tag = entry_data.get("tag", "(unnamed)");

    Button *tag_label = memnew(Button);
    tag_label->set_text(_truncate_front(tag, int(MAX_TITLE_DISPLAY_CHARS)));
    tag_label->set_tooltip_text(tag);
    tag_label->set_flat(true);
    tag_label->set_focus_mode(Control::FOCUS_NONE);
    tag_label->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    tag_label->add_theme_color_override("font_color", text_color);
    tag_label->add_theme_color_override("font_hover_color", text_color);
    Ref<StyleBoxEmpty> empty_sb;
    empty_sb.instantiate();
    for (const char *state : {"normal", "hover", "pressed", "focus"})
        tag_label->add_theme_stylebox_override(state, empty_sb);
    hbox->add_child(tag_label);

    LineEdit *tag_edit = memnew(LineEdit);
    tag_edit->set_text(tag);
    tag_edit->set_visible(false);
    tag_edit->set_custom_minimum_size(Vector2(std::clamp(tag.length() * 7.0f + 16.0f, 100.0f, MAX_TITLE_EDIT_WIDTH), 0));
    _compactify_line_edit(tag_edit);
    hbox->add_child(tag_edit);

    tag_label->connect("pressed", callable_mp(this, &OghamGraphNode::_on_tag_label_pressed).bind(tag_label, tag_edit));
    tag_edit->connect("text_submitted", callable_mp(this, &OghamGraphNode::_on_tag_edit_committed).bind(tag_edit, tag_label).unbind(1));
    tag_edit->connect("focus_exited", callable_mp(this, &OghamGraphNode::_on_tag_edit_committed).bind(tag_edit, tag_label));

    Control *spacer = memnew(Control);
    spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    spacer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    hbox->add_child(spacer);

    Button *fork_btn = _tiny_button(is_fork ? "F" : "C", Vector2(20, 16));
    fork_btn->add_theme_color_override("font_color", text_color);
    fork_btn->set_tooltip_text(is_fork ? "Fork node" : "Content node — click to make this a Fork");
    fork_btn->connect("pressed", callable_mp(this, &OghamGraphNode::_emit_fork_toggled).bind(!is_fork));
    hbox->add_child(fork_btn);
}

Ref<StyleBoxFlat> OghamGraphNode::_compact_stylebox(const Color &bg)
{
    Ref<StyleBoxFlat> sb;
    sb.instantiate();
    sb->set_bg_color(bg);
    sb->set_content_margin(SIDE_LEFT, 6);
    sb->set_content_margin(SIDE_RIGHT, 6);
    sb->set_content_margin(SIDE_TOP, 0);
    sb->set_content_margin(SIDE_BOTTOM, 0);
    sb->set_corner_radius_all(3);
    return sb;
}

Button *OghamGraphNode::_tiny_button(const String &text, const Vector2 &min_size)
{
    Button *b = memnew(Button);
    b->set_text(text);
    b->set_flat(true);
    b->set_focus_mode(Control::FOCUS_NONE);
    b->set_custom_minimum_size(min_size);
    b->add_theme_font_size_override("font_size", 10);
    Ref<StyleBoxEmpty> empty;
    empty.instantiate();
    for (const char *state : {"normal", "hover", "pressed", "disabled", "focus"})
        b->add_theme_stylebox_override(state, empty);
    return b;
}

Control *OghamGraphNode::_row_label_control(const String &text, const Callable &on_click)
{
    PanelContainer *panel = memnew(PanelContainer);
    panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    panel->set_mouse_filter(Control::MOUSE_FILTER_STOP);
    panel->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
    Ref<StyleBox> normal_style = _compact_stylebox(Color(0.16, 0.16, 0.16));
    Ref<StyleBox> hover_style = _compact_stylebox(Color(0.24, 0.24, 0.24));
    panel->add_theme_stylebox_override("panel", normal_style);
    Control *panel_as_control = panel;
    panel->connect("mouse_entered", callable_mp(panel_as_control, &Control::add_theme_stylebox_override).bind(StringName("panel"), hover_style));
    panel->connect("mouse_exited", callable_mp(panel_as_control, &Control::add_theme_stylebox_override).bind(StringName("panel"), normal_style));

    Label *lbl = memnew(Label);
    lbl->set_text(text);
    lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    lbl->add_theme_font_size_override("font_size", COMPACT_FONT_SIZE);
    lbl->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    // See OghamGraphNode.gd's identically-worded comment: a word-wrapping
    // Label doesn't know its assigned width until after the parent
    // Container's first layout pass, so an explicit floor avoids the
    // "builds huge, then shrinks" first-frame flicker.
    Vector2 min_size = lbl->get_custom_minimum_size();
    min_size.x = 440.0;
    lbl->set_custom_minimum_size(min_size);
    panel->add_child(lbl);

    panel->connect("gui_input", callable_mp(this, &OghamGraphNode::_on_row_label_gui_input).bind(panel, on_click));
    return panel;
}

void OghamGraphNode::_on_row_label_gui_input(const Ref<InputEvent> &event, Control *panel, const Callable &on_click)
{
    Ref<InputEventMouseButton> mb = event;
    if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT && mb->is_pressed())
    {
        on_click.call();
        panel->accept_event();
    }
}

void OghamGraphNode::_compactify_line_edit(LineEdit *le)
{
    le->add_theme_font_size_override("font_size", COMPACT_FONT_SIZE);
    le->add_theme_stylebox_override("normal", _compact_stylebox(Color(0.14, 0.14, 0.14)));
    le->add_theme_stylebox_override("focus", _compact_stylebox(Color(0.14, 0.14, 0.14)));
    le->add_theme_stylebox_override("read_only", _compact_stylebox(Color(0.14, 0.14, 0.14)));
}

int OghamGraphNode::_add_section_header(int row_index, const String &key, const String &label_text, int count, const Callable &on_add, bool carries_input_port)
{
    HBoxContainer *row = memnew(HBoxContainer);
    bool expanded = section_expanded.get(key, true);
    Button *toggle_btn = _tiny_button(expanded ? String::utf8("▾") : String::utf8("▸"), Vector2(20, 8));
    toggle_btn->connect("pressed", callable_mp(this, &OghamGraphNode::_emit_section_toggle).bind(key));
    row->add_child(toggle_btn);
    Label *label = memnew(Label);
    label->set_text(String("{0} ({1})").format(Array::make(label_text, count)));
    label->add_theme_font_size_override("font_size", COMPACT_FONT_SIZE);
    label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    row->add_child(label);
    Button *add_btn = _tiny_button("+", Vector2(20, 8));
    add_btn->connect("pressed", on_add);
    row->add_child(add_btn);
    add_child(row);
    if (carries_input_port)
    {
        Color color = has_incoming_connection ? Color(1, 1, 1) : Color(1, 1, 1, 0.5);
        set_slot(row_index, true, 0, color, false, 0, Color(1, 1, 1), arrow_icon(color, true, has_incoming_connection));
    }
    else
    {
        set_slot(row_index, false, 0, Color(1, 1, 1), false, 0, Color(1, 1, 1));
    }
    return row_index + 1;
}

void OghamGraphNode::_add_reorder_controls(HBoxContainer *row, int index, int count, const Callable &on_move)
{
    HBoxContainer *move_box = memnew(HBoxContainer);
    move_box->add_theme_constant_override("separation", 0);
    Button *up_btn = _tiny_button(String::utf8("▲"), Vector2(14, 8));
    up_btn->set_disabled(index <= 0);
    up_btn->connect("pressed", on_move.bind(index, -1));
    move_box->add_child(up_btn);
    Button *down_btn = _tiny_button(String::utf8("▼"), Vector2(14, 8));
    down_btn->set_disabled(index >= count - 1);
    down_btn->connect("pressed", on_move.bind(index, 1));
    move_box->add_child(down_btn);
    row->add_child(move_box);
}

void OghamGraphNode::_add_delete_button(HBoxContainer *row, const Callable &on_delete)
{
    Button *delete_btn = _tiny_button("x", Vector2(16, 8));
    delete_btn->add_theme_color_override("font_color", Color(1.0, 0.45, 0.45));
    delete_btn->connect("pressed", on_delete);
    row->add_child(delete_btn);
}

HBoxContainer *OghamGraphNode::_add_list_row(int row_index, int index, int count, const String &label_text, const Callable &on_click, const Callable &on_move, const Callable &on_delete, const Ref<Texture2D> &icon)
{
    HBoxContainer *row = memnew(HBoxContainer);
    _add_reorder_controls(row, index, count, on_move);
    if (icon.is_valid())
    {
        TextureRect *icon_rect = memnew(TextureRect);
        icon_rect->set_texture(icon);
        icon_rect->set_custom_minimum_size(Vector2(14, 14));
        icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
        icon_rect->set_tooltip_text(label_text);
        row->add_child(icon_rect);
    }
    row->add_child(_row_label_control(label_text, on_click));
    _add_delete_button(row, on_delete);
    add_child(row);
    return row;
}

Ref<Texture2D> OghamGraphNode::_field_type_icon(const String &type)
{
    auto it = CONTENT_TYPE_ICONS.find(to_std(type));
    if (it == CONTENT_TYPE_ICONS.end())
        return nullptr;
    return _editor_icon_or_null(String::utf8(it->second.c_str()));
}

Ref<Texture2D> OghamGraphNode::_editor_icon_or_null(const String &icon_name)
{
    if (!Engine::get_singleton()->is_editor_hint())
        return nullptr;
    if (has_theme_icon(icon_name, "EditorIcons"))
        return get_theme_icon(icon_name, "EditorIcons");
    return nullptr;
}

String OghamGraphNode::_format_operation_summary(const Dictionary &op)
{
    String tag = op.get("tag", "?");
    Variant raw_value = op.get("value", 0);
    double value_d = raw_value;
    String value = (value_d == std::floor(value_d)) ? String::num_int64(int64_t(value_d)) : String::num(value_d);
    String arithmetic = op.get("arithmetic", "Set");
    auto it = ARITHMETIC_SYMBOLS.find(to_std(arithmetic));
    if (it != ARITHMETIC_SYMBOLS.end())
        return String("{0} {1} {2}").format(Array::make(tag, String::utf8(it->second.c_str()), value));
    return String("{0} = {1}({0}, {2})").format(Array::make(tag, arithmetic.to_lower(), value));
}

int OghamGraphNode::_add_entry_operations_section(int row_index)
{
    Array ops = entry_data.get("entryOperations", Array());
    row_index = _add_section_header(row_index, "ops", "On Enter", ops.size(), callable_mp(this, &OghamGraphNode::_emit_add_entry_operation), true);
    if (!bool(section_expanded.get("ops", true)))
        return row_index;
    for (int i = 0; i < ops.size(); i++)
    {
        Dictionary op = ops[i];
        _add_list_row(row_index, i, ops.size(), _format_operation_summary(op),
                       callable_mp(this, &OghamGraphNode::_emit_entry_operation_clicked).bind(i),
                       callable_mp(this, &OghamGraphNode::_emit_entry_operation_move),
                       callable_mp(this, &OghamGraphNode::_emit_entry_operation_delete).bind(i));
        set_slot(row_index, false, 0, Color(1, 1, 1), false, 0, Color(1, 1, 1));
        row_index++;
    }
    return row_index;
}

int OghamGraphNode::_add_fields_section(int row_index)
{
    Array content_keys = entry_data.get("contentKeys", Array());
    row_index = _add_section_header(row_index, "fields", "Fields", content_keys.size(), callable_mp(this, &OghamGraphNode::_emit_add_content_key));
    if (!bool(section_expanded.get("fields", true)))
        return row_index;
    for (int i = 0; i < content_keys.size(); i++)
    {
        Dictionary ck = content_keys[i];
        String ck_type = ck.get("type", "?");
        String preview = String(ck.get("key", ""));
        if (preview.is_empty())
            preview = String(ck.get("path", "")).get_file();
        Ref<Texture2D> icon = _field_type_icon(ck_type);
        if (!icon.is_valid())
            preview = String("[{0}] {1}").format(Array::make(ck_type, preview));
        String key_label = _key_label_for_index(i);
        if (!key_label.is_empty())
            preview = String("{0}: {1}").format(Array::make(key_label, preview));
        _add_list_row(row_index, i, content_keys.size(), preview,
                       callable_mp(this, &OghamGraphNode::_emit_content_key_clicked).bind(i),
                       callable_mp(this, &OghamGraphNode::_emit_content_key_move),
                       callable_mp(this, &OghamGraphNode::_emit_content_key_delete).bind(i),
                       icon);
        set_slot(row_index, false, 0, Color(1, 1, 1), false, 0, Color(1, 1, 1));
        row_index++;
    }
    return row_index;
}

int OghamGraphNode::_add_options_section(int row_index)
{
    Array options = entry_data.get("options", Array());
    row_index = _add_section_header(row_index, "choices", "Options", options.size(), callable_mp(this, &OghamGraphNode::_emit_add_option));
    if (!bool(section_expanded.get("choices", true)))
    {
        for (int i = 0; i < options.size(); i++)
            option_port_indices_.push_back(-1);
        return row_index;
    }
    int compacted_port = 0;
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary option = options[i];
        String target = option.get("targetTag", "");
        String display = option.get("textKey", "");
        if (display.is_empty())
            display = option.get("tag", "?");
        String option_tag = option.get("tag", "");
        String own_tag = entry_data.get("tag", "");
        bool is_loopback = !target.is_empty() && target == own_tag;

        HBoxContainer *row = _add_list_row(row_index, i, options.size(), display,
                                            callable_mp(this, &OghamGraphNode::_emit_option_clicked).bind(option_tag),
                                            callable_mp(this, &OghamGraphNode::_emit_option_move),
                                            callable_mp(this, &OghamGraphNode::_emit_option_delete).bind(option_tag));

        if (is_loopback)
        {
            row->add_child(_loopback_indicator());
            set_slot(row_index, false, 0, Color(1, 1, 1), false, 0, Color(1, 1, 1));
            option_port_indices_.push_back(-1);
        }
        else if (tab_flags.has(option_tag))
        {
            row->add_child(_tab_flag_control(tab_flags[option_tag], target));
            set_slot(row_index, false, 0, Color(1, 1, 1), false, 0, Color(1, 1, 1));
            option_port_indices_.push_back(-1);
        }
        else
        {
            bool is_connected = !target.is_empty();
            Color port_color = is_connected ? Color(0.9, 0.7, 0.3) : Color(0.9, 0.7, 0.3, 0.5);
            set_slot(row_index, false, 0, Color(1, 1, 1), true, 0, port_color, nullptr, arrow_icon(port_color, true, is_connected));
            option_port_indices_.push_back(compacted_port);
            compacted_port++;
        }

        if (!target.is_empty() && !is_loopback)
        {
            Control *go_to_btn = _go_to_target_button(target);
            row->add_child(go_to_btn);
            row->move_child(go_to_btn, row->get_child_count() - 2);
        }
        row_index++;
    }
    return row_index;
}

Control *OghamGraphNode::_go_to_target_button(const String &target_tag)
{
    Button *btn = _tiny_button(">", Vector2(16, 8));
    btn->set_tooltip_text(String("Go to \"{0}\"").format(Array::make(target_tag)));
    btn->connect("pressed", callable_mp(this, &OghamGraphNode::_emit_go_to_target).bind(target_tag));
    return btn;
}

Control *OghamGraphNode::_loopback_indicator()
{
    Label *lbl = memnew(Label);
    lbl->set_text(String::utf8("↻"));
    lbl->set_tooltip_text("Loops back to this same entry");
    lbl->add_theme_font_size_override("font_size", 14);
    lbl->add_theme_color_override("font_color", Color(0.9, 0.7, 0.3));
    lbl->set_custom_minimum_size(Vector2(16, 0));
    return lbl;
}

Control *OghamGraphNode::_tab_flag_control(const Color &fill_color, const String &target_tag)
{
    String abbreviated = target_tag.left(4);
    Button *btn = memnew(Button);
    btn->set_text(String::utf8("▶") + abbreviated);
    btn->set_custom_minimum_size(Vector2(36, 8));
    btn->set_focus_mode(Control::FOCUS_NONE);
    btn->add_theme_font_size_override("font_size", 10);
    Ref<StyleBoxFlat> style = _compact_stylebox(fill_color);
    for (const char *state : {"normal", "hover", "pressed"})
        btn->add_theme_stylebox_override(state, style);
    Ref<StyleBoxEmpty> empty_focus;
    empty_focus.instantiate();
    btn->add_theme_stylebox_override("focus", empty_focus);
    btn->add_theme_color_override("font_color", _adaptive_text_color(fill_color));
    btn->add_theme_color_override("font_hover_color", _adaptive_text_color(fill_color));
    btn->set_tooltip_text(target_tag);
    btn->connect("mouse_entered", callable_mp(btn, &Button::set_text).bind(String::utf8("▶ ") + target_tag));
    btn->connect("mouse_exited", callable_mp(btn, &Button::set_text).bind(String::utf8("▶") + abbreviated));
    btn->connect("pressed", callable_mp(this, &OghamGraphNode::_emit_tab_flag_clicked).bind(target_tag));
    return btn;
}

String OghamGraphNode::_truncate_front(const String &text, int max_chars)
{
    if (text.length() <= max_chars)
        return text;
    return String("...") + text.right(max_chars - 3);
}

Color OghamGraphNode::_adaptive_text_color(const Color &bg)
{
    double luminance = 0.299 * bg.r + 0.587 * bg.g + 0.114 * bg.b;
    return luminance > 0.6 ? Color(0, 0, 0) : Color(1, 1, 1);
}

String OghamGraphNode::_key_label_for_index(int index)
{
    return OghamKeyLabelsNative::label_for_index(index);
}

void OghamGraphNode::_emit_content_key_clicked(int index) { emit_signal("content_key_clicked", this, index); }
void OghamGraphNode::_emit_content_key_delete(int index) { emit_signal("content_key_delete_requested", this, index); }
void OghamGraphNode::_emit_content_key_move(int index, int delta) { emit_signal("content_key_move_requested", this, index, delta); }
void OghamGraphNode::_emit_add_content_key() { emit_signal("add_content_key_requested", this); }
void OghamGraphNode::_emit_option_clicked(String option_tag) { emit_signal("option_clicked", this, option_tag); }
void OghamGraphNode::_emit_option_delete(String option_tag) { emit_signal("option_delete_requested", this, option_tag); }
void OghamGraphNode::_emit_option_move(int index, int delta) { emit_signal("option_move_requested", this, index, delta); }
void OghamGraphNode::_emit_add_option() { emit_signal("add_option_requested", this); }
void OghamGraphNode::_emit_entry_operation_clicked(int index) { emit_signal("entry_operation_clicked", this, index); }
void OghamGraphNode::_emit_entry_operation_delete(int index) { emit_signal("entry_operation_delete_requested", this, index); }
void OghamGraphNode::_emit_entry_operation_move(int index, int delta) { emit_signal("entry_operation_move_requested", this, index, delta); }
void OghamGraphNode::_emit_add_entry_operation() { emit_signal("add_entry_operation_requested", this); }
void OghamGraphNode::_emit_section_toggle(String key) { emit_signal("section_toggle_requested", this, key); }
void OghamGraphNode::_emit_tab_flag_clicked(String target_tag) { emit_signal("tab_flag_clicked", this, target_tag); }
void OghamGraphNode::_emit_go_to_target(String target_tag) { emit_signal("go_to_option_target_requested", this, target_tag); }
void OghamGraphNode::_emit_fork_toggled(bool new_is_fork) { emit_signal("fork_toggled", this, new_is_fork); }

Ref<ImageTexture> OghamGraphNode::arrow_icon(const Color &color, bool point_right, bool filled)
{
    std::string cache_key = to_std(color.to_html(true)) + "_" + (point_right ? "1" : "0") + "_" + (filled ? "1" : "0");
    auto found = icon_cache_.find(cache_key);
    if (found != icon_cache_.end())
        return found->second;

    int size = 14;
    Ref<Image> img = Image::create(size, size, false, Image::FORMAT_RGBA8);
    img->fill(Color(0, 0, 0, 0));
    int apex_x = point_right ? size - 1 : 0;
    int base_x = size - 1 - apex_x;
    float span = std::max(std::abs(apex_x - base_x), 1);
    float border = 1.6f;
    for (int x = 0; x < size; x++)
    {
        float t = std::abs(x - base_x) / span;
        float half_h = (size / 2.0f) * (1.0f - t);
        if (half_h <= 0.0f)
            continue;
        int y0 = int(size / 2.0f - half_h);
        int y1 = int(size / 2.0f + half_h);
        for (int y = y0; y <= y1; y++)
        {
            if (y < 0 || y >= size)
                continue;
            if (filled)
            {
                img->set_pixel(x, y, color);
            }
            else
            {
                float dist_from_edge = half_h - std::abs(y - size / 2.0f);
                float dist_from_base = std::abs(x - base_x);
                if (dist_from_edge < border || dist_from_base < border)
                    img->set_pixel(x, y, color);
            }
        }
    }
    Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
    icon_cache_[cache_key] = tex;
    return tex;
}

void OghamGraphNode::_bind_methods()
{
    ADD_SIGNAL(MethodInfo("content_key_clicked", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("content_key_delete_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("content_key_move_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::INT, "delta")));
    ADD_SIGNAL(MethodInfo("option_clicked", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "option_tag")));
    ADD_SIGNAL(MethodInfo("option_delete_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "option_tag")));
    ADD_SIGNAL(MethodInfo("option_move_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::INT, "delta")));
    ADD_SIGNAL(MethodInfo("entry_operation_clicked", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("entry_operation_delete_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index")));
    ADD_SIGNAL(MethodInfo("entry_operation_move_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::INT, "delta")));
    ADD_SIGNAL(MethodInfo("tag_renamed", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "new_tag")));
    ADD_SIGNAL(MethodInfo("fork_toggled", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::BOOL, "is_fork")));
    ADD_SIGNAL(MethodInfo("add_content_key_requested", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("add_option_requested", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("add_entry_operation_requested", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("section_toggle_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "section")));
    ADD_SIGNAL(MethodInfo("tab_flag_clicked", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "target_tag")));
    ADD_SIGNAL(MethodInfo("context_menu_requested", PropertyInfo(Variant::OBJECT, "node")));
    ADD_SIGNAL(MethodInfo("go_to_option_target_requested", PropertyInfo(Variant::OBJECT, "node"), PropertyInfo(Variant::STRING, "target_tag")));

    ClassDB::bind_method(D_METHOD("setup", "entry", "expanded", "incoming_connection", "annotations"), &OghamGraphNode::setup, DEFVAL(false), DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("rebuild"), &OghamGraphNode::rebuild);
    ClassDB::bind_method(D_METHOD("option_port_index", "option_index"), &OghamGraphNode::option_port_index);
    ClassDB::bind_method(D_METHOD("option_index_for_port", "port"), &OghamGraphNode::option_index_for_port);

    ClassDB::bind_method(D_METHOD("set_entry_data", "value"), &OghamGraphNode::set_entry_data);
    ClassDB::bind_method(D_METHOD("get_entry_data"), &OghamGraphNode::get_entry_data);
    ClassDB::bind_method(D_METHOD("set_owner_path", "value"), &OghamGraphNode::set_owner_path);
    ClassDB::bind_method(D_METHOD("get_owner_path"), &OghamGraphNode::get_owner_path);
    ClassDB::bind_method(D_METHOD("set_header_color", "value"), &OghamGraphNode::set_header_color);
    ClassDB::bind_method(D_METHOD("get_header_color"), &OghamGraphNode::get_header_color);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "entry_data"), "set_entry_data", "get_entry_data");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "owner_path"), "set_owner_path", "get_owner_path");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "header_color"), "set_header_color", "get_header_color");
    // _gui_input is NOT bound here — it's a Control virtual override, picked up
    // automatically by name/signature match (GDCLASS's virtual-call machinery),
    // not a regular method; explicitly binding it conflicted with that.
}
