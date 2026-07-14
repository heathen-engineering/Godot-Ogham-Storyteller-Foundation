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

#include "editor/OghamPlayWindow.h"

#include <unordered_set>

#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "OghamInlineLinkParser.h"
#include "OghamInterop.h"
#include "OghamVariables.h"
#include "editor/OghamDocument.h"

using namespace godot;

void OghamPlayWindow::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("open", "all_story_data"), &OghamPlayWindow::open);
}

void OghamPlayWindow::open(const Dictionary &p_all_story_data)
{
	ensure_built();
	all_story_data = p_all_story_data;
	reset_story();
	popup_centered(Vector2i(760, 560));
}

void OghamPlayWindow::ensure_built()
{
	if (built)
		return;
	built = true;

	set_title("Ogham Test Play");
	set_size(Vector2i(760, 560));
	set_min_size(Vector2i(700, 520));
	connect("close_requested", callable_mp(this, &OghamPlayWindow::_on_close_pressed));

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_anchors_preset(Control::PRESET_FULL_RECT);
	add_child(root);

	HBoxContainer *toolbar = memnew(HBoxContainer);
	start_tag_option = memnew(OptionButton);
	start_tag_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	toolbar->add_child(start_tag_option);

	Button *start_btn = memnew(Button);
	start_btn->set_text("Start");
	start_btn->connect("pressed", callable_mp(this, &OghamPlayWindow::_on_start_pressed));
	toolbar->add_child(start_btn);

	Button *reset_btn = memnew(Button);
	reset_btn->set_text("Reset");
	reset_btn->connect("pressed", callable_mp(this, &OghamPlayWindow::_on_reset_pressed));
	toolbar->add_child(reset_btn);

	Button *close_btn = memnew(Button);
	close_btn->set_text("Close");
	close_btn->connect("pressed", callable_mp(this, &OghamPlayWindow::_on_close_pressed));
	toolbar->add_child(close_btn);

	root->add_child(toolbar);
	root->add_child(memnew(HSeparator));

	HBoxContainer *body = memnew(HBoxContainer);
	body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(body);

	// ── Left: Narrative State ───────────────────────────────────────────
	VBoxContainer *state_col = memnew(VBoxContainer);
	state_col->set_custom_minimum_size(Vector2(180, 0));
	Label *state_header = memnew(Label);
	state_header->set_text("Narrative State");
	state_col->add_child(state_header);
	ScrollContainer *state_scroll = memnew(ScrollContainer);
	state_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	state_list = memnew(VBoxContainer);
	state_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	state_scroll->add_child(state_list);
	state_col->add_child(state_scroll);
	body->add_child(state_col);

	body->add_child(memnew(VSeparator));

	// ── Center: current content ─────────────────────────────────────────
	ScrollContainer *content_scroll = memnew(ScrollContainer);
	content_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	content_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	content_box = memnew(VBoxContainer);
	content_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	content_scroll->add_child(content_box);
	body->add_child(content_scroll);

	body->add_child(memnew(VSeparator));

	// ── Right: History ───────────────────────────────────────────────────
	VBoxContainer *history_col = memnew(VBoxContainer);
	history_col->set_custom_minimum_size(Vector2(210, 0));
	Label *history_header = memnew(Label);
	history_header->set_text("History");
	history_col->add_child(history_header);
	ScrollContainer *history_scroll = memnew(ScrollContainer);
	history_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	history_list = memnew(VBoxContainer);
	history_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	history_scroll->add_child(history_list);
	history_col->add_child(history_scroll);
	body->add_child(history_col);
}

// Merges every currently loaded document's LIVE in-memory state (including unsaved
// edits — OghamDocument::to_json_text(), not a re-read from disk) into one combined
// OghamStory, matching Unity's "the currently loaded assets" scope but testing
// current edits rather than only what's saved.
void OghamPlayWindow::reset_story()
{
	if (session.is_valid())
	{
		if (session->is_connected("entered", callable_mp(this, &OghamPlayWindow::_on_session_entered)))
			session->disconnect("entered", callable_mp(this, &OghamPlayWindow::_on_session_entered));
		if (session->is_connected("closed", callable_mp(this, &OghamPlayWindow::_on_session_closed)))
			session->disconnect("closed", callable_mp(this, &OghamPlayWindow::_on_session_closed));
	}

	TypedArray<OghamEntry> merged_entries;
	tag_names.clear();

	Array paths = all_story_data.keys();
	for (int i = 0; i < paths.size(); i++)
	{
		Ref<OghamDocument> doc = all_story_data[paths[i]];
		if (doc.is_null())
			continue;
		Ref<OghamStory> parsed = OghamStory::parse_manifest(doc->to_json_text());
		TypedArray<OghamEntry> entries = parsed->get_entries();
		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			merged_entries.push_back(entry);
			if (entry->get_tag() != 0)
				tag_names[int64_t(entry->get_tag())] = entry->get_tag_name();
			TypedArray<OghamOption> options = entry->get_options();
			for (int o = 0; o < options.size(); o++)
			{
				Ref<OghamOption> opt = options[o];
				if (opt->get_tag() != 0)
					tag_names[int64_t(opt->get_tag())] = opt->get_tag_name();
			}
		}
	}

	story.instantiate();
	story->set_entries(merged_entries);

	session.instantiate();
	session->set_story(story);
	session->connect("entered", callable_mp(this, &OghamPlayWindow::_on_session_entered));
	session->connect("closed", callable_mp(this, &OghamPlayWindow::_on_session_closed));

	rebuild_start_tag_list();
	refresh_content();
	refresh_state_panel();
	refresh_history_panel();
}

void OghamPlayWindow::rebuild_start_tag_list()
{
	start_tag_paths.clear();
	TypedArray<OghamEntry> entries = story->get_entries();
	for (int i = 0; i < entries.size(); i++)
	{
		Ref<OghamEntry> entry = entries[i];
		String path = entry->get_tag_name();
		if (!path.is_empty() && !start_tag_paths.has(path))
			start_tag_paths.push_back(path);
	}
	start_tag_paths.sort();

	start_tag_option->clear();
	for (int i = 0; i < start_tag_paths.size(); i++)
		start_tag_option->add_item(start_tag_paths[i]);
	start_tag_idx = 0;
	if (start_tag_option->get_item_count() > 0)
		start_tag_option->select(0);
}

void OghamPlayWindow::_on_start_pressed()
{
	if (start_tag_paths.is_empty())
		return;
	int idx = start_tag_option->get_selected();
	if (idx < 0)
		idx = 0;
	String tag_path = start_tag_paths[idx];
	uint64_t tag = OghamInterop::hash_tag(tag_path);
	if (!session->enter(tag))
		UtilityFunctions::push_warning("Ogham Play: entry tag not found: ", tag_path);
}

void OghamPlayWindow::_on_reset_pressed()
{
	reset_story();
}

void OghamPlayWindow::_on_close_pressed()
{
	if (session.is_valid())
		session->close();
	hide();
}

void OghamPlayWindow::_on_session_entered(int64_t)
{
	refresh_content();
	refresh_state_panel();
	refresh_history_panel();
}

void OghamPlayWindow::_on_session_closed()
{
	refresh_content();
	refresh_history_panel();
}

String OghamPlayWindow::resolve_tag_name(uint64_t id) const
{
	if (tag_names.has(int64_t(id)))
		return tag_names[int64_t(id)];
	Object *registry = OghamInterop::get_gameplay_tag_registry();
	if (registry != nullptr)
	{
		String name = registry->call("get_name", int64_t(id));
		if (!name.is_empty())
			return name;
	}
	return String::num_uint64(id, 16, true);
}

Ref<OghamOption> OghamPlayWindow::find_active_option_by_tag_path(const String &tag_path) const
{
	if (!session->get_is_active())
		return Ref<OghamOption>();
	uint64_t target_tag = OghamInterop::hash_tag(tag_path);
	PackedInt64Array active = session->get_current_options();
	if (!active.has(int64_t(target_tag)))
		return Ref<OghamOption>();

	// current_all_options isn't directly exposed as Resources, only tags — re-derive
	// the OghamOption from the current entry so its resolve_text()/target are usable.
	uint64_t entry_tag = session->get_current_entry_tag();
	Ref<OghamEntry> entry = story->find_entry(entry_tag);
	if (entry.is_null())
		return Ref<OghamOption>();
	TypedArray<OghamOption> options = entry->get_options();
	for (int i = 0; i < options.size(); i++)
	{
		Ref<OghamOption> opt = options[i];
		if (opt->get_tag() == target_tag)
			return opt;
	}
	return Ref<OghamOption>();
}

void OghamPlayWindow::refresh_content()
{
	for (int i = 0; i < content_box->get_child_count(); i++)
		content_box->get_child(i)->queue_free();
	// Children are queue_free()'d, not immediately removed — clear the container's
	// child list view too so a rapid re-refresh doesn't see stale nodes.
	while (content_box->get_child_count() > 0)
		content_box->remove_child(content_box->get_child(0));

	if (!session->get_is_active())
	{
		Label *placeholder = memnew(Label);
		placeholder->set_text("No active conversation.\nSelect an entry above and press Start.");
		placeholder->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
		content_box->add_child(placeholder);
		return;
	}

	uint64_t entry_tag = session->get_current_entry_tag();

	Label *header = memnew(Label);
	header->set_text(resolve_tag_name(entry_tag));
	header->add_theme_font_size_override("font_size", 16);
	content_box->add_child(header);

	std::unordered_set<std::string> inline_linked;

	int count = session->get_current_content_count();
	for (int i = 0; i < count; i++)
	{
		String text = session->get_current_text(i);
		if (text.is_empty())
			continue;
		text = OghamVariables::get_singleton()->interpolate(text, session->get_state());

		RichTextLabel *rtl = memnew(RichTextLabel);
		rtl->set_use_bbcode(true);
		rtl->set_fit_content(true);
		rtl->set_scroll_active(false);
		rtl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		rtl->set_text(OghamInlineLinkParser::to_bbcode(text));
		rtl->connect("meta_clicked", callable_mp(this, &OghamPlayWindow::_on_content_meta_clicked));
		content_box->add_child(rtl);

		// Track which options this text already rendered as an inline link, so
		// DrawOptions-equivalent below doesn't show them a second time.
		Array links = OghamInlineLinkParser::extract_links(text);
		for (int l = 0; l < links.size(); l++)
		{
			Dictionary link = links[l];
			String url = link.get("url", "");
			if (OghamInlineLinkParser::is_ogham_link(url))
				inline_linked.insert(std::string(OghamInlineLinkParser::get_tag_path(url).utf8().get_data()));
		}
	}

	content_box->add_child(memnew(HSeparator));

	PackedInt64Array active_options = session->get_current_options();
	bool any_option_shown = false;
	for (int i = 0; i < active_options.size(); i++)
	{
		uint64_t option_tag = active_options[i];
		String option_path = resolve_tag_name(option_tag);
		if (inline_linked.count(std::string(option_path.utf8().get_data())) > 0)
			continue;

		Ref<OghamEntry> entry = story->find_entry(entry_tag);
		String label = session->get_option_text(option_tag);
		if (label.is_empty())
			label = option_path;

		Button *btn = memnew(Button);
		btn->set_text(label);
		btn->connect("pressed", callable_mp(session.ptr(), &OghamSession::choose).bind(option_tag));
		btn->connect("pressed", callable_mp(this, &OghamPlayWindow::refresh_content));
		content_box->add_child(btn);
		any_option_shown = true;
	}
	if (!any_option_shown && active_options.size() == 0)
	{
		Label *end_label = memnew(Label);
		end_label->set_text("(end of conversation)");
		content_box->add_child(end_label);
	}
}

void OghamPlayWindow::_on_content_meta_clicked(const Variant &meta)
{
	String url = meta;
	if (!OghamInlineLinkParser::is_ogham_link(url))
		return;
	String tag_path = OghamInlineLinkParser::get_tag_path(url);
	Ref<OghamOption> opt = find_active_option_by_tag_path(tag_path);
	if (opt.is_null())
		return; // exists in text but not currently active — not clickable
	session->choose(opt->get_tag());
	refresh_content();
	refresh_state_panel();
	refresh_history_panel();
}

void OghamPlayWindow::refresh_state_panel()
{
	while (state_list->get_child_count() > 0)
	{
		Node *child = state_list->get_child(0);
		state_list->remove_child(child);
		child->queue_free();
	}

	Ref<RefCounted> state = session->get_state();
	if (state.is_null())
		return;
	Dictionary all = state->call("get_all");
	Array keys = all.keys();
	for (int i = 0; i < keys.size(); i++)
	{
		int64_t tag = keys[i];
		Label *row = memnew(Label);
		row->set_text(String("{0}: {1}").format(Array::make(resolve_tag_name(uint64_t(tag)), all[keys[i]])));
		row->add_theme_font_size_override("font_size", 11);
		state_list->add_child(row);
	}
}

void OghamPlayWindow::refresh_history_panel()
{
	while (history_list->get_child_count() > 0)
	{
		Node *child = history_list->get_child(0);
		history_list->remove_child(child);
		child->queue_free();
	}

	Array history = session->get_history();
	for (int i = history.size() - 1; i >= 0; i--)
	{
		Dictionary h = history[i];
		int64_t entry_id = h.get("entry_id", 0);
		int64_t selected_option = h.get("selected_option", 0);
		String entry_name = resolve_tag_name(uint64_t(entry_id));
		String option_name = selected_option == 0 ? "(closed)" : resolve_tag_name(uint64_t(selected_option));

		HBoxContainer *row = memnew(HBoxContainer);
		Label *lbl = memnew(Label);
		lbl->set_text(entry_name + " -> " + option_name);
		lbl->add_theme_font_size_override("font_size", 11);
		lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_child(lbl);

		Button *return_btn = memnew(Button);
		return_btn->set_text("<-");
		return_btn->set_tooltip_text("Return to this entry");
		return_btn->connect("pressed", callable_mp(session.ptr(), &OghamSession::return_to).bind(uint64_t(entry_id)));
		return_btn->connect("pressed", callable_mp(this, &OghamPlayWindow::refresh_content));
		return_btn->connect("pressed", callable_mp(this, &OghamPlayWindow::refresh_state_panel));
		return_btn->connect("pressed", callable_mp(this, &OghamPlayWindow::refresh_history_panel));
		row->add_child(return_btn);

		history_list->add_child(row);
	}
}
