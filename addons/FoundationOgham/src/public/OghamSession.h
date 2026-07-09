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

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "OghamOption.h"
#include "OghamStory.h"

using namespace godot;

/// <summary>
/// One playthrough of an OghamStory. The story definition is immutable and shared;
/// the session owns all mutable state — the narrative-state GameplayTagCollection
/// ("state"), the append-only choice history, and which entry is currently active.
///
/// Ogham has no condition/operation logic of its own: every guard and every side
/// effect is a GameplayTagCondition/GameplayTagOperation (Foundation GameplayTags
/// Resources), evaluated/applied against this session's state collection via Variant
/// calls (see OghamInterop.h — Ogham has no C++ compile-time dependency on that
/// extension).
///
/// Ported from Unity-Ogham-Storyteller-Foundation's OghamSession. Unity's separate
/// read-only StoryNode/StoryOption facade classes are consolidated into direct query
/// methods here (get_current_text, get_current_options, ...) — Godot's signal-driven
/// UI idiom doesn't need the extra indirection those existed for.
/// </summary>
class OghamSession : public RefCounted
{
    GDCLASS(OghamSession, RefCounted);

private:
    Ref<OghamStory> story;
    Ref<RefCounted> state; // GameplayTagCollection instance (instantiated dynamically)
    Array history;         // Array[Dictionary{entry_id:int, selected_option:int}]

    bool is_active_flag = false;
    uint64_t current_entry_id = 0;
    Ref<OghamEntry> current_entry;
    TypedArray<OghamOption> current_active_options;
    TypedArray<OghamOption> current_all_options;

    void ensure_state();
    void close_internal();
    bool enter_entry(Ref<OghamEntry> entry);
    void build_options(const Ref<OghamEntry> &entry, TypedArray<OghamOption> &out_active, TypedArray<OghamOption> &out_all) const;
    static void apply_operations(const Array &operations, const Ref<RefCounted> &target_state);
    Ref<OghamOption> find_active_option(uint64_t option_tag) const;
    Ref<OghamOption> find_any_option(uint64_t option_tag) const;

public:
    OghamSession() = default;

    void set_story(const Ref<OghamStory> &v);
    Ref<OghamStory> get_story() const;

    /// The narrative-state GameplayTagCollection. Lazily instantiated on first access.
    Ref<RefCounted> get_state();

    /// Closes any active conversation, then enters the given entry tag.
    bool enter(uint64_t entry_tag);

    /// Chooses an option currently active on the displayed entry; applies its
    /// operations, records the choice, and navigates to its target (or closes the
    /// session if the option has no target). Returns false only if option_tag isn't
    /// a currently active option.
    bool choose(uint64_t option_tag);

    void close();

    /// Re-surfaces the current entry without re-running its entry_operations or
    /// Fork routing. No-ops (returns false) if there is no current entry or it's a Fork.
    bool resume();

    /// Navigates back to a previously visited entry, clearing narrative-state tags
    /// recorded for every entry reachable below it (via the option graph), then
    /// re-enters normally (entry_operations/Fork routing re-fire).
    bool return_to(uint64_t entry_tag);

    bool get_is_active() const;
    uint64_t get_current_entry_tag() const;
    int get_current_content_count() const;
    String get_current_text(int index) const;
    Ref<Resource> get_current_asset(int index) const;

    /// Tags of options whose conditions currently pass.
    PackedInt64Array get_current_options() const;

    /// Tags of every option on the current entry, gated or not — for inline-link
    /// rendering, where a gated option should still be shown (styled differently).
    PackedInt64Array get_current_all_options() const;

    bool is_option_active(uint64_t option_tag) const;
    String get_option_text(uint64_t option_tag) const;

    Array get_history() const;

    /// Serialises {name, current_entry_id, state, history} to a Dictionary suitable
    /// for JSON/ConfigFile/etc. persistence.
    Dictionary snapshot(const String &save_name) const;

    /// Restores from a Dictionary produced by snapshot(). Leaves the session inactive
    /// — call enter()/resume() afterward to redisplay.
    bool restore(const Dictionary &save_state);

protected:
    static void _bind_methods();
};
