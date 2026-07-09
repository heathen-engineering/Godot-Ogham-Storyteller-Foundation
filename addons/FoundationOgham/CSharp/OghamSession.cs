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

using Godot;
using Godot.Collections;

namespace Heathen.Ogham
{
    /// <summary>
    /// Wrapper over a native OghamSession (RefCounted). One playthrough of an
    /// OghamStory — the definition is immutable/shared; the session owns all mutable
    /// state (narrative-state GameplayTagCollection, choice history, active entry).
    /// </summary>
    public class OghamSession
    {
        private readonly GodotObject _instance;

        public GodotObject ToGDNative() => _instance;

        public OghamSession(GodotObject instance) => _instance = instance;

        public OghamStory Story
        {
            get
            {
                Variant result = _instance.Get("story");
                return result.Obj is Resource r ? new OghamStory(r) : null;
            }
            set => _instance.Set("story", value?.ToGDNative());
        }

        /// <summary>The narrative-state GameplayTagCollection (lazily instantiated).</summary>
        public GodotObject GetState()
        {
            Variant result = _instance.Call("get_state");
            return result.Obj as GodotObject;
        }

        public bool Enter(ulong entryTag) => (bool)_instance.Call("enter", entryTag);
        public bool Choose(ulong optionTag) => (bool)_instance.Call("choose", optionTag);
        public void Close() => _instance.Call("close");
        public bool Resume() => (bool)_instance.Call("resume");
        public bool ReturnTo(ulong entryTag) => (bool)_instance.Call("return_to", entryTag);

        public bool IsActive => (bool)_instance.Call("get_is_active");
        public ulong CurrentEntryTag => (ulong)_instance.Call("get_current_entry_tag");
        public int CurrentContentCount => (int)_instance.Call("get_current_content_count");
        public string GetCurrentText(int index) => (string)_instance.Call("get_current_text", index);

        public Resource GetCurrentAsset(int index)
        {
            Variant result = _instance.Call("get_current_asset", index);
            return result.Obj as Resource;
        }

        public long[] CurrentOptions => (long[])_instance.Call("get_current_options");
        public long[] CurrentAllOptions => (long[])_instance.Call("get_current_all_options");
        public bool IsOptionActive(ulong optionTag) => (bool)_instance.Call("is_option_active", optionTag);
        public string GetOptionText(ulong optionTag) => (string)_instance.Call("get_option_text", optionTag);
        public Array History => (Array)_instance.Call("get_history");

        public Dictionary Snapshot(string saveName) => (Dictionary)_instance.Call("snapshot", saveName);
        public bool Restore(Dictionary saveState) => (bool)_instance.Call("restore", saveState);

        /// <summary>Fires as (entryTag: ulong) whenever a non-Fork entry is entered/resumed.</summary>
        public void ConnectEntered(Callable callback) => _instance.Connect("entered", callback);

        /// <summary>Fires as (optionTag: ulong) whenever an option is chosen.</summary>
        public void ConnectChoice(Callable callback) => _instance.Connect("choice", callback);

        /// <summary>Fires whenever the session closes.</summary>
        public void ConnectClosed(Callable callback) => _instance.Connect("closed", callback);
    }
}
