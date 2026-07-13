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

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include "editor/OghamEditorTypes.h"

using namespace godot;

/// <summary>
/// The one Godot-exposed handle onto a live, in-memory .ogham document — the only
/// reason this is a RefCounted GDCLASS at all is that GDScript (OghamGraphView, the
/// popups) needs an object it can hold in a variable and pass around, exactly like
/// FileAccess/JSON/Image already work. Everything it actually owns internally is the
/// plain C++ OghamEditorDoc from OghamEditorTypes.h — no per-field GDCLASS wrapping.
///
/// Every accessor here returns/takes Dictionary/Array/Rect2/Color shaped IDENTICALLY
/// to what OghamManifestIO.gd's equivalent function used to hand back, so
/// OghamGraphNode.gd (which only ever reads an entry snapshot to render, never touches
/// this class directly) needs no changes at all — only OghamGraphView.gd and the
/// popups, which drove OghamManifestIO.gd's static functions directly, get updated to
/// call these instance methods instead.
///
/// Replaces OghamManifestIO.gd's per-entry/per-document accessor functions (the static
/// entry point that PRODUCES an OghamDocument — load_manifest/find_ogham_files — is
/// OghamManifestIO.h, the C++ class of the same name).
/// </summary>
class OghamDocument : public RefCounted
{
    GDCLASS(OghamDocument, RefCounted);

private:
    OghamEditorDoc doc;

    OghamEditorEntry *find_entry_mut(const String &tag);

public:
    OghamDocument() = default;

    // -- construction / persistence --
    void load_from_json(const String &json_text);
    Error save(const String &path);

    // -- entries --
    Array get_entries() const;
    Dictionary find_entry(const String &tag) const;
    Dictionary add_entry(const String &tag);
    void remove_entry(const String &tag);
    void rename_entry(const String &old_tag, const String &new_tag);

    // -- options (entry-scoped) --
    Dictionary add_option(const String &entry_tag, const String &option_tag, const String &target_tag, const String &text_key);
    void remove_option(const String &entry_tag, const String &option_tag);
    void set_option_target(const String &entry_tag, const String &option_tag, const String &target_tag);
    void move_option(const String &entry_tag, int index, int delta);
    // Wholesale replace — for editors (OghamOptionPopup) that mutate a local copy
    // of the whole option's fields (including tag rename, mode, conditions,
    // operations — everything this layer keeps opaque in `extra`) and commit
    // once at the end, rather than one field at a time.
    void update_option(const String &entry_tag, const String &original_option_tag, const Dictionary &full_option);

    // -- entryOperations ("On Enter", entry-scoped) --
    Dictionary add_entry_operation(const String &entry_tag, const String &tag);
    void remove_entry_operation(const String &entry_tag, int index);
    void move_entry_operation(const String &entry_tag, int index, int delta);
    void update_entry_operation(const String &entry_tag, int index, const Dictionary &full_op);

    // -- contentKeys (entry-scoped; contents themselves are opaque Dictionaries —
    // this layer only ever adds/removes/reorders/replaces whole items, never
    // reads a single field of one) --
    Dictionary add_content_key(const String &entry_tag, const Dictionary &content_key);
    void remove_content_key(const String &entry_tag, int index);
    void move_content_key(const String &entry_tag, int index, int delta);
    void update_content_key(const String &entry_tag, int index, const Dictionary &content_key);

    // -- fork/content mode (the "nodeMode" field, otherwise opaque passthrough) --
    bool get_fork_mode(const String &entry_tag) const;
    void set_fork_mode(const String &entry_tag, bool is_fork);

    // -- _editor.nodes[] metadata --
    Dictionary get_editor_node(const String &tag) const;
    Rect2 get_node_rect(const String &tag) const;
    void set_node_rect(const String &tag, const Rect2 &rect);
    bool get_section_expanded(const String &tag, const String &section) const;
    void set_section_expanded(const String &tag, const String &section, bool expanded);
    Color get_highlight_color(const String &tag) const;
    void set_highlight_color(const String &tag, const Color &color);
    Array get_tab_flag_options(const String &tag) const;
    bool is_tab_flag(const String &tag, const String &option_tag) const;
    void set_tab_flag(const String &tag, const String &option_tag, bool is_tab);

    // -- _editor.headerColor (per-file) --
    Color get_header_color() const;
    void set_header_color(const Color &color);

    // -- _editor.labels + per-node assignedLabelIds --
    Array get_labels() const;
    Dictionary find_label(int id) const;
    Dictionary add_label(const String &name, const Color &color);
    void remove_label(int id);
    Dictionary update_label(int id, const String &name, const Color &color);
    Array get_assigned_label_ids(const String &tag) const;
    void set_label_assigned(const String &tag, int id, bool assigned);

protected:
    static void _bind_methods();
};
