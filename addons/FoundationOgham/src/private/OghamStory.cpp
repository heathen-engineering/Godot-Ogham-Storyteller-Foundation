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

#include "OghamStory.h"
#include "OghamInterop.h"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <unordered_set>

using namespace godot;

namespace
{
    int parse_content_type(const String &s)
    {
        const String l = s.to_lower();
        if (l == "image") return OghamContentKey::CONTENT_IMAGE;
        if (l == "sprite") return OghamContentKey::CONTENT_SPRITE;
        if (l == "audio") return OghamContentKey::CONTENT_AUDIO;
        if (l == "prefab" || l == "packedscene" || l == "scene") return OghamContentKey::CONTENT_PACKED_SCENE;
        return OghamContentKey::CONTENT_TEXT;
    }

    int parse_loc_mode(const String &s)
    {
        const String l = s.to_lower();
        if (l == "localised" || l == "localized") return OghamContentKey::MODE_LOCALISED;
        if (l == "invariant") return OghamContentKey::MODE_INVARIANT;
        return OghamContentKey::MODE_LITERAL;
    }

    // Accepts O3DE short forms (Sub/Mul/Div) for cross-engine .ogham portability,
    // matching Unity-Ogham-Storyteller-Foundation's manifest parser tolerance.
    int parse_arithmetic(const String &s)
    {
        const String l = s.to_lower();
        if (l == "add") return 1;
        if (l == "subtract" || l == "sub") return 2;
        if (l == "multiply" || l == "mul") return 3;
        if (l == "divide" || l == "div") return 4;
        if (l == "min") return 5;
        if (l == "max") return 6;
        return 0; // Set
    }

    int parse_comparison(const String &s)
    {
        const String l = s.to_lower();
        if (l == "notexists") return 1;
        if (l == "equal") return 2;
        if (l == "notequal") return 3;
        if (l == "less") return 4;
        if (l == "lessequal") return 5;
        if (l == "greater") return 6;
        if (l == "greaterequal") return 7;
        if (l == "ismemberof") return 8;
        if (l == "isparentof") return 9;
        if (l == "isexactly") return 10;
        return 0; // Exists
    }

    int parse_logic_op(const String &s)
    {
        const String l = s.to_lower();
        if (l == "or") return 1;
        if (l == "xor") return 2;
        return 0; // And
    }

    int parse_value_type(const String &s)
    {
        const String l = s.to_lower();
        if (l == "signed") return 1;
        if (l == "decimal") return 2;
        if (l == "tag") return 3;
        return 0; // Unsigned
    }

    Ref<Resource> build_condition(const Dictionary &d)
    {
        const Variant inst = ClassDB::instantiate("GameplayTagCondition");
        Ref<Resource> cond = inst;
        if (cond.is_null())
        {
            UtilityFunctions::push_warning("Ogham: GameplayTagCondition unavailable — is GameplayTags Foundation loaded?");
            return cond;
        }
        if (d.has("tag")) cond->set("tag_name", String(d["tag"]));
        if (d.has("comparison")) cond->set("comparison", parse_comparison(String(d["comparison"])));
        if (d.has("compareValue")) cond->set("compare_value", int64_t(d["compareValue"]));
        if (d.has("compareTag")) cond->set("compare_tag_name", String(d["compareTag"]));
        if (d.has("exactMatch")) cond->set("exact_match", bool(d["exactMatch"]));
        if (d.has("logicOp")) cond->set("logic_op", parse_logic_op(String(d["logicOp"])));
        if (d.has("compareValueType")) cond->set("compare_value_type", parse_value_type(String(d["compareValueType"])));
        return cond;
    }

    Ref<Resource> build_operation(const Dictionary &d)
    {
        const Variant inst = ClassDB::instantiate("GameplayTagOperation");
        Ref<Resource> op = inst;
        if (op.is_null())
        {
            UtilityFunctions::push_warning("Ogham: GameplayTagOperation unavailable — is GameplayTags Foundation loaded?");
            return op;
        }
        if (d.has("tag")) op->set("tag_name", String(d["tag"]));
        if (d.has("arithmetic")) op->set("arithmetic", parse_arithmetic(String(d["arithmetic"])));
        if (d.has("value")) op->set("value", int64_t(d["value"]));
        if (d.has("valueTag")) op->set("value_tag_name", String(d["valueTag"]));
        if (d.has("valueType")) op->set("value_type", parse_value_type(String(d["valueType"])));
        if (d.has("conditions"))
        {
            const Array condArr = d["conditions"];
            Array parsed;
            for (int64_t i = 0; i < condArr.size(); ++i)
                parsed.push_back(build_condition(Dictionary(condArr[i])));
            op->set("conditions", parsed);
        }
        return op;
    }

    // Collects tag_name/compare_tag_name from a list of GameplayTagCondition Resources
    // into tag_batch, so tags referenced only as comparison targets (never appearing as
    // an entry or option) still get registered.
    void collect_condition_tags(const Array &conditions, String &tag_batch)
    {
        for (int64_t i = 0; i < conditions.size(); ++i)
        {
            const Ref<Resource> cond = conditions[i];
            if (cond.is_null())
                continue;
            const String tag_name = cond->get("tag_name");
            if (!tag_name.is_empty())
            {
                tag_batch += tag_name;
                tag_batch += "\n";
            }
            const String compare_tag_name = cond->get("compare_tag_name");
            if (!compare_tag_name.is_empty())
            {
                tag_batch += compare_tag_name;
                tag_batch += "\n";
            }
        }
    }

    // Collects tag_name/value_tag_name from a list of GameplayTagOperation Resources,
    // and recurses into each operation's own nested conditions.
    void collect_operation_tags(const Array &operations, String &tag_batch)
    {
        for (int64_t i = 0; i < operations.size(); ++i)
        {
            const Ref<Resource> op = operations[i];
            if (op.is_null())
                continue;
            const String tag_name = op->get("tag_name");
            if (!tag_name.is_empty())
            {
                tag_batch += tag_name;
                tag_batch += "\n";
            }
            const String value_tag_name = op->get("value_tag_name");
            if (!value_tag_name.is_empty())
            {
                tag_batch += value_tag_name;
                tag_batch += "\n";
            }
            const Array nested_conditions = op->get("conditions");
            collect_condition_tags(nested_conditions, tag_batch);
        }
    }

    Ref<OghamContentKey> build_content_key(const Dictionary &d)
    {
        Ref<OghamContentKey> key;
        key.instantiate();
        key->set_type(parse_content_type(String(d.get("type", "Text"))));
        key->set_mode(parse_loc_mode(String(d.get("mode", "Literal"))));
        key->set_key_or_value(d.get("key", String()));
        key->set_asset_path(d.get("path", String()));
        key->set_asset_sub_name(d.get("sub", String()));
        return key;
    }

    Ref<OghamOption> build_option(const Dictionary &d)
    {
        Ref<OghamOption> opt;
        opt.instantiate();
        opt->set_tag_name(d.get("tag", String()));
        opt->set_target_entry_path(d.get("targetTag", String()));

        if (d.has("textKey"))
        {
            const Variant inst = ClassDB::instantiate("LexiconText");
            Ref<Resource> lt = inst;
            if (lt.is_valid())
            {
                lt->set("mode", 1); // Literal
                lt->set("key_or_value", String(d["textKey"]));
                opt->set_text_key(lt);
            }
        }

        if (d.has("conditions"))
        {
            const Array arr = d["conditions"];
            Array parsed;
            for (int64_t i = 0; i < arr.size(); ++i)
                parsed.push_back(build_condition(Dictionary(arr[i])));
            opt->set_conditions(parsed);
        }
        if (d.has("operations"))
        {
            const Array arr = d["operations"];
            Array parsed;
            for (int64_t i = 0; i < arr.size(); ++i)
                parsed.push_back(build_operation(Dictionary(arr[i])));
            opt->set_operations(parsed);
        }
        return opt;
    }

    Ref<OghamEntry> build_entry(const Dictionary &d)
    {
        Ref<OghamEntry> entry;
        entry.instantiate();
        entry->set_tag_name(d.get("tag", String()));
        entry->set_mode(String(d.get("nodeMode", "Content")).to_lower() == "fork" ? OghamEntry::MODE_FORK : OghamEntry::MODE_CONTENT);

        if (d.has("contentKeys"))
        {
            const Array arr = d["contentKeys"];
            TypedArray<OghamContentKey> keys;
            for (int64_t i = 0; i < arr.size(); ++i)
                keys.push_back(build_content_key(Dictionary(arr[i])));
            entry->set_content_keys(keys);
        }
        if (d.has("entryOperations"))
        {
            const Array arr = d["entryOperations"];
            Array ops;
            for (int64_t i = 0; i < arr.size(); ++i)
                ops.push_back(build_operation(Dictionary(arr[i])));
            entry->set_entry_operations(ops);
        }
        if (d.has("options"))
        {
            const Array arr = d["options"];
            TypedArray<OghamOption> opts;
            for (int64_t i = 0; i < arr.size(); ++i)
                opts.push_back(build_option(Dictionary(arr[i])));
            entry->set_options(opts);
        }
        return entry;
    }
} // namespace

void OghamStory::set_story_name(const String &v) { story_name = v; }
String OghamStory::get_story_name() const { return story_name; }

void OghamStory::set_entries(const TypedArray<OghamEntry> &v)
{
    entries = v;
    index_built = false;
}
TypedArray<OghamEntry> OghamStory::get_entries() const { return entries; }

void OghamStory::invalidate_index() { index_built = false; }

void OghamStory::ensure_index() const
{
    if (index_built)
        return;

    entry_index.clear();
    child_index.clear();

    for (int64_t i = 0; i < entries.size(); ++i)
    {
        const Ref<OghamEntry> e = entries[i];
        if (e.is_null())
            continue;
        entry_index[e->get_tag()] = e;
    }

    for (const auto &[tag, e] : entry_index)
    {
        const TypedArray<OghamOption> opts = e->get_options();
        for (int64_t j = 0; j < opts.size(); ++j)
        {
            const Ref<OghamOption> opt = opts[j];
            if (opt.is_null() || !opt->has_target())
                continue;
            child_index[tag].push_back(opt->get_target_entry());
        }
    }

    index_built = true;
}

Ref<OghamEntry> OghamStory::find_entry(uint64_t tag) const
{
    ensure_index();
    auto it = entry_index.find(tag);
    return (it != entry_index.end()) ? it->second : Ref<OghamEntry>();
}

Ref<OghamEntry> OghamStory::find_entry_by_name(const String &tag_path) const
{
    return find_entry(OghamInterop::hash_tag(tag_path));
}

PackedInt64Array OghamStory::collect_descendants(uint64_t tag) const
{
    ensure_index();
    PackedInt64Array result;
    std::unordered_set<uint64_t> visited;
    std::vector<uint64_t> queue;

    auto it = child_index.find(tag);
    if (it != child_index.end())
        queue.insert(queue.end(), it->second.begin(), it->second.end());

    while (!queue.empty())
    {
        const uint64_t id = queue.back();
        queue.pop_back();
        if (!visited.insert(id).second)
            continue;
        result.push_back(static_cast<int64_t>(id));

        auto cit = child_index.find(id);
        if (cit != child_index.end())
            for (const uint64_t c : cit->second)
                if (visited.find(c) == visited.end())
                    queue.push_back(c);
    }
    return result;
}

PackedInt64Array OghamStory::collect_within_depth(uint64_t tag, int depth) const
{
    ensure_index();
    PackedInt64Array result;
    std::unordered_set<uint64_t> visited;
    struct Item
    {
        uint64_t id;
        int depth;
    };
    std::vector<Item> queue{{tag, 0}};
    visited.insert(tag);
    result.push_back(static_cast<int64_t>(tag));

    size_t head = 0;
    while (head < queue.size())
    {
        const Item cur = queue[head++];
        if (cur.depth >= depth)
            continue;
        auto it = child_index.find(cur.id);
        if (it == child_index.end())
            continue;
        for (const uint64_t c : it->second)
        {
            if (visited.insert(c).second)
            {
                result.push_back(static_cast<int64_t>(c));
                queue.push_back({c, cur.depth + 1});
            }
        }
    }
    return result;
}

Ref<OghamStory> OghamStory::parse_manifest(const String &json_text)
{
    Ref<OghamStory> story;
    story.instantiate();

    Ref<JSON> json;
    json.instantiate();
    if (json->parse(json_text) != OK)
    {
        UtilityFunctions::push_warning("OghamStory: failed to parse .ogham JSON: ", json->get_error_message());
        return story;
    }

    const Variant data = json->get_data();
    if (data.get_type() != Variant::DICTIONARY)
    {
        UtilityFunctions::push_warning("OghamStory: .ogham root is not a JSON object");
        return story;
    }

    const Dictionary root = data;
    story->set_story_name(root.get("storyTag", String()));

    TypedArray<OghamEntry> parsed_entries;
    String tag_batch;

    if (root.has("entries"))
    {
        const Array entries_arr = root["entries"];
        for (int64_t i = 0; i < entries_arr.size(); ++i)
        {
            const Ref<OghamEntry> entry = build_entry(Dictionary(entries_arr[i]));
            parsed_entries.push_back(entry);

            if (!entry->get_tag_name().is_empty())
                tag_batch += entry->get_tag_name() + "\n";

            collect_operation_tags(entry->get_entry_operations(), tag_batch);

            const TypedArray<OghamOption> opts = entry->get_options();
            for (int64_t j = 0; j < opts.size(); ++j)
            {
                const Ref<OghamOption> opt = opts[j];
                if (opt.is_null())
                    continue;
                if (!opt->get_tag_name().is_empty())
                    tag_batch += opt->get_tag_name() + "\n";
                if (opt->has_target())
                    tag_batch += opt->get_target_entry_path() + "\n";

                collect_condition_tags(opt->get_conditions(), tag_batch);
                collect_operation_tags(opt->get_operations(), tag_batch);
            }
        }
    }

    story->set_entries(parsed_entries);

    Object *registry = OghamInterop::get_gameplay_tag_registry();
    if (registry != nullptr && !tag_batch.is_empty())
        registry->call("register_from_string", tag_batch);

    return story;
}

void OghamStory::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_story_name", "name"), &OghamStory::set_story_name);
    ClassDB::bind_method(D_METHOD("get_story_name"), &OghamStory::get_story_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "story_name"), "set_story_name", "get_story_name");

    ClassDB::bind_method(D_METHOD("set_entries", "value"), &OghamStory::set_entries);
    ClassDB::bind_method(D_METHOD("get_entries"), &OghamStory::get_entries);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "OghamEntry")),
        "set_entries", "get_entries");

    ClassDB::bind_method(D_METHOD("find_entry", "tag"), &OghamStory::find_entry);
    ClassDB::bind_method(D_METHOD("find_entry_by_name", "tag_path"), &OghamStory::find_entry_by_name);
    ClassDB::bind_method(D_METHOD("collect_descendants", "tag"), &OghamStory::collect_descendants);
    ClassDB::bind_method(D_METHOD("collect_within_depth", "tag", "depth"), &OghamStory::collect_within_depth);
    ClassDB::bind_method(D_METHOD("invalidate_index"), &OghamStory::invalidate_index);

    ClassDB::bind_static_method("OghamStory", D_METHOD("parse_manifest", "json_text"), &OghamStory::parse_manifest);
}
