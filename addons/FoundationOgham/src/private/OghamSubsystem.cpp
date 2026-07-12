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

#include "OghamSubsystem.h"

#include "Storyteller.h"

std::string OghamSubsystem::display_name() const
{
    return "Ogham";
}

std::vector<std::pair<std::string, std::string>> OghamSubsystem::debug_info() const
{
    std::vector<std::pair<std::string, std::string>> result;

    Storyteller *storyteller = Storyteller::get_singleton();
    if (storyteller == nullptr)
    {
        result.emplace_back("Storyteller", "not constructed");
        return result;
    }

    result.emplace_back("Sessions", std::to_string(storyteller->session_count()));
    return result;
}
