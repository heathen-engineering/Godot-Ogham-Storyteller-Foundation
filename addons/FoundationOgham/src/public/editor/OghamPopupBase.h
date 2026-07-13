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

#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// <summary>
/// Shared close/free guard for the 5 Ogham toolwindow popups — every one of
/// them (in GDScript, and now here) follows "closing twice must be a no-op,
/// hide() then queue_free()". What differs per popup (commit-on-close vs.
/// mutate-immediately-on-every-edit) stays in each concrete subclass, since
/// forcing those genuinely different interaction patterns through one shared
/// virtual method would obscure more than it'd save for ~5 lines of
/// boilerplate.
/// </summary>
class OghamPopupBase : public PopupPanel
{
    GDCLASS(OghamPopupBase, PopupPanel);

protected:
    bool closed_ = false;
    /// Every subclass builds its Controls lazily, on first open() call, into
    /// a private _ensure_built() guarded by this flag — NOT in the
    /// constructor (calling engine Node methods there crashes: the native
    /// instance binding isn't fully established until after construction
    /// completes) and NOT relied on _ready() timing either (empirically not
    /// synchronous with add_child() in every context these popups get opened
    /// from). open() is always called after add_child() — the one timing
    /// guarantee every caller actually provides.
    bool built_ = false;

public:
    OghamPopupBase() = default;

    /// Idempotent — safe to call from multiple signal handlers (popup_hide,
    /// an explicit Cancel/Save button) without double-freeing. Public (not
    /// protected) specifically so callable_mp(this, &OghamPopupBase::close_and_free)
    /// works from a subclass — C++ only allows forming a pointer-to-member to
    /// an inherited PROTECTED member when named through the derived class,
    /// not the base, which callable_mp's base-typed overload needs to do.
    void close_and_free();

protected:
    static void _bind_methods() {}
};
