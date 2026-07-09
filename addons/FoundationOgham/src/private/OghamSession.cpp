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

#include "OghamSession.h"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <unordered_set>

using namespace godot;

void OghamSession::set_story(const Ref<OghamStory> &v) { story = v; }
Ref<OghamStory> OghamSession::get_story() const { return story; }

void OghamSession::ensure_state()
{
    if (state.is_valid())
        return;
    const Variant inst = ClassDB::instantiate("GameplayTagCollection");
    state = inst;
    if (state.is_null())
        UtilityFunctions::push_error("OghamSession: GameplayTagCollection unavailable — is GameplayTags Foundation loaded?");
}

Ref<RefCounted> OghamSession::get_state()
{
    ensure_state();
    return state;
}

void OghamSession::apply_operations(const Array &operations, const Ref<RefCounted> &target_state)
{
    if (target_state.is_null())
        return;
    for (int64_t i = 0; i < operations.size(); ++i)
    {
        const Ref<Resource> op = operations[i];
        if (op.is_valid())
            op->call("apply", target_state);
    }
}

void OghamSession::build_options(const Ref<OghamEntry> &entry, TypedArray<OghamOption> &out_active, TypedArray<OghamOption> &out_all) const
{
    const TypedArray<OghamOption> opts = entry->get_options();
    for (int64_t i = 0; i < opts.size(); ++i)
    {
        const Ref<OghamOption> opt = opts[i];
        if (opt.is_null())
            continue;
        out_all.push_back(opt);
        if (opt->is_active(state))
            out_active.push_back(opt);
    }
}

void OghamSession::close_internal()
{
    is_active_flag = false;
    current_entry_id = 0;
    current_entry = Ref<OghamEntry>();
    current_active_options.clear();
    current_all_options.clear();
    emit_signal("closed");
}

bool OghamSession::enter_entry(Ref<OghamEntry> entry)
{
    ensure_state();
    std::unordered_set<uint64_t> visited_forks;

    while (true)
    {
        if (entry.is_null())
        {
            close_internal();
            return false;
        }

        apply_operations(entry->get_entry_operations(), state);

        if (!entry->is_fork())
        {
            current_entry_id = entry->get_tag();
            current_entry = entry;
            is_active_flag = true;

            TypedArray<OghamOption> active, all;
            build_options(entry, active, all);
            current_active_options = active;
            current_all_options = all;

            emit_signal("entered", static_cast<int64_t>(current_entry_id));
            return true;
        }

        // Fork: silent routing node — never surfaced, never recorded in history.
        if (!visited_forks.insert(entry->get_tag()).second)
        {
            UtilityFunctions::push_error("OghamSession: fork cycle detected at '", entry->get_tag_name(), "' — closing session.");
            close_internal();
            return false;
        }

        TypedArray<OghamOption> active, all;
        build_options(entry, active, all);
        if (active.size() == 0)
        {
            // No route's conditions passed — nowhere to go.
            close_internal();
            return false;
        }

        const Ref<OghamOption> route = active[0];
        apply_operations(route->get_operations(), state);

        entry = route->has_target() ? story->find_entry(route->get_target_entry()) : Ref<OghamEntry>();
    }
}

bool OghamSession::enter(uint64_t entry_tag)
{
    if (story.is_null())
        return false;
    if (is_active_flag)
        close_internal();

    const Ref<OghamEntry> entry = story->find_entry(entry_tag);
    if (entry.is_null())
        return false;

    return enter_entry(entry);
}

Ref<OghamOption> OghamSession::find_active_option(uint64_t option_tag) const
{
    for (int64_t i = 0; i < current_active_options.size(); ++i)
    {
        const Ref<OghamOption> opt = current_active_options[i];
        if (opt.is_valid() && opt->get_tag() == option_tag)
            return opt;
    }
    return Ref<OghamOption>();
}

Ref<OghamOption> OghamSession::find_any_option(uint64_t option_tag) const
{
    for (int64_t i = 0; i < current_all_options.size(); ++i)
    {
        const Ref<OghamOption> opt = current_all_options[i];
        if (opt.is_valid() && opt->get_tag() == option_tag)
            return opt;
    }
    return Ref<OghamOption>();
}

bool OghamSession::choose(uint64_t option_tag)
{
    if (!is_active_flag)
        return false;

    const Ref<OghamOption> chosen = find_active_option(option_tag);
    if (chosen.is_null())
        return false;

    emit_signal("choice", static_cast<int64_t>(option_tag));

    apply_operations(chosen->get_operations(), state);

    // Record the choice: the entry's own tag is set (as a state variable) to the
    // id of the chosen option. ReturnTo uses this to know what was picked at a node.
    if (state.is_valid())
        state->call("apply", static_cast<int64_t>(current_entry_id), 0 /* ARITHMETIC_SET */, static_cast<int64_t>(option_tag));

    Dictionary hist;
    hist["entry_id"] = static_cast<int64_t>(current_entry_id);
    hist["selected_option"] = static_cast<int64_t>(option_tag);
    history.push_back(hist);

    if (!chosen->has_target())
    {
        close_internal();
        return true;
    }

    const Ref<OghamEntry> target = story.is_valid() ? story->find_entry(chosen->get_target_entry()) : Ref<OghamEntry>();
    if (target.is_null())
    {
        // Dangling reference (graph authoring error) — close cleanly, not a failure.
        close_internal();
        return true;
    }

    return enter_entry(target);
}

void OghamSession::close()
{
    if (is_active_flag)
        close_internal();
}

bool OghamSession::resume()
{
    if (story.is_null() || current_entry_id == 0)
        return false;

    const Ref<OghamEntry> entry = story->find_entry(current_entry_id);
    if (entry.is_null() || entry->is_fork())
        return false;

    ensure_state();
    current_entry = entry;

    TypedArray<OghamOption> active, all;
    build_options(entry, active, all);
    current_active_options = active;
    current_all_options = all;
    is_active_flag = true;

    emit_signal("entered", static_cast<int64_t>(current_entry_id));
    return true;
}

bool OghamSession::return_to(uint64_t entry_tag)
{
    if (story.is_null())
        return false;

    const Ref<OghamEntry> entry = story->find_entry(entry_tag);
    if (entry.is_null())
        return false;

    ensure_state();
    const PackedInt64Array descendants = story->collect_descendants(entry_tag);
    for (int64_t i = 0; i < descendants.size(); ++i)
        state->call("remove_tag", descendants[i]);

    return enter_entry(entry);
}

bool OghamSession::get_is_active() const { return is_active_flag; }
uint64_t OghamSession::get_current_entry_tag() const { return current_entry_id; }

int OghamSession::get_current_content_count() const
{
    return current_entry.is_valid() ? current_entry->get_content_count() : 0;
}

String OghamSession::get_current_text(int index) const
{
    if (current_entry.is_null())
        return String();
    const Ref<OghamContentKey> key = current_entry->get_content_key(index);
    return key.is_valid() ? key->resolve_text() : String();
}

Ref<Resource> OghamSession::get_current_asset(int index) const
{
    if (current_entry.is_null())
        return Ref<Resource>();
    const Ref<OghamContentKey> key = current_entry->get_content_key(index);
    return key.is_valid() ? key->resolve_asset() : Ref<Resource>();
}

PackedInt64Array OghamSession::get_current_options() const
{
    PackedInt64Array result;
    for (int64_t i = 0; i < current_active_options.size(); ++i)
    {
        const Ref<OghamOption> opt = current_active_options[i];
        if (opt.is_valid())
            result.push_back(static_cast<int64_t>(opt->get_tag()));
    }
    return result;
}

PackedInt64Array OghamSession::get_current_all_options() const
{
    PackedInt64Array result;
    for (int64_t i = 0; i < current_all_options.size(); ++i)
    {
        const Ref<OghamOption> opt = current_all_options[i];
        if (opt.is_valid())
            result.push_back(static_cast<int64_t>(opt->get_tag()));
    }
    return result;
}

bool OghamSession::is_option_active(uint64_t option_tag) const
{
    return find_active_option(option_tag).is_valid();
}

String OghamSession::get_option_text(uint64_t option_tag) const
{
    const Ref<OghamOption> opt = find_any_option(option_tag);
    return opt.is_valid() ? opt->resolve_text() : String();
}

Array OghamSession::get_history() const { return history; }

Dictionary OghamSession::snapshot(const String &save_name) const
{
    Dictionary d;
    d["name"] = save_name;
    d["current_entry_id"] = static_cast<int64_t>(current_entry_id);
    d["state"] = state.is_valid() ? state->call("get_all") : Variant(Dictionary());
    d["history"] = history;
    return d;
}

bool OghamSession::restore(const Dictionary &save_state)
{
    ensure_state();
    close_internal();

    current_entry_id = static_cast<uint64_t>(int64_t(save_state.get("current_entry_id", 0)));

    state->call("clear");
    const Dictionary state_dict = save_state.get("state", Dictionary());
    const Array keys = state_dict.keys();
    for (int64_t i = 0; i < keys.size(); ++i)
    {
        const Variant key = keys[i];
        state->call("apply", key, 0 /* ARITHMETIC_SET */, state_dict[key]);
    }

    history = save_state.get("history", Array());
    return true;
}

void OghamSession::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_story", "story"), &OghamSession::set_story);
    ClassDB::bind_method(D_METHOD("get_story"), &OghamSession::get_story);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "story", PROPERTY_HINT_RESOURCE_TYPE, "OghamStory"), "set_story", "get_story");

    ClassDB::bind_method(D_METHOD("get_state"), &OghamSession::get_state);

    ClassDB::bind_method(D_METHOD("enter", "entry_tag"), &OghamSession::enter);
    ClassDB::bind_method(D_METHOD("choose", "option_tag"), &OghamSession::choose);
    ClassDB::bind_method(D_METHOD("close"), &OghamSession::close);
    ClassDB::bind_method(D_METHOD("resume"), &OghamSession::resume);
    ClassDB::bind_method(D_METHOD("return_to", "entry_tag"), &OghamSession::return_to);

    ClassDB::bind_method(D_METHOD("get_is_active"), &OghamSession::get_is_active);
    ClassDB::bind_method(D_METHOD("get_current_entry_tag"), &OghamSession::get_current_entry_tag);
    ClassDB::bind_method(D_METHOD("get_current_content_count"), &OghamSession::get_current_content_count);
    ClassDB::bind_method(D_METHOD("get_current_text", "index"), &OghamSession::get_current_text);
    ClassDB::bind_method(D_METHOD("get_current_asset", "index"), &OghamSession::get_current_asset);
    ClassDB::bind_method(D_METHOD("get_current_options"), &OghamSession::get_current_options);
    ClassDB::bind_method(D_METHOD("get_current_all_options"), &OghamSession::get_current_all_options);
    ClassDB::bind_method(D_METHOD("is_option_active", "option_tag"), &OghamSession::is_option_active);
    ClassDB::bind_method(D_METHOD("get_option_text", "option_tag"), &OghamSession::get_option_text);
    ClassDB::bind_method(D_METHOD("get_history"), &OghamSession::get_history);

    ClassDB::bind_method(D_METHOD("snapshot", "save_name"), &OghamSession::snapshot);
    ClassDB::bind_method(D_METHOD("restore", "save_state"), &OghamSession::restore);

    ADD_SIGNAL(MethodInfo("entered", PropertyInfo(Variant::INT, "entry_tag")));
    ADD_SIGNAL(MethodInfo("choice", PropertyInfo(Variant::INT, "option_tag")));
    ADD_SIGNAL(MethodInfo("closed"));
}
