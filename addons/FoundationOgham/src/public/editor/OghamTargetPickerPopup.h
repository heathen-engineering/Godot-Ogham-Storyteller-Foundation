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

#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "editor/OghamPopupBase.h"

using namespace godot;

/// <summary>
/// Cascading, searchable tree popup for picking an Option's target entry
/// tag — replaces the old flat OptionButton (every entry tag in the story
/// in one giant dropdown) with a dot-path-segment-grouped Tree (Act1 >
/// Protagonist > Bedroom > WakingUp > 01/02/03…), per user feedback that
/// the flat list didn't scale.
///
/// Same design as FoundationGameplayTags' GameplayTagPickerProperty popup —
/// two-column Tree (label + CELL_MODE_CHECK "select this exact path" per
/// row, branch or leaf alike, so a branch that's also a meaningful target
/// stays independently pickable from its own children) plus a filter
/// LineEdit — but built separately here rather than shared cross-extension:
/// this project's addons are deliberately siblings with no shared base (see
/// project_architecture memory), and Ogham's entry-tag vocabulary isn't
/// GameplayTags data anyway (different source: OghamGraphView::_all_entry_tags(),
/// not the GameplayTagRegistry/.gptags vocabulary), so there's no real
/// dependency to share even if the two gems' picker code looks similar.
/// </summary>
class OghamTargetPickerPopup : public OghamPopupBase
{
    GDCLASS(OghamTargetPickerPopup, OghamPopupBase);

private:
    LineEdit *filter_ = nullptr;
    Tree *tree_ = nullptr;
    PackedStringArray all_tags_;
    Callable on_selected;

    void _ensure_built();
    void _on_filter_changed(String new_text);
    void _rebuild_tree(const String &filter);
    void _add_tree_nodes(TreeItem *parent, const Dictionary &node, const String &path_prefix, bool auto_expand);
    void _on_tree_item_activated();
    void _on_tree_item_edited();
    void _select_and_close(const String &tag);

public:
    OghamTargetPickerPopup() = default;

    void open(const PackedStringArray &all_tags, const String &current_value, const Vector2i &screen_pos, const Callable &on_selected_cb);

protected:
    static void _bind_methods();
};
