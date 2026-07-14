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

#include "editor/OghamExportWindow.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "OghamInterop.h"
#include "OghamStory.h"
#include "editor/OghamDocument.h"
#include "editor/OghamKeyLabelsNative.h"
#include "editor/OghamScriptExporter.h"

using namespace godot;

void OghamExportWindow::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("open", "all_story_data"), &OghamExportWindow::open);
}

void OghamExportWindow::open(const Dictionary &p_all_story_data)
{
	ensure_built();
	all_story_data = p_all_story_data;
	rebuild_label_checks();
	rebuild_pick_list();
	popup_centered(Vector2i(420, 560));
}

void OghamExportWindow::ensure_built()
{
	if (built)
		return;
	built = true;

	set_title("Export VO Script");
	set_size(Vector2i(420, 560));
	set_min_size(Vector2i(380, 460));
	connect("close_requested", callable_mp(this, &OghamExportWindow::_on_close_requested));

	ScrollContainer *scroll = memnew(ScrollContainer);
	scroll->set_anchors_preset(Control::PRESET_FULL_RECT);
	add_child(scroll);

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll->add_child(root);

	// ── Format ───────────────────────────────────────────────────────────
	Label *format_label = memnew(Label);
	format_label->set_text("Output Format");
	root->add_child(format_label);

	format_option = memnew(OptionButton);
	format_option->add_item("CSV (.csv)");
	format_option->add_item("Markdown (.md)");
	format_option->add_item("HTML (.html)");
	format_option->add_item("Plain Text (.txt)");
	root->add_child(format_option);

	root->add_child(memnew(HSeparator));

	// ── Culture ──────────────────────────────────────────────────────────
	Label *culture_label = memnew(Label);
	culture_label->set_text("Culture Code (blank = raw keys, unresolved)");
	root->add_child(culture_label);

	culture_edit = memnew(LineEdit);
	Object *lexicon = OghamInterop::get_lexicon_registry();
	if (lexicon != nullptr)
		culture_edit->set_text(lexicon->call("get_active_culture"));
	culture_edit->set_placeholder("e.g. en-GB");
	root->add_child(culture_edit);

	root->add_child(memnew(HSeparator));

	// ── Options ──────────────────────────────────────────────────────────
	Label *options_label = memnew(Label);
	options_label->set_text("Options");
	root->add_child(options_label);

	strip_trailing_check = memnew(CheckBox);
	strip_trailing_check->set_text("Strip trailing navigation links");
	strip_trailing_check->set_tooltip_text("Removes [<- Back]-style links that appear only at the very end of a content key, after all spoken text. Links embedded inside spoken text are kept.");
	strip_trailing_check->set_pressed(true);
	root->add_child(strip_trailing_check);

	list_options_check = memnew(CheckBox);
	list_options_check->set_text("List player options");
	list_options_check->set_tooltip_text("Appends an Options section to each node showing the player choices that follow. Disable for a narrator-only script.");
	list_options_check->set_pressed(true);
	root->add_child(list_options_check);

	remove_formatting_check = memnew(CheckBox);
	remove_formatting_check->set_text("Remove content formatting");
	remove_formatting_check->set_tooltip_text("Strips markdown marks (* _ ` ~~) and non-speakable characters. Recommended for CSV/HTML/TXT; disable for Markdown to preserve rendering.");
	remove_formatting_check->set_pressed(true);
	root->add_child(remove_formatting_check);

	root->add_child(memnew(HSeparator));

	// ── Content Labels — the real filter, not just a caption (see
	// OghamScriptExporter.h's doc comment for why this differs from Unity). ──
	Label *labels_header = memnew(Label);
	labels_header->set_text("Content Labels to Export");
	root->add_child(labels_header);
	label_checks_box = memnew(VBoxContainer);
	root->add_child(label_checks_box);

	root->add_child(memnew(HSeparator));

	// ── Node selection mode ──────────────────────────────────────────────
	Label *mode_label = memnew(Label);
	mode_label->set_text("Nodes to Export");
	root->add_child(mode_label);
	mode_option = memnew(OptionButton);
	mode_option->add_item("All");
	mode_option->add_item("Pick");
	mode_option->connect("item_selected", callable_mp(this, &OghamExportWindow::_on_mode_changed));
	root->add_child(mode_option);

	pick_list_box = memnew(VBoxContainer);
	pick_list_box->set_visible(false);
	root->add_child(pick_list_box);

	root->add_child(memnew(HSeparator));

	Button *export_btn = memnew(Button);
	export_btn->set_text("Export...");
	export_btn->connect("pressed", callable_mp(this, &OghamExportWindow::_on_export_pressed));
	root->add_child(export_btn);

	save_dialog = memnew(FileDialog);
	save_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
	save_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	save_dialog->connect("file_selected", callable_mp(this, &OghamExportWindow::_on_save_path_selected));
	add_child(save_dialog);
}

void OghamExportWindow::_on_close_requested()
{
	hide();
}

void OghamExportWindow::rebuild_label_checks()
{
	while (label_checks_box->get_child_count() > 0)
	{
		Node *child = label_checks_box->get_child(0);
		label_checks_box->remove_child(child);
		child->queue_free();
	}
	label_checks.clear();

	PackedStringArray labels = OghamKeyLabelsNative::get_labels();
	bool any_named = false;
	for (int i = 0; i < labels.size(); i++)
	{
		String label = labels[i];
		if (label.is_empty())
			continue;
		any_named = true;
		CheckBox *check = memnew(CheckBox);
		check->set_text(label);
		check->set_pressed(true);
		label_checks_box->add_child(check);
		label_checks[std::string(label.utf8().get_data())] = check;
	}

	if (!any_named)
	{
		Label *empty_label = memnew(Label);
		empty_label->set_text("No Content Labels defined yet — every content slot will be exported.");
		empty_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
		label_checks_box->add_child(empty_label);
	}
}

void OghamExportWindow::rebuild_pick_list()
{
	while (pick_list_box->get_child_count() > 0)
	{
		Node *child = pick_list_box->get_child(0);
		pick_list_box->remove_child(child);
		child->queue_free();
	}
	pick_checks.clear();

	Array paths = all_story_data.keys();
	for (int p = 0; p < paths.size(); p++)
	{
		Ref<OghamDocument> doc = all_story_data[paths[p]];
		if (doc.is_null())
			continue;
		Array entries = doc->get_entries();
		for (int e = 0; e < entries.size(); e++)
		{
			Dictionary entry = entries[e];
			if (String(entry.get("nodeMode", "")) == "Fork")
				continue;
			String tag = entry.get("tag", "");
			if (tag.is_empty())
				continue;
			CheckBox *check = memnew(CheckBox);
			check->set_text(tag);
			check->set_pressed(true);
			pick_list_box->add_child(check);
			pick_checks[std::string(tag.utf8().get_data())] = check;
		}
	}
}

void OghamExportWindow::_on_mode_changed(int idx)
{
	pick_list_box->set_visible(idx == 1);
}

TypedArray<OghamEntry> OghamExportWindow::merge_entries() const
{
	TypedArray<OghamEntry> merged;
	Array paths = all_story_data.keys();
	bool pick_mode = mode_option->get_selected() == 1;

	for (int p = 0; p < paths.size(); p++)
	{
		Ref<OghamDocument> doc = all_story_data[paths[p]];
		if (doc.is_null())
			continue;
		Ref<OghamStory> parsed = OghamStory::parse_manifest(doc->to_json_text());
		TypedArray<OghamEntry> entries = parsed->get_entries();
		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			if (entry->is_fork())
				continue;
			if (pick_mode)
			{
				auto it = pick_checks.find(std::string(entry->get_tag_name().utf8().get_data()));
				if (it == pick_checks.end() || !it->second->is_pressed())
					continue;
			}
			merged.push_back(entry);
		}
	}
	return merged;
}

Dictionary OghamExportWindow::gather_notes(const TypedArray<OghamEntry> &entries) const
{
	Dictionary notes_by_tag;
	Array paths = all_story_data.keys();
	for (int p = 0; p < paths.size(); p++)
	{
		Ref<OghamDocument> doc = all_story_data[paths[p]];
		if (doc.is_null())
			continue;
		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			String tag = entry->get_tag_name();
			String notes = doc->get_director_notes(tag);
			if (!notes.is_empty())
				notes_by_tag[tag] = notes;
		}
	}
	return notes_by_tag;
}

void OghamExportWindow::_on_export_pressed()
{
	static const char *extensions[] = { "csv", "md", "html", "txt" };
	int fmt_idx = format_option->get_selected();
	save_dialog->clear_filters();
	save_dialog->add_filter(String("*.") + extensions[fmt_idx]);
	save_dialog->set_current_file(String("VOScript.") + extensions[fmt_idx]);
	save_dialog->popup_centered_ratio(0.6);
}

void OghamExportWindow::_on_save_path_selected(const String &path)
{
	TypedArray<OghamEntry> entries = merge_entries();
	Dictionary notes_by_tag = gather_notes(entries);

	OghamScriptExporter::ExportOptions opts;
	opts.format = static_cast<OghamScriptExporter::Format>(format_option->get_selected());
	opts.strip_trailing_links = strip_trailing_check->is_pressed();
	opts.list_options = list_options_check->is_pressed();
	opts.remove_formatting = remove_formatting_check->is_pressed();

	PackedStringArray enabled_labels;
	for (const auto &pair : label_checks)
		if (pair.second->is_pressed())
			enabled_labels.push_back(String(pair.first.c_str()));
	opts.enabled_labels = enabled_labels;

	// Resolve via the real LexiconRegistry, temporarily switching to the
	// requested culture and restoring whatever was active afterward — see
	// OghamScriptExporter.h's doc comment for why this isn't a second
	// hand-rolled .helex parser.
	String culture = culture_edit->get_text().strip_edges();
	Object *lexicon = OghamInterop::get_lexicon_registry();
	String previous_culture;
	bool switched = false;
	if (lexicon != nullptr && !culture.is_empty())
	{
		previous_culture = lexicon->call("get_active_culture");
		lexicon->call("use_culture", culture);
		switched = true;
		// Callable::bind() appends bound args AFTER the call-supplied ones,
		// so this lambda takes (key, registry) in that order, not the more
		// natural (registry, key) — resolve_key.call(key_or_value) below
		// then invokes it as (key_or_value, lexicon).
		opts.resolve_key = callable_mp_static(+[](const String &key, Object *registry) -> String {
			uint64_t hash = OghamInterop::hash_tag(key);
			return registry->call("resolve_string", int64_t(hash));
		}).bind(lexicon);
	}

	String output = OghamScriptExporter::export_story(entries, notes_by_tag, opts);

	if (switched)
		lexicon->call("use_culture", previous_culture);

	Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
	if (file.is_null())
	{
		UtilityFunctions::push_warning("Ogham Export: could not write ", path);
		return;
	}
	file->store_string(output);
	UtilityFunctions::print("[Ogham] Exported VO script: ", path, " (", entries.size(), " nodes)");
}
