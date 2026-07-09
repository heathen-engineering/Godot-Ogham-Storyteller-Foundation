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
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/// <summary>
/// One content slot on an OghamEntry — narrator text, a speaker name, body copy, a
/// portrait, a voice line, ... An entry holds an ordered list of these, indexed
/// positionally by the presentation layer.
///
/// Localised/Literal/Invariant follows the same convention as LexiconText/LexiconAsset
/// (mode 0/1/2 line up across extensions). Asset content resolves through
/// LexiconRegistry when Localised, or loads asset_path directly (a res:// path,
/// optionally "path::sub_resource") when Literal/Invariant.
///
/// Ported from Unity-Ogham-Storyteller-Foundation's OghamContentKey.
/// </summary>
class OghamContentKey : public Resource
{
    GDCLASS(OghamContentKey, Resource);

public:
    enum ContentType
    {
        CONTENT_TEXT = 0,
        CONTENT_IMAGE = 1,
        CONTENT_SPRITE = 2,
        CONTENT_AUDIO = 3,
        CONTENT_PACKED_SCENE = 4, // Unity's "Prefab"
    };

    enum Mode
    {
        MODE_LOCALISED = 0,
        MODE_LITERAL = 1,
        MODE_INVARIANT = 2,
    };

private:
    int type = CONTENT_TEXT;
    int mode = MODE_LITERAL;
    String key_or_value;
    String asset_path;
    String asset_sub_name;

public:
    OghamContentKey() = default;

    void set_type(int v);
    int get_type() const;

    void set_mode(int v);
    int get_mode() const;

    void set_key_or_value(const String &v);
    String get_key_or_value() const;

    void set_asset_path(const String &v);
    String get_asset_path() const;

    void set_asset_sub_name(const String &v);
    String get_asset_sub_name() const;

    uint64_t get_hash() const;

    /// Resolves display text (Localised via Lexicon, else key_or_value directly).
    String resolve_text() const;

    /// Resolves the asset (Localised via Lexicon, else loads asset_path directly).
    Ref<Resource> resolve_asset() const;

protected:
    static void _bind_methods();
};

VARIANT_ENUM_CAST(OghamContentKey::ContentType);
VARIANT_ENUM_CAST(OghamContentKey::Mode);
