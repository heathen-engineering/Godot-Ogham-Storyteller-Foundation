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

#include "OghamVariables.h"
#include "OghamInterop.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

OghamVariables *OghamVariables::singleton = nullptr;

OghamVariables::OghamVariables()
{
    singleton = this;
}

OghamVariables::~OghamVariables()
{
    if (singleton == this)
        singleton = nullptr;
}

OghamVariables *OghamVariables::get_singleton() { return singleton; }

void OghamVariables::register_variable(const String &name, const Callable &callback)
{
    custom_variables[std::string(name.utf8().get_data())] = callback;
}

void OghamVariables::unregister_variable(const String &name)
{
    custom_variables.erase(std::string(name.utf8().get_data()));
}

void OghamVariables::reset_to_defaults()
{
    custom_variables.clear();
}

String OghamVariables::resolve_token(const String &name, const String &tag_path, const String &extra_arg, const Ref<RefCounted> &state) const
{
    auto custom = custom_variables.find(std::string(name.utf8().get_data()));
    if (custom != custom_variables.end() && custom->second.is_valid())
    {
        const Variant result = custom->second.call(tag_path, extra_arg, state);
        return result;
    }

    if (state.is_null())
        return String();

    const uint64_t tag = OghamInterop::hash_tag(tag_path);

    if (name == "String")
    {
        const int64_t key = state->call("get_value", static_cast<int64_t>(tag));
        Object *lexicon = OghamInterop::get_lexicon_registry();
        if (lexicon != nullptr)
        {
            const String resolved = lexicon->call("resolve_string", key);
            if (!resolved.is_empty())
                return resolved;
        }
        Object *registry = OghamInterop::get_gameplay_tag_registry();
        if (registry != nullptr)
        {
            const String tag_name = registry->call("get_name", key);
            if (!tag_name.is_empty())
                return tag_name;
        }
        return String();
    }

    const int decimals = extra_arg.is_valid_int() ? extra_arg.to_int() : -1;

    if (name == "Float")
    {
        const double v = state->call("get_float", static_cast<int64_t>(tag));
        return decimals >= 0 ? String::num(v, decimals) : String::num(v);
    }
    if (name == "Double")
    {
        const double v = state->call("get_double", static_cast<int64_t>(tag));
        return decimals >= 0 ? String::num(v, decimals) : String::num(v);
    }
    if (name == "Long")
    {
        const int64_t v = state->call("get_long", static_cast<int64_t>(tag));
        return String::num_int64(v);
    }
    if (name == "Ulong" || name == "UInt64")
    {
        const int64_t v = state->call("get_value", static_cast<int64_t>(tag));
        return String::num_uint64(static_cast<uint64_t>(v));
    }
    if (name == "Int")
    {
        const int v = state->call("get_int", static_cast<int64_t>(tag));
        return String::num_int64(v);
    }
    if (name == "UInt")
    {
        const int v = state->call("get_int", static_cast<int64_t>(tag));
        return String::num_uint64(static_cast<uint32_t>(v));
    }

    return String(); // unknown token
}

String OghamVariables::interpolate(const String &text, const Ref<RefCounted> &state) const
{
    Ref<RegEx> re = RegEx::create_from_string(R"(@(\w+)\(([^)]*)\))");
    const TypedArray<RegExMatch> matches = re->search_all(text);
    if (matches.size() == 0)
        return text;

    String result;
    int64_t cursor = 0;

    for (int64_t i = 0; i < matches.size(); ++i)
    {
        const Ref<RegExMatch> m = matches[i];

        const int64_t start = m->get_start(0);
        const int64_t end = m->get_end(0);

        const String token_name = m->get_string(1);
        const String args = m->get_string(2);

        String tag_path = args;
        String extra_arg;
        const int64_t comma = args.find(",");
        if (comma >= 0)
        {
            tag_path = args.substr(0, comma).strip_edges();
            extra_arg = args.substr(comma + 1).strip_edges();
        }
        else
        {
            tag_path = args.strip_edges();
        }

        result += text.substr(cursor, start - cursor);

        auto custom = custom_variables.find(std::string(token_name.utf8().get_data()));
        const bool known_builtin = token_name == "String" || token_name == "Float" || token_name == "Double" ||
                                    token_name == "Long" || token_name == "Ulong" || token_name == "UInt64" ||
                                    token_name == "Int" || token_name == "UInt";

        if (custom != custom_variables.end() || known_builtin)
            result += resolve_token(token_name, tag_path, extra_arg, state);
        else
            result += text.substr(start, end - start); // unknown token: leave untouched

        cursor = end;
    }
    result += text.substr(cursor);

    return result;
}

void OghamVariables::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("register_variable", "name", "callback"), &OghamVariables::register_variable);
    ClassDB::bind_method(D_METHOD("unregister_variable", "name"), &OghamVariables::unregister_variable);
    ClassDB::bind_method(D_METHOD("reset_to_defaults"), &OghamVariables::reset_to_defaults);
    ClassDB::bind_method(D_METHOD("interpolate", "text", "state"), &OghamVariables::interpolate);
}
