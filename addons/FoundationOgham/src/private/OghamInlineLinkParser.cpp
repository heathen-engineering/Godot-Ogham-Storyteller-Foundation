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

#include "OghamInlineLinkParser.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace
{
    const char *LINK_PATTERN = R"(\[([^\]]*)\]\(([^)]*)\))";
    const char *BOLD_PATTERN = R"(\*\*(.+?)\*\*)";
    const char *ITALIC_PATTERN = R"(\*(.+?)\*)";
} // namespace

bool OghamInlineLinkParser::is_ogham_link(const String &url)
{
    return url.begins_with("Ogham://");
}

String OghamInlineLinkParser::get_tag_path(const String &url)
{
    if (!is_ogham_link(url))
        return String();
    return url.substr(8);
}

bool OghamInlineLinkParser::is_pure_link(const String &text)
{
    const String trimmed = text.strip_edges();
    Ref<RegEx> re = RegEx::create_from_string(String("^") + LINK_PATTERN + "$");
    return re->search(trimmed).is_valid();
}

Array OghamInlineLinkParser::extract_links(const String &text)
{
    Array result;
    Ref<RegEx> re = RegEx::create_from_string(LINK_PATTERN);
    TypedArray<RegExMatch> matches = re->search_all(text);
    for (int64_t i = 0; i < matches.size(); ++i)
    {
        const Ref<RegExMatch> m = matches[i];
        Dictionary link;
        link["display"] = m->get_string(1);
        link["url"] = m->get_string(2);
        result.push_back(link);
    }
    return result;
}

String OghamInlineLinkParser::to_bbcode(const String &text)
{
    Ref<RegEx> link_re = RegEx::create_from_string(LINK_PATTERN);
    String result = link_re->sub(text, "[url=$2]$1[/url]", true);

    Ref<RegEx> bold_re = RegEx::create_from_string(BOLD_PATTERN);
    result = bold_re->sub(result, "[b]$1[/b]", true);

    Ref<RegEx> italic_re = RegEx::create_from_string(ITALIC_PATTERN);
    result = italic_re->sub(result, "[i]$1[/i]", true);

    return result;
}

String OghamInlineLinkParser::strip_markup(const String &text)
{
    Ref<RegEx> link_re = RegEx::create_from_string(LINK_PATTERN);
    String result = link_re->sub(text, "$1", true);

    Ref<RegEx> bold_re = RegEx::create_from_string(BOLD_PATTERN);
    result = bold_re->sub(result, "$1", true);

    Ref<RegEx> italic_re = RegEx::create_from_string(ITALIC_PATTERN);
    result = italic_re->sub(result, "$1", true);

    return result;
}

String OghamInlineLinkParser::normalise_for_tag(const String &text)
{
    String result;
    bool capitalize_next = true;

    const CharString utf8 = text.utf8();
    const char *s = utf8.ptr();
    const int64_t len = utf8.length();

    for (int64_t i = 0; i < len; ++i)
    {
        const char c = s[i];
        const bool is_alnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (!is_alnum)
        {
            capitalize_next = true;
            continue;
        }

        char out = c;
        if (capitalize_next && out >= 'a' && out <= 'z')
            out = static_cast<char>(out - 'a' + 'A');
        else if (!capitalize_next && out >= 'A' && out <= 'Z')
            out = static_cast<char>(out - 'A' + 'a');

        result += String::chr(static_cast<char32_t>(static_cast<unsigned char>(out)));
        capitalize_next = false;
    }

    return result;
}

void OghamInlineLinkParser::_bind_methods()
{
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("is_ogham_link", "url"), &OghamInlineLinkParser::is_ogham_link);
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("get_tag_path", "url"), &OghamInlineLinkParser::get_tag_path);
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("is_pure_link", "text"), &OghamInlineLinkParser::is_pure_link);
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("extract_links", "text"), &OghamInlineLinkParser::extract_links);
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("to_bbcode", "text"), &OghamInlineLinkParser::to_bbcode);
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("strip_markup", "text"), &OghamInlineLinkParser::strip_markup);
    ClassDB::bind_static_method("OghamInlineLinkParser", D_METHOD("normalise_for_tag", "text"), &OghamInlineLinkParser::normalise_for_tag);
}
