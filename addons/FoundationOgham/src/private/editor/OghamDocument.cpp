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

#include "editor/OghamDocument.h"

#include <algorithm>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
    std::string to_std(const String &s) { return std::string(s.utf8().get_data()); }
    String to_godot(const std::string &s) { return String::utf8(s.c_str()); }

    // Same _editor.nodes[] field-name mapping as OghamManifestIO.gd's SECTION_FIELD_NAMES.
    std::string section_field_name(const String &section)
    {
        const std::string s = to_std(section);
        if (s == "ops")
            return "opsExpanded";
        if (s == "fields")
            return "fieldsExpanded";
        if (s == "choices")
            return "choicesExpanded";
        return "";
    }

    Color color_from_hex(const std::string &hex)
    {
        if (hex.empty())
            return Color(0, 0, 0, 0);
        return Color::html(to_godot(hex));
    }

    std::string hex_from_color(const Color &color)
    {
        return to_std(String("#") + color.to_html(true));
    }

    template <typename T>
    void move_in_vector(std::vector<T> &vec, int index, int delta)
    {
        const int new_index = index + delta;
        if (index < 0 || index >= int(vec.size()) || new_index < 0 || new_index >= int(vec.size()))
            return;
        std::swap(vec[index], vec[new_index]);
    }
}

OghamEditorEntry *OghamDocument::find_entry_mut(const String &tag)
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return nullptr;
    return &doc.entries[it->second];
}

void OghamDocument::load_from_json(const String &json_text)
{
    doc = ogham_editor_doc_from_json(json_text);
}

Error OghamDocument::save(const String &path)
{
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    if (file.is_null())
        return ERR_CANT_OPEN;
    file->store_string(ogham_editor_doc_to_json(doc));
    return OK;
}

Array OghamDocument::get_entries() const
{
    Array result;
    for (const OghamEditorEntry &entry : doc.entries)
        result.push_back(find_entry(to_godot(entry.tag)));
    return result;
}

Dictionary OghamDocument::find_entry(const String &tag) const
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return Dictionary();
    return ogham_entry_to_dict(doc.entries[it->second]);
}

Dictionary OghamDocument::add_entry(const String &tag)
{
    OghamEditorEntry entry;
    entry.tag = to_std(tag);
    entry.has_content_keys_key = true;
    entry.has_options_key = true;
    doc.entries.push_back(entry);
    ogham_editor_doc_reindex(doc);
    return find_entry(tag);
}

void OghamDocument::remove_entry(const String &tag)
{
    const std::string tag_std = to_std(tag);
    doc.entries.erase(std::remove_if(doc.entries.begin(), doc.entries.end(),
                                      [&](const OghamEditorEntry &e)
                                      { return e.tag == tag_std; }),
                       doc.entries.end());
    ogham_editor_doc_reindex(doc);
}

void OghamDocument::rename_entry(const String &old_tag, const String &new_tag)
{
    OghamEditorEntry *entry = find_entry_mut(old_tag);
    if (entry == nullptr)
        return;
    entry->tag = to_std(new_tag);

    const std::string old_std = to_std(old_tag);
    const std::string new_std = to_std(new_tag);
    for (OghamEditorEntry &other : doc.entries)
    {
        for (OghamEditorOption &option : other.options)
        {
            if (option.target_tag == old_std)
                option.target_tag = new_std;
        }
    }
    ogham_editor_doc_reindex(doc);
}

bool OghamDocument::get_fork_mode(const String &tag) const
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return false;
    return String(doc.entries[it->second].extra.get("nodeMode", "")) == "Fork";
}

void OghamDocument::set_fork_mode(const String &tag, bool is_fork)
{
    OghamEditorEntry *entry = find_entry_mut(tag);
    if (entry == nullptr)
        return;
    if (is_fork)
        entry->extra["nodeMode"] = String("Fork");
    else
        entry->extra.erase("nodeMode");
}

Dictionary OghamDocument::add_option(const String &entry_tag, const String &option_tag, const String &target_tag, const String &text_key)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return Dictionary();
    OghamEditorOption opt;
    opt.tag = to_std(option_tag);
    opt.target_tag = to_std(target_tag);
    opt.text_key = to_std(text_key);
    entry->options.push_back(opt);
    return ogham_option_to_dict(opt);
}

void OghamDocument::remove_option(const String &entry_tag, const String &option_tag)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    const std::string tag_std = to_std(option_tag);
    entry->options.erase(std::remove_if(entry->options.begin(), entry->options.end(),
                                         [&](const OghamEditorOption &o)
                                         { return o.tag == tag_std; }),
                          entry->options.end());
}

void OghamDocument::set_option_target(const String &entry_tag, const String &option_tag, const String &target_tag)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    const std::string tag_std = to_std(option_tag);
    for (OghamEditorOption &opt : entry->options)
    {
        if (opt.tag == tag_std)
        {
            opt.target_tag = to_std(target_tag);
            return;
        }
    }
}

void OghamDocument::move_option(const String &entry_tag, int index, int delta)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    move_in_vector(entry->options, index, delta);
}

void OghamDocument::update_option(const String &entry_tag, const String &original_option_tag, const Dictionary &full_option)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    const std::string tag_std = to_std(original_option_tag);
    for (OghamEditorOption &opt : entry->options)
    {
        if (opt.tag == tag_std)
        {
            opt = ogham_option_from_dict(full_option);
            return;
        }
    }
}

Dictionary OghamDocument::add_entry_operation(const String &entry_tag, const String &tag)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return Dictionary();
    OghamEditorEntryOperation op;
    op.tag = to_std(tag);
    op.arithmetic = "Set";
    op.value = 0;
    entry->entry_operations.push_back(op);
    entry->has_entry_operations_key = true;
    return ogham_entry_operation_to_dict(op);
}

void OghamDocument::remove_entry_operation(const String &entry_tag, int index)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    if (index >= 0 && index < int(entry->entry_operations.size()))
        entry->entry_operations.erase(entry->entry_operations.begin() + index);
}

void OghamDocument::move_entry_operation(const String &entry_tag, int index, int delta)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    move_in_vector(entry->entry_operations, index, delta);
}

void OghamDocument::update_entry_operation(const String &entry_tag, int index, const Dictionary &full_op)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    if (index < 0 || index >= int(entry->entry_operations.size()))
        return;
    entry->entry_operations[index] = ogham_entry_operation_from_dict(full_op);
}

Dictionary OghamDocument::add_content_key(const String &entry_tag, const Dictionary &content_key)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return Dictionary();
    Array arr = entry->content_keys;
    // Array/Variant assignment in Godot shares the underlying buffer rather than
    // copying it, so 'arr' here IS entry->content_keys — append() mutates the
    // entry in place. entry->content_keys is reassigned anyway, explicitly,
    // rather than relying on that implicit aliasing (which breaks the moment
    // content_keys didn't already hold an Array, e.g. a freshly-added entry).
    arr.push_back(content_key);
    entry->content_keys = arr;
    entry->has_content_keys_key = true;
    return content_key;
}

void OghamDocument::remove_content_key(const String &entry_tag, int index)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    Array arr = entry->content_keys;
    if (index < 0 || index >= arr.size())
        return;
    arr.remove_at(index);
    entry->content_keys = arr;
}

void OghamDocument::update_content_key(const String &entry_tag, int index, const Dictionary &content_key)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    Array arr = entry->content_keys;
    if (index < 0 || index >= arr.size())
        return;
    arr[index] = content_key;
    entry->content_keys = arr;
}

void OghamDocument::move_content_key(const String &entry_tag, int index, int delta)
{
    OghamEditorEntry *entry = find_entry_mut(entry_tag);
    if (entry == nullptr)
        return;
    Array arr = entry->content_keys;
    const int new_index = index + delta;
    if (index < 0 || index >= arr.size() || new_index < 0 || new_index >= arr.size())
        return;
    Variant tmp = arr[index];
    arr[index] = arr[new_index];
    arr[new_index] = tmp;
    entry->content_keys = arr;
}

Dictionary OghamDocument::get_editor_node(const String &tag) const
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end() || !doc.entries[it->second].node_meta.exists)
        return Dictionary();
    const OghamEditorEntry &entry = doc.entries[it->second];
    Dictionary d;
    d["tag"] = tag;
    d["position"] = Array::make(entry.node_meta.rect.position.x, entry.node_meta.rect.position.y,
                                 entry.node_meta.rect.size.x, entry.node_meta.rect.size.y);
    d["opsExpanded"] = entry.node_meta.ops_expanded;
    d["fieldsExpanded"] = entry.node_meta.fields_expanded;
    d["choicesExpanded"] = entry.node_meta.choices_expanded;
    if (!entry.node_meta.highlight_color_hex.empty())
        d["highlightColor"] = to_godot(entry.node_meta.highlight_color_hex);
    return d;
}

Rect2 OghamDocument::get_node_rect(const String &tag) const
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return Rect2(0.0, 0.0, 520.0, 200.0);
    return doc.entries[it->second].node_meta.rect;
}

void OghamDocument::set_node_rect(const String &tag, const Rect2 &rect)
{
    OghamEditorEntry *entry = find_entry_mut(tag);
    if (entry == nullptr)
        return;
    entry->node_meta.exists = true;
    entry->node_meta.rect = rect;
}

bool OghamDocument::get_section_expanded(const String &tag, const String &section) const
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return true;
    const OghamEditorNodeMeta &meta = doc.entries[it->second].node_meta;
    const std::string field = section_field_name(section);
    if (field == "opsExpanded")
        return meta.ops_expanded;
    if (field == "fieldsExpanded")
        return meta.fields_expanded;
    if (field == "choicesExpanded")
        return meta.choices_expanded;
    return true;
}

void OghamDocument::set_section_expanded(const String &tag, const String &section, bool expanded)
{
    const std::string field = section_field_name(section);
    if (field.empty())
        return;
    OghamEditorEntry *entry = find_entry_mut(tag);
    if (entry == nullptr)
        return;
    entry->node_meta.exists = true;
    if (field == "opsExpanded")
        entry->node_meta.ops_expanded = expanded;
    else if (field == "fieldsExpanded")
        entry->node_meta.fields_expanded = expanded;
    else if (field == "choicesExpanded")
        entry->node_meta.choices_expanded = expanded;
}

Color OghamDocument::get_highlight_color(const String &tag) const
{
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return Color(0, 0, 0, 0);
    return color_from_hex(doc.entries[it->second].node_meta.highlight_color_hex);
}

void OghamDocument::set_highlight_color(const String &tag, const Color &color)
{
    OghamEditorEntry *entry = find_entry_mut(tag);
    if (entry == nullptr)
        return;
    entry->node_meta.exists = true;
    entry->node_meta.highlight_color_hex = hex_from_color(color);
}

Array OghamDocument::get_tab_flag_options(const String &tag) const
{
    Array result;
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return result;
    for (const std::string &s : doc.entries[it->second].node_meta.tab_flag_options)
        result.push_back(to_godot(s));
    return result;
}

bool OghamDocument::is_tab_flag(const String &tag, const String &option_tag) const
{
    const std::string opt_std = to_std(option_tag);
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return false;
    const std::vector<std::string> &list = doc.entries[it->second].node_meta.tab_flag_options;
    return std::find(list.begin(), list.end(), opt_std) != list.end();
}

void OghamDocument::set_tab_flag(const String &tag, const String &option_tag, bool is_tab)
{
    OghamEditorEntry *entry = find_entry_mut(tag);
    if (entry == nullptr)
        return;
    entry->node_meta.exists = true;
    std::vector<std::string> &list = entry->node_meta.tab_flag_options;
    const std::string opt_std = to_std(option_tag);
    auto found = std::find(list.begin(), list.end(), opt_std);
    if (is_tab)
    {
        if (found == list.end())
            list.push_back(opt_std);
    }
    else if (found != list.end())
    {
        list.erase(found);
    }
}

Color OghamDocument::get_header_color() const
{
    return color_from_hex(doc.header_color_hex);
}

void OghamDocument::set_header_color(const Color &color)
{
    doc.has_editor_key = true;
    doc.header_color_hex = hex_from_color(color);
}

Array OghamDocument::get_labels() const
{
    Array result;
    for (const OghamEditorLabelDef &label : doc.labels)
    {
        Dictionary d;
        d["id"] = label.id;
        d["name"] = to_godot(label.name);
        d["color"] = to_godot(label.color_hex);
        result.push_back(d);
    }
    return result;
}

Dictionary OghamDocument::find_label(int id) const
{
    for (const OghamEditorLabelDef &label : doc.labels)
    {
        if (label.id == id)
        {
            Dictionary d;
            d["id"] = label.id;
            d["name"] = to_godot(label.name);
            d["color"] = to_godot(label.color_hex);
            return d;
        }
    }
    return Dictionary();
}

Dictionary OghamDocument::add_label(const String &name, const Color &color)
{
    doc.has_editor_key = true;
    doc.has_labels_key = true;
    int next_id = 0;
    for (const OghamEditorLabelDef &label : doc.labels)
        next_id = std::max(next_id, label.id + 1);
    OghamEditorLabelDef def;
    def.id = next_id;
    def.name = to_std(name);
    def.color_hex = hex_from_color(color);
    doc.labels.push_back(def);
    return find_label(next_id);
}

void OghamDocument::remove_label(int id)
{
    doc.labels.erase(std::remove_if(doc.labels.begin(), doc.labels.end(),
                                     [&](const OghamEditorLabelDef &l)
                                     { return l.id == id; }),
                      doc.labels.end());
    for (OghamEditorEntry &entry : doc.entries)
    {
        std::vector<int> &ids = entry.node_meta.assigned_label_ids;
        ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    }
}

Dictionary OghamDocument::update_label(int id, const String &name, const Color &color)
{
    for (OghamEditorLabelDef &label : doc.labels)
    {
        if (label.id == id)
        {
            label.name = to_std(name);
            label.color_hex = hex_from_color(color);
            break;
        }
    }
    return find_label(id);
}

Array OghamDocument::get_assigned_label_ids(const String &tag) const
{
    Array result;
    const std::string tag_std = to_std(tag);
    auto it = doc.tag_index.find(tag_std);
    if (it == doc.tag_index.end())
        return result;
    for (int id : doc.entries[it->second].node_meta.assigned_label_ids)
        result.push_back(id);
    return result;
}

void OghamDocument::set_label_assigned(const String &tag, int id, bool assigned)
{
    OghamEditorEntry *entry = find_entry_mut(tag);
    if (entry == nullptr)
        return;
    entry->node_meta.exists = true;
    std::vector<int> &ids = entry->node_meta.assigned_label_ids;
    auto found = std::find(ids.begin(), ids.end(), id);
    if (assigned)
    {
        if (found == ids.end())
            ids.push_back(id);
    }
    else if (found != ids.end())
    {
        ids.erase(found);
    }
}

void OghamDocument::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("save", "path"), &OghamDocument::save);

    ClassDB::bind_method(D_METHOD("get_entries"), &OghamDocument::get_entries);
    ClassDB::bind_method(D_METHOD("find_entry", "tag"), &OghamDocument::find_entry);
    ClassDB::bind_method(D_METHOD("add_entry", "tag"), &OghamDocument::add_entry);
    ClassDB::bind_method(D_METHOD("remove_entry", "tag"), &OghamDocument::remove_entry);
    ClassDB::bind_method(D_METHOD("rename_entry", "old_tag", "new_tag"), &OghamDocument::rename_entry);
    ClassDB::bind_method(D_METHOD("get_fork_mode", "entry_tag"), &OghamDocument::get_fork_mode);
    ClassDB::bind_method(D_METHOD("set_fork_mode", "entry_tag", "is_fork"), &OghamDocument::set_fork_mode);

    ClassDB::bind_method(D_METHOD("add_option", "entry_tag", "option_tag", "target_tag", "text_key"), &OghamDocument::add_option, DEFVAL(String()), DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("remove_option", "entry_tag", "option_tag"), &OghamDocument::remove_option);
    ClassDB::bind_method(D_METHOD("set_option_target", "entry_tag", "option_tag", "target_tag"), &OghamDocument::set_option_target);
    ClassDB::bind_method(D_METHOD("move_option", "entry_tag", "index", "delta"), &OghamDocument::move_option);
    ClassDB::bind_method(D_METHOD("update_option", "entry_tag", "original_option_tag", "full_option"), &OghamDocument::update_option);

    ClassDB::bind_method(D_METHOD("add_entry_operation", "entry_tag", "tag"), &OghamDocument::add_entry_operation, DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("remove_entry_operation", "entry_tag", "index"), &OghamDocument::remove_entry_operation);
    ClassDB::bind_method(D_METHOD("move_entry_operation", "entry_tag", "index", "delta"), &OghamDocument::move_entry_operation);
    ClassDB::bind_method(D_METHOD("update_entry_operation", "entry_tag", "index", "full_op"), &OghamDocument::update_entry_operation);

    ClassDB::bind_method(D_METHOD("add_content_key", "entry_tag", "content_key"), &OghamDocument::add_content_key);
    ClassDB::bind_method(D_METHOD("remove_content_key", "entry_tag", "index"), &OghamDocument::remove_content_key);
    ClassDB::bind_method(D_METHOD("move_content_key", "entry_tag", "index", "delta"), &OghamDocument::move_content_key);
    ClassDB::bind_method(D_METHOD("update_content_key", "entry_tag", "index", "content_key"), &OghamDocument::update_content_key);

    ClassDB::bind_method(D_METHOD("get_editor_node", "tag"), &OghamDocument::get_editor_node);
    ClassDB::bind_method(D_METHOD("get_node_rect", "tag"), &OghamDocument::get_node_rect);
    ClassDB::bind_method(D_METHOD("set_node_rect", "tag", "rect"), &OghamDocument::set_node_rect);
    ClassDB::bind_method(D_METHOD("get_section_expanded", "tag", "section"), &OghamDocument::get_section_expanded);
    ClassDB::bind_method(D_METHOD("set_section_expanded", "tag", "section", "expanded"), &OghamDocument::set_section_expanded);
    ClassDB::bind_method(D_METHOD("get_highlight_color", "tag"), &OghamDocument::get_highlight_color);
    ClassDB::bind_method(D_METHOD("set_highlight_color", "tag", "color"), &OghamDocument::set_highlight_color);
    ClassDB::bind_method(D_METHOD("get_tab_flag_options", "tag"), &OghamDocument::get_tab_flag_options);
    ClassDB::bind_method(D_METHOD("is_tab_flag", "tag", "option_tag"), &OghamDocument::is_tab_flag);
    ClassDB::bind_method(D_METHOD("set_tab_flag", "tag", "option_tag", "is_tab"), &OghamDocument::set_tab_flag);

    ClassDB::bind_method(D_METHOD("get_header_color"), &OghamDocument::get_header_color);
    ClassDB::bind_method(D_METHOD("set_header_color", "color"), &OghamDocument::set_header_color);

    ClassDB::bind_method(D_METHOD("get_labels"), &OghamDocument::get_labels);
    ClassDB::bind_method(D_METHOD("find_label", "id"), &OghamDocument::find_label);
    ClassDB::bind_method(D_METHOD("add_label", "name", "color"), &OghamDocument::add_label);
    ClassDB::bind_method(D_METHOD("remove_label", "id"), &OghamDocument::remove_label);
    ClassDB::bind_method(D_METHOD("update_label", "id", "name", "color"), &OghamDocument::update_label);
    ClassDB::bind_method(D_METHOD("get_assigned_label_ids", "tag"), &OghamDocument::get_assigned_label_ids);
    ClassDB::bind_method(D_METHOD("set_label_assigned", "tag", "id", "assigned"), &OghamDocument::set_label_assigned);
}
