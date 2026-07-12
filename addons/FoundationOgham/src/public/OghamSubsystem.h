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

#include <gameframework/Subsystem.h>

/// <summary>
/// Global framework Subsystem exposing Storyteller's state to Godot-Game-
/// Framework's Subsystems dock and to other gems' depends_on() ordering.
///
/// Deliberately Global scope, not World scope like Unity's original
/// StorytellerSubsystem — Storyteller.h's own header comment already
/// documents why: "Godot has no per-World subsystem framework equivalent to
/// port, and this project doesn't need multi-world semantics," collapsing
/// Unity's StorytellerSubsystem/Storyteller pair into one global singleton.
/// This Subsystem reports on that same single global Storyteller; it does
/// not introduce World-scoped multi-instance behavior Ogham doesn't
/// currently have. If Ogham ever needs genuine per-World story isolation,
/// that's a real architectural change to Storyteller itself first, not
/// something to retrofit into this reporting shim.
///
/// Same read-only-reporting reasoning as GameplayTagsSubsystem/
/// LexiconSubsystem (see GameplayTagsSubsystem's header comment) — no
/// forced session reset in initialize().
/// </summary>
class OghamSubsystem : public gameframework::Subsystem
{
public:
    std::string display_name() const override;
    std::vector<std::pair<std::string, std::string>> debug_info() const override;
};
