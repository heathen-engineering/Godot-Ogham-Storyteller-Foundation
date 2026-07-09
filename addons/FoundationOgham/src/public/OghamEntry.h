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
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "OghamContentKey.h"
#include "OghamOption.h"

using namespace godot;

/// <summary>
/// A single node in an Ogham story graph: a tag identity, an ordered list of content
/// slots (text/image/audio/...), operations applied on entry, and a list of options
/// (choices) leading to other entries.
///
/// mode == FORK makes this a silent routing node: never shown to the player, never
/// recorded in history. Its options are evaluated as routes in order and the first
/// whose conditions pass is taken immediately (see OghamSession::enter_entry).
///
/// Ported from Unity-Ogham-Storyteller-Foundation's DialogueEntry.
/// </summary>
class OghamEntry : public Resource
{
    GDCLASS(OghamEntry, Resource);

public:
    enum Mode
    {
        MODE_CONTENT = 0,
        MODE_FORK = 1,
    };

private:
    String tag_name;
    int mode = MODE_CONTENT;
    TypedArray<OghamContentKey> content_keys;
    Array entry_operations; // Array[GameplayTagOperation]
    TypedArray<OghamOption> options;

public:
    OghamEntry() = default;

    void set_tag_name(const String &v);
    String get_tag_name() const;
    uint64_t get_tag() const;

    void set_mode(int v);
    int get_mode() const;
    bool is_fork() const;

    void set_content_keys(const TypedArray<OghamContentKey> &v);
    TypedArray<OghamContentKey> get_content_keys() const;
    int get_content_count() const;
    Ref<OghamContentKey> get_content_key(int index) const;

    void set_entry_operations(const Array &v);
    Array get_entry_operations() const;

    void set_options(const TypedArray<OghamOption> &v);
    TypedArray<OghamOption> get_options() const;

protected:
    static void _bind_methods();
};

VARIANT_ENUM_CAST(OghamEntry::Mode);
