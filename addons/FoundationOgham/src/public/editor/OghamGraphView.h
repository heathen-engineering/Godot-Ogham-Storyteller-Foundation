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

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/graph_edit.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "editor/OghamDocument.h"
#include "editor/OghamGraphNode.h"

using namespace godot;

/// <summary>
/// Full-authoring node-graph editor for .ogham stories — direct C++ port of
/// OghamGraphView.gd (Phase 2 of moving the Ogham Storyteller editor off
/// GDScript; see OghamDocument.h for Phase 1's rationale). Reads/writes the
/// .ogham JSON directly via OghamManifestIO/OghamDocument.
///
/// Two deliberate departures from the GDScript original, per the plan agreed
/// with the user:
///  - File loading and "Layout All Files" scoring no longer use a background
///    Thread — both existed purely because GDScript interpretation made
///    parsing/scoring hundreds of entries visibly slow; in compiled C++ that
///    reason is gone (confirmed in Phase 1: loading 239 real entries was
///    near-instant), and neither touches Godot's Control/theme/layout engine
///    machinery, so there's no engine-side cost left to hide behind a thread.
///  - The multi-frame node-building/fit-to-content-settle waits (genuinely
///    needed — that part IS an engine-side cost, building hundreds of
///    GraphNode Controls, that runs at the same speed regardless of caller
///    language) are reimplemented as explicit continuation chains
///    (SceneTree::process_frame one-shot connections / call_deferred)
///    instead of `await`, which doesn't exist in GDExtension C++. State is
///    threaded through as Callable::bind() arguments rather than held in
///    member fields, and nodes are re-fetched by tag/name at each step rather
///    than holding a raw OghamGraphNode* across a frame boundary — a raw
///    pointer could dangle if a newer rebuild frees the node first; a
///    name-based re-lookup (the same pattern _refresh_node_by_tag already
///    used in the GDScript version) just safely returns null instead.
/// </summary>
class OghamGraphView : public Control
{
    GDCLASS(OghamGraphView, Control);

private:
    // -- state (mirrors OghamGraphView.gd's vars) --
    Dictionary _all_story_data; // path -> Ref<OghamDocument>
    Dictionary _visible_paths;  // path -> bool
    String _current_path;
    String _search_pattern;

    GraphEdit *_graph = nullptr;
    PackedStringArray _story_paths;
    Label *_path_label = nullptr;
    FileDialog *_file_dialog = nullptr;
    VBoxContainer *_file_list_box = nullptr;
    Tree *_tree = nullptr;
    Dictionary _tree_collapsed_state;
    HFlowContainer *_tree_pill_row = nullptr;
    static std::unordered_map<std::string, Ref<ImageTexture>> _swatch_icon_cache;
    Label *_loading_label = nullptr;
    Control *_loading_panel = nullptr;

    Dictionary _tag_to_node_name;

    int _rebuild_generation = 0;
    bool _built = false;

    static const int NODES_PER_FRAME = 25;
    static constexpr double LAYOUT_COLUMN_WIDTH = 620.0;
    static constexpr double LAYOUT_ROW_GAP = 60.0;
    static constexpr double LAYOUT_PROVISIONAL_FILE_GAP = 3000.0;
    static constexpr double LAYOUT_GROUP_GAP = 200.0;
    static constexpr double EST_ROW_MIN_HEIGHT = 26.0;
    static constexpr double EST_CHARS_PER_LINE = 55.0;

    // -- setup / UI construction --
    void _ensure_built();
    void _build_toolbar(VBoxContainer *root_vbox);
    void _build_left_panel(HSplitContainer *split);
    void _build_graph_canvas(HSplitContainer *split);

    // -- discovery / load (now synchronous, no Thread — see class doc) --
    void _discover_stories();
    void _show_loading(const String &text);
    void _hide_loading();

    // -- left panel (Files + Tree views) --
    void _rebuild_file_panel();
    void _add_tag_row(const String &tag, const Callable &on_pressed);
    void _rebuild_tree_panel();
    void _add_tree_children(TreeItem *parent_item, const Dictionary &children, const Dictionary &file_colors, const String &parent_key);
    static Ref<ImageTexture> _swatch_icon(const Color &color);

public:
    /// Must be called before the extension unloads (see register_types.cpp's
    /// uninitialize_foundation_ogham_module) — _swatch_icon_cache holds
    /// Ref<ImageTexture>, and static-duration Godot Resource references that
    /// outlive RenderingServer's teardown crash on destruction ("Parameter
    /// RenderingServer::get_singleton() is null", confirmed via a headless
    /// --editor --quit run). Clearing here, while RenderingServer is still
    /// alive, releases every cached Ref cleanly instead of leaving it for a
    /// static destructor to find the engine already gone.
    static void clear_swatch_icon_cache();

private:
    void _on_tree_item_selected();
    void _on_tree_item_collapsed(TreeItem *item);
    void _set_all_collapsed(TreeItem *item, bool collapsed);
    void _on_expand_all_pressed();
    void _on_collapse_all_pressed();
    void _rebuild_tree_pill_row();
    void _on_search_text_changed(String new_text);
    void _on_search_result_clicked(String path, String tag);
    static bool _tag_matches_pattern(const String &tag, const String &pattern);
    static bool _segments_match(const PackedStringArray &tag_segs, const PackedStringArray &pat_segs, int ti, int pi);
    void _on_file_visibility_toggled(bool is_visible, String path);
    void _set_current_path(const String &path);
    Color _color_for_path(const String &path, int index);
    void _on_file_color_changed(Color color, String path);
    void _focus_node(const String &tag);
    void _on_focus_target_signal(OghamGraphNode *node, String target_tag); // tab_flag_clicked/go_to_option_target_requested adapter — signal delivers (node, target_tag), we only want target_tag
    Ref<OghamDocument> _data_for_tag(const String &tag);

    // -- layout math (pure data, synchronous now — see class doc) --
    static double _estimate_row_height(const String &text);
    static double _estimate_node_height(const Dictionary &entry);
    void _auto_layout_missing(const String &path, bool force = false);
    static String _score_node_group(const String &tag, const Dictionary &entry, const Dictionary &owner_by_tag, const Dictionary &incoming_by_target);
    static Dictionary _compute_global_layout(const Dictionary &entries_by_tag, const Dictionary &owner_by_tag);
    void _on_layout_all_files_pressed();

    void _on_open_pressed();
    void _on_file_selected(String path);
    void _on_save_pressed();
    void _on_graph_popup_request(Vector2 at_position);
    void _add_entry_at(Vector2 graph_position);

    // -- rebuild_graph: explicit continuation chain replacing GDScript's
    // await-based multi-frame sequence (see class doc). Each step threads its
    // state via bound Callable arguments; 'generation' is the
    // _rebuild_generation snapshot taken at the start, checked at every
    // resumption point so a superseded in-flight rebuild bails out instead of
    // acting on freed nodes — same cancellation check the GDScript did.
    // Godot's Callable equality for callable_mp(...).bind(...) compares only
    // the target object+method, NOT the bound argument values (confirmed
    // empirically — two Callables bound to the same method with different
    // literal args compare ==). That means a plain connect() of the same
    // continuation method to "process_frame" while an earlier one-shot for
    // that method is still pending silently fails (Godot logs an "already
    // connected" error and does nothing), and since callers here don't
    // check connect()'s return value, the NEWER (intended-to-win) chain
    // would stall permanently instead of the older, superseded one. This
    // helper disconnects any same-method pending connection first so the
    // newest continuation always wins — matching the existing
    // _rebuild_generation cancellation intent instead of silently breaking it.
    void _reconnect_process_frame_once(const Callable &callable);

    void _rebuild_graph(bool show_progress = false);
    void _rebuild_build_next_batch(Array visible_items, int cursor, bool show_progress, int generation);
    void _rebuild_wait_layout(Array visible_items, int frames_left, int pass, int generation);
    void _rebuild_apply_fit_pass(const Array &visible_items);
    void _on_node_dragged(Vector2 from, Vector2 to, OghamGraphNode *node);

    Array _gather_visible_items();
    Dictionary _file_color_by_path();
    Dictionary _section_expanded_for(const Ref<OghamDocument> &doc, const String &tag);
    Dictionary _compute_link_annotations(const Array &visible_items, const Dictionary &file_colors);
    Dictionary _node_render_annotations(const String &tag, const Dictionary &entry, const Ref<OghamDocument> &doc, const Color &file_color, const Dictionary &link_ann);
    void _connect_all_wires(const Array &visible_items);
    void _sync_connections();

    // -- refresh_node: explicit continuation chain replacing GDScript's
    // await-based single-node refresh. Re-fetches the node by tag at each
    // step rather than holding a raw pointer across a frame boundary (see
    // class doc) — 'on_done' is called (if valid) once the fit-to-content
    // settle finishes, letting callers that need to chain further work
    // (_refresh_node_and_targets, _on_connection_request, ...) do so without
    // a native coroutine.
    void _refresh_node(const String &tag, const Callable &on_done = Callable());
    void _refresh_wait_frame(String tag, int frames_left, int pass, Callable on_done);
    void _refresh_node_by_tag(const String &tag);
    void _refresh_node_and_targets(const String &tag, const Callable &on_done = Callable());
    void _refresh_node_and_targets_step2(String tag, Callable on_done);

    Dictionary _label_defs_for(const Ref<OghamDocument> &doc);
    String _entry_tag_for_node_name(const String &node_name);
    void _on_connection_request(String from_node, int from_port, String to_node, int to_port);
    void _on_connection_request_done(String old_target, String to_tag);
    void _on_disconnection_request(String from_node, int from_port, String to_node, int to_port);
    void _on_disconnection_request_done(String old_target);
    void _on_delete_nodes_request(Array nodes);
    void _on_node_renamed(OghamGraphNode *node, String new_tag);
    void _on_node_fork_toggled(OghamGraphNode *node, bool is_fork);
    PackedStringArray _all_entry_tags();
    static Vector2i _mouse_screen_pos();

    void _on_key_labels_pressed();
    void _on_content_key_clicked(OghamGraphNode *node, int index);
    void _on_content_key_committed(Ref<OghamDocument> doc, String tag, int index, Dictionary key_copy);
    void _on_entry_operation_clicked(OghamGraphNode *node, int index);
    void _on_entry_operation_committed(Ref<OghamDocument> doc, String tag, int index, Dictionary op_copy);
    void _on_content_key_delete_requested(OghamGraphNode *node, int index);
    void _on_content_key_move_requested(OghamGraphNode *node, int index, int delta);
    void _on_add_content_key_requested(OghamGraphNode *node);
    void _on_entry_operation_delete_requested(OghamGraphNode *node, int index);
    void _on_entry_operation_move_requested(OghamGraphNode *node, int index, int delta);
    void _on_add_entry_operation_requested(OghamGraphNode *node);
    void _on_section_toggle_requested(OghamGraphNode *node, String section);
    void _on_option_clicked(OghamGraphNode *node, String option_tag);
    void _on_option_committed(Ref<OghamDocument> doc, String entry_tag, String option_tag, Dictionary option_copy, String old_target);
    void _on_option_delete_requested(OghamGraphNode *node, String option_tag);
    void _on_option_delete_done(String old_target);
    void _on_option_move_requested(OghamGraphNode *node, int index, int delta);
    void _on_add_option_requested(OghamGraphNode *node);
    void _on_node_context_menu_requested(OghamGraphNode *node);
    void _menu_action_toggle_tab(Ref<OghamDocument> doc, String tag, String opt_tag, bool is_tab);
    void _menu_action_set_highlight(Ref<OghamDocument> doc, String tag, Color hl_color);
    void _menu_action_open_labels(Ref<OghamDocument> doc, String tag);
    void _menu_action_delete_node(Ref<OghamDocument> doc, String tag);
    void _run_menu_action(int pressed_id, Dictionary id_actions, PopupMenu *menu);

public:
    OghamGraphView() = default;

    void _ready();

protected:
    static void _bind_methods();
};
