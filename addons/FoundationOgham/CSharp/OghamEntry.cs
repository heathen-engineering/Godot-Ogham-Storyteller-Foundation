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
    public enum OghamNodeMode
    {
        Content = 0,
        Fork = 1,
    }

    /// <summary>
    /// Wrapper over a native OghamEntry (Resource). A single node in an Ogham story
    /// graph. Mode == Fork makes it a silent routing node (see OghamSession).
    /// </summary>
    public class OghamEntry
    {
        private readonly Resource _instance;

        public Resource ToGDNative() => _instance;

        public OghamEntry() => _instance = (Resource)ClassDB.Instantiate("OghamEntry");
        public OghamEntry(Resource instance) => _instance = instance;

        public string TagName
        {
            get => (string)_instance.Get("tag_name");
            set => _instance.Set("tag_name", value);
        }

        public ulong Tag => (ulong)_instance.Call("get_tag");

        public OghamNodeMode Mode
        {
            get => (OghamNodeMode)(int)_instance.Get("mode");
            set => _instance.Set("mode", (int)value);
        }

        public bool IsFork => (bool)_instance.Call("is_fork");

        public Array ContentKeys
        {
            get => (Array)_instance.Get("content_keys");
            set => _instance.Set("content_keys", value);
        }

        public int ContentCount => (int)_instance.Call("get_content_count");

        public OghamContentKey GetContentKey(int index)
        {
            Variant result = _instance.Call("get_content_key", index);
            return result.Obj is Resource r ? new OghamContentKey(r) : null;
        }

        public Array EntryOperations
        {
            get => (Array)_instance.Get("entry_operations");
            set => _instance.Set("entry_operations", value);
        }

        public Array Options
        {
            get => (Array)_instance.Get("options");
            set => _instance.Set("options", value);
        }
    }
}
