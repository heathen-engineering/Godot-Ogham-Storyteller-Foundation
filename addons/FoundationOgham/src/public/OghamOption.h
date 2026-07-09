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

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class OghamSession;

/// <summary>
/// A single choice on an OghamEntry: a display label, a target entry to navigate to
/// (empty = close the conversation), guard conditions, and side-effect operations.
///
/// conditions/operations are plain Arrays of GameplayTagCondition/GameplayTagOperation
/// (Foundation GameplayTags Resources) rather than TypedArray — Ogham has no C++
/// compile-time dependency on that extension, only a runtime one (see OghamInterop.h),
/// so the element type is expressed as an Inspector hint string, not a C++ template
/// argument. Evaluated/applied via Variant calls (evaluate_all/apply) at runtime.
///
/// Ogham has no condition/operation logic of its own — every guard and every side
/// effect on an option IS a GameplayTagCondition/GameplayTagOperation, applied to the
/// owning OghamSession's GameplayTagCollection narrative state. Ported from
/// Unity-Ogham-Storyteller-Foundation's DialogueOption.
/// </summary>
class OghamOption : public Resource
{
    GDCLASS(OghamOption, Resource);

private:
    String tag_name;
    String target_entry_path;
    Ref<Resource> text_key; // LexiconText
    Array conditions;       // Array[GameplayTagCondition]
    Array operations;       // Array[GameplayTagOperation]
    bool synthesized_from_inline_link = false;
    String inline_link_source_key_path;

public:
    OghamOption() = default;

    void set_tag_name(const String &v);
    String get_tag_name() const;
    uint64_t get_tag() const;

    void set_target_entry_path(const String &v);
    String get_target_entry_path() const;
    uint64_t get_target_entry() const;
    bool has_target() const;

    void set_text_key(const Ref<Resource> &v);
    Ref<Resource> get_text_key() const;

    void set_conditions(const Array &v);
    Array get_conditions() const;
    void add_condition(const Ref<Resource> &condition);

    void set_operations(const Array &v);
    Array get_operations() const;
    void add_operation(const Ref<Resource> &operation);

    void set_synthesized_from_inline_link(bool v);
    bool get_synthesized_from_inline_link() const;

    void set_inline_link_source_key_path(const String &v);
    String get_inline_link_source_key_path() const;

    /// Resolves the option's display text via text_key, or falls back to tag_name.
    String resolve_text() const;

    /// Evaluates all conditions against the collection (AND-before-OR-before-XOR).
    /// Empty conditions list is unconditionally true.
    bool is_active(const Ref<RefCounted> &collection) const;

protected:
    static void _bind_methods();
};
