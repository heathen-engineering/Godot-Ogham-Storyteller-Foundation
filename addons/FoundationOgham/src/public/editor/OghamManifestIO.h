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
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "editor/OghamDocument.h"

using namespace godot;

/// <summary>
/// Static entry point for opening/discovering .ogham files — the C++ replacement for
/// OghamManifestIO.gd. Pure static-method utility (no instance state), same pattern as
/// OghamInlineLinkParser. load_manifest() now returns an OghamDocument handle instead
/// of a raw Dictionary — see OghamDocument.h for every other accessor that used to be
/// a static OghamManifestIO.gd function taking a `data: Dictionary` first argument.
/// </summary>
class OghamManifestIO : public Object
{
    GDCLASS(OghamManifestIO, Object);

public:
    /// Parses the .ogham JSON at 'path' into a live OghamDocument. Returns a document
    /// with zero entries (not null) if the file is missing or isn't valid JSON —
    /// matches OghamManifestIO.gd's load_manifest() falling back to {} in the same
    /// situations.
    static Ref<OghamDocument> load_manifest(const String &path);

    /// Recursively finds every ".ogham" file under 'root' (default res://), skipping
    /// .godot — matches Unity's editor auto-discovering every story in the project.
    static PackedStringArray find_ogham_files(const String &root);

protected:
    static void _bind_methods();
};
