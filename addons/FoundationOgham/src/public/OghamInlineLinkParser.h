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

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/// <summary>
/// Parses Ogham's inline authoring markup embedded in content-key text:
///   [display text](Ogham://Tag.Path)   — a story link; synthesizes an OghamOption
///                                         when it's the only authored path to a target
///   [display text](https://...)        — an ordinary hyperlink, passed through
///   **bold**, *italic*                 — simple emphasis markup
///
/// Pure string/regex logic with no engine dependency beyond Godot's RegEx class — a
/// direct port of Unity-Ogham-Storyteller-Foundation's OghamInlineLinkParser. The only
/// change is the target markup dialect: Unity emits TextMesh Pro tags, this emits
/// Godot RichTextLabel BBCode.
/// </summary>
class OghamInlineLinkParser : public Object
{
    GDCLASS(OghamInlineLinkParser, Object);

public:
    /// True if the given [display](url) target uses the "Ogham://" scheme.
    static bool is_ogham_link(const String &url);

    /// Strips the "Ogham://" scheme, returning the bare dot-path tag. Empty if not
    /// an Ogham link.
    static String get_tag_path(const String &url);

    /// True if 'text' is, in its entirety, a single [display](url) link (used to
    /// auto-synthesize an option from a node whose sole content is a link).
    static bool is_pure_link(const String &text);

    /// Finds every [display](url) span in 'text'. Returns an Array of
    /// Dictionary{display: String, url: String}.
    static Array extract_links(const String &text);

    /// Converts authoring markup to Godot RichTextLabel BBCode. Ogham:// links become
    /// [url=Ogham://Tag]display[/url]; other links become plain [url=...]; **/*
    /// become [b]/[i].
    static String to_bbcode(const String &text);

    /// Strips all markup, returning plain display text (link display text is kept).
    static String strip_markup(const String &text);

    /// Converts free text to a PascalCase tag segment, e.g. "go north" -> "GoNorth".
    /// Used to derive a stable tag path for a synthesized link-option.
    static String normalise_for_tag(const String &text);

protected:
    static void _bind_methods();
};
