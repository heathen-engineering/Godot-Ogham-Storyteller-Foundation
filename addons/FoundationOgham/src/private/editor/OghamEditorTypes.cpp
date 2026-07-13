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

#include "editor/OghamEditorTypes.h"

#include <algorithm>

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace
{
    std::string to_std(const String &s)
    {
        return std::string(s.utf8().get_data());
    }

    String to_godot(const std::string &s)
    {
        return String::utf8(s.c_str());
    }

    // Copies every key from 'src' into 'dst' except the ones in 'skip' — used to
    // capture "everything this layer doesn't model" into an item's extra bag.
    Dictionary extract_extra(const Dictionary &src, const std::vector<std::string> &skip)
    {
        Dictionary extra;
        Array keys = src.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            const Variant &key = keys[i];
            if (key.get_type() != Variant::STRING)
                continue;
            const std::string key_std = to_std(String(key));
            bool skipped = false;
            for (const std::string &s : skip)
            {
                if (s == key_std)
                {
                    skipped = true;
                    break;
                }
            }
            if (!skipped)
                extra[key] = src[key];
        }
        return extra;
    }

    OghamEditorNodeMeta node_meta_from_dict(const Dictionary &d)
    {
        OghamEditorNodeMeta meta;
        if (d.is_empty())
            return meta;
        meta.exists = true;

        Array pos = d.get("position", Array());
        if (pos.size() >= 4)
            meta.rect = Rect2(pos[0], pos[1], pos[2], pos[3]);

        meta.ops_expanded = d.get("opsExpanded", true);
        meta.fields_expanded = d.get("fieldsExpanded", true);
        meta.choices_expanded = d.get("choicesExpanded", true);
        meta.highlight_color_hex = to_std(String(d.get("highlightColor", "")));

        Array tab_flags = d.get("tabFlagOptions", Array());
        for (int i = 0; i < tab_flags.size(); i++)
            meta.tab_flag_options.push_back(to_std(String(tab_flags[i])));

        Array label_ids = d.get("assignedLabelIds", Array());
        for (int i = 0; i < label_ids.size(); i++)
            meta.assigned_label_ids.push_back(int(label_ids[i]));

        meta.extra = extract_extra(d, {"tag", "position", "opsExpanded", "fieldsExpanded",
                                        "choicesExpanded", "highlightColor", "tabFlagOptions",
                                        "assignedLabelIds"});
        return meta;
    }

    // 'tag' is passed separately since the caller (entry loop) already knows it and
    // node records are looked up/rebuilt by tag, not carried redundantly per-field.
    Dictionary node_meta_to_dict(const std::string &tag, const OghamEditorNodeMeta &meta)
    {
        Dictionary d;
        d["tag"] = to_godot(tag);
        Array pos;
        pos.push_back(meta.rect.position.x);
        pos.push_back(meta.rect.position.y);
        pos.push_back(meta.rect.size.x);
        pos.push_back(meta.rect.size.y);
        d["position"] = pos;
        if (!meta.ops_expanded)
            d["opsExpanded"] = false;
        if (!meta.fields_expanded)
            d["fieldsExpanded"] = false;
        if (!meta.choices_expanded)
            d["choicesExpanded"] = false;
        if (!meta.highlight_color_hex.empty())
            d["highlightColor"] = to_godot(meta.highlight_color_hex);
        if (!meta.tab_flag_options.empty())
        {
            Array arr;
            for (const std::string &s : meta.tab_flag_options)
                arr.push_back(to_godot(s));
            d["tabFlagOptions"] = arr;
        }
        if (!meta.assigned_label_ids.empty())
        {
            Array arr;
            for (int id : meta.assigned_label_ids)
                arr.push_back(id);
            d["assignedLabelIds"] = arr;
        }
        Array keys = meta.extra.keys();
        for (int i = 0; i < keys.size(); i++)
            d[keys[i]] = meta.extra[keys[i]];
        return d;
    }
}

OghamEditorOption ogham_option_from_dict(const Dictionary &d)
{
    OghamEditorOption opt;
    opt.tag = to_std(String(d.get("tag", "")));
    opt.target_tag = to_std(String(d.get("targetTag", "")));
    opt.text_key = to_std(String(d.get("textKey", "")));
    opt.extra = extract_extra(d, {"tag", "targetTag", "textKey"});
    return opt;
}

Dictionary ogham_option_to_dict(const OghamEditorOption &opt)
{
    Dictionary d;
    d["tag"] = to_godot(opt.tag);
    d["targetTag"] = to_godot(opt.target_tag);
    d["textKey"] = to_godot(opt.text_key);
    Array keys = opt.extra.keys();
    for (int i = 0; i < keys.size(); i++)
        d[keys[i]] = opt.extra[keys[i]];
    return d;
}

OghamEditorEntryOperation ogham_entry_operation_from_dict(const Dictionary &d)
{
    OghamEditorEntryOperation op;
    op.tag = to_std(String(d.get("tag", "")));
    op.arithmetic = to_std(String(d.get("arithmetic", "Set")));
    op.value = d.get("value", 0);
    return op;
}

Dictionary ogham_entry_operation_to_dict(const OghamEditorEntryOperation &op)
{
    Dictionary d;
    d["tag"] = to_godot(op.tag);
    d["arithmetic"] = to_godot(op.arithmetic);
    d["value"] = op.value;
    return d;
}

OghamEditorEntry ogham_entry_from_dict(const Dictionary &d)
{
    OghamEditorEntry entry;
    entry.tag = to_std(String(d.get("tag", "")));

    entry.has_content_keys_key = d.has("contentKeys");
    entry.content_keys = d.get("contentKeys", Array());

    entry.has_entry_operations_key = d.has("entryOperations");
    Array ops = d.get("entryOperations", Array());
    for (int i = 0; i < ops.size(); i++)
        entry.entry_operations.push_back(ogham_entry_operation_from_dict(Dictionary(ops[i])));

    entry.has_options_key = d.has("options");
    Array opts = d.get("options", Array());
    for (int i = 0; i < opts.size(); i++)
        entry.options.push_back(ogham_option_from_dict(Dictionary(opts[i])));

    entry.extra = extract_extra(d, {"tag", "contentKeys", "entryOperations", "options"});
    return entry;
}

Dictionary ogham_entry_to_dict(const OghamEditorEntry &entry)
{
    Dictionary d;
    d["tag"] = to_godot(entry.tag);

    if (entry.has_content_keys_key || (entry.content_keys.get_type() == Variant::ARRAY && Array(entry.content_keys).size() > 0))
        d["contentKeys"] = entry.content_keys;

    if (entry.has_entry_operations_key || !entry.entry_operations.empty())
    {
        Array ops;
        for (const OghamEditorEntryOperation &op : entry.entry_operations)
            ops.push_back(ogham_entry_operation_to_dict(op));
        d["entryOperations"] = ops;
    }

    if (entry.has_options_key || !entry.options.empty())
    {
        Array opts;
        for (const OghamEditorOption &opt : entry.options)
            opts.push_back(ogham_option_to_dict(opt));
        d["options"] = opts;
    }

    Array keys = entry.extra.keys();
    for (int i = 0; i < keys.size(); i++)
        d[keys[i]] = entry.extra[keys[i]];
    return d;
}

OghamEditorLabelDef ogham_label_from_dict(const Dictionary &d)
{
    OghamEditorLabelDef label;
    label.id = int(d.get("id", -1));
    label.name = to_std(String(d.get("name", "")));
    label.color_hex = to_std(String(d.get("color", "")));
    return label;
}

Dictionary ogham_label_to_dict(const OghamEditorLabelDef &label)
{
    Dictionary d;
    d["id"] = label.id;
    d["name"] = to_godot(label.name);
    d["color"] = to_godot(label.color_hex);
    return d;
}

void ogham_editor_doc_reindex(OghamEditorDoc &doc)
{
    doc.tag_index.clear();
    for (size_t i = 0; i < doc.entries.size(); i++)
        doc.tag_index[doc.entries[i].tag] = i;
}

OghamEditorDoc ogham_editor_doc_from_json(const String &json_text)
{
    OghamEditorDoc doc;

    Variant parsed = JSON::parse_string(json_text);
    if (parsed.get_type() != Variant::DICTIONARY)
        return doc;
    Dictionary root = parsed;

    Array entries = root.get("entries", Array());
    for (int i = 0; i < entries.size(); i++)
        doc.entries.push_back(ogham_entry_from_dict(Dictionary(entries[i])));

    doc.has_editor_key = root.has("_editor");
    Dictionary editor = root.get("_editor", Dictionary());

    doc.header_color_hex = to_std(String(editor.get("headerColor", "")));

    doc.has_labels_key = editor.has("labels");
    Array labels = editor.get("labels", Array());
    for (int i = 0; i < labels.size(); i++)
        doc.labels.push_back(ogham_label_from_dict(Dictionary(labels[i])));

    // Attach each entry's _editor.nodes[] record by matching tag — kept as a
    // per-entry field (not a parallel array) so entry/node-meta can never drift
    // out of sync with each other.
    Array nodes = editor.get("nodes", Array());
    for (int i = 0; i < nodes.size(); i++)
    {
        Dictionary node = nodes[i];
        std::string tag = to_std(String(node.get("tag", "")));
        auto it = std::find_if(doc.entries.begin(), doc.entries.end(),
                                [&](const OghamEditorEntry &e)
                                { return e.tag == tag; });
        if (it != doc.entries.end())
            it->node_meta = node_meta_from_dict(node);
    }

    doc.editor_extra = extract_extra(editor, {"headerColor", "labels", "nodes"});
    doc.extra = extract_extra(root, {"entries", "_editor"});

    ogham_editor_doc_reindex(doc);
    return doc;
}

String ogham_editor_doc_to_json(const OghamEditorDoc &doc)
{
    Dictionary root;

    Array entries;
    for (const OghamEditorEntry &entry : doc.entries)
        entries.push_back(ogham_entry_to_dict(entry));
    root["entries"] = entries;

    if (doc.has_editor_key || !doc.header_color_hex.empty() || doc.has_labels_key)
    {
        Dictionary editor;
        if (!doc.header_color_hex.empty())
            editor["headerColor"] = to_godot(doc.header_color_hex);

        if (doc.has_labels_key || !doc.labels.empty())
        {
            Array labels;
            for (const OghamEditorLabelDef &label : doc.labels)
                labels.push_back(ogham_label_to_dict(label));
            editor["labels"] = labels;
        }

        Array nodes;
        for (const OghamEditorEntry &entry : doc.entries)
        {
            if (entry.node_meta.exists)
                nodes.push_back(node_meta_to_dict(entry.tag, entry.node_meta));
        }
        if (nodes.size() > 0)
            editor["nodes"] = nodes;

        Array extra_keys = doc.editor_extra.keys();
        for (int i = 0; i < extra_keys.size(); i++)
            editor[extra_keys[i]] = doc.editor_extra[extra_keys[i]];

        root["_editor"] = editor;
    }

    Array extra_keys = doc.extra.keys();
    for (int i = 0; i < extra_keys.size(); i++)
        root[extra_keys[i]] = doc.extra[extra_keys[i]];

    return JSON::stringify(root, "  ");
}
