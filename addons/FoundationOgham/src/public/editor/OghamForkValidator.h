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

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

/// Validates that Fork routing terminates. Every path out of a Fork must resolve to
/// a Content node or to no target (ending the conversation); a chain of Forks that
/// loops back on itself never resolves and would spin forever at runtime. Ported
/// from Unity-Ogham-Storyteller-Foundation's OghamForkValidator.cs — same
/// fork-only-adjacency + reaches-self DFS algorithm, operating on the entry
/// Dictionary shape OghamGraphView already builds from one or more loaded
/// documents (see docs/extensions/09-ogham-editor-tooling.md in TheBarrow for the
/// design this was ported against). Editor-only, same as Unity's version — not
/// wired into OghamStory::parse_manifest()'s runtime load path.
namespace OghamForkValidator
{
	/// entries: Array of entry Dictionaries — each shaped like OghamDocument's own
	/// per-entry snapshot ("tag" String, "nodeMode" String, "options" Array of
	/// Dictionary each with a "targetTag" String). Works across every entry passed
	/// in regardless of which loaded document/file it came from, matching Unity's
	/// canvas-wide (not per-file) check.
	///
	/// Returns the tags of every Fork entry that lies on a fork-to-fork cycle — an
	/// empty array means all Fork routing is well-formed.
	PackedStringArray find_cyclic_forks(const Array &entries);
}
