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

#include "OghamEntry.h"
#include "OghamInterop.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void OghamEntry::set_tag_name(const String &v) { tag_name = v; }
String OghamEntry::get_tag_name() const { return tag_name; }
uint64_t OghamEntry::get_tag() const { return OghamInterop::hash_tag(tag_name); }

void OghamEntry::set_mode(int v) { mode = v; }
int OghamEntry::get_mode() const { return mode; }
bool OghamEntry::is_fork() const { return mode == MODE_FORK; }

void OghamEntry::set_content_keys(const TypedArray<OghamContentKey> &v) { content_keys = v; }
TypedArray<OghamContentKey> OghamEntry::get_content_keys() const { return content_keys; }
int OghamEntry::get_content_count() const { return static_cast<int>(content_keys.size()); }

Ref<OghamContentKey> OghamEntry::get_content_key(int index) const
{
    if (index < 0 || index >= content_keys.size())
        return Ref<OghamContentKey>();
    return content_keys[index];
}

void OghamEntry::set_entry_operations(const Array &v) { entry_operations = v; }
Array OghamEntry::get_entry_operations() const { return entry_operations; }

void OghamEntry::set_options(const TypedArray<OghamOption> &v) { options = v; }
TypedArray<OghamOption> OghamEntry::get_options() const { return options; }

void OghamEntry::_bind_methods()
{
    BIND_ENUM_CONSTANT(MODE_CONTENT);
    BIND_ENUM_CONSTANT(MODE_FORK);

    ClassDB::bind_method(D_METHOD("set_tag_name", "name"), &OghamEntry::set_tag_name);
    ClassDB::bind_method(D_METHOD("get_tag_name"), &OghamEntry::get_tag_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tag_name"), "set_tag_name", "get_tag_name");

    ClassDB::bind_method(D_METHOD("get_tag"), &OghamEntry::get_tag);

    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &OghamEntry::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &OghamEntry::get_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Content,Fork"), "set_mode", "get_mode");
    ClassDB::bind_method(D_METHOD("is_fork"), &OghamEntry::is_fork);

    ClassDB::bind_method(D_METHOD("set_content_keys", "value"), &OghamEntry::set_content_keys);
    ClassDB::bind_method(D_METHOD("get_content_keys"), &OghamEntry::get_content_keys);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "content_keys", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "OghamContentKey")),
        "set_content_keys", "get_content_keys");
    ClassDB::bind_method(D_METHOD("get_content_count"), &OghamEntry::get_content_count);
    ClassDB::bind_method(D_METHOD("get_content_key", "index"), &OghamEntry::get_content_key);

    ClassDB::bind_method(D_METHOD("set_entry_operations", "value"), &OghamEntry::set_entry_operations);
    ClassDB::bind_method(D_METHOD("get_entry_operations"), &OghamEntry::get_entry_operations);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entry_operations", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagOperation")),
        "set_entry_operations", "get_entry_operations");

    ClassDB::bind_method(D_METHOD("set_options", "value"), &OghamEntry::set_options);
    ClassDB::bind_method(D_METHOD("get_options"), &OghamEntry::get_options);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "options", PROPERTY_HINT_ARRAY_TYPE,
                     vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "OghamOption")),
        "set_options", "get_options");
}
