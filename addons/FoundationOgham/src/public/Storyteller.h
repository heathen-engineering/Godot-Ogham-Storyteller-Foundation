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
#include <godot_cpp/variant/string.hpp>

#include "OghamSession.h"

#include <unordered_map>

using namespace godot;

/// <summary>
/// Engine singleton ("Storyteller") — the global entry point for starting and finding
/// stories. Holds one OghamSession per story tag, so many stories can be open and
/// independently stepped at once (e.g. a main quest story plus several NPC banter
/// stories). The first-opened session becomes "main" automatically; open_session's
/// set_as_main flag or set_main_session() can override that.
///
/// Ported from Unity-Ogham-Storyteller-Foundation's StorytellerSubsystem/Storyteller
/// pair, collapsed into a single global singleton — Godot has no per-World subsystem
/// framework equivalent to port, and this project doesn't need multi-world semantics.
/// </summary>
class Storyteller : public Object
{
    GDCLASS(Storyteller, Object);

private:
    static Storyteller *singleton;
    std::unordered_map<uint64_t, Ref<OghamSession>> sessions;
    uint64_t main_session_id = 0;

public:
    Storyteller();
    ~Storyteller() override;

    static Storyteller *get_singleton();

    /// Returns the existing session for story_tag if one is open, otherwise creates
    /// one bound to 'story'. The first session ever opened becomes main automatically.
    Ref<OghamSession> open_session(const String &story_tag, const Ref<OghamStory> &story, bool set_as_main);

    Ref<OghamSession> get_session(const String &story_tag) const;
    bool has_session(const String &story_tag) const;

    /// Closes and forgets the session for story_tag (does not call OghamSession::close()
    /// on it first — do that yourself if you want the "closed" signal to fire).
    void forget_session(const String &story_tag);

    Ref<OghamSession> get_main_session() const;
    void set_main_session(const String &story_tag);

protected:
    static void _bind_methods();
};
