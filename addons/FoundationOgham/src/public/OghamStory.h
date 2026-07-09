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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "OghamEntry.h"

#include <unordered_map>
#include <vector>

using namespace godot;

/// <summary>
/// An immutable story definition: a flat list of OghamEntry nodes. Sessions
/// (OghamSession) hold all mutable play-state; a single OghamStory can be played by
/// any number of concurrent sessions.
///
/// Builds and caches a tag->entry index and a parent->children index (derived from
/// option targets) lazily on first hierarchy query, invalidated whenever entries is
/// reassigned. Ported from Unity-Ogham-Storyteller-Foundation's OghamStory (the
/// immutable-definition half — OghamData's authoring/iteration concerns and
/// OghamStoryCatalog's global registry are not ported 1:1; see README).
/// </summary>
class OghamStory : public Resource
{
    GDCLASS(OghamStory, Resource);

private:
    String story_name;
    TypedArray<OghamEntry> entries;

    mutable std::unordered_map<uint64_t, Ref<OghamEntry>> entry_index;
    mutable std::unordered_map<uint64_t, std::vector<uint64_t>> child_index;
    mutable bool index_built = false;

    void ensure_index() const;

public:
    OghamStory() = default;

    void set_story_name(const String &v);
    String get_story_name() const;

    void set_entries(const TypedArray<OghamEntry> &v);
    TypedArray<OghamEntry> get_entries() const;

    Ref<OghamEntry> find_entry(uint64_t tag) const;
    Ref<OghamEntry> find_entry_by_name(const String &tag_path) const;

    /// All registered descendants of 'tag' in the option-graph child index (BFS,
    /// not tree-shaped — a node reachable via multiple paths appears once).
    PackedInt64Array collect_descendants(uint64_t tag) const;

    /// BFS window: 'tag' and every entry reachable within 'depth' option-hops.
    PackedInt64Array collect_within_depth(uint64_t tag, int depth) const;

    /// Forces the tag/child index to rebuild on the next query (call after mutating
    /// entries in place rather than reassigning the property).
    void invalidate_index();

    /// Parses .ogham JSON text (see README for schema) into a new OghamStory,
    /// registering every referenced tag into GameplayTagRegistry and every inline
    /// literal string into LexiconRegistry (as "default" fallback entries) along the
    /// way, matching Unity's OghamStoryBuilder behaviour. Never throws: malformed
    /// JSON logs a warning and returns an empty story.
    static Ref<OghamStory> parse_manifest(const String &json_text);

protected:
    static void _bind_methods();
};
