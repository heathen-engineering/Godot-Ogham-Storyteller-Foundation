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

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "OghamSession.h"
#include "OghamStory.h"

using namespace godot;

/// <summary>
/// In-editor dialogue playtester — steps a live OghamSession against the graph's
/// currently loaded (and possibly unsaved) documents, with no build step and no game
/// simulation. Ported from Unity-Ogham-Storyteller-Foundation's OghamPlayWindow — same
/// three-column layout (Narrative State / Content / History) and interaction model,
/// but content rendering goes through OghamInlineLinkParser::to_bbcode() into a real
/// RichTextLabel (bbcode_enabled, meta_clicked) instead of Unity's manual
/// line-by-line regex-segment IMGUI rendering, since Godot's RichTextLabel already
/// covers that ground natively. Non-modal (Unity's own Open() calls Show(), not
/// ShowModal() despite its class doc comment claiming otherwise) — lets the graph
/// stay open alongside it. Lazy-built on first open() call, single instance (opening
/// a new one closes any existing one), matching this codebase's established
/// popup-window contract elsewhere.
/// </summary>
class OghamPlayWindow : public Window
{
	GDCLASS(OghamPlayWindow, Window);

private:
	bool built = false;

	Dictionary all_story_data; // cached from open()'s param, re-read by Reset so edits made while the window is open are picked up
	Ref<OghamStory> story;
	Ref<OghamSession> session;
	Dictionary tag_names; // tag hash (as int64) -> display path, built from every merged entry/option

	PackedStringArray start_tag_paths;
	int start_tag_idx = 0;

	// UI
	OptionButton *start_tag_option = nullptr;
	VBoxContainer *state_list = nullptr;
	VBoxContainer *content_box = nullptr;
	VBoxContainer *history_list = nullptr;

	void ensure_built();
	void rebuild_start_tag_list();
	void reset_story();
	void refresh_content();
	void refresh_state_panel();
	void refresh_history_panel();

	String resolve_tag_name(uint64_t id) const;
	Ref<OghamOption> find_active_option_by_tag_path(const String &tag_path) const;

	void _on_start_pressed();
	void _on_reset_pressed();
	void _on_close_pressed();
	void _on_content_meta_clicked(const Variant &meta);
	void _on_session_entered(int64_t entry_tag);
	void _on_session_closed();

public:
	OghamPlayWindow() = default;

	/// all_story_data: Dictionary{path (String) -> Ref<OghamDocument>} — the graph
	/// view's own _all_story_data, merged into one combined story definition (see
	/// .cpp). Reads each document's CURRENT in-memory state (OghamDocument::to_json_text()),
	/// including unsaved edits, not just what's on disk.
	void open(const Dictionary &all_story_data);

protected:
	static void _bind_methods();
};
