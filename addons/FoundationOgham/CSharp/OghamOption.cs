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
    /// Wrapper over a native OghamOption (Resource). A single choice on an OghamEntry.
    /// Conditions/operations are Foundation GameplayTags Resources (GameplayTagCondition/
    /// GameplayTagOperation) — Ogham has no logic of its own, see the repo README.
    /// </summary>
    public class OghamOption
    {
        private readonly Resource _instance;

        public Resource ToGDNative() => _instance;

        public OghamOption() => _instance = (Resource)ClassDB.Instantiate("OghamOption");
        public OghamOption(Resource instance) => _instance = instance;

        public string TagName
        {
            get => (string)_instance.Get("tag_name");
            set => _instance.Set("tag_name", value);
        }

        public ulong Tag => (ulong)_instance.Call("get_tag");

        public string TargetEntryPath
        {
            get => (string)_instance.Get("target_entry_path");
            set => _instance.Set("target_entry_path", value);
        }

        public ulong TargetEntry => (ulong)_instance.Call("get_target_entry");
        public bool HasTarget => (bool)_instance.Call("has_target");

        public Resource TextKey
        {
            get => _instance.Get("text_key").Obj as Resource;
            set => _instance.Set("text_key", value);
        }

        public Array Conditions
        {
            get => (Array)_instance.Get("conditions");
            set => _instance.Set("conditions", value);
        }

        public Array Operations
        {
            get => (Array)_instance.Get("operations");
            set => _instance.Set("operations", value);
        }

        public bool SynthesizedFromInlineLink
        {
            get => (bool)_instance.Get("synthesized_from_inline_link");
            set => _instance.Set("synthesized_from_inline_link", value);
        }

        public string InlineLinkSourceKeyPath
        {
            get => (string)_instance.Get("inline_link_source_key_path");
            set => _instance.Set("inline_link_source_key_path", value);
        }

        public string ResolveText() => (string)_instance.Call("resolve_text");

        public bool IsActive(GodotObject narrativeState) => (bool)_instance.Call("is_active", narrativeState);
    }
}
