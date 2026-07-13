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

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

/// <summary>
/// Free functions for the project-wide "ogham/editor/key_labels"
/// ProjectSettings entry (position-based contentKey slot labels — Stage,
/// Narrator, Speaker Name, etc.). Originally ported from OghamKeyLabels.gd,
/// which has since been deleted as dead code (nothing referenced it once
/// this file existed — see OghamGraphNode, which reads one label by index,
/// and OghamKeyLabelsPopup, which does full get/set).
/// </summary>
namespace OghamKeyLabelsNative
{
    godot::PackedStringArray get_labels();
    void set_labels(const godot::PackedStringArray &labels);
    godot::String label_for_index(int index);
}
