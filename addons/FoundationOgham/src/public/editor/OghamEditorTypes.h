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

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/variant.hpp>

/// <summary>
/// Plain C++ editor-side model of a parsed .ogham JSON document — deliberately NOT a
/// GDCLASS/Object hierarchy (see OghamDocument.h for the one thin RefCounted wrapper
/// GDScript actually needs to hold). Fields here are the ones OghamManifestIO's own
/// logic reads or writes (entry lookup by tag, _editor node metadata, labels, option
/// tag/target/text, entry-operation tag/arithmetic/value). Anything this layer never
/// inspects — content-key internals, an option's "conditions"/"operations" sub-language
/// (owned by FoundationGameplayTags), any authoring cruft — is preserved verbatim in an
/// `extra` Dictionary per item rather than modeled, so a load/edit/save cycle never
/// silently drops a field this editor doesn't actively understand. This mirrors
/// OghamManifestIO.gd's own stated design goal (see its header comment, ported below).
///
/// Ported from OghamManifestIO.gd (GDScript) as part of moving the Ogham Storyteller
/// editor's document-ownership layer to C++ — the tool's own logic should be compiled
/// and typed; Godot/GDScript is only handed what it needs to draw.
/// </summary>

using godot::Dictionary;
using godot::Rect2;
using godot::Variant;

struct OghamEditorOption
{
    std::string tag;
    std::string target_tag;
    std::string text_key;
    Dictionary extra; // mode, conditions, operations, and any other unmodeled fields
};

struct OghamEditorEntryOperation
{
    std::string tag;
    std::string arithmetic = "Set";
    Variant value = 0; // int or float in JSON; kept as Variant to avoid lossy coercion
};

/// One entry's `_editor.nodes[]` record — canvas position/size, per-section fold
/// state, optional highlight color, Tab-flag option tags, assigned label ids. Same
/// field names/shapes as Unity's OghamNodeMeta (see OghamManifestIO.gd comments).
struct OghamEditorNodeMeta
{
    bool exists = false; // whether this entry has ever gotten an _editor.nodes[] record
    Rect2 rect = Rect2(0.0, 0.0, 520.0, 200.0);
    bool ops_expanded = true;
    bool fields_expanded = true;
    bool choices_expanded = true;
    std::string highlight_color_hex; // empty = unset (Color(0,0,0,0) sentinel at the API boundary)
    std::vector<std::string> tab_flag_options;
    std::vector<int> assigned_label_ids;
    Dictionary extra; // labelColor/collapsed/directorNotes/... — anything else under this node
};

struct OghamEditorEntry
{
    std::string tag;
    bool has_entry_operations_key = false; // was "entryOperations" present in the source JSON at all
    std::vector<OghamEditorEntryOperation> entry_operations;
    bool has_content_keys_key = false; // Fork/routing entries legitimately omit this key entirely
    godot::Variant content_keys = godot::Array(); // opaque passthrough — ManifestIO only reorders these
    bool has_options_key = false;
    std::vector<OghamEditorOption> options;
    OghamEditorNodeMeta node_meta;
    Dictionary extra; // nodeMode, assetGuid, and any other unmodeled top-level entry fields
};

struct OghamEditorLabelDef
{
    int id = 0;
    std::string name;
    std::string color_hex;
};

struct OghamEditorDoc
{
    std::vector<OghamEditorEntry> entries;
    std::unordered_map<std::string, size_t> tag_index; // kept in sync by OghamDocument

    bool has_editor_key = false; // was "_editor" present at all
    std::string header_color_hex; // empty = unset
    bool has_labels_key = false;
    std::vector<OghamEditorLabelDef> labels;
    Dictionary editor_extra; // unmodeled _editor.* keys (not nodes/headerColor/labels)

    Dictionary extra; // unmodeled top-level document keys (e.g. storyTag)
};

/// Parses raw .ogham JSON text into a doc. Returns an empty doc (no entries) if the
/// text isn't valid JSON or isn't a JSON object at the top level — matches
/// OghamManifestIO.gd's load_manifest() falling back to {} in the same situations.
OghamEditorDoc ogham_editor_doc_from_json(const godot::String &json_text);

/// Serializes a doc back to .ogham JSON text, 2-space indented (matches
/// OghamManifestIO.gd's `JSON.stringify(data, "  ")`).
godot::String ogham_editor_doc_to_json(const OghamEditorDoc &doc);

/// Rebuilds tag_index from entries — call after any structural edit (add/remove/rename)
/// that doesn't go through OghamDocument's own bookkeeping helpers.
void ogham_editor_doc_reindex(OghamEditorDoc &doc);

// -- Single-item ⇄ Dictionary conversions, shared by the doc-level marshaling above
// and OghamDocument's per-call accessor snapshots, so there is exactly one place that
// knows each item's JSON shape (no risk of the two drifting out of sync). --

OghamEditorOption ogham_option_from_dict(const Dictionary &d);
Dictionary ogham_option_to_dict(const OghamEditorOption &opt);

OghamEditorEntryOperation ogham_entry_operation_from_dict(const Dictionary &d);
Dictionary ogham_entry_operation_to_dict(const OghamEditorEntryOperation &op);

OghamEditorEntry ogham_entry_from_dict(const Dictionary &d);
Dictionary ogham_entry_to_dict(const OghamEditorEntry &entry);

OghamEditorLabelDef ogham_label_from_dict(const Dictionary &d);
Dictionary ogham_label_to_dict(const OghamEditorLabelDef &label);
