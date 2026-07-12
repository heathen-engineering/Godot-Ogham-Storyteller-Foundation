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

#include "Storyteller.h"
#include "OghamInterop.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

Storyteller *Storyteller::singleton = nullptr;

Storyteller::Storyteller()
{
    singleton = this;
}

Storyteller::~Storyteller()
{
    if (singleton == this)
        singleton = nullptr;
}

Storyteller *Storyteller::get_singleton() { return singleton; }

Ref<OghamSession> Storyteller::open_session(const String &story_tag, const Ref<OghamStory> &story, bool set_as_main)
{
    const uint64_t id = OghamInterop::hash_tag(story_tag);

    auto it = sessions.find(id);
    if (it != sessions.end())
    {
        if (set_as_main)
            main_session_id = id;
        return it->second;
    }

    Ref<OghamSession> session;
    session.instantiate();
    session->set_story(story);
    sessions[id] = session;

    if (set_as_main || main_session_id == 0)
        main_session_id = id;

    return session;
}

Ref<OghamSession> Storyteller::get_session(const String &story_tag) const
{
    auto it = sessions.find(OghamInterop::hash_tag(story_tag));
    return (it != sessions.end()) ? it->second : Ref<OghamSession>();
}

bool Storyteller::has_session(const String &story_tag) const
{
    return sessions.find(OghamInterop::hash_tag(story_tag)) != sessions.end();
}

void Storyteller::forget_session(const String &story_tag)
{
    const uint64_t id = OghamInterop::hash_tag(story_tag);
    sessions.erase(id);
    if (main_session_id == id)
        main_session_id = 0;
}

Ref<OghamSession> Storyteller::get_main_session() const
{
    if (main_session_id == 0)
        return Ref<OghamSession>();
    auto it = sessions.find(main_session_id);
    return (it != sessions.end()) ? it->second : Ref<OghamSession>();
}

void Storyteller::set_main_session(const String &story_tag)
{
    main_session_id = OghamInterop::hash_tag(story_tag);
}

int Storyteller::session_count() const
{
    return int(sessions.size());
}

void Storyteller::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("open_session", "story_tag", "story", "set_as_main"), &Storyteller::open_session, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_session", "story_tag"), &Storyteller::get_session);
    ClassDB::bind_method(D_METHOD("has_session", "story_tag"), &Storyteller::has_session);
    ClassDB::bind_method(D_METHOD("forget_session", "story_tag"), &Storyteller::forget_session);
    ClassDB::bind_method(D_METHOD("get_main_session"), &Storyteller::get_main_session);
    ClassDB::bind_method(D_METHOD("set_main_session", "story_tag"), &Storyteller::set_main_session);
}
