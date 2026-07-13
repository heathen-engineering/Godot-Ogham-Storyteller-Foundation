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

#include "editor/OghamGraphView.h"
#include "editor/OghamContentKeyPopup.h"
#include "editor/OghamKeyLabelsPopup.h"
#include "editor/OghamLabelPickerPopup.h"
#include "editor/OghamManifestIO.h"
#include "editor/OghamOperationPopup.h"
#include "editor/OghamOptionPopup.h"

#include <algorithm>
#include <cmath>

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

std::unordered_map<std::string, Ref<ImageTexture>> OghamGraphView::_swatch_icon_cache;

namespace
{
    std::string to_std(const String &s) { return std::string(s.utf8().get_data()); }

    const Color PALETTE[6] = {
        Color(0.22, 0.44, 0.75), // blue
        Color(0.25, 0.70, 0.35), // green
        Color(0.75, 0.55, 0.18), // amber
        Color(0.55, 0.33, 0.70), // purple
        Color(0.75, 0.30, 0.30), // red
        Color(0.30, 0.65, 0.65), // teal
    };

    struct HighlightPreset
    {
        const char *label;
        Color color;
    };
    const HighlightPreset HIGHLIGHT_PRESETS[6] = {
        {"Clear", Color(0, 0, 0, 0)},
        {"Red", Color(0.85, 0.25, 0.25)},
        {"Green", Color(0.25, 0.70, 0.35)},
        {"Blue", Color(0.25, 0.55, 0.90)},
        {"Yellow", Color(0.90, 0.80, 0.20)},
        {"Purple", Color(0.60, 0.25, 0.85)},
    };
}

// ============================== setup / UI construction ==============================

void OghamGraphView::_ready()
{
    _ensure_built();
}

void OghamGraphView::_ensure_built()
{
    if (_built)
        return;
    _built = true;

    set_anchors_preset(Control::PRESET_FULL_RECT);
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    VBoxContainer *root_vbox = memnew(VBoxContainer);
    root_vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
    add_child(root_vbox);

    _build_toolbar(root_vbox);

    HSplitContainer *split = memnew(HSplitContainer);
    split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    root_vbox->add_child(split);

    _build_left_panel(split);
    _build_graph_canvas(split);

    _file_dialog = memnew(FileDialog);
    _file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
    _file_dialog->set_access(FileDialog::ACCESS_RESOURCES);
    _file_dialog->add_filter("*.ogham", "Ogham Story");
    _file_dialog->connect("file_selected", callable_mp(this, &OghamGraphView::_on_file_selected));
    add_child(_file_dialog);

    _discover_stories();
}

void OghamGraphView::_build_toolbar(VBoxContainer *root_vbox)
{
    HBoxContainer *toolbar = memnew(HBoxContainer);
    Button *save_btn = memnew(Button);
    save_btn->set_text("Save All");
    save_btn->set_tooltip_text("Saves every loaded .ogham file (not just the active one).");
    save_btn->connect("pressed", callable_mp(this, &OghamGraphView::_on_save_pressed));
    toolbar->add_child(save_btn);

    Button *layout_btn = memnew(Button);
    layout_btn->set_text("Layout All Files");
    layout_btn->set_tooltip_text("Relayout every node in every loaded file, grouped by which file each node actually reads as belonging to and packed tightly by each group's real computed size.");
    layout_btn->connect("pressed", callable_mp(this, &OghamGraphView::_on_layout_all_files_pressed));
    toolbar->add_child(layout_btn);

    Button *key_labels_btn = memnew(Button);
    key_labels_btn->set_text("Key Labels...");
    key_labels_btn->set_tooltip_text("Project-wide names for contentKey slots by position.");
    key_labels_btn->connect("pressed", callable_mp(this, &OghamGraphView::_on_key_labels_pressed));
    toolbar->add_child(key_labels_btn);

    _path_label = memnew(Label);
    _path_label->set_text("(no active file)");
    _path_label->set_tooltip_text("The active file — where Add Entry/Add Fork create new nodes. Click a filename in the left panel to change it.");
    toolbar->add_child(_path_label);
    root_vbox->add_child(toolbar);
}

void OghamGraphView::_build_left_panel(HSplitContainer *split)
{
    VBoxContainer *left_panel = memnew(VBoxContainer);
    left_panel->set_custom_minimum_size(Vector2(240, 0));

    HBoxContainer *left_toolbar = memnew(HBoxContainer);
    Button *refresh_btn = memnew(Button);
    refresh_btn->set_text("Rescan Project");
    refresh_btn->connect("pressed", callable_mp(this, &OghamGraphView::_discover_stories));
    left_toolbar->add_child(refresh_btn);
    Button *open_btn = memnew(Button);
    open_btn->set_text("Open Other...");
    open_btn->set_tooltip_text("Browse for a .ogham file outside the project.");
    open_btn->connect("pressed", callable_mp(this, &OghamGraphView::_on_open_pressed));
    left_toolbar->add_child(open_btn);
    left_panel->add_child(left_toolbar);

    LineEdit *search_box = memnew(LineEdit);
    search_box->set_placeholder("Search tags (*.Kitchen, Act1.*.Kitchen.*)");
    search_box->set_clear_button_enabled(true);
    search_box->set_tooltip_text("Glob-style filter over the dot-hierarchy tag.");
    search_box->connect("text_changed", callable_mp(this, &OghamGraphView::_on_search_text_changed));
    left_panel->add_child(search_box);

    TabContainer *view_tabs = memnew(TabContainer);
    view_tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);

    ScrollContainer *left_scroll = memnew(ScrollContainer);
    left_scroll->set_name("Files");
    _file_list_box = memnew(VBoxContainer);
    _file_list_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    left_scroll->add_child(_file_list_box);
    view_tabs->add_child(left_scroll);

    VBoxContainer *tree_tab = memnew(VBoxContainer);
    tree_tab->set_name("Tree");
    _tree_pill_row = memnew(HFlowContainer);
    tree_tab->add_child(_tree_pill_row);

    HBoxContainer *fold_toolbar = memnew(HBoxContainer);
    Button *expand_all_btn = memnew(Button);
    expand_all_btn->set_text("Expand All");
    expand_all_btn->connect("pressed", callable_mp(this, &OghamGraphView::_on_expand_all_pressed));
    fold_toolbar->add_child(expand_all_btn);
    Button *collapse_all_btn = memnew(Button);
    collapse_all_btn->set_text("Collapse All");
    collapse_all_btn->connect("pressed", callable_mp(this, &OghamGraphView::_on_collapse_all_pressed));
    fold_toolbar->add_child(collapse_all_btn);
    tree_tab->add_child(fold_toolbar);

    _tree = memnew(Tree);
    _tree->set_hide_root(true);
    _tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _tree->connect("item_selected", callable_mp(this, &OghamGraphView::_on_tree_item_selected));
    _tree->connect("item_collapsed", callable_mp(this, &OghamGraphView::_on_tree_item_collapsed));
    tree_tab->add_child(_tree);
    view_tabs->add_child(tree_tab);

    view_tabs->set_current_tab(1);

    left_panel->add_child(view_tabs);
    split->add_child(left_panel);
}

void OghamGraphView::_build_graph_canvas(HSplitContainer *split)
{
    _graph = memnew(GraphEdit);
    _graph->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _graph->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    _graph->set_show_arrange_button(false);
    _graph->connect("connection_request", callable_mp(this, &OghamGraphView::_on_connection_request));
    _graph->connect("disconnection_request", callable_mp(this, &OghamGraphView::_on_disconnection_request));
    _graph->connect("delete_nodes_request", callable_mp(this, &OghamGraphView::_on_delete_nodes_request));
    _graph->connect("popup_request", callable_mp(this, &OghamGraphView::_on_graph_popup_request));
    split->add_child(_graph);

    _loading_panel = memnew(PanelContainer);
    _loading_panel->set_anchors_preset(Control::PRESET_CENTER);
    _loading_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    Ref<StyleBoxFlat> loading_style;
    loading_style.instantiate();
    loading_style->set_bg_color(Color(0.05, 0.05, 0.07, 0.9));
    loading_style->set_content_margin(SIDE_LEFT, 24);
    loading_style->set_content_margin(SIDE_RIGHT, 24);
    loading_style->set_content_margin(SIDE_TOP, 16);
    loading_style->set_content_margin(SIDE_BOTTOM, 16);
    loading_style->set_corner_radius_all(6);
    static_cast<PanelContainer *>(_loading_panel)->add_theme_stylebox_override("panel", loading_style);
    _loading_label = memnew(Label);
    _loading_label->set_text("Loading...");
    _loading_label->add_theme_font_size_override("font_size", 16);
    _loading_panel->add_child(_loading_label);
    _loading_panel->set_visible(false);
    _graph->add_child(_loading_panel);
}

// ============================== discovery / load (synchronous) ==============================

void OghamGraphView::_discover_stories()
{
    _story_paths = OghamManifestIO::find_ogham_files("res://");

    Array new_paths;
    for (int i = 0; i < _story_paths.size(); i++)
    {
        if (!_all_story_data.has(_story_paths[i]))
            new_paths.push_back(_story_paths[i]);
    }

    Array existing_keys = _all_story_data.keys();
    for (int i = 0; i < existing_keys.size(); i++)
    {
        String path = existing_keys[i];
        if (!_story_paths.has(path))
        {
            _all_story_data.erase(path);
            _visible_paths.erase(path);
        }
    }

    if (!new_paths.is_empty())
    {
        _show_loading(String("Loading {0} file(s)...").format(Array::make(new_paths.size())));
        for (int i = 0; i < new_paths.size(); i++)
        {
            String path = new_paths[i];
            Ref<OghamDocument> doc = OghamManifestIO::load_manifest(path);
            _all_story_data[path] = doc;
            // Hidden by default — see class doc comment on why "show
            // everything at once" doesn't scale as the default.
            _visible_paths[path] = false;
            _auto_layout_missing(path);
        }
    }

    if (_current_path.is_empty() || !_all_story_data.has(_current_path))
        _current_path = _story_paths.is_empty() ? String() : _story_paths[0];
    _path_label->set_text(_current_path.is_empty() ? String("(no active file)") : _current_path);

    _rebuild_file_panel();
    _rebuild_tree_panel();
    _rebuild_tree_pill_row();
    _rebuild_graph(true);
    _hide_loading();
}

void OghamGraphView::_show_loading(const String &text)
{
    if (_loading_label == nullptr)
        return;
    _loading_label->set_text(text);
    // _loading_panel is a child of _graph, added once at _ensure_built()
    // time; every rebuild adds fresh GraphNode children afterward, and
    // Control/CanvasItem siblings draw in child order, so without this the
    // overlay ends up drawn UNDER nodes built after it (i.e. any node
    // added since the graph was first constructed). move_to_front() keeps
    // it on top regardless of how many nodes have been rebuilt since.
    _loading_panel->move_to_front();
    _loading_panel->set_visible(true);
}

void OghamGraphView::_hide_loading()
{
    if (_loading_label == nullptr)
        return;
    _loading_panel->set_visible(false);
}

// ============================== left panel (Files + Tree) ==============================

void OghamGraphView::_rebuild_file_panel()
{
    TypedArray<Node> children = _file_list_box->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    for (int i = 0; i < _story_paths.size(); i++)
    {
        String path = _story_paths[i];
        Ref<OghamDocument> doc = _all_story_data.get(path, Variant());
        if (doc.is_null())
            continue;

        PackedStringArray matching_tags;
        bool has_matches_or_no_search = true;
        if (!_search_pattern.is_empty())
        {
            Array entries = doc->get_entries();
            for (int e = 0; e < entries.size(); e++)
            {
                Dictionary entry = entries[e];
                String tag = entry.get("tag", "");
                if (_tag_matches_pattern(tag, _search_pattern))
                    matching_tags.push_back(tag);
            }
            has_matches_or_no_search = !matching_tags.is_empty();
        }
        if (!has_matches_or_no_search)
            continue; // no matches in this file — don't even show its row

        HBoxContainer *file_row = memnew(HBoxContainer);

        CheckBox *vis_check = memnew(CheckBox);
        // set_pressed_no_signal, not set_pressed — this checkbox is a fresh
        // Control being initialized from existing state, not a user
        // gesture; set_pressed() would emit "toggled" on the new instance
        // (its default is false, so setting true is a real transition) and
        // recursively re-trigger _on_file_visibility_toggled -> _rebuild_graph.
        vis_check->set_pressed_no_signal(bool(_visible_paths.get(path, true)));
        vis_check->set_tooltip_text("Show this file's nodes on the canvas");
        vis_check->connect("toggled", callable_mp(this, &OghamGraphView::_on_file_visibility_toggled).bind(path));
        file_row->add_child(vis_check);

        ColorPickerButton *swatch = memnew(ColorPickerButton);
        swatch->set_custom_minimum_size(Vector2(24, 24));
        swatch->set_pick_color(_color_for_path(path, i));
        swatch->connect("color_changed", callable_mp(this, &OghamGraphView::_on_file_color_changed).bind(path));
        file_row->add_child(swatch);

        Button *file_btn = memnew(Button);
        file_btn->set_text(path.get_file());
        file_btn->set_flat(path == _current_path);
        file_btn->set_tooltip_text("Make this the active file (where Add Entry/Add Fork create new nodes)");
        file_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        file_btn->connect("pressed", callable_mp(this, &OghamGraphView::_set_current_path).bind(path));
        file_row->add_child(file_btn);
        _file_list_box->add_child(file_row);

        if (!_search_pattern.is_empty())
        {
            for (int t = 0; t < matching_tags.size(); t++)
            {
                String tag = matching_tags[t];
                _add_tag_row(tag, callable_mp(this, &OghamGraphView::_on_search_result_clicked).bind(path, tag));
            }
        }
        else if (path == _current_path)
        {
            Array entries = doc->get_entries();
            for (int e = 0; e < entries.size(); e++)
            {
                Dictionary entry = entries[e];
                String tag = entry.get("tag", "");
                _add_tag_row(tag, callable_mp(this, &OghamGraphView::_focus_node).bind(tag));
            }
        }
    }
}

void OghamGraphView::_add_tag_row(const String &tag, const Callable &on_pressed)
{
    Button *entry_btn = memnew(Button);
    entry_btn->set_text(String("    ") + tag);
    entry_btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    entry_btn->set_flat(true);
    entry_btn->connect("pressed", on_pressed);
    _file_list_box->add_child(entry_btn);
}

void OghamGraphView::_rebuild_tree_panel()
{
    if (_tree == nullptr)
        return;
    _tree->clear();
    TreeItem *root = _tree->create_item();

    Dictionary file_colors = _file_color_by_path();
    Dictionary trie;
    Array paths = _all_story_data.keys();
    for (int p = 0; p < paths.size(); p++)
    {
        String path = paths[p];
        Ref<OghamDocument> doc = _all_story_data[path];
        Array entries = doc->get_entries();
        for (int e = 0; e < entries.size(); e++)
        {
            Dictionary entry = entries[e];
            String tag = entry.get("tag", "");
            if (!_search_pattern.is_empty() && !_tag_matches_pattern(tag, _search_pattern))
                continue;
            PackedStringArray segments = tag.split(".");
            // Dictionary is implicitly shared in Godot (assignment shares the
            // same underlying buffer, not a copy), so descending via
            // cursor = cursor["children"][seg] and mutating 'cursor' in place
            // mutates the same structure 'trie' is rooted at — matches
            // GDScript's identical reliance on Dictionary reference semantics
            // for this local, throwaway trie (unrelated to OghamDocument's
            // deliberately-not-shared entry snapshots).
            Dictionary cursor = trie;
            for (int s = 0; s < segments.size(); s++)
            {
                String seg = segments[s];
                if (!cursor.has("children"))
                    cursor["children"] = Dictionary();
                Dictionary cursor_children = cursor["children"];
                if (!cursor_children.has(seg))
                    cursor_children[seg] = Dictionary();
                cursor = cursor_children[seg];
            }
            cursor["tag"] = tag;
            cursor["path"] = path;
        }
    }

    _add_tree_children(root, trie.get("children", Dictionary()), file_colors, "");
}

void OghamGraphView::_add_tree_children(TreeItem *parent_item, const Dictionary &children, const Dictionary &file_colors, const String &parent_key)
{
    Array seg_names = children.keys();
    seg_names.sort();
    for (int i = 0; i < seg_names.size(); i++)
    {
        String seg = seg_names[i];
        Dictionary node = children[seg];
        String path_key = parent_key.is_empty() ? seg : String("{0}.{1}").format(Array::make(parent_key, seg));
        TreeItem *item = _tree->create_item(parent_item);
        item->set_text(0, seg);
        Dictionary meta;
        meta["path_key"] = path_key;
        if (node.has("tag"))
        {
            String tag = node["tag"];
            String path = node["path"];
            meta["tag"] = tag;
            meta["path"] = path;
            item->set_icon(0, _swatch_icon(file_colors.get(path, Color(1, 1, 1))));
            item->set_tooltip_text(0, String("{0}\n({1})").format(Array::make(tag, path.get_file())));
        }
        item->set_metadata(0, meta);
        if (node.has("children"))
        {
            item->set_collapsed(bool(_tree_collapsed_state.get(path_key, false)));
            _add_tree_children(item, node["children"], file_colors, path_key);
        }
    }
}

Ref<ImageTexture> OghamGraphView::_swatch_icon(const Color &color)
{
    std::string cache_key = to_std(color.to_html(true));
    auto found = _swatch_icon_cache.find(cache_key);
    if (found != _swatch_icon_cache.end())
        return found->second;
    int size = 12;
    Ref<Image> img = Image::create(size, size, false, Image::FORMAT_RGBA8);
    img->fill(color);
    Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
    _swatch_icon_cache[cache_key] = tex;
    return tex;
}

void OghamGraphView::clear_swatch_icon_cache()
{
    _swatch_icon_cache.clear();
}

void OghamGraphView::_on_tree_item_selected()
{
    TreeItem *item = _tree->get_selected();
    if (item == nullptr)
        return;
    Variant meta = item->get_metadata(0);
    if (meta.get_type() != Variant::DICTIONARY)
        return;
    Dictionary meta_dict = meta;
    if (!meta_dict.has("tag"))
        return; // a branch (shared dot-segment), not an actual entry
    _on_search_result_clicked(meta_dict["path"], meta_dict["tag"]);
}

void OghamGraphView::_on_tree_item_collapsed(TreeItem *item)
{
    Variant meta = item->get_metadata(0);
    if (meta.get_type() == Variant::DICTIONARY)
    {
        Dictionary meta_dict = meta;
        if (meta_dict.has("path_key"))
            _tree_collapsed_state[meta_dict["path_key"]] = item->is_collapsed();
    }
}

void OghamGraphView::_set_all_collapsed(TreeItem *item, bool collapsed)
{
    TypedArray<TreeItem> children = item->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        TreeItem *child = Object::cast_to<TreeItem>(children[i]);
        child->set_collapsed(collapsed);
        Variant meta = child->get_metadata(0);
        if (meta.get_type() == Variant::DICTIONARY)
        {
            Dictionary meta_dict = meta;
            if (meta_dict.has("path_key"))
                _tree_collapsed_state[meta_dict["path_key"]] = collapsed;
        }
        _set_all_collapsed(child, collapsed);
    }
}

void OghamGraphView::_on_expand_all_pressed()
{
    TreeItem *root = _tree->get_root();
    if (root != nullptr)
        _set_all_collapsed(root, false);
}

void OghamGraphView::_on_collapse_all_pressed()
{
    TreeItem *root = _tree->get_root();
    if (root != nullptr)
        _set_all_collapsed(root, true);
}

void OghamGraphView::_rebuild_tree_pill_row()
{
    if (_tree_pill_row == nullptr)
        return;
    TypedArray<Node> children = _tree_pill_row->get_children();
    for (int i = 0; i < children.size(); i++)
        Object::cast_to<Node>(children[i])->queue_free();

    Dictionary file_colors = _file_color_by_path();
    for (int i = 0; i < _story_paths.size(); i++)
    {
        String path = _story_paths[i];
        String display_name = path.get_file().get_basename();
        Button *pill = memnew(Button);
        pill->set_text(display_name);
        pill->set_toggle_mode(true);
        bool pressed = bool(_visible_paths.get(path, true));
        // set_pressed_no_signal — same reasoning as the file-list checkbox
        // above: this pill is freshly recreated on every rebuild, and
        // set_pressed() would re-emit "toggled" and recursively
        // re-trigger _rebuild_graph.
        pill->set_pressed_no_signal(pressed);
        pill->set_tooltip_text(String("Show/hide {0}'s nodes on the canvas").format(Array::make(display_name)));
        pill->add_theme_font_size_override("font_size", 10);
        Color color = file_colors.get(path, Color(1, 1, 1));
        Ref<StyleBoxFlat> style;
        style.instantiate();
        style->set_bg_color(pressed ? color : color.darkened(0.6));
        style->set_corner_radius_all(8);
        style->set_content_margin(SIDE_LEFT, 6);
        style->set_content_margin(SIDE_RIGHT, 6);
        style->set_content_margin(SIDE_TOP, 1);
        style->set_content_margin(SIDE_BOTTOM, 1);
        pill->add_theme_stylebox_override("normal", style);
        pill->add_theme_stylebox_override("pressed", style);
        pill->add_theme_stylebox_override("hover", style);
        pill->connect("toggled", callable_mp(this, &OghamGraphView::_on_file_visibility_toggled).bind(path));
        _tree_pill_row->add_child(pill);
    }
}

void OghamGraphView::_on_search_text_changed(String new_text)
{
    _search_pattern = new_text.strip_edges();
    _rebuild_file_panel();
    _rebuild_tree_panel();
}

void OghamGraphView::_on_search_result_clicked(String path, String tag)
{
    if (!bool(_visible_paths.get(path, true)))
    {
        _visible_paths[path] = true;
        _rebuild_graph(true);
        _hide_loading();
    }
    _focus_node(tag);
}

bool OghamGraphView::_tag_matches_pattern(const String &tag, const String &pattern)
{
    if (pattern.is_empty())
        return true;
    return _segments_match(tag.split("."), pattern.split("."), 0, 0);
}

bool OghamGraphView::_segments_match(const PackedStringArray &tag_segs, const PackedStringArray &pat_segs, int ti, int pi)
{
    if (pi >= pat_segs.size())
        return ti >= tag_segs.size();
    String pseg = pat_segs[pi];
    if (pseg == "*")
    {
        for (int skip = ti; skip <= tag_segs.size(); skip++)
        {
            if (_segments_match(tag_segs, pat_segs, skip, pi + 1))
                return true;
        }
        return false;
    }
    if (ti >= tag_segs.size() || tag_segs[ti] != pseg)
        return false;
    return _segments_match(tag_segs, pat_segs, ti + 1, pi + 1);
}

void OghamGraphView::_on_file_visibility_toggled(bool is_visible, String path)
{
    _visible_paths[path] = is_visible;
    _rebuild_graph(true);
    _hide_loading();
}

void OghamGraphView::_set_current_path(const String &path)
{
    _current_path = path;
    _path_label->set_text(path);
    _rebuild_file_panel();
}

Color OghamGraphView::_color_for_path(const String &path, int index)
{
    Ref<OghamDocument> doc = _all_story_data.get(path, Variant());
    if (doc.is_null())
        return PALETTE[index % 6];
    Color color = doc->get_header_color();
    if (color.a == 0.0)
        color = PALETTE[index % 6];
    return color;
}

void OghamGraphView::_on_file_color_changed(Color color, String path)
{
    Ref<OghamDocument> doc = _all_story_data.get(path, Variant());
    if (doc.is_null())
        return;
    doc->set_header_color(color);
    _rebuild_graph();
}

void OghamGraphView::_focus_node(const String &tag)
{
    String node_name = _tag_to_node_name.get(tag, "");
    Node *node = node_name.is_empty() ? nullptr : _graph->get_node_or_null(NodePath(node_name));
    if (node == nullptr)
        return;
    TypedArray<Node> children = _graph->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        GraphNode *gn = Object::cast_to<GraphNode>(children[i]);
        if (gn != nullptr)
            gn->set_selected(gn == node);
    }
    OghamGraphNode *ogn = Object::cast_to<OghamGraphNode>(node);
    Vector2 pos = ogn->get_position_offset() + ogn->get_size() / 2.0;
    _graph->set_scroll_offset(pos * _graph->get_zoom() - _graph->get_size() / 2.0);
}

void OghamGraphView::_on_focus_target_signal(OghamGraphNode *node, String target_tag)
{
    _focus_node(target_tag);
}

Ref<OghamDocument> OghamGraphView::_data_for_tag(const String &tag)
{
    for (int i = 0; i < _story_paths.size(); i++)
    {
        String path = _story_paths[i];
        if (!bool(_visible_paths.get(path, true)))
            continue;
        Ref<OghamDocument> doc = _all_story_data.get(path, Variant());
        if (doc.is_valid() && !doc->find_entry(tag).is_empty())
            return doc;
    }
    return Ref<OghamDocument>();
}

// ============================== layout math (synchronous — see class doc) ==============================

double OghamGraphView::_estimate_row_height(const String &text)
{
    int lines = std::max(1, int(std::ceil(text.length() / EST_CHARS_PER_LINE)));
    return lines * 20.0 + 6.0;
}

double OghamGraphView::_estimate_node_height(const Dictionary &entry)
{
    double height = 90.0; // titlebar + node margins
    int section_header_rows = 3;
    height += section_header_rows * 24.0;
    Array content_keys = entry.get("contentKeys", Array());
    for (int i = 0; i < content_keys.size(); i++)
    {
        Dictionary ck = content_keys[i];
        height += std::max(_estimate_row_height(String(ck.get("key", ""))), EST_ROW_MIN_HEIGHT);
    }
    Array ops = entry.get("entryOperations", Array());
    height += ops.size() * EST_ROW_MIN_HEIGHT;
    Array options = entry.get("options", Array());
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary opt = options[i];
        height += std::max(_estimate_row_height(String(opt.get("textKey", ""))), EST_ROW_MIN_HEIGHT);
    }
    return std::max(height, 160.0);
}

void OghamGraphView::_auto_layout_missing(const String &path, bool force)
{
    Ref<OghamDocument> doc = _all_story_data.get(path, Variant());
    if (doc.is_null())
        return;
    Array entries = doc->get_entries();
    PackedStringArray needs_layout;
    Dictionary tag_to_entry;
    for (int i = 0; i < entries.size(); i++)
    {
        Dictionary e = entries[i];
        String tag = e.get("tag", "");
        tag_to_entry[tag] = e;
        if (force || doc->get_editor_node(tag).is_empty())
            needs_layout.push_back(tag);
    }
    if (needs_layout.is_empty())
        return;

    Dictionary indegree;
    for (int i = 0; i < needs_layout.size(); i++)
        indegree[needs_layout[i]] = 0;
    for (int i = 0; i < entries.size(); i++)
    {
        Dictionary e = entries[i];
        Array options = e.get("options", Array());
        for (int o = 0; o < options.size(); o++)
        {
            Dictionary opt = options[o];
            String target = opt.get("targetTag", "");
            if (indegree.has(target))
                indegree[target] = int(indegree[target]) + 1;
        }
    }

    Dictionary depth;
    Array queue;
    for (int i = 0; i < needs_layout.size(); i++)
    {
        String tag = needs_layout[i];
        if (int(indegree[tag]) == 0)
        {
            depth[tag] = 0;
            queue.push_back(tag);
        }
    }
    int qi = 0;
    while (qi < queue.size())
    {
        String tag = queue[qi];
        qi++;
        Dictionary e = tag_to_entry.get(tag, Dictionary());
        Array options = e.get("options", Array());
        for (int o = 0; o < options.size(); o++)
        {
            Dictionary opt = options[o];
            String target = opt.get("targetTag", "");
            if (needs_layout.has(target) && !depth.has(target))
            {
                depth[target] = int(depth[tag]) + 1;
                queue.push_back(target);
            }
        }
    }
    for (int i = 0; i < needs_layout.size(); i++)
    {
        String tag = needs_layout[i];
        if (!depth.has(tag))
            depth[tag] = 0;
    }

    int file_index = std::max(int(_story_paths.find(path)), 0);
    double y_base = file_index * LAYOUT_PROVISIONAL_FILE_GAP;
    Dictionary col_y_cursor;
    for (int i = 0; i < needs_layout.size(); i++)
    {
        String tag = needs_layout[i];
        int d = depth[tag];
        double y = col_y_cursor.get(d, 0.0);
        double h = _estimate_node_height(tag_to_entry.get(tag, Dictionary()));
        Vector2 pos(d * LAYOUT_COLUMN_WIDTH, y_base + y);
        doc->set_node_rect(tag, Rect2(pos, Vector2(520, h)));
        col_y_cursor[d] = y + h + LAYOUT_ROW_GAP;
    }
}

String OghamGraphView::_score_node_group(const String &tag, const Dictionary &entry, const Dictionary &owner_by_tag, const Dictionary &incoming_by_target)
{
    String own_file = owner_by_tag.get(tag, "");
    Dictionary score;
    score[own_file] = int(score.get(own_file, 0)) + 1;

    Array incomers = incoming_by_target.get(tag, Array());
    if (incomers.is_empty())
    {
        score[own_file] = int(score.get(own_file, 0)) + 1;
    }
    else
    {
        for (int i = 0; i < incomers.size(); i++)
        {
            String from_tag = incomers[i];
            String fp = owner_by_tag.get(from_tag, own_file);
            score[fp] = int(score.get(fp, 0)) + 1;
        }
    }

    Dictionary seen_foreign;
    Array options = entry.get("options", Array());
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary opt = options[i];
        String target = opt.get("targetTag", "");
        if (target.is_empty() || target == tag)
            continue;
        if (!owner_by_tag.has(target))
            continue;
        String target_file = owner_by_tag[target];
        if (target_file == own_file)
            score[own_file] = int(score.get(own_file, 0)) + 1;
        else if (seen_foreign.has(target_file))
            score[target_file] = int(score.get(target_file, 0)) + 1;
        else
            seen_foreign[target_file] = true;
    }

    String best_file = own_file;
    int best_score = score.get(own_file, 0);
    Array score_keys = score.keys();
    for (int i = 0; i < score_keys.size(); i++)
    {
        String f = score_keys[i];
        int s = score[f];
        if (s > best_score)
        {
            best_score = s;
            best_file = f;
        }
    }
    return best_file;
}

Dictionary OghamGraphView::_compute_global_layout(const Dictionary &entries_by_tag, const Dictionary &owner_by_tag)
{
    Dictionary incoming_by_target;
    Array all_tags = entries_by_tag.keys();
    for (int i = 0; i < all_tags.size(); i++)
    {
        String tag = all_tags[i];
        Dictionary entry = entries_by_tag[tag];
        Array options = entry.get("options", Array());
        for (int o = 0; o < options.size(); o++)
        {
            Dictionary opt = options[o];
            String target = opt.get("targetTag", "");
            if (target.is_empty() || target == tag)
                continue;
            if (!incoming_by_target.has(target))
                incoming_by_target[target] = Array();
            Array arr = incoming_by_target[target];
            arr.push_back(tag);
        }
    }

    Dictionary groups; // group_file -> Array[tag]
    for (int i = 0; i < all_tags.size(); i++)
    {
        String tag = all_tags[i];
        String group_file = _score_node_group(tag, entries_by_tag[tag], owner_by_tag, incoming_by_target);
        if (!groups.has(group_file))
            groups[group_file] = Array();
        Array arr = groups[group_file];
        arr.push_back(tag);
    }

    Array group_files = groups.keys();
    group_files.sort();

    Dictionary rects;
    double y_cursor = 0.0;
    for (int g = 0; g < group_files.size(); g++)
    {
        String group_file = group_files[g];
        Array group_tags = groups[group_file];
        Dictionary group_tag_set;
        for (int i = 0; i < group_tags.size(); i++)
            group_tag_set[group_tags[i]] = true;

        Dictionary indegree;
        for (int i = 0; i < group_tags.size(); i++)
            indegree[group_tags[i]] = 0;
        for (int i = 0; i < group_tags.size(); i++)
        {
            String t = group_tags[i];
            Dictionary entry = entries_by_tag[t];
            Array options = entry.get("options", Array());
            for (int o = 0; o < options.size(); o++)
            {
                Dictionary opt = options[o];
                String target = opt.get("targetTag", "");
                if (group_tag_set.has(target))
                    indegree[target] = int(indegree[target]) + 1;
            }
        }

        Dictionary depth;
        Array queue;
        for (int i = 0; i < group_tags.size(); i++)
        {
            String t = group_tags[i];
            if (int(indegree[t]) == 0)
            {
                depth[t] = 0;
                queue.push_back(t);
            }
        }
        int qi = 0;
        while (qi < queue.size())
        {
            String t = queue[qi];
            qi++;
            Dictionary entry = entries_by_tag[t];
            Array options = entry.get("options", Array());
            for (int o = 0; o < options.size(); o++)
            {
                Dictionary opt = options[o];
                String target = opt.get("targetTag", "");
                if (group_tag_set.has(target) && !depth.has(target))
                {
                    depth[target] = int(depth[t]) + 1;
                    queue.push_back(target);
                }
            }
        }
        for (int i = 0; i < group_tags.size(); i++)
        {
            String t = group_tags[i];
            if (!depth.has(t))
                depth[t] = 0;
        }

        Dictionary col_y_cursor;
        for (int i = 0; i < group_tags.size(); i++)
        {
            String t = group_tags[i];
            int d = depth[t];
            double local_y = col_y_cursor.get(d, 0.0);
            double h = _estimate_node_height(entries_by_tag[t]);
            rects[t] = Rect2(Vector2(d * LAYOUT_COLUMN_WIDTH, y_cursor + local_y), Vector2(520, h));
            col_y_cursor[d] = local_y + h + LAYOUT_ROW_GAP;
        }

        double group_height = 0.0;
        Array col_keys = col_y_cursor.keys();
        for (int i = 0; i < col_keys.size(); i++)
            group_height = std::max(group_height, double(col_y_cursor[col_keys[i]]));
        y_cursor += group_height + LAYOUT_GROUP_GAP;
    }

    return rects;
}

void OghamGraphView::_on_layout_all_files_pressed()
{
    _show_loading("Computing layout...");
    Dictionary entries_by_tag;
    Dictionary owner_by_tag;
    Array paths = _all_story_data.keys();
    for (int p = 0; p < paths.size(); p++)
    {
        String path = paths[p];
        Ref<OghamDocument> doc = _all_story_data[path];
        Array entries = doc->get_entries();
        for (int i = 0; i < entries.size(); i++)
        {
            Dictionary entry = entries[i];
            String tag = entry.get("tag", "");
            entries_by_tag[tag] = entry;
            owner_by_tag[tag] = path;
        }
    }

    Dictionary rects = _compute_global_layout(entries_by_tag, owner_by_tag);

    Array tags = rects.keys();
    for (int i = 0; i < tags.size(); i++)
    {
        String tag = tags[i];
        String path = owner_by_tag.get(tag, "");
        if (path.is_empty())
            continue;
        Ref<OghamDocument> doc = _all_story_data[path];
        doc->set_node_rect(tag, rects[tag]);
    }

    _rebuild_graph(true);
    _hide_loading();
}

void OghamGraphView::_on_open_pressed()
{
    _file_dialog->popup_centered_ratio(0.6);
}

void OghamGraphView::_on_file_selected(String path)
{
    if (!_story_paths.has(path))
        _story_paths.push_back(path);
    if (!_all_story_data.has(path))
    {
        _show_loading(String("Loading {0}...").format(Array::make(path.get_file())));
        Ref<OghamDocument> doc = OghamManifestIO::load_manifest(path);
        _all_story_data[path] = doc;
        _visible_paths[path] = false;
        _auto_layout_missing(path);
    }
    _set_current_path(path);
    _rebuild_graph(true);
    _hide_loading();
}

void OghamGraphView::_on_save_pressed()
{
    Array paths = _all_story_data.keys();
    for (int i = 0; i < paths.size(); i++)
    {
        String path = paths[i];
        Ref<OghamDocument> doc = _all_story_data[path];
        doc->save(path);
    }
}

void OghamGraphView::_on_graph_popup_request(Vector2 at_position)
{
    PopupMenu *menu = memnew(PopupMenu);
    add_child(menu);
    menu->add_item("Add Node", 0);
    menu->connect("id_pressed", callable_mp(this, &OghamGraphView::_add_entry_at).bind(at_position).unbind(1));
    Node *menu_as_node = menu;
    menu->connect("id_pressed", callable_mp(menu_as_node, &Node::queue_free).unbind(1));
    menu->popup(Rect2i(_mouse_screen_pos(), Vector2i()));
}

void OghamGraphView::_add_entry_at(Vector2 graph_position)
{
    if (_current_path.is_empty())
        return;
    Ref<OghamDocument> doc = _all_story_data.get(_current_path, Variant());
    if (doc.is_null())
        return;
    String base_tag = "NewEntry";
    String tag = base_tag;
    int suffix = 1;
    while (!doc->find_entry(tag).is_empty())
    {
        suffix++;
        tag = String("{0}{1}").format(Array::make(base_tag, suffix));
    }
    doc->add_entry(tag);
    doc->set_node_rect(tag, Rect2(graph_position, Vector2(520, 200)));
    _rebuild_graph();
}

// ============================== rebuild_graph (continuation chain — see class doc) ==============================

Array OghamGraphView::_gather_visible_items()
{
    Array visible_items;
    for (int i = 0; i < _story_paths.size(); i++)
    {
        String path = _story_paths[i];
        if (!bool(_visible_paths.get(path, true)))
            continue;
        Ref<OghamDocument> doc = _all_story_data.get(path, Variant());
        if (doc.is_null())
            continue;
        Array entries = doc->get_entries();
        for (int e = 0; e < entries.size(); e++)
        {
            Dictionary item;
            item["path"] = path;
            item["entry"] = entries[e];
            item["data"] = doc;
            visible_items.push_back(item);
        }
    }
    return visible_items;
}

Dictionary OghamGraphView::_file_color_by_path()
{
    Dictionary colors;
    for (int i = 0; i < _story_paths.size(); i++)
        colors[_story_paths[i]] = _color_for_path(_story_paths[i], i);
    return colors;
}

Dictionary OghamGraphView::_section_expanded_for(const Ref<OghamDocument> &doc, const String &tag)
{
    Dictionary d;
    d["ops"] = doc->get_section_expanded(tag, "ops");
    d["fields"] = doc->get_section_expanded(tag, "fields");
    d["choices"] = doc->get_section_expanded(tag, "choices");
    return d;
}

Dictionary OghamGraphView::_compute_link_annotations(const Array &visible_items, const Dictionary &file_colors)
{
    Dictionary incoming_tags;
    Dictionary tab_incoming_by_target;
    for (int i = 0; i < visible_items.size(); i++)
    {
        Dictionary item = visible_items[i];
        Dictionary entry = item["entry"];
        String entry_tag = entry.get("tag", "");
        Ref<OghamDocument> item_doc = item["data"];
        Array entry_tab_options = item_doc->get_tab_flag_options(entry_tag);
        Array options = entry.get("options", Array());
        for (int o = 0; o < options.size(); o++)
        {
            Dictionary option = options[o];
            String target = option.get("targetTag", "");
            if (target.is_empty())
                continue;
            incoming_tags[target] = true;
            String opt_tag = option.get("tag", "");
            if (entry_tab_options.has(opt_tag) && !tab_incoming_by_target.has(target))
                tab_incoming_by_target[target] = file_colors.get(item["path"], Color(1, 1, 1));
        }
    }
    Dictionary result;
    result["incoming_tags"] = incoming_tags;
    result["tab_incoming_by_target"] = tab_incoming_by_target;
    return result;
}

Dictionary OghamGraphView::_node_render_annotations(const String &tag, const Dictionary &entry, const Ref<OghamDocument> &doc, const Color &file_color, const Dictionary &link_ann)
{
    Color own_highlight = doc->get_highlight_color(tag);
    Dictionary tab_incoming_by_target = link_ann["tab_incoming_by_target"];
    Color border_color = own_highlight.a > 0.0 ? own_highlight : Color(tab_incoming_by_target.get(tag, Color(0, 0, 0, 0)));

    Dictionary tab_flags;
    Array this_tab_options = doc->get_tab_flag_options(tag);
    Array options = entry.get("options", Array());
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary option = options[i];
        String opt_tag = option.get("tag", "");
        String target = option.get("targetTag", "");
        if (opt_tag.is_empty() || target.is_empty() || target == tag)
            continue;
        if (this_tab_options.has(opt_tag))
        {
            Ref<OghamDocument> target_doc = _data_for_tag(target);
            Color target_hl = target_doc.is_valid() ? target_doc->get_highlight_color(target) : Color(0, 0, 0, 0);
            tab_flags[opt_tag] = target_hl.a > 0.0 ? target_hl : file_color;
        }
    }

    Dictionary incoming_tags = link_ann["incoming_tags"];
    Dictionary result;
    result["incoming"] = incoming_tags.has(tag);
    result["border_color"] = border_color;
    result["tab_flags"] = tab_flags;
    return result;
}

void OghamGraphView::_connect_all_wires(const Array &visible_items)
{
    for (int i = 0; i < visible_items.size(); i++)
    {
        Dictionary item = visible_items[i];
        Dictionary entry = item["entry"];
        String from_tag = entry.get("tag", "");
        String from_node_name = _tag_to_node_name.get(from_tag, "");
        OghamGraphNode *from_node = from_node_name.is_empty() ? nullptr : Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(from_node_name)));
        if (from_node == nullptr)
            continue;
        Array options = entry.get("options", Array());
        for (int o = 0; o < options.size(); o++)
        {
            Dictionary opt = options[o];
            String target = opt.get("targetTag", "");
            String to_node_name = _tag_to_node_name.get(target, "");
            int port = from_node->option_port_index(o);
            if (!to_node_name.is_empty() && port >= 0)
                _graph->connect_node(from_node->get_name(), port, to_node_name, 0);
        }
    }
}

void OghamGraphView::_sync_connections()
{
    Array visible_items = _gather_visible_items();

    Dictionary desired;
    for (int i = 0; i < visible_items.size(); i++)
    {
        Dictionary item = visible_items[i];
        Dictionary entry = item["entry"];
        String from_tag = entry.get("tag", "");
        String from_node_name = _tag_to_node_name.get(from_tag, "");
        OghamGraphNode *from_node = from_node_name.is_empty() ? nullptr : Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(from_node_name)));
        if (from_node == nullptr)
            continue;
        Array options = entry.get("options", Array());
        for (int o = 0; o < options.size(); o++)
        {
            Dictionary opt = options[o];
            String target = opt.get("targetTag", "");
            String to_node_name = _tag_to_node_name.get(target, "");
            int port = from_node->option_port_index(o);
            if (to_node_name.is_empty() || port < 0)
                continue;
            String key = String("{0}|{1}|{2}|0").format(Array::make(String(from_node->get_name()), port, to_node_name));
            Array d;
            d.push_back(String(from_node->get_name()));
            d.push_back(port);
            d.push_back(to_node_name);
            desired[key] = d;
        }
    }

    Dictionary current;
    Array connections = _graph->get_connection_list();
    for (int i = 0; i < connections.size(); i++)
    {
        Dictionary c = connections[i];
        String key = String("{0}|{1}|{2}|{3}").format(Array::make(String(c["from_node"]), int(c["from_port"]), String(c["to_node"]), int(c["to_port"])));
        current[key] = c;
    }

    Array desired_keys = desired.keys();
    for (int i = 0; i < desired_keys.size(); i++)
    {
        String key = desired_keys[i];
        if (!current.has(key))
        {
            Array d = desired[key];
            _graph->connect_node(d[0], int(d[1]), d[2], 0);
        }
    }
    Array current_keys = current.keys();
    for (int i = 0; i < current_keys.size(); i++)
    {
        String key = current_keys[i];
        if (!desired.has(key))
        {
            Dictionary c = current[key];
            _graph->disconnect_node(c["from_node"], int(c["from_port"]), c["to_node"], int(c["to_port"]));
        }
    }
}

void OghamGraphView::_reconnect_process_frame_once(const Callable &callable)
{
    SceneTree *tree = get_tree();
    if (tree->is_connected("process_frame", callable))
        tree->disconnect("process_frame", callable);
    tree->connect("process_frame", callable, CONNECT_ONE_SHOT);
}

void OghamGraphView::_rebuild_graph(bool show_progress)
{
    _rebuild_generation++;
    int generation = _rebuild_generation;

    TypedArray<Node> children = _graph->get_children();
    for (int i = 0; i < children.size(); i++)
    {
        GraphNode *child = Object::cast_to<GraphNode>(children[i]);
        if (child == nullptr)
            continue;
        // remove_child() frees the name immediately — see OghamGraphView.gd's
        // identically-worded comment: queue_free() alone only defers actual
        // deallocation, and a rebuild triggered again within the same couple
        // of frames could reuse a still-reserved name, get silently
        // auto-renamed by Godot, and leave _tag_to_node_name pointing at a
        // node that no longer exists under that name.
        _graph->remove_child(child);
        child->queue_free();
    }
    _graph->clear_connections();
    _tag_to_node_name.clear();

    Array visible_items = _gather_visible_items();
    _rebuild_build_next_batch(visible_items, 0, show_progress, generation);
}

void OghamGraphView::_rebuild_build_next_batch(Array visible_items, int cursor, bool show_progress, int generation)
{
    if (generation != _rebuild_generation)
        return; // superseded

    Dictionary file_colors = _file_color_by_path();
    Dictionary link_ann = _compute_link_annotations(visible_items, file_colors);

    int built_this_batch = 0;
    while (cursor < visible_items.size() && built_this_batch < NODES_PER_FRAME)
    {
        Dictionary item = visible_items[cursor];
        Dictionary entry = item["entry"];
        Ref<OghamDocument> doc = item["data"];
        String path = item["path"];
        String tag = entry.get("tag", "");
        Color file_color = file_colors.get(path, Color(1, 1, 1));
        Dictionary node_ann = _node_render_annotations(tag, entry, doc, file_color, link_ann);

        OghamGraphNode *node = memnew(OghamGraphNode);
        node->set_name(tag);
        node->set_owner_path(path);
        node->set_header_color(file_color);
        Dictionary expanded = _section_expanded_for(doc, tag);

        // Added to the tree BEFORE setup()/rebuild() — see
        // OghamGraphNode._field_type_icon's need to resolve a theme from an
        // ancestor, same reasoning as the GDScript original.
        _graph->add_child(node);
        Dictionary annotations;
        annotations["label_defs"] = _label_defs_for(doc);
        annotations["assigned_label_ids"] = doc->get_assigned_label_ids(tag);
        annotations["border_color"] = node_ann["border_color"];
        annotations["tab_flags"] = node_ann["tab_flags"];
        node->setup(entry, expanded, node_ann["incoming"], annotations);
        _tag_to_node_name[tag] = String(node->get_name());

        node->connect("content_key_clicked", callable_mp(this, &OghamGraphView::_on_content_key_clicked));
        node->connect("content_key_delete_requested", callable_mp(this, &OghamGraphView::_on_content_key_delete_requested));
        node->connect("content_key_move_requested", callable_mp(this, &OghamGraphView::_on_content_key_move_requested));
        node->connect("option_clicked", callable_mp(this, &OghamGraphView::_on_option_clicked));
        node->connect("option_delete_requested", callable_mp(this, &OghamGraphView::_on_option_delete_requested));
        node->connect("option_move_requested", callable_mp(this, &OghamGraphView::_on_option_move_requested));
        node->connect("entry_operation_clicked", callable_mp(this, &OghamGraphView::_on_entry_operation_clicked));
        node->connect("entry_operation_delete_requested", callable_mp(this, &OghamGraphView::_on_entry_operation_delete_requested));
        node->connect("entry_operation_move_requested", callable_mp(this, &OghamGraphView::_on_entry_operation_move_requested));
        node->connect("tag_renamed", callable_mp(this, &OghamGraphView::_on_node_renamed));
        node->connect("fork_toggled", callable_mp(this, &OghamGraphView::_on_node_fork_toggled));
        node->connect("add_content_key_requested", callable_mp(this, &OghamGraphView::_on_add_content_key_requested));
        node->connect("add_option_requested", callable_mp(this, &OghamGraphView::_on_add_option_requested));
        node->connect("add_entry_operation_requested", callable_mp(this, &OghamGraphView::_on_add_entry_operation_requested));
        node->connect("section_toggle_requested", callable_mp(this, &OghamGraphView::_on_section_toggle_requested));
        node->connect("tab_flag_clicked", callable_mp(this, &OghamGraphView::_on_focus_target_signal));
        node->connect("go_to_option_target_requested", callable_mp(this, &OghamGraphView::_on_focus_target_signal));
        node->connect("context_menu_requested", callable_mp(this, &OghamGraphView::_on_node_context_menu_requested));

        Rect2 rect = doc->get_node_rect(tag);
        node->set_position_offset(rect.position);
        // Width is a real, worth-persisting preference — height is not (see
        // OghamGraphView.gd's identically-worded comment); height is snapped
        // to the node's real fit-to-content minimum after layout below.
        node->set_size(Vector2(rect.size.x, 0));
        node->connect("dragged", callable_mp(this, &OghamGraphView::_on_node_dragged).bind(node));

        cursor++;
        built_this_batch++;
    }

    if (cursor < visible_items.size())
    {
        if (show_progress)
            _show_loading(String("Building graph... {0} / {1}").format(Array::make(cursor, visible_items.size())));
        _reconnect_process_frame_once(callable_mp(this, &OghamGraphView::_rebuild_build_next_batch).bind(visible_items, cursor, show_progress, generation));
        return;
    }

    _rebuild_file_panel();
    _rebuild_tree_panel();
    _rebuild_tree_pill_row();

    // GraphNode builds its port-position cache lazily during layout, not
    // immediately on add_child() — wait for an actual frame before
    // connecting wires or measuring fit-to-content height. See
    // OghamGraphView.gd's identically-worded comment for why.
    _rebuild_wait_layout(visible_items, 2, 1, generation);
}

void OghamGraphView::_on_node_dragged(Vector2 from, Vector2 to, OghamGraphNode *node)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_valid())
        doc->set_node_rect(node->get_entry_data().get("tag", ""), Rect2(to, node->get_size()));
}

void OghamGraphView::_rebuild_wait_layout(Array visible_items, int frames_left, int pass, int generation)
{
    frames_left--;
    if (frames_left > 0)
    {
        _reconnect_process_frame_once(callable_mp(this, &OghamGraphView::_rebuild_wait_layout).bind(visible_items, frames_left, pass, generation));
        return;
    }
    if (generation != _rebuild_generation)
        return; // superseded by a newer _rebuild_graph() call; its nodes are already gone

    _rebuild_apply_fit_pass(visible_items);

    if (pass == 1)
    {
        // A second pass, a couple frames later — see OghamGraphView.gd's
        // identically-worded comment: on a big first-ever load, two frames
        // isn't always reliably enough for get_combined_minimum_size() to
        // already reflect the final wrapped text layout.
        _rebuild_wait_layout(visible_items, 2, 2, generation);
        return;
    }

    _connect_all_wires(visible_items);

    // This is the true end of _rebuild_graph's async continuation chain —
    // _rebuild_graph(true)'s callers all call _hide_loading() right after
    // kicking it off, but that only ever hides whatever was showing before
    // this rebuild started; the "Building graph... N / total" text set by
    // _rebuild_build_next_batch's last partial batch (see its call to
    // _show_loading above) is never hidden anywhere else, so without this
    // call the loading overlay freezes on that text forever once the batch
    // count doesn't divide evenly by NODES_PER_FRAME — the graph underneath
    // still finishes building correctly, but looks permanently stuck.
    _hide_loading();
}

void OghamGraphView::_rebuild_apply_fit_pass(const Array &visible_items)
{
    for (int i = 0; i < visible_items.size(); i++)
    {
        Dictionary item = visible_items[i];
        Dictionary entry = item["entry"];
        String from_tag = entry.get("tag", "");
        String node_name = _tag_to_node_name.get(from_tag, "");
        OghamGraphNode *node = node_name.is_empty() ? nullptr : Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(node_name)));
        if (node != nullptr)
        {
            Vector2 size = node->get_size();
            size.y = node->get_combined_minimum_size().y;
            node->set_size(size);
        }
    }
}

// ============================== refresh_node (continuation chain — see class doc) ==============================

void OghamGraphView::_refresh_node(const String &tag, const Callable &on_done)
{
    String node_name = _tag_to_node_name.get(tag, "");
    OghamGraphNode *node = node_name.is_empty() ? nullptr : Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(node_name)));
    if (node == nullptr)
    {
        if (on_done.is_valid())
            on_done.call();
        return;
    }
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
    {
        if (on_done.is_valid())
            on_done.call();
        return;
    }
    // node.entry_data is a snapshot, not a live reference (see OghamDocument's
    // doc comment) — every caller of _refresh_node has already written its
    // edit back into 'doc' via an explicit method, so re-fetch a fresh
    // snapshot here rather than re-rendering the stale one still on the node.
    Dictionary fresh_entry = doc->find_entry(tag);
    if (fresh_entry.is_empty())
    {
        if (on_done.is_valid())
            on_done.call(); // deleted out from under this refresh — nothing to render
        return;
    }
    Array visible_items = _gather_visible_items();
    Dictionary file_colors = _file_color_by_path();
    Dictionary link_ann = _compute_link_annotations(visible_items, file_colors);
    Color file_color = file_colors.get(node->get_owner_path(), Color(1, 1, 1));
    Dictionary node_ann = _node_render_annotations(tag, fresh_entry, doc, file_color, link_ann);

    Dictionary annotations;
    annotations["label_defs"] = _label_defs_for(doc);
    annotations["assigned_label_ids"] = doc->get_assigned_label_ids(tag);
    annotations["border_color"] = node_ann["border_color"];
    annotations["tab_flags"] = node_ann["tab_flags"];
    node->setup(fresh_entry, _section_expanded_for(doc, tag), node_ann["incoming"], annotations);

    // Same fit-to-content settle as the full rebuild's tail — see
    // OghamGraphView.gd's identically-worded comment on why.
    _reconnect_process_frame_once(callable_mp(this, &OghamGraphView::_refresh_wait_frame).bind(tag, 2, 1, on_done));
}

void OghamGraphView::_refresh_wait_frame(String tag, int frames_left, int pass, Callable on_done)
{
    frames_left--;
    if (frames_left > 0)
    {
        _reconnect_process_frame_once(callable_mp(this, &OghamGraphView::_refresh_wait_frame).bind(tag, frames_left, pass, on_done));
        return;
    }

    String node_name = _tag_to_node_name.get(tag, "");
    OghamGraphNode *node = node_name.is_empty() ? nullptr : Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(node_name)));
    if (node != nullptr)
    {
        Vector2 size = node->get_size();
        size.y = node->get_combined_minimum_size().y;
        node->set_size(size);
    }

    if (pass == 1)
    {
        _reconnect_process_frame_once(callable_mp(this, &OghamGraphView::_refresh_wait_frame).bind(tag, 2, 2, on_done));
        return;
    }

    if (node != nullptr)
    {
        Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
        if (doc.is_valid())
            doc->set_node_rect(tag, Rect2(node->get_position_offset(), node->get_size()));
    }

    if (on_done.is_valid())
        on_done.call();
}

void OghamGraphView::_refresh_node_by_tag(const String &tag)
{
    _refresh_node(tag);
}

void OghamGraphView::_refresh_node_and_targets(const String &tag, const Callable &on_done)
{
    _refresh_node(tag, callable_mp(this, &OghamGraphView::_refresh_node_and_targets_step2).bind(tag, on_done));
}

void OghamGraphView::_refresh_node_and_targets_step2(String tag, Callable on_done)
{
    String node_name = _tag_to_node_name.get(tag, "");
    OghamGraphNode *node = node_name.is_empty() ? nullptr : Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(node_name)));
    if (node != nullptr)
    {
        Dictionary targets;
        Array options = node->get_entry_data().get("options", Array());
        for (int i = 0; i < options.size(); i++)
        {
            Dictionary option = options[i];
            String target = option.get("targetTag", "");
            if (!target.is_empty() && target != String(node->get_entry_data().get("tag", "")))
                targets[target] = true;
        }
        Array target_tags = targets.keys();
        for (int i = 0; i < target_tags.size(); i++)
            _refresh_node_by_tag(target_tags[i]); // fire-and-forget, matches GDScript's un-awaited loop
    }
    if (on_done.is_valid())
        on_done.call();
}

// ============================== connections + node edit handlers ==============================

Dictionary OghamGraphView::_label_defs_for(const Ref<OghamDocument> &doc)
{
    Dictionary label_defs;
    Array labels = doc->get_labels();
    for (int i = 0; i < labels.size(); i++)
    {
        Dictionary def = labels[i];
        int lid = int(def.get("id", -1));
        Dictionary entry;
        entry["name"] = def.get("name", "");
        entry["color"] = Color::html(def.get("color", "#6699CC"));
        label_defs[lid] = entry;
    }
    return label_defs;
}

String OghamGraphView::_entry_tag_for_node_name(const String &node_name)
{
    Node *node = _graph->get_node_or_null(NodePath(node_name));
    OghamGraphNode *ogn = Object::cast_to<OghamGraphNode>(node);
    if (ogn != nullptr)
        return ogn->get_entry_data().get("tag", "");
    return String();
}

void OghamGraphView::_on_connection_request(String from_node, int from_port, String to_node, int to_port)
{
    OghamGraphNode *from_gn = Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(from_node)));
    String to_tag = _entry_tag_for_node_name(to_node);
    if (from_gn == nullptr)
        return;
    Array options = from_gn->get_entry_data().get("options", Array());
    int option_index = from_gn->option_index_for_port(from_port);
    if (option_index < 0 || option_index >= options.size())
        return;
    Dictionary option = options[option_index];
    String option_tag = option.get("tag", "");
    String old_target = option.get("targetTag", "");
    String tag = from_gn->get_entry_data().get("tag", "");
    Ref<OghamDocument> from_doc = _all_story_data.get(from_gn->get_owner_path(), Variant());
    if (from_doc.is_null())
        return;
    from_doc->set_option_target(tag, option_tag, to_tag);
    _refresh_node_and_targets(tag, callable_mp(this, &OghamGraphView::_on_connection_request_done).bind(old_target, to_tag));
}

void OghamGraphView::_on_disconnection_request(String from_node, int from_port, String to_node, int to_port)
{
    OghamGraphNode *from_gn = Object::cast_to<OghamGraphNode>(_graph->get_node_or_null(NodePath(from_node)));
    if (from_gn == nullptr)
        return;
    Array options = from_gn->get_entry_data().get("options", Array());
    int option_index = from_gn->option_index_for_port(from_port);
    if (option_index < 0 || option_index >= options.size())
        return;
    Dictionary option = options[option_index];
    String option_tag = option.get("tag", "");
    String old_target = option.get("targetTag", "");
    String tag = from_gn->get_entry_data().get("tag", "");
    Ref<OghamDocument> from_doc = _all_story_data.get(from_gn->get_owner_path(), Variant());
    if (from_doc.is_null())
        return;
    from_doc->set_option_target(tag, option_tag, "");
    _refresh_node(tag, callable_mp(this, &OghamGraphView::_on_disconnection_request_done).bind(old_target));
}

void OghamGraphView::_on_delete_nodes_request(Array nodes)
{
    for (int i = 0; i < nodes.size(); i++)
    {
        Variant node_path = nodes[i];
        Node *node = node_path.get_type() == Variant::STRING ? _graph->get_node<Node>(NodePath(String(node_path))) : Object::cast_to<Node>(node_path);
        OghamGraphNode *ogn = Object::cast_to<OghamGraphNode>(node);
        if (ogn != nullptr)
        {
            Ref<OghamDocument> doc = _all_story_data.get(ogn->get_owner_path(), Variant());
            if (doc.is_valid())
                doc->remove_entry(ogn->get_entry_data().get("tag", ""));
        }
    }
    _rebuild_graph();
}

void OghamGraphView::_on_node_renamed(OghamGraphNode *node, String new_tag)
{
    String old_tag = node->get_entry_data().get("tag", "");
    if (new_tag.is_empty() || new_tag == old_tag)
        return;
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    // Only fixes up option targets within the same file — see
    // OghamGraphView.gd's identically-worded comment (known limitation, not
    // an oversight).
    doc->rename_entry(old_tag, new_tag);
    _rebuild_graph();
}

void OghamGraphView::_on_node_fork_toggled(OghamGraphNode *node, bool is_fork)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    doc->set_fork_mode(node->get_entry_data().get("tag", ""), is_fork);
    _refresh_node(node->get_entry_data().get("tag", "")); // only this node's own titlebar tint/label changes
}

PackedStringArray OghamGraphView::_all_entry_tags()
{
    PackedStringArray tags;
    Array paths = _all_story_data.keys();
    for (int p = 0; p < paths.size(); p++)
    {
        Ref<OghamDocument> doc = _all_story_data[paths[p]];
        Array entries = doc->get_entries();
        for (int i = 0; i < entries.size(); i++)
        {
            Dictionary e = entries[i];
            tags.push_back(e.get("tag", ""));
        }
    }
    return tags;
}

Vector2i OghamGraphView::_mouse_screen_pos()
{
    return DisplayServer::get_singleton()->mouse_get_position();
}

void OghamGraphView::_on_key_labels_pressed()
{
    OghamKeyLabelsPopup *popup = memnew(OghamKeyLabelsPopup);
    add_child(popup);
    popup->open(_mouse_screen_pos(), callable_mp(this, &OghamGraphView::_rebuild_graph).bind(false));
}

void OghamGraphView::_on_content_key_clicked(OghamGraphNode *node, int index)
{
    Array content_keys = node->get_entry_data().get("contentKeys", Array());
    if (index < 0 || index >= content_keys.size())
        return;
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    Dictionary key_copy = Dictionary(content_keys[index]).duplicate(true);
    OghamContentKeyPopup *popup = memnew(OghamContentKeyPopup);
    add_child(popup);
    popup->open(key_copy, _mouse_screen_pos(), callable_mp(this, &OghamGraphView::_on_content_key_committed).bind(doc, tag, index, key_copy));
}

void OghamGraphView::_on_entry_operation_clicked(OghamGraphNode *node, int index)
{
    Array ops = node->get_entry_data().get("entryOperations", Array());
    if (index < 0 || index >= ops.size())
        return;
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    Dictionary op_copy = Dictionary(ops[index]).duplicate(true);
    OghamOperationPopup *popup = memnew(OghamOperationPopup);
    add_child(popup);
    popup->open(op_copy, _mouse_screen_pos(), callable_mp(this, &OghamGraphView::_on_entry_operation_committed).bind(doc, tag, index, op_copy));
}

void OghamGraphView::_on_content_key_delete_requested(OghamGraphNode *node, int index)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    doc->remove_content_key(tag, index);
    _refresh_node(tag);
}

void OghamGraphView::_on_content_key_move_requested(OghamGraphNode *node, int index, int delta)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    doc->move_content_key(tag, index, delta);
    _refresh_node(tag);
}

void OghamGraphView::_on_add_content_key_requested(OghamGraphNode *node)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    Dictionary ck;
    ck["type"] = "Text";
    ck["mode"] = "Literal";
    ck["key"] = "";
    doc->add_content_key(tag, ck);
    _refresh_node(tag);
}

void OghamGraphView::_on_entry_operation_delete_requested(OghamGraphNode *node, int index)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    doc->remove_entry_operation(tag, index);
    _refresh_node(tag);
}

void OghamGraphView::_on_entry_operation_move_requested(OghamGraphNode *node, int index, int delta)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    doc->move_entry_operation(tag, index, delta);
    _refresh_node(tag);
}

void OghamGraphView::_on_add_entry_operation_requested(OghamGraphNode *node)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    doc->add_entry_operation(tag, "");
    _refresh_node(tag);
}

void OghamGraphView::_on_section_toggle_requested(OghamGraphNode *node, String section)
{
    String tag = node->get_entry_data().get("tag", "");
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    bool current = doc->get_section_expanded(tag, section);
    doc->set_section_expanded(tag, section, !current);
    // Folding "choices" changes which options currently have a port at all,
    // so wires need re-syncing along with the visual.
    _refresh_node(tag, callable_mp(this, &OghamGraphView::_sync_connections));
}

void OghamGraphView::_on_option_clicked(OghamGraphNode *node, String option_tag)
{
    Array options = node->get_entry_data().get("options", Array());
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String entry_tag = node->get_entry_data().get("tag", "");
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary option = options[i];
        if (String(option.get("tag", "")) == option_tag)
        {
            String old_target = option.get("targetTag", "");
            Dictionary option_copy = option.duplicate(true);
            OghamOptionPopup *popup = memnew(OghamOptionPopup);
            add_child(popup);
            popup->open(option_copy, _all_entry_tags(), _mouse_screen_pos(),
                        callable_mp(this, &OghamGraphView::_on_option_committed).bind(doc, entry_tag, option_tag, option_copy, old_target));
            return;
        }
    }
}

void OghamGraphView::_on_option_delete_requested(OghamGraphNode *node, String option_tag)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String old_target;
    Array options = node->get_entry_data().get("options", Array());
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary option = options[i];
        if (String(option.get("tag", "")) == option_tag)
        {
            old_target = option.get("targetTag", "");
            break;
        }
    }
    String tag = node->get_entry_data().get("tag", "");
    doc->remove_option(tag, option_tag);
    _refresh_node(tag, callable_mp(this, &OghamGraphView::_on_option_delete_done).bind(old_target));
}

void OghamGraphView::_on_option_move_requested(OghamGraphNode *node, int index, int delta)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "");
    doc->move_option(tag, index, delta);
    // Reordering shifts compacted port indices, so existing GraphEdit
    // connections (keyed by port number) need re-syncing.
    _refresh_node(tag, callable_mp(this, &OghamGraphView::_sync_connections));
}

void OghamGraphView::_on_add_option_requested(OghamGraphNode *node)
{
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;
    String tag = node->get_entry_data().get("tag", "Entry");
    Array options = node->get_entry_data().get("options", Array());
    int index = options.size() + 1;
    doc->add_option(tag, String("{0}.Option{1}").format(Array::make(tag, index)), "", "");
    _refresh_node(tag);
}

void OghamGraphView::_on_node_context_menu_requested(OghamGraphNode *node)
{
    String tag = node->get_entry_data().get("tag", "");
    Array options = node->get_entry_data().get("options", Array());
    Ref<OghamDocument> doc = _all_story_data.get(node->get_owner_path(), Variant());
    if (doc.is_null())
        return;

    PopupMenu *menu = memnew(PopupMenu);
    add_child(menu);
    Dictionary id_actions;
    int next_id = 0;

    Array non_loopback_options;
    for (int i = 0; i < options.size(); i++)
    {
        Dictionary o = options[i];
        String o_tag = o.get("tag", "");
        String o_target = o.get("targetTag", "");
        if (!o_tag.is_empty() && o_target != tag)
            non_loopback_options.push_back(o);
    }
    if (!non_loopback_options.is_empty())
    {
        PopupMenu *tab_menu = memnew(PopupMenu);
        menu->add_child(tab_menu);
        for (int i = 0; i < non_loopback_options.size(); i++)
        {
            Dictionary option = non_loopback_options[i];
            String opt_tag = option.get("tag", "");
            bool is_tab = doc->is_tab_flag(tag, opt_tag);
            String label_text = option.get("textKey", "");
            if (label_text.is_empty())
                label_text = opt_tag;
            int item_id = next_id++;
            tab_menu->add_check_item(label_text, item_id);
            tab_menu->set_item_checked(tab_menu->get_item_index(item_id), is_tab);
            Callable action = callable_mp(this, &OghamGraphView::_menu_action_toggle_tab).bind(doc, tag, opt_tag, is_tab);
            id_actions[item_id] = action;
        }
        menu->add_submenu_node_item("Show as Tab", tab_menu);
        tab_menu->connect("id_pressed", callable_mp(this, &OghamGraphView::_run_menu_action).bind(id_actions, menu));
    }

    PopupMenu *hl_menu = memnew(PopupMenu);
    menu->add_child(hl_menu);
    Color current_hl = doc->get_highlight_color(tag);
    for (int i = 0; i < 6; i++)
    {
        String hl_label = HIGHLIGHT_PRESETS[i].label;
        Color hl_color = HIGHLIGHT_PRESETS[i].color;
        int item_id = next_id++;
        hl_menu->add_check_item(hl_label, item_id);
        hl_menu->set_item_checked(hl_menu->get_item_index(item_id), current_hl.is_equal_approx(hl_color));
        Callable action = callable_mp(this, &OghamGraphView::_menu_action_set_highlight).bind(doc, tag, hl_color);
        id_actions[item_id] = action;
    }
    menu->add_submenu_node_item("Highlight", hl_menu);
    hl_menu->connect("id_pressed", callable_mp(this, &OghamGraphView::_run_menu_action).bind(id_actions, menu));

    menu->add_separator();
    int labels_id = next_id++;
    menu->add_item("Labels...", labels_id);
    id_actions[labels_id] = callable_mp(this, &OghamGraphView::_menu_action_open_labels).bind(doc, tag);

    menu->add_separator();
    int delete_id = next_id++;
    menu->add_item("Delete Node", delete_id);
    id_actions[delete_id] = callable_mp(this, &OghamGraphView::_menu_action_delete_node).bind(doc, tag);

    menu->connect("id_pressed", callable_mp(this, &OghamGraphView::_run_menu_action).bind(id_actions, menu));
    menu->popup(Rect2i(_mouse_screen_pos(), Vector2i()));
}

void OghamGraphView::_run_menu_action(int pressed_id, Dictionary id_actions, PopupMenu *menu)
{
    if (id_actions.has(pressed_id))
    {
        Callable action = id_actions[pressed_id];
        action.call();
    }
    menu->queue_free();
}

// ============================== deferred continuations for the handlers above ==============================

void OghamGraphView::_on_connection_request_done(String old_target, String to_tag)
{
    if (!old_target.is_empty() && old_target != to_tag)
        _refresh_node_by_tag(old_target); // its incoming-pin state may have just gone hollow
    _sync_connections();
}

void OghamGraphView::_on_disconnection_request_done(String old_target)
{
    if (!old_target.is_empty())
        _refresh_node_by_tag(old_target);
    _sync_connections();
}

void OghamGraphView::_on_content_key_committed(Ref<OghamDocument> doc, String tag, int index, Dictionary key_copy)
{
    doc->update_content_key(tag, index, key_copy);
    _refresh_node(tag);
}

void OghamGraphView::_on_entry_operation_committed(Ref<OghamDocument> doc, String tag, int index, Dictionary op_copy)
{
    doc->update_entry_operation(tag, index, op_copy);
    _refresh_node(tag);
}

void OghamGraphView::_on_option_committed(Ref<OghamDocument> doc, String entry_tag, String option_tag, Dictionary option_copy, String old_target)
{
    doc->update_option(entry_tag, option_tag, option_copy);
    String new_target = option_copy.get("targetTag", "");
    _refresh_node_and_targets(entry_tag, callable_mp(this, &OghamGraphView::_on_connection_request_done).bind(old_target, new_target));
}

void OghamGraphView::_on_option_delete_done(String old_target)
{
    if (!old_target.is_empty())
        _refresh_node_by_tag(old_target);
    _sync_connections();
}

void OghamGraphView::_menu_action_toggle_tab(Ref<OghamDocument> doc, String tag, String opt_tag, bool is_tab)
{
    doc->set_tab_flag(tag, opt_tag, !is_tab);
    // Toggling Tab mode swaps a wire for a flag (or back) — this node's own
    // row AND the target's border (tab_incoming_by_target) both need
    // refreshing, plus the wire itself.
    _refresh_node_and_targets(tag, callable_mp(this, &OghamGraphView::_sync_connections));
}

void OghamGraphView::_menu_action_set_highlight(Ref<OghamDocument> doc, String tag, Color hl_color)
{
    doc->set_highlight_color(tag, hl_color);
    _refresh_node(tag); // own border only — see OghamGraphView.gd's identically-worded comment
}

void OghamGraphView::_menu_action_open_labels(Ref<OghamDocument> doc, String tag)
{
    OghamLabelPickerPopup *popup = memnew(OghamLabelPickerPopup);
    add_child(popup);
    popup->open(doc, tag, _mouse_screen_pos(), callable_mp(this, &OghamGraphView::_refresh_node_by_tag).bind(tag));
}

void OghamGraphView::_menu_action_delete_node(Ref<OghamDocument> doc, String tag)
{
    doc->remove_entry(tag);
    _rebuild_graph();
}

void OghamGraphView::_bind_methods()
{
    // No GDScript-visible API surface needed — OghamEditorPlugin.gd only
    // ever does OghamGraphView.new() and add_child()s it; every interaction
    // from that point on is internal signal wiring set up by this class
    // itself in _ensure_built(). _ready() is picked up automatically as a
    // Control virtual override, the same as OghamGraphNode's _gui_input.
}
