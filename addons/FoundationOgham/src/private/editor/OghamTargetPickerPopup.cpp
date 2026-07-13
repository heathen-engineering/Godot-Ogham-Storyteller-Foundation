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

#include "editor/OghamTargetPickerPopup.h"

#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

namespace
{
    // Same trie-build algorithm as GameplayTagVocabulary::build_tree — not
    // shared cross-extension, see this class's header comment for why.
    Dictionary build_trie(const PackedStringArray &tags)
    {
        Dictionary root;
        for (int i = 0; i < tags.size(); i++)
        {
            Dictionary node = root;
            PackedStringArray segments = String(tags[i]).split(".");
            for (int s = 0; s < segments.size(); s++)
            {
                const String &segment = segments[s];
                if (!node.has(segment))
                    node[segment] = Dictionary();
                node = node[segment];
            }
        }
        return root;
    }

    // Same prune-in-place-by-filter algorithm as
    // GameplayTagPickerProperty.cpp's anonymous-namespace prune_tree.
    bool prune_trie(Dictionary &node, const String &path_prefix, const String &filter_lower)
    {
        Array keys = node.keys();
        Array keys_to_erase;
        bool any_kept = false;
        for (int i = 0; i < keys.size(); i++)
        {
            String key = keys[i];
            String full_path = path_prefix.is_empty() ? key : path_prefix + String(".") + key;
            Dictionary child = node[key];
            bool self_matches = full_path.to_lower().contains(filter_lower);
            bool child_kept = prune_trie(child, full_path, filter_lower);
            if (self_matches || child_kept)
                any_kept = true;
            else
                keys_to_erase.push_back(key);
        }
        for (int i = 0; i < keys_to_erase.size(); i++)
            node.erase(keys_to_erase[i]);
        return any_kept;
    }
}

void OghamTargetPickerPopup::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    VBoxContainer *vbox = memnew(VBoxContainer);
    add_child(vbox);

    filter_ = memnew(LineEdit);
    filter_->set_placeholder("Filter tags...");
    filter_->set_custom_minimum_size(Vector2(320, 0));
    filter_->connect("text_changed", callable_mp(this, &OghamTargetPickerPopup::_on_filter_changed));
    vbox->add_child(filter_);

    tree_ = memnew(Tree);
    tree_->set_custom_minimum_size(Vector2(320, 360));
    tree_->set_hide_root(true);
    tree_->set_columns(2);
    tree_->set_column_expand(1, false);
    tree_->set_column_custom_minimum_width(1, 28);
    tree_->connect("item_activated", callable_mp(this, &OghamTargetPickerPopup::_on_tree_item_activated));
    tree_->connect("item_edited", callable_mp(this, &OghamTargetPickerPopup::_on_tree_item_edited));
    vbox->add_child(tree_);

    OghamPopupBase *self_as_base = this;
    connect("popup_hide", callable_mp(self_as_base, &OghamPopupBase::close_and_free));
}

void OghamTargetPickerPopup::open(const PackedStringArray &all_tags, const String &current_value, const Vector2i &screen_pos, const Callable &on_selected_cb)
{
    _ensure_built();
    all_tags_ = all_tags;
    on_selected = on_selected_cb;
    closed_ = false;

    filter_->set_text("");
    _rebuild_tree("");

    popup(Rect2i(screen_pos, Vector2i(320, 400)));
    filter_->grab_focus();
}

void OghamTargetPickerPopup::_on_filter_changed(String new_text)
{
    _rebuild_tree(new_text);
}

void OghamTargetPickerPopup::_rebuild_tree(const String &filter)
{
    tree_->clear();
    TreeItem *root = tree_->create_item();
    Dictionary trie = build_trie(all_tags_);
    String filter_lower = filter.to_lower();
    if (!filter_lower.is_empty())
        prune_trie(trie, "", filter_lower);
    _add_tree_nodes(root, trie, "", !filter_lower.is_empty());
}

void OghamTargetPickerPopup::_add_tree_nodes(TreeItem *parent, const Dictionary &node, const String &path_prefix, bool auto_expand)
{
    Array keys = node.keys();
    keys.sort();
    for (int i = 0; i < keys.size(); i++)
    {
        String key = keys[i];
        String full_path = path_prefix.is_empty() ? key : path_prefix + String(".") + key;
        Dictionary child = node[key];

        TreeItem *item = tree_->create_item(parent);
        item->set_text(0, key);
        item->set_metadata(0, full_path);
        item->set_cell_mode(1, TreeItem::CELL_MODE_CHECK);
        item->set_editable(1, true);
        item->set_collapsed(!(path_prefix.is_empty() || auto_expand));

        _add_tree_nodes(item, child, full_path, auto_expand);
    }
}

void OghamTargetPickerPopup::_on_tree_item_activated()
{
    TreeItem *selected = tree_->get_selected();
    if (selected == nullptr)
        return;
    if (selected->get_first_child() != nullptr)
        return; // branch — label click just navigates, use the checkbox to select it
    _select_and_close(String(selected->get_metadata(0)));
}

void OghamTargetPickerPopup::_on_tree_item_edited()
{
    TreeItem *item = tree_->get_edited();
    if (item == nullptr || tree_->get_edited_column() != 1)
        return;
    _select_and_close(String(item->get_metadata(0)));
}

void OghamTargetPickerPopup::_select_and_close(const String &tag)
{
    if (on_selected.is_valid())
        on_selected.call(tag);
    close_and_free();
}

void OghamTargetPickerPopup::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open", "all_tags", "current_value", "screen_pos", "on_selected"), &OghamTargetPickerPopup::open);
}
