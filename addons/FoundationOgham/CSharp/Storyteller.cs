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
    /// <summary>
    /// Static facade over the native "Storyteller" engine singleton — the global entry
    /// point for opening and finding stories. Holds one OghamSession per story tag, so
    /// many stories can be open and independently stepped at once.
    /// </summary>
    public static class Storyteller
    {
        static GodotObject Instance => Engine.GetSingleton("Storyteller");

        /// <summary>Returns the existing session for storyTag if open, else creates one bound to story.</summary>
        public static OghamSession OpenSession(string storyTag, OghamStory story, bool setAsMain = true)
        {
            Variant result = Instance.Call("open_session", storyTag, story.ToGDNative(), setAsMain);
            return result.Obj is GodotObject obj ? new OghamSession(obj) : null;
        }

        public static OghamSession GetSession(string storyTag)
        {
            Variant result = Instance.Call("get_session", storyTag);
            return result.Obj is GodotObject obj ? new OghamSession(obj) : null;
        }

        public static bool HasSession(string storyTag) => (bool)Instance.Call("has_session", storyTag);
        public static void ForgetSession(string storyTag) => Instance.Call("forget_session", storyTag);

        public static OghamSession MainSession
        {
            get
            {
                Variant result = Instance.Call("get_main_session");
                return result.Obj is GodotObject obj ? new OghamSession(obj) : null;
            }
        }

        public static void SetMainSession(string storyTag) => Instance.Call("set_main_session", storyTag);
    }
}
