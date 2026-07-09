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

namespace Heathen.Ogham
{
    public enum OghamContentType
    {
        Text = 0,
        Image = 1,
        Sprite = 2,
        Audio = 3,
        PackedScene = 4,
    }

    public enum OghamLocMode
    {
        Localised = 0,
        Literal = 1,
        Invariant = 2,
    }

    /// <summary>Wrapper over a native OghamContentKey (Resource). One content slot on an OghamEntry.</summary>
    public class OghamContentKey
    {
        private readonly Resource _instance;

        public Resource ToGDNative() => _instance;

        public OghamContentKey() => _instance = (Resource)ClassDB.Instantiate("OghamContentKey");
        public OghamContentKey(Resource instance) => _instance = instance;

        public OghamContentType Type
        {
            get => (OghamContentType)(int)_instance.Get("type");
            set => _instance.Set("type", (int)value);
        }

        public OghamLocMode Mode
        {
            get => (OghamLocMode)(int)_instance.Get("mode");
            set => _instance.Set("mode", (int)value);
        }

        public string KeyOrValue
        {
            get => (string)_instance.Get("key_or_value");
            set => _instance.Set("key_or_value", value);
        }

        public string AssetPath
        {
            get => (string)_instance.Get("asset_path");
            set => _instance.Set("asset_path", value);
        }

        public string AssetSubName
        {
            get => (string)_instance.Get("asset_sub_name");
            set => _instance.Set("asset_sub_name", value);
        }

        public ulong GetHash() => (ulong)_instance.Call("get_hash");
        public string ResolveText() => (string)_instance.Call("resolve_text");

        public Resource ResolveAsset()
        {
            Variant result = _instance.Call("resolve_asset");
            return result.Obj as Resource;
        }
    }
}
