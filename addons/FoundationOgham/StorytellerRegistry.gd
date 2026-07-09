# Copyright (c) 2026 Heathen Engineering Limited
# Irish Registered Company #556277
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

extends Node

## Drop into a scene to open an OghamStory session at boot. On _ready(), opens (or
## finds) the Storyteller session for story_tag, bound to the given story definition.
## Mirrors Unity-Ogham-Storyteller-Foundation's StorytellerRegistry.
@export var story_tag: String = ""
@export var story: OghamStory
@export var set_as_main: bool = true

func _ready() -> void:
	if story_tag.is_empty() or story == null:
		return
	Engine.get_singleton("Storyteller").open_session(story_tag, story, set_as_main)
