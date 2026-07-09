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
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>

#include <string>
#include <unordered_map>

using namespace godot;

/// <summary>
/// Engine singleton ("OghamVariables") resolving "@Token(Tag.Path, args...)" markup
/// embedded in content text — distinct from the "[display](Ogham://Tag)" link syntax
/// (see OghamInlineLinkParser). Both read the same GameplayTagCollection narrative
/// state; call interpolate() before to_bbcode() when rendering (matches Unity's
/// OghamLinkFormatter call order).
///
/// Built-in tokens: @String, @Float, @Double, @Long, @Ulong, @Int, @UInt — read the
/// tag's stored value off a GameplayTagCollection via Variant calls (get_value/
/// get_float/get_double/get_long/get_int). @String treats the stored value as a
/// Lexicon key hash and resolves localised text. Numeric tokens accept an optional
/// second argument as a decimal-places count, e.g. "@Float(Shop.Total, 2)".
///
/// Unresolved/unknown tokens are left untouched in the text — a visible
/// authoring-error signal, matching Unity's OghamVariables.
/// </summary>
class OghamVariables : public Object
{
    GDCLASS(OghamVariables, Object);

private:
    static OghamVariables *singleton;
    std::unordered_map<std::string, Callable> custom_variables;

    String resolve_token(const String &name, const String &tag_path, const String &extra_arg, const Ref<RefCounted> &state) const;

public:
    OghamVariables();
    ~OghamVariables() override;

    static OghamVariables *get_singleton();

    /// Registers a custom token handler: callback(tag_path: String, extra_arg: String,
    /// state: GameplayTagCollection) -> String. Overrides a built-in of the same name.
    void register_variable(const String &name, const Callable &callback);
    void unregister_variable(const String &name);
    void reset_to_defaults();

    /// Replaces every "@Token(Tag.Path[, arg])" span in 'text' with its resolved
    /// value. Unrecognised tokens are left untouched.
    String interpolate(const String &text, const Ref<RefCounted> &state) const;

protected:
    static void _bind_methods();
};
