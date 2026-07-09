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

#include "OghamContentKey.h"
#include "OghamInterop.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void OghamContentKey::set_type(int v) { type = v; }
int OghamContentKey::get_type() const { return type; }

void OghamContentKey::set_mode(int v) { mode = v; }
int OghamContentKey::get_mode() const { return mode; }

void OghamContentKey::set_key_or_value(const String &v) { key_or_value = v; }
String OghamContentKey::get_key_or_value() const { return key_or_value; }

void OghamContentKey::set_asset_path(const String &v) { asset_path = v; }
String OghamContentKey::get_asset_path() const { return asset_path; }

void OghamContentKey::set_asset_sub_name(const String &v) { asset_sub_name = v; }
String OghamContentKey::get_asset_sub_name() const { return asset_sub_name; }

uint64_t OghamContentKey::get_hash() const
{
    return OghamInterop::hash_tag(key_or_value);
}

String OghamContentKey::resolve_text() const
{
    if (mode == MODE_LOCALISED)
    {
        Object *lexicon = OghamInterop::get_lexicon_registry();
        if (lexicon != nullptr)
        {
            const String resolved = lexicon->call("resolve_string", get_hash());
            if (!resolved.is_empty())
                return resolved;
        }
    }
    return key_or_value;
}

Ref<Resource> OghamContentKey::resolve_asset() const
{
    if (mode == MODE_LOCALISED)
    {
        Object *lexicon = OghamInterop::get_lexicon_registry();
        if (lexicon != nullptr)
        {
            const Variant result = lexicon->call("resolve_asset", get_hash());
            if (result.get_type() == Variant::OBJECT)
            {
                Ref<Resource> res = result;
                if (res.is_valid())
                    return res;
            }
        }
    }

    if (asset_path.is_empty())
        return Ref<Resource>();

    const String load_path = asset_sub_name.is_empty() ? asset_path : asset_path + String("::") + asset_sub_name;
    return ResourceLoader::get_singleton()->load(load_path);
}

void OghamContentKey::_bind_methods()
{
    BIND_ENUM_CONSTANT(CONTENT_TEXT);
    BIND_ENUM_CONSTANT(CONTENT_IMAGE);
    BIND_ENUM_CONSTANT(CONTENT_SPRITE);
    BIND_ENUM_CONSTANT(CONTENT_AUDIO);
    BIND_ENUM_CONSTANT(CONTENT_PACKED_SCENE);

    BIND_ENUM_CONSTANT(MODE_LOCALISED);
    BIND_ENUM_CONSTANT(MODE_LITERAL);
    BIND_ENUM_CONSTANT(MODE_INVARIANT);

    ClassDB::bind_method(D_METHOD("set_type", "type"), &OghamContentKey::set_type);
    ClassDB::bind_method(D_METHOD("get_type"), &OghamContentKey::get_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM, "Text,Image,Sprite,Audio,PackedScene"), "set_type", "get_type");

    ClassDB::bind_method(D_METHOD("set_mode", "mode"), &OghamContentKey::set_mode);
    ClassDB::bind_method(D_METHOD("get_mode"), &OghamContentKey::get_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Localised,Literal,Invariant"), "set_mode", "get_mode");

    ClassDB::bind_method(D_METHOD("set_key_or_value", "value"), &OghamContentKey::set_key_or_value);
    ClassDB::bind_method(D_METHOD("get_key_or_value"), &OghamContentKey::get_key_or_value);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "key_or_value"), "set_key_or_value", "get_key_or_value");

    ClassDB::bind_method(D_METHOD("set_asset_path", "path"), &OghamContentKey::set_asset_path);
    ClassDB::bind_method(D_METHOD("get_asset_path"), &OghamContentKey::get_asset_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "asset_path", PROPERTY_HINT_FILE), "set_asset_path", "get_asset_path");

    ClassDB::bind_method(D_METHOD("set_asset_sub_name", "name"), &OghamContentKey::set_asset_sub_name);
    ClassDB::bind_method(D_METHOD("get_asset_sub_name"), &OghamContentKey::get_asset_sub_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "asset_sub_name"), "set_asset_sub_name", "get_asset_sub_name");

    ClassDB::bind_method(D_METHOD("get_hash"), &OghamContentKey::get_hash);
    ClassDB::bind_method(D_METHOD("resolve_text"), &OghamContentKey::resolve_text);
    ClassDB::bind_method(D_METHOD("resolve_asset"), &OghamContentKey::resolve_asset);
}
