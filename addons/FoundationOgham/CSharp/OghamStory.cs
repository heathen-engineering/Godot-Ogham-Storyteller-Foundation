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
    /// <summary>Wrapper over a native OghamStory (Resource). An immutable story definition.</summary>
    public class OghamStory
    {
        private readonly Resource _instance;

        public Resource ToGDNative() => _instance;

        public OghamStory() => _instance = (Resource)ClassDB.Instantiate("OghamStory");
        public OghamStory(Resource instance) => _instance = instance;

        public string StoryName
        {
            get => (string)_instance.Get("story_name");
            set => _instance.Set("story_name", value);
        }

        public Array Entries
        {
            get => (Array)_instance.Get("entries");
            set => _instance.Set("entries", value);
        }

        public OghamEntry FindEntry(ulong tag)
        {
            Variant result = _instance.Call("find_entry", tag);
            return result.Obj is Resource r ? new OghamEntry(r) : null;
        }

        public OghamEntry FindEntryByName(string tagPath)
        {
            Variant result = _instance.Call("find_entry_by_name", tagPath);
            return result.Obj is Resource r ? new OghamEntry(r) : null;
        }

        public long[] CollectDescendants(ulong tag) => (long[])_instance.Call("collect_descendants", tag);
        public long[] CollectWithinDepth(ulong tag, int depth) => (long[])_instance.Call("collect_within_depth", tag, depth);
        public void InvalidateIndex() => _instance.Call("invalidate_index");

        /// <summary>Parses .ogham JSON text into a new OghamStory (see repo README for schema).</summary>
        public static OghamStory ParseManifest(string jsonText)
        {
            Variant result = ClassDB.ClassCallStatic("OghamStory", "parse_manifest", jsonText);
            return result.Obj is Resource r ? new OghamStory(r) : null;
        }
    }
}
