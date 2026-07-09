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

using Godot;
using Godot.Collections;

namespace Heathen.Ogham
{
    /// <summary>
    /// Static facade over the native OghamInlineLinkParser utility class. Parses
    /// "[display](Ogham://Tag.Path)" story links and **bold**/*italic* markup embedded
    /// in content text; converts authoring markup to Godot RichTextLabel BBCode.
    /// </summary>
    public static class OghamInlineLinkParser
    {
        public static bool IsOghamLink(string url) => (bool)ClassDB.ClassCallStatic("OghamInlineLinkParser", "is_ogham_link", url);
        public static string GetTagPath(string url) => (string)ClassDB.ClassCallStatic("OghamInlineLinkParser", "get_tag_path", url);
        public static bool IsPureLink(string text) => (bool)ClassDB.ClassCallStatic("OghamInlineLinkParser", "is_pure_link", text);
        public static Array ExtractLinks(string text) => (Array)ClassDB.ClassCallStatic("OghamInlineLinkParser", "extract_links", text);
        public static string ToBBCode(string text) => (string)ClassDB.ClassCallStatic("OghamInlineLinkParser", "to_bbcode", text);
        public static string StripMarkup(string text) => (string)ClassDB.ClassCallStatic("OghamInlineLinkParser", "strip_markup", text);
        public static string NormaliseForTag(string text) => (string)ClassDB.ClassCallStatic("OghamInlineLinkParser", "normalise_for_tag", text);
    }
}
