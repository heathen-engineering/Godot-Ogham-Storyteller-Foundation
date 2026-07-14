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

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "OghamEntry.h"

using namespace godot;

/// <summary>
/// VO script exporter — direct port of Unity's OghamScriptExporter (CSV/Markdown/
/// HTML/Plain Text, shared text-cleaning pipeline), plus one thing Unity's version
/// never had: content is filtered by Content Label (OghamKeyLabelsNative), not just
/// captioned by it — a project can export "only Narration fields" instead of every
/// content-key slot unconditionally. Pure algorithm, no Godot object identity needed
/// (same shape as OghamForkValidator — a plain namespace, not a GDCLASS).
/// </summary>
namespace OghamScriptExporter
{
	enum Format
	{
		FORMAT_CSV = 0,
		FORMAT_MARKDOWN = 1,
		FORMAT_HTML = 2,
		FORMAT_PLAIN_TEXT = 3,
	};

	struct ExportOptions
	{
		Format format = FORMAT_CSV;
		bool strip_trailing_links = true;
		bool list_options = true;
		bool remove_formatting = true;
		// Subset of OghamKeyLabelsNative::get_labels() to include. Empty means
		// every label (and every unlabeled slot) is included — matches "no
		// filter configured yet" rather than "export nothing".
		PackedStringArray enabled_labels;
		// (String key_or_value) -> String. Called only for MODE_LOCALISED
		// content/option text, already bound by the caller to a specific
		// target culture (see OghamExportWindow) — invalid Callable means
		// raw key text is used unresolved.
		Callable resolve_key;
	};

	/// entries: every loaded OghamEntry to consider (Fork entries are always
	/// skipped, matching Unity). notes_by_tag: {String tag -> String director
	/// notes}, from OghamDocument::get_director_notes() per entry.
	String export_story(const TypedArray<OghamEntry> &entries, const Dictionary &notes_by_tag, const ExportOptions &opts);
}
