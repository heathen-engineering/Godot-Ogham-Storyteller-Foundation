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

#include <string>
#include <unordered_map>

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "OghamEntry.h"

using namespace godot;

/// <summary>
/// VO script export window — picks format/options/Content-Label filter/node
/// set, then writes via OghamScriptExporter. Ported from Unity's
/// OghamExportWindow, with two deliberate differences: Content Label
/// checkboxes are a real filter here (see OghamScriptExporter.h's doc
/// comment — Unity's ContentLabels was caption-only), and the node-selection
/// mode is All/Pick only, not All/Selected/Pick — this codebase doesn't
/// track GraphEdit node selection anywhere yet, and adding that plumbing
/// solely for this window's "Selected" mode wasn't worth it versus just
/// using Pick for a manual subset.
/// </summary>
class OghamExportWindow : public Window
{
	GDCLASS(OghamExportWindow, Window);

private:
	bool built = false;
	Dictionary all_story_data; // path -> Ref<OghamDocument>, cached from open()

	OptionButton *format_option = nullptr;
	LineEdit *culture_edit = nullptr;
	CheckBox *strip_trailing_check = nullptr;
	CheckBox *list_options_check = nullptr;
	CheckBox *remove_formatting_check = nullptr;

	VBoxContainer *label_checks_box = nullptr;
	std::unordered_map<std::string, CheckBox *> label_checks;

	OptionButton *mode_option = nullptr; // 0 = All, 1 = Pick
	VBoxContainer *pick_list_box = nullptr;
	std::unordered_map<std::string, CheckBox *> pick_checks;

	FileDialog *save_dialog = nullptr;

	void ensure_built();
	void rebuild_label_checks();
	void rebuild_pick_list();
	TypedArray<OghamEntry> merge_entries() const;
	Dictionary gather_notes(const TypedArray<OghamEntry> &entries) const;

	void _on_mode_changed(int idx);
	void _on_export_pressed();
	void _on_save_path_selected(const String &path);
	void _on_close_requested();

public:
	OghamExportWindow() = default;

	void open(const Dictionary &p_all_story_data);

protected:
	static void _bind_methods();
};
