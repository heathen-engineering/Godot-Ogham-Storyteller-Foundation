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

#include "editor/OghamKeyLabelsNative.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

namespace
{
    // A function-local static (not a namespace/file-scope global) — godot::String's
    // constructor calls into the GDExtension API, which isn't bound yet during the
    // shared library's static-initialization pass at dlopen() time (before Godot
    // calls this module's init callback). A file-scope `const String X = "...";`
    // here crashed with a null-pointer segfault during dlopen for exactly that
    // reason; a function-local static is only constructed on first CALL, by which
    // point the API is guaranteed bound.
    const String &setting_path()
    {
        static const String path = "ogham/editor/key_labels";
        return path;
    }
}

PackedStringArray OghamKeyLabelsNative::get_labels()
{
    ProjectSettings *ps = ProjectSettings::get_singleton();
    if (!ps->has_setting(setting_path()))
        return PackedStringArray();
    Variant value = ps->get_setting(setting_path());
    if (value.get_type() == Variant::PACKED_STRING_ARRAY)
        return value;
    if (value.get_type() == Variant::ARRAY)
        return PackedStringArray(Array(value));
    return PackedStringArray();
}

void OghamKeyLabelsNative::set_labels(const PackedStringArray &labels)
{
    ProjectSettings *ps = ProjectSettings::get_singleton();
    ps->set_setting(setting_path(), labels);
    ps->set_initial_value(setting_path(), PackedStringArray());
    ps->save();
}

String OghamKeyLabelsNative::label_for_index(int index)
{
    PackedStringArray labels = get_labels();
    if (index >= 0 && index < labels.size())
        return labels[index];
    return String();
}
