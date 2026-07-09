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

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/// <summary>
/// Ogham has no C++ compile-time dependency on GameplayTags Foundation or Lexicon
/// Foundation — each is a separately loaded GDExtension, and cross-extension calls go
/// through Godot's Variant/Object API (the same boundary the C# facades already use),
/// not shared C++ headers or link-time symbols. This keeps every Foundation extension
/// independently buildable and loadable in any combination.
///
/// These helpers centralise that lookup. Every method returns nullptr (and callers treat
/// that as "not registered/absent") when the dependency extension isn't loaded.
/// </summary>
namespace OghamInterop
{
    inline Object *get_gameplay_tag_registry()
    {
        Engine *engine = Engine::get_singleton();
        if (engine == nullptr || !engine->has_singleton("GameplayTagRegistry"))
            return nullptr;
        return engine->get_singleton("GameplayTagRegistry");
    }

    inline Object *get_lexicon_registry()
    {
        Engine *engine = Engine::get_singleton();
        if (engine == nullptr || !engine->has_singleton("LexiconRegistry"))
            return nullptr;
        return engine->get_singleton("LexiconRegistry");
    }

    /// Hash via GameplayTagRegistry (falls back to LexiconRegistry, then 0) — both
    /// extensions share the same xxHash3-64 hash space by design, so either works.
    inline uint64_t hash_tag(const String &text)
    {
        if (text.is_empty())
            return 0;
        Object *registry = get_gameplay_tag_registry();
        if (registry == nullptr)
            registry = get_lexicon_registry();
        if (registry == nullptr)
        {
            UtilityFunctions::push_warning("Ogham: neither GameplayTagRegistry nor LexiconRegistry is loaded; hashing '", text, "' as 0.");
            return 0;
        }
        return static_cast<uint64_t>(static_cast<int64_t>(registry->call("hash", text)));
    }
} // namespace OghamInterop
