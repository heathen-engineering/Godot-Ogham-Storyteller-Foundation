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

#include "editor/OghamForkValidator.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace
{
	// Mirrors Unity's OghamForkValidator.ReachesSelf exactly: DFS from `from`
	// over the fork-only adjacency, true if `target` is reachable (i.e. `from`
	// sits on a cycle back to itself when called with from == target).
	bool reaches_self(const std::string &from, const std::string &target,
		const std::unordered_map<std::string, std::vector<std::string>> &adjacency,
		std::unordered_set<std::string> &visited)
	{
		auto it = adjacency.find(from);
		if (it == adjacency.end())
			return false;
		for (const std::string &n : it->second)
		{
			if (n == target)
				return true;
			if (visited.insert(n).second && reaches_self(n, target, adjacency, visited))
				return true;
		}
		return false;
	}
}

namespace OghamForkValidator
{
	PackedStringArray find_cyclic_forks(const Array &entries)
	{
		PackedStringArray result;

		// Index Fork entries by tag.
		std::unordered_map<std::string, Dictionary> forks;
		for (int i = 0; i < entries.size(); i++)
		{
			Dictionary entry = entries[i];
			if (String(entry.get("nodeMode", "")) != "Fork")
				continue;
			std::string tag = String(entry.get("tag", "")).utf8().get_data();
			if (!tag.empty())
				forks[tag] = entry;
		}
		if (forks.empty())
			return result;

		// Build the fork-only adjacency: a Fork's option targets that are
		// themselves Forks. Routes to a Content node or to no target are
		// terminal and ignored — they are always valid.
		std::unordered_map<std::string, std::vector<std::string>> adjacency;
		for (const auto &pair : forks)
		{
			std::vector<std::string> next;
			Array options = pair.second.get("options", Array());
			for (int i = 0; i < options.size(); i++)
			{
				Dictionary opt = options[i];
				std::string target = String(opt.get("targetTag", "")).utf8().get_data();
				if (!target.empty() && forks.count(target))
					next.push_back(target);
			}
			adjacency[pair.first] = next;
		}

		// A Fork is unsafe when a path of Forks leaving it can return to it.
		for (const auto &pair : forks)
		{
			std::unordered_set<std::string> visited;
			if (reaches_self(pair.first, pair.first, adjacency, visited))
				result.push_back(String(pair.first.c_str()));
		}

		return result;
	}
}
