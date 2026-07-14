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

#include "editor/OghamScriptExporter.h"

#include <vector>

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>

#include "OghamOption.h"
#include "editor/OghamKeyLabelsNative.h"

using namespace godot;

namespace
{
	const char *STATE_VAR_PATTERN = R"(@([A-Za-z_][A-Za-z0-9_]*)\(\s*([A-Za-z_][A-Za-z0-9_.]*)\s*(?:,[^)]*)?\))";
	const char *INLINE_LINK_PATTERN = R"(\[([^\]]*)\]\(Ogham://[^)]*\))";
	const char *BOLD_PATTERN = R"(\*\*(.+?)\*\*)";
	const char *ITALIC_PATTERN = R"(\*(.+?)\*)";

	// Same non-speakable set as Unity's s_StripChars.
	bool is_strip_char(char32_t c)
	{
		static const char32_t chars[] = {
			U'→', U'←', U'↑', U'↓', U'·', U'—', U'…',
			U'■', U'▶', U'▼', U'►', U'◄', U'▸', U'▾',
			U'●', U'○', U'◆', U'◇', U'★', U'☆',
			U'|', U'~', U'^',
		};
		for (char32_t sc : chars)
			if (sc == c)
				return true;
		return false;
	}

	bool is_non_speakable(char32_t c)
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || is_strip_char(c);
	}

	// Replaces every "@Token(Tag.Path[, args])" span with the tag path's last
	// dot-segment, wrapped in state_var_open/close — same behavior as Unity's
	// CleanText, just without a callback-capable regex replace (Godot's RegEx
	// only does $N-group substitution), so this walks matches manually.
	String replace_state_vars(const String &text, const String &open, const String &close)
	{
		Ref<RegEx> re = RegEx::create_from_string(STATE_VAR_PATTERN);
		TypedArray<RegExMatch> matches = re->search_all(text);
		if (matches.is_empty())
			return text;

		String result;
		int64_t pos = 0;
		for (int64_t i = 0; i < matches.size(); i++)
		{
			Ref<RegExMatch> m = matches[i];
			int64_t match_start = m->get_start(0);
			int64_t match_end = m->get_end(0);
			result += text.substr(pos, match_start - pos);

			String path = m->get_string(2);
			int dot = path.rfind(".");
			String name = dot >= 0 ? path.substr(dot + 1) : path;
			result += open + name + close;

			pos = match_end;
		}
		result += text.substr(pos);
		return result;
	}

	// Removes inline Ogham:// links that appear only at the END of the text —
	// after the last human-readable character. Links embedded in spoken text
	// (more text follows them) are left in place. Direct port of Unity's
	// StripTrailingLinks.
	String strip_trailing_links(const String &text)
	{
		Ref<RegEx> re = RegEx::create_from_string(INLINE_LINK_PATTERN);
		TypedArray<RegExMatch> matches = re->search_all(text);
		if (matches.is_empty())
			return text;

		int64_t len = text.length();
		std::vector<bool> in_link(len, false);
		for (int64_t i = 0; i < matches.size(); i++)
		{
			Ref<RegExMatch> m = matches[i];
			int64_t start = m->get_start(0);
			int64_t end = m->get_end(0);
			for (int64_t p = start; p < end && p < len; p++)
				in_link[p] = true;
		}

		int64_t cut_at = -1;
		for (int64_t i = len - 1; i >= 0; i--)
		{
			if (!in_link[i] && !is_non_speakable(text.unicode_at(i)))
			{
				cut_at = i + 1;
				break;
			}
		}

		if (cut_at < 0)
			return re->sub(text, "", true).strip_edges();

		return text.substr(0, cut_at).strip_edges(false, true); // trim trailing only
	}

	String clean_text(const String &raw, const OghamScriptExporter::ExportOptions &opts, bool allow_trailing_strip)
	{
		if (raw.is_empty())
			return raw;

		String text = raw;
		String state_open = "<<";
		String state_close = ">>";
		if (opts.format == OghamScriptExporter::FORMAT_MARKDOWN || opts.format == OghamScriptExporter::FORMAT_HTML)
		{
			state_open = "{";
			state_close = "}";
		}
		text = replace_state_vars(text, state_open, state_close);

		if (allow_trailing_strip && opts.strip_trailing_links)
			text = strip_trailing_links(text);

		// Inline links -> display text only (unconditional, same as Unity).
		Ref<RegEx> link_re = RegEx::create_from_string(INLINE_LINK_PATTERN);
		text = link_re->sub(text, "$1", true);

		if (opts.remove_formatting)
		{
			Ref<RegEx> bold_re = RegEx::create_from_string(BOLD_PATTERN);
			text = bold_re->sub(text, "$1", true);
			Ref<RegEx> italic_re = RegEx::create_from_string(ITALIC_PATTERN);
			text = italic_re->sub(text, "$1", true);

			String stripped;
			int64_t tlen = text.length();
			for (int64_t i = 0; i < tlen; i++)
			{
				char32_t c = text.unicode_at(i);
				stripped += is_strip_char(c) ? String(" ") : String::chr(c);
			}
			text = stripped;
		}

		// Normalise whitespace.
		Ref<RegEx> multi_space = RegEx::create_from_string(R"([ \t]+)");
		text = multi_space->sub(text, " ", true);
		Ref<RegEx> newline_space = RegEx::create_from_string(R"(\n[ \t]+)");
		text = newline_space->sub(text, "\n", true);
		Ref<RegEx> multi_newline = RegEx::create_from_string(R"(\n{3,})");
		text = multi_newline->sub(text, "\n\n", true);
		return text.strip_edges();
	}

	String resolve_key_text(const String &key_or_value, int mode, const OghamScriptExporter::ExportOptions &opts)
	{
		String raw = key_or_value;
		if (mode == 0 /* MODE_LOCALISED */ && opts.resolve_key.is_valid())
		{
			Variant resolved = opts.resolve_key.call(key_or_value);
			if (resolved.get_type() == Variant::STRING && !String(resolved).is_empty())
				raw = resolved;
		}
		return raw;
	}

	String option_display_text(const Ref<OghamOption> &opt, const OghamScriptExporter::ExportOptions &opts)
	{
		Ref<Resource> text_key = opt->get_text_key();
		String raw;
		int mode = 1; // MODE_LITERAL
		if (text_key.is_valid())
		{
			mode = int(text_key->call("get_mode"));
			raw = String(text_key->call("get_key_or_value"));
		}
		else
		{
			raw = opt->get_tag_name();
		}
		raw = resolve_key_text(raw, mode, opts);
		// Options never get trailing-link stripping — the option's own text
		// IS the link label, matching Unity's GatherOptions.
		return clean_text(raw, opts, false);
	}

	Array gather_option_texts(const Ref<OghamEntry> &entry, const OghamScriptExporter::ExportOptions &opts)
	{
		Array result;
		TypedArray<OghamOption> options = entry->get_options();
		for (int i = 0; i < options.size(); i++)
		{
			Ref<OghamOption> opt = options[i];
			String text = option_display_text(opt, opts);
			if (!text.strip_edges().is_empty())
				result.push_back(text);
		}
		return result;
	}

	String tag_path(const Ref<OghamEntry> &entry)
	{
		String path = entry->get_tag_name();
		return path.is_empty() ? String::num_uint64(entry->get_tag(), 16, true) : path;
	}

	// index -> label text ("" if this slot isn't included by the caller's
	// filter). A slot with no project-wide name at all is always included
	// (filtering only ever excludes explicitly-named-but-unchecked slots).
	bool label_included(int index, const OghamScriptExporter::ExportOptions &opts)
	{
		String label = OghamKeyLabelsNative::label_for_index(index);
		if (label.is_empty())
			return true;
		if (opts.enabled_labels.is_empty())
			return true;
		return opts.enabled_labels.has(label);
	}

	String csv_cell(const String &value)
	{
		if (value.is_empty())
			return "";
		bool needs_quote = value.find(",") >= 0 || value.find("\"") >= 0 || value.find("\n") >= 0 || value.find("\r") >= 0;
		if (!needs_quote)
			return value;
		return "\"" + value.replace("\"", "\"\"") + "\"";
	}

	String html_encode(const String &s)
	{
		return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
	}

	String md_escape(const String &s)
	{
		return s.replace("*", "\\*").replace("_", "\\_").replace("`", "\\`");
	}

	String export_csv(const TypedArray<OghamEntry> &entries, const Dictionary &notes_by_tag, const OghamScriptExporter::ExportOptions &opts)
	{
		String out;
		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			if (entry->is_fork())
				continue;
			String path = tag_path(entry);
			out += "\n" + csv_cell(path) + "\n";
			out += csv_cell("Notes") + "," + csv_cell(notes_by_tag.get(path, "")) + "\n";

			for (int i = 0; i < entry->get_content_count(); i++)
			{
				if (!label_included(i, opts))
					continue;
				Ref<OghamContentKey> key = entry->get_content_key(i);
				String text = clean_text(resolve_key_text(key->get_key_or_value(), key->get_mode(), opts), opts, true);
				String label = OghamKeyLabelsNative::label_for_index(i);
				out += csv_cell(label) + "," + csv_cell(text) + "\n";
			}

			if (opts.list_options)
			{
				Array opt_texts = gather_option_texts(entry, opts);
				if (!opt_texts.is_empty())
				{
					out += csv_cell("Options") + "," + csv_cell(opt_texts[0]) + "\n";
					for (int oi = 1; oi < opt_texts.size(); oi++)
						out += "," + csv_cell(opt_texts[oi]) + "\n";
				}
			}
		}
		out += "\n";
		return out;
	}

	String export_markdown(const TypedArray<OghamEntry> &entries, const Dictionary &notes_by_tag, const OghamScriptExporter::ExportOptions &opts)
	{
		String out;
		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			if (entry->is_fork())
				continue;
			String path = tag_path(entry);
			out += "\n---\n\n## " + path + "\n\n";

			String notes = notes_by_tag.get(path, "");
			if (!notes.strip_edges().is_empty())
				out += "> " + notes.replace("\n", "\n> ") + "\n\n";

			for (int i = 0; i < entry->get_content_count(); i++)
			{
				if (!label_included(i, opts))
					continue;
				Ref<OghamContentKey> key = entry->get_content_key(i);
				String text = clean_text(resolve_key_text(key->get_key_or_value(), key->get_mode(), opts), opts, true);
				String label = OghamKeyLabelsNative::label_for_index(i);
				if (!label.is_empty())
					out += "**" + md_escape(label) + ":** " + text + "  \n";
				else
					out += text + "  \n";
			}

			if (opts.list_options)
			{
				Array opt_texts = gather_option_texts(entry, opts);
				if (!opt_texts.is_empty())
				{
					out += "\n**Options:**\n";
					for (int oi = 0; oi < opt_texts.size(); oi++)
						out += "- " + String(opt_texts[oi]) + "\n";
				}
			}
			out += "\n";
		}
		out += "---\n";
		return out;
	}

	String export_html(const TypedArray<OghamEntry> &entries, const Dictionary &notes_by_tag, const OghamScriptExporter::ExportOptions &opts)
	{
		String out = "<!DOCTYPE html>\n<html lang=\"en\"><head><meta charset=\"UTF-8\">\n<style>\n"
					  "body{font-family:sans-serif;max-width:900px;margin:40px auto;padding:0 20px;color:#222}\n"
					  "h2{color:#1a4080;border-bottom:2px solid #1a4080;padding-bottom:4px;margin-top:2em}\n"
					  "table{border-collapse:collapse;width:100%;margin:8px 0}\n"
					  "td{padding:6px 12px;vertical-align:top;border:1px solid #ddd}\n"
					  "td:first-child{font-weight:bold;width:120px;white-space:nowrap;background:#f5f5f5}\n"
					  ".notes{background:#fffbe6;border-left:4px solid #f0c040;padding:8px 12px;margin:8px 0;font-style:italic}\n"
					  ".options li{margin:2px 0}\n"
					  "hr{border:none;border-top:1px solid #ccc;margin:2em 0}\n"
					  "</style></head><body>\n";

		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			if (entry->is_fork())
				continue;
			String path = tag_path(entry);
			out += "<hr><h2>" + html_encode(path) + "</h2>\n";

			String notes = notes_by_tag.get(path, "");
			if (!notes.strip_edges().is_empty())
				out += "<div class=\"notes\"><strong>Director Notes:</strong> " + html_encode(notes) + "</div>\n";

			bool any_row = false;
			String table;
			for (int i = 0; i < entry->get_content_count(); i++)
			{
				if (!label_included(i, opts))
					continue;
				Ref<OghamContentKey> key = entry->get_content_key(i);
				String text = clean_text(resolve_key_text(key->get_key_or_value(), key->get_mode(), opts), opts, true);
				String label = OghamKeyLabelsNative::label_for_index(i);
				table += "<tr><td>" + html_encode(label) + "</td><td>" + html_encode(text).replace("\n", "<br>") + "</td></tr>\n";
				any_row = true;
			}
			if (any_row)
				out += "<table>\n" + table + "</table>\n";

			if (opts.list_options)
			{
				Array opt_texts = gather_option_texts(entry, opts);
				if (!opt_texts.is_empty())
				{
					out += "<p><strong>Options:</strong></p><ul class=\"options\">\n";
					for (int oi = 0; oi < opt_texts.size(); oi++)
						out += "<li>" + html_encode(opt_texts[oi]) + "</li>\n";
					out += "</ul>\n";
				}
			}
		}
		out += "<hr></body></html>\n";
		return out;
	}

	String export_plain_text(const TypedArray<OghamEntry> &entries, const Dictionary &notes_by_tag, const OghamScriptExporter::ExportOptions &opts)
	{
		const String divider = "================================================================================";
		String out;
		for (int e = 0; e < entries.size(); e++)
		{
			Ref<OghamEntry> entry = entries[e];
			if (entry->is_fork())
				continue;
			String path = tag_path(entry);
			out += "\n" + divider + "\n" + path + "\n" + divider + "\n\n";

			String notes = notes_by_tag.get(path, "");
			if (!notes.strip_edges().is_empty())
				out += "NOTES: " + notes + "\n\n";

			for (int i = 0; i < entry->get_content_count(); i++)
			{
				if (!label_included(i, opts))
					continue;
				Ref<OghamContentKey> key = entry->get_content_key(i);
				String text = clean_text(resolve_key_text(key->get_key_or_value(), key->get_mode(), opts), opts, true);
				String label = OghamKeyLabelsNative::label_for_index(i);
				String prefix = label.is_empty() ? "" : label + ": ";
				out += prefix + text + "\n\n";
			}

			if (opts.list_options)
			{
				Array opt_texts = gather_option_texts(entry, opts);
				if (!opt_texts.is_empty())
				{
					out += "Options:\n";
					for (int oi = 0; oi < opt_texts.size(); oi++)
						out += "  - " + String(opt_texts[oi]) + "\n";
					out += "\n";
				}
			}
		}
		return out;
	}
} // namespace

namespace OghamScriptExporter
{
	String export_story(const TypedArray<OghamEntry> &entries, const Dictionary &notes_by_tag, const ExportOptions &opts)
	{
		switch (opts.format)
		{
			case FORMAT_MARKDOWN:
				return export_markdown(entries, notes_by_tag, opts);
			case FORMAT_HTML:
				return export_html(entries, notes_by_tag, opts);
			case FORMAT_PLAIN_TEXT:
				return export_plain_text(entries, notes_by_tag, opts);
			default:
				return export_csv(entries, notes_by_tag, opts);
		}
	}
}
