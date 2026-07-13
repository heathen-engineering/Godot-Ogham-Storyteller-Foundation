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

#include <string>
#include <unordered_map>
#include <vector>

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/graph_node.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

/// <summary>
/// One GraphNode per OghamEntry — title bar plus three collapsible sections
/// ("On Enter"/entryOperations, "Fields"/contentKeys, "Options"). Direct C++
/// port of OghamGraphNode.gd (Phase 2 of moving the Ogham Storyteller editor
/// off GDScript — see OghamDocument.h for Phase 1 and the design doc comments
/// there for the overall rationale).
///
/// entry_data is a read-mostly SNAPSHOT handed in by OghamGraphView.setup(),
/// not a live reference into the document (see OghamDocument.h) — this class
/// never mutates the document itself, only emits signals OghamGraphView
/// handles by calling back into the owning OghamDocument.
///
/// IMPORTANT: every button here uses text glyphs (Button::set_text()), never
/// Button::set_icon() — confirmed this session that Button.icon silently
/// fails to render in this Godot build/theme (see the
/// feedback_godot_button_icon_broken memory), while every text-glyph button
/// already works. Port arrows/loopback-icons still use a procedurally-drawn
/// ImageTexture (_arrow_icon) since those go through GraphNode::set_slot's
/// custom_icon parameter, a different code path that IS confirmed working.
/// </summary>
class OghamGraphNode : public GraphNode
{
    GDCLASS(OghamGraphNode, GraphNode);

public:
    static constexpr float MAX_TITLE_DISPLAY_CHARS = 34;
    static constexpr float MAX_TITLE_EDIT_WIDTH = 260.0;
    static constexpr int COMPACT_FONT_SIZE = 12;

private:
    Dictionary entry_data;
    String owner_path;
    Color header_color = Color(0.22, 0.44, 0.75);
    Dictionary section_expanded; // {"ops": bool, "fields": bool, "choices": bool}
    bool has_incoming_connection = false;
    Dictionary label_defs; // {int id -> {"name": String, "color": Color}}
    Array assigned_label_ids;
    Color border_color = Color(0, 0, 0, 0);
    Dictionary tab_flags; // {option tag -> fill Color}
    std::vector<int> option_port_indices_;

    static std::unordered_map<std::string, Ref<ImageTexture>> icon_cache_;

    void _apply_border();
    int _add_label_strip(int row_index);
    void _rebuild_titlebar(bool is_fork, const Color &title_bg);
    static Ref<StyleBoxFlat> _compact_stylebox(const Color &bg);
    Button *_tiny_button(const String &text, const Vector2 &min_size = Vector2(16, 8));
    Control *_row_label_control(const String &text, const Callable &on_click);
    void _compactify_line_edit(LineEdit *le);
    int _add_section_header(int row_index, const String &key, const String &label_text, int count, const Callable &on_add, bool carries_input_port = false);
    void _add_reorder_controls(HBoxContainer *row, int index, int count, const Callable &on_move);
    void _add_delete_button(HBoxContainer *row, const Callable &on_delete);
    HBoxContainer *_add_list_row(int row_index, int index, int count, const String &label_text, const Callable &on_click, const Callable &on_move, const Callable &on_delete, const Ref<Texture2D> &icon = nullptr);
    Ref<Texture2D> _field_type_icon(const String &type);
    Ref<Texture2D> _editor_icon_or_null(const String &icon_name);
    String _format_operation_summary(const Dictionary &op);
    int _add_entry_operations_section(int row_index);
    int _add_fields_section(int row_index);
    int _add_options_section(int row_index);
    Control *_go_to_target_button(const String &target_tag);
    Control *_loopback_indicator();
    Control *_tab_flag_control(const Color &fill_color, const String &target_tag);
    static String _truncate_front(const String &text, int max_chars);
    static Color _adaptive_text_color(const Color &bg);
    static String _key_label_for_index(int index); // reads ProjectSettings directly — see .cpp

    // -- bound emit-helpers (Callable::bind() targets, mirroring GDScript's
    // per-row capturing lambdas) --
    void _emit_content_key_clicked(int index);
    void _emit_content_key_delete(int index);
    void _emit_content_key_move(int index, int delta);
    void _emit_add_content_key();
    void _emit_option_clicked(String option_tag);
    void _emit_option_delete(String option_tag);
    void _emit_option_move(int index, int delta);
    void _emit_add_option();
    void _emit_entry_operation_clicked(int index);
    void _emit_entry_operation_delete(int index);
    void _emit_entry_operation_move(int index, int delta);
    void _emit_add_entry_operation();
    void _emit_section_toggle(String key);
    void _emit_tab_flag_clicked(String target_tag);
    void _emit_go_to_target(String target_tag);
    void _emit_fork_toggled(bool new_is_fork);
    void _on_tag_label_pressed(Button *tag_label, LineEdit *tag_edit);
    void _on_tag_edit_committed(LineEdit *tag_edit, Button *tag_label);
    void _on_row_label_gui_input(const Ref<InputEvent> &event, Control *panel, const Callable &on_click);

public:
    OghamGraphNode() = default;

    void setup(const Dictionary &entry, const Dictionary &expanded, bool incoming_connection = false, const Dictionary &annotations = Dictionary());
    void rebuild();
    int option_port_index(int option_index) const;
    int option_index_for_port(int port) const;

    void set_entry_data(const Dictionary &v) { entry_data = v; }
    Dictionary get_entry_data() const { return entry_data; }
    void set_owner_path(const String &v) { owner_path = v; }
    String get_owner_path() const { return owner_path; }
    void set_header_color(const Color &v) { header_color = v; }
    Color get_header_color() const { return header_color; }

    void _gui_input(const Ref<InputEvent> &event);

    static Ref<ImageTexture> arrow_icon(const Color &color, bool point_right, bool filled);

protected:
    static void _bind_methods();
};
