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

#include "OghamOption.h"
#include "OghamInterop.h"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void OghamOption::set_tag_name(const String &v) { tag_name = v; }
String OghamOption::get_tag_name() const { return tag_name; }
uint64_t OghamOption::get_tag() const { return OghamInterop::hash_tag(tag_name); }

void OghamOption::set_target_entry_path(const String &v) { target_entry_path = v; }
String OghamOption::get_target_entry_path() const { return target_entry_path; }
uint64_t OghamOption::get_target_entry() const { return OghamInterop::hash_tag(target_entry_path); }
bool OghamOption::has_target() const { return !target_entry_path.is_empty(); }

void OghamOption::set_text_key(const Ref<Resource> &v) { text_key = v; }
Ref<Resource> OghamOption::get_text_key() const { return text_key; }

void OghamOption::set_conditions(const Array &v) { conditions = v; }
Array OghamOption::get_conditions() const { return conditions; }
void OghamOption::add_condition(const Ref<Resource> &condition) { conditions.push_back(condition); }

void OghamOption::set_operations(const Array &v) { operations = v; }
Array OghamOption::get_operations() const { return operations; }
void OghamOption::add_operation(const Ref<Resource> &operation) { operations.push_back(operation); }

void OghamOption::set_synthesized_from_inline_link(bool v) { synthesized_from_inline_link = v; }
bool OghamOption::get_synthesized_from_inline_link() const { return synthesized_from_inline_link; }

void OghamOption::set_inline_link_source_key_path(const String &v) { inline_link_source_key_path = v; }
String OghamOption::get_inline_link_source_key_path() const { return inline_link_source_key_path; }

String OghamOption::resolve_text() const
{
    if (text_key.is_valid())
    {
        const String resolved = text_key->call("resolve");
        if (!resolved.is_empty())
            return resolved;
    }
    return tag_name;
}

bool OghamOption::is_active(const Ref<RefCounted> &collection) const
{
    if (conditions.is_empty())
        return true;
    const Variant result = ClassDB::class_call_static("GameplayTagCondition", "evaluate_all", conditions, collection);
    return result;
}

void OghamOption::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_tag_name", "name"), &OghamOption::set_tag_name);
    ClassDB::bind_method(D_METHOD("get_tag_name"), &OghamOption::get_tag_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tag_name"), "set_tag_name", "get_tag_name");

    ClassDB::bind_method(D_METHOD("get_tag"), &OghamOption::get_tag);

    ClassDB::bind_method(D_METHOD("set_target_entry_path", "path"), &OghamOption::set_target_entry_path);
    ClassDB::bind_method(D_METHOD("get_target_entry_path"), &OghamOption::get_target_entry_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_entry_path"), "set_target_entry_path", "get_target_entry_path");

    ClassDB::bind_method(D_METHOD("get_target_entry"), &OghamOption::get_target_entry);
    ClassDB::bind_method(D_METHOD("has_target"), &OghamOption::has_target);

    ClassDB::bind_method(D_METHOD("set_text_key", "value"), &OghamOption::set_text_key);
    ClassDB::bind_method(D_METHOD("get_text_key"), &OghamOption::get_text_key);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "text_key", PROPERTY_HINT_RESOURCE_TYPE, "LexiconText"), "set_text_key", "get_text_key");

    ClassDB::bind_method(D_METHOD("set_conditions", "value"), &OghamOption::set_conditions);
    ClassDB::bind_method(D_METHOD("get_conditions"), &OghamOption::get_conditions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditions", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagCondition")),
        "set_conditions", "get_conditions");
    ClassDB::bind_method(D_METHOD("add_condition", "condition"), &OghamOption::add_condition);

    ClassDB::bind_method(D_METHOD("set_operations", "value"), &OghamOption::set_operations);
    ClassDB::bind_method(D_METHOD("get_operations"), &OghamOption::get_operations);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "operations", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagOperation")),
        "set_operations", "get_operations");
    ClassDB::bind_method(D_METHOD("add_operation", "operation"), &OghamOption::add_operation);

    ClassDB::bind_method(D_METHOD("set_synthesized_from_inline_link", "value"), &OghamOption::set_synthesized_from_inline_link);
    ClassDB::bind_method(D_METHOD("get_synthesized_from_inline_link"), &OghamOption::get_synthesized_from_inline_link);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "synthesized_from_inline_link"), "set_synthesized_from_inline_link", "get_synthesized_from_inline_link");

    ClassDB::bind_method(D_METHOD("set_inline_link_source_key_path", "path"), &OghamOption::set_inline_link_source_key_path);
    ClassDB::bind_method(D_METHOD("get_inline_link_source_key_path"), &OghamOption::get_inline_link_source_key_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "inline_link_source_key_path"), "set_inline_link_source_key_path", "get_inline_link_source_key_path");

    ClassDB::bind_method(D_METHOD("resolve_text"), &OghamOption::resolve_text);
    ClassDB::bind_method(D_METHOD("is_active", "collection"), &OghamOption::is_active);
}
