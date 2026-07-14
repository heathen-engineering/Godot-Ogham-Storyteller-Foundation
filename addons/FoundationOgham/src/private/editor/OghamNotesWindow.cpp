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

#include "editor/OghamNotesWindow.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

void OghamNotesWindow::_bind_methods()
{
}

void OghamNotesWindow::_ensure_built()
{
    if (built_)
        return;
    built_ = true;

    set_size(Vector2i(360, 200));
    set_min_size(Vector2i(320, 160));

    VBoxContainer *vbox = memnew(VBoxContainer);
    vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
    vbox->add_theme_constant_override("margin_left", 8);
    add_child(vbox);

    Label *hint = memnew(Label);
    hint->set_text("Director notes for this node (VO export only):");
    hint->add_theme_font_size_override("font_size", 11);
    vbox->add_child(hint);

    notes_edit_ = memnew(TextEdit);
    notes_edit_->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    notes_edit_->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
    vbox->add_child(notes_edit_);

    HBoxContainer *button_row = memnew(HBoxContainer);
    button_row->set_alignment(BoxContainer::ALIGNMENT_END);

    OghamPopupBase *self_as_base = this;

    Button *cancel_btn = memnew(Button);
    cancel_btn->set_text("Cancel");
    cancel_btn->connect("pressed", callable_mp(self_as_base, &OghamPopupBase::close_and_free));
    button_row->add_child(cancel_btn);

    Button *save_btn = memnew(Button);
    save_btn->set_text("Save");
    save_btn->connect("pressed", callable_mp(this, &OghamNotesWindow::_commit));
    button_row->add_child(save_btn);

    vbox->add_child(button_row);

    notes_edit_->connect("focus_exited", callable_mp(this, &OghamNotesWindow::_on_focus_exited));
}

void OghamNotesWindow::open(const String &current_notes, const Vector2i &screen_pos, const Callable &on_commit_cb)
{
    _ensure_built();
    on_commit = on_commit_cb;
    closed_ = false;
    notes_edit_->set_text(current_notes);
    popup(Rect2i(screen_pos, get_size()));
    notes_edit_->grab_focus();
}

void OghamNotesWindow::_commit()
{
    if (closed_)
        return;
    if (on_commit.is_valid())
        on_commit.call(notes_edit_->get_text());
    close_and_free();
}

void OghamNotesWindow::_on_focus_exited()
{
    // Losing focus commits (matches Unity's OnLostFocus behavior) — an
    // explicit Cancel click is the only way to discard an edit.
    _commit();
}
