# Ogham Storyteller Foundation for Godot

![License](https://img.shields.io/badge/License-Apache_2.0-blue?style=flat-square)
![Maintained](https://img.shields.io/badge/Maintained%3F-yes-green?style=flat-square)
![Godot](https://img.shields.io/badge/Godot-4.6%20%2B-%23478CBF?style=flat-square&logo=godotengine&logoColor=white)

A tag-driven narrative/dialogue graph system for Godot 4. Nodes are identified by dot-path `GameplayTag`s, exactly like every other Heathen Foundation extension — Ogham has no identifier scheme, condition system, or state store of its own; it is entirely composed from [GameplayTags Foundation](https://github.com/heathen-engineering/Godot-GameplayTags-Foundation) and [Lexicon Foundation](https://github.com/heathen-engineering/Godot-Lexicon-Localisation-Foundation).

- **License:** Apache 2.0
- **Origin:** Heathen Group
- **Platforms:** Windows, Linux, macOS

> [!TIP]
> **Looking for the easiest way to install?**
> Copy `addons/FoundationOgham/` straight into your project's `addons/` folder — there's no external package manager step. See [Install](#install) below.

---

## 🛠 Also Available For

[![O3DE](https://img.shields.io/badge/O3DE-25.10%20%2B-%2300AEEF?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0id2hpdGUiIGQ9Ik0xMiAxTDEgNy40djkuMkwxMiAyM2wxMS02LjRWNy40TDEyIDF6bTkuMSAxNC45TDExLjUgMjEuM2wtOC42LTYuNFY4LjFsOC42LTYuNCA5LjEgNi40djYuOHpNMTEuNSA0LjZMMi45IDkuNnY0LjhsOC42IDUuMSA4LjYtNS4xVjkuNmwtOC42LTUuMHoiLz48L3N2Zz4=)](https://github.com/heathen-engineering/O3DE-Ogham-Storytelling-Foundation)
[![Unity](https://img.shields.io/badge/Unity-6000%20%2B-black?style=for-the-badge&logo=unity&logoColor=white)](https://github.com/heathen-engineering/Unity-Ogham-Storyteller-Foundation)

> [!NOTE]
> This Godot port treats [Unity-Ogham-Storyteller-Foundation](https://github.com/heathen-engineering/Unity-Ogham-Storyteller-Foundation) as the gold-standard reference. The `.ogham` JSON schema, the fork-routing/history/ReturnTo state machine, and the inline-link/`@Token` markup are ported from it directly.

---

## Requirements

- Godot **4.6** or compatible
- [godot-cpp](https://github.com/godotengine/godot-cpp), checked out locally, for building from source
- [GameplayTags Foundation](https://github.com/heathen-engineering/Godot-GameplayTags-Foundation) and [Lexicon Foundation](https://github.com/heathen-engineering/Godot-Lexicon-Localisation-Foundation) **enabled in the consuming project** — see [Dependency model](#dependency-model) below. Not required at Ogham's own build time.

---

## Become a GitHub Sponsor
[![Discord](https://img.shields.io/badge/Discord--1877F2?style=social&logo=discord)](https://discord.gg/6X3xrRc)
[![GitHub followers](https://img.shields.io/github/followers/heathen-engineering?style=social)](https://github.com/heathen-engineering?tab=followers)

Support Heathen by becoming a [GitHub Sponsor](https://github.com/sponsors/heathen-engineering). Sponsorship directly funds the development and maintenance of free tools like this, as well as our game development [Knowledge Base](https://heathen.group/) and community on [Discord](https://discord.gg/6X3xrRc).

Sponsors also get access to our private SourceRepo, which includes developer tools for O3DE, Unreal, Unity, and Godot.
Learn more or explore other ways to support @ [heathen.group/kb](https://heathen.group/kb/do-more/)

---

## What it does

| Type | Purpose |
|------|---------|
| `OghamEntry` | A `Resource` node in the story graph: content slots, entry operations, options |
| `OghamOption` | A `Resource` choice on an entry: target, guard conditions, side-effect operations |
| `OghamContentKey` | A `Resource` content slot (text/image/sprite/audio/packed scene), Localised/Literal/Invariant like Lexicon fields |
| `OghamStory` | A `Resource` immutable story definition — a flat list of entries plus a lazily-built tag/child index |
| `OghamSession` | A `RefCounted` state machine — one playthrough: narrative state, choice history, current entry |
| `Storyteller` | Engine singleton — the global entry point, one session per open story tag |
| `OghamInlineLinkParser` | Static utility — parses `[text](Ogham://Tag)` links and `**bold**`/`*italic*` markup |
| `OghamVariables` | Engine singleton — resolves `@Token(Tag.Path, args...)` text interpolation |

### Ogham has no logic of its own

Every guard on an `OghamOption` **is** a `GameplayTagCondition`; every side effect on an `OghamEntry`/`OghamOption` **is** a `GameplayTagOperation`; the narrative state an `OghamSession` carries **is** a `GameplayTagCollection`. Ogham calls `GameplayTagCondition.evaluate_all()` and `GameplayTagOperation.apply()` directly — it doesn't reimplement condition/operation logic in any form. This mirrors Unity's actual architecture exactly (confirmed by reading `OghamSession`/`OghamStoryBuilder` source): Ogham is a graph and a state machine wrapped around the GameplayTags gem's types, nothing more.

### Dependency model

Ogham has **no C++ compile-time dependency** on GameplayTags Foundation or Lexicon Foundation — each is a separately loaded GDExtension `.so`/`.dll`, and cross-extension calls go through Godot's `Variant`/`Object` API (`Engine.get_singleton(...)`, `ClassDB.instantiate(...)`, `object->call(...)`), exactly the boundary the C# facades already use. This is deliberate, not a shortcut: real cross-`.so` C++ linking between independently-versioned GDExtensions is fragile (build-order, ABI, load-order). The Variant boundary is what GDExtension actually guarantees is stable.

Practical effect: `addons/FoundationOgham/CMakeLists.txt` only needs `GODOT_CPP_PATH` — no `GODOT_XXHASH_PATH`, no GameplayTags/Lexicon source paths. At **runtime**, though, Ogham genuinely needs both extensions enabled in the project — every hash, every condition/operation, every localised string resolves through them. `Array` fields that hold `GameplayTagCondition`/`GameplayTagOperation`/`LexiconText` instances are typed generically (`Array` + a `PROPERTY_HINT_ARRAY_TYPE`/`PROPERTY_HINT_RESOURCE_TYPE` string, not a C++ template argument) so the Inspector still filters correctly without a compile-time class reference.

---

## Install

Copy `addons/FoundationOgham/` into your project's `addons/` folder, alongside `addons/FoundationGameplayTags/` and `addons/FoundationLexicon/`. Enable all three plugins from **Project Settings → Plugins**.

Drop `StorytellerRegistry.gd` onto a Node in your scene (`story_tag` + `story` exports) to open a session at boot, or call `Storyteller.open_session(...)` yourself from code.

## Build

Requires [godot-cpp](https://github.com/godotengine/godot-cpp) checked out locally.

```bash
cmake -S addons/FoundationOgham -B addons/FoundationOgham/build -DGODOT_CPP_PATH=/path/to/Godot-cpp
cmake --build addons/FoundationOgham/build
```

Output lands in `addons/FoundationOgham/bin/`.

---

## `.ogham` file format

```json
{
  "name": "CornerShop",
  "entries": [
    {
      "tag": "Shop.Outside",
      "mode": "Content",
      "contentKeys": [ { "type": "Text", "mode": "Literal", "key": "You stand outside a small shop." } ],
      "entryOperations": [ { "tag": "Shop.Visited", "arithmetic": "Set", "value": 1 } ],
      "options": [
        {
          "tag": "Shop.Outside.Enter",
          "targetTag": "Shop.Hub",
          "textKey": "Go inside.",
          "conditions": [],
          "operations": []
        }
      ]
    }
  ]
}
```

| Field | Description |
|-------|-------------|
| `entries[].tag` | The entry's dot-path identity |
| `entries[].mode` | `"Content"` (default, shown to the player) or `"Fork"` (silent routing node — see below) |
| `entries[].contentKeys[]` | `{type, mode, key, path, sub}` — `type` is `Text`/`Image`/`Sprite`/`Audio`/`Prefab` (accepted as an alias for `PackedScene`); `mode`/`key` follow the same Localised/Literal/Invariant convention as Lexicon; `path`/`sub` are used for asset entries (a `res://` path, optionally a sub-resource name) |
| `entries[].entryOperations[]` | `GameplayTagOperation` DTOs (`tag`, `arithmetic`, `value`, `valueTag`, `valueType`, `conditions`) applied on entry, before display |
| `entries[].options[].tag` | The option's dot-path identity |
| `entries[].options[].targetTag` | The entry to navigate to; omit/empty to close the conversation |
| `entries[].options[].textKey` | Plain string; becomes a `LexiconText` in Literal mode (author Localised text via a full `LexiconText` resource assigned in the Inspector instead, for now) |
| `entries[].options[].conditions[]` | `GameplayTagCondition` DTOs (`tag`, `comparison`, `compareValue`, `compareTag`, `exactMatch`, `logicOp`, `compareValueType`) |
| `entries[].options[].operations[]` | `GameplayTagOperation` DTOs, same shape as `entryOperations` |

**Arithmetic/comparison string values** accept both the full Unity spelling and O3DE's short forms for cross-engine `.ogham` portability (`"Subtract"`/`"Sub"`, `"Multiply"`/`"Mul"`, `"Divide"`/`"Div"`), matching an explicit compatibility seam documented in Unity's manifest parser.

Every tag referenced by an entry or option (including option targets) is registered into `GameplayTagRegistry` when a manifest is parsed, so hierarchy-based conditions work immediately.

---

## Usage overview

### Fork nodes — silent routing

An entry with `mode == Fork` is never shown to the player and never recorded in history. `OghamSession` evaluates its options as routes (in order) and immediately takes the first whose conditions pass, applying that route's operations and looping — this is how Ogham expresses "if state X, go to A, else go to B" without a dedicated branch-node type. A cycle between forks is detected at runtime (a safety net behind editor-time validation) and closes the session with an error rather than hanging.

### Playing a story

```gdscript
var story := OghamStory.parse_manifest(FileAccess.open("res://stories/corner_shop.ogham", FileAccess.READ).get_as_text())
var storyteller := Engine.get_singleton("Storyteller")
var session: Object = storyteller.open_session("CornerShop", story, true)

session.connect("entered", func(entry_tag: int):
    print(session.get_current_text(0))
    for option_tag in session.get_current_options():
        print(" -> ", session.get_option_text(option_tag)))

var registry := Engine.get_singleton("GameplayTagRegistry")
session.enter(registry.hash("Shop.Outside"))
session.choose(registry.hash("Shop.Outside.Enter"))
```

### Save / load

```gdscript
var save_dict := session.snapshot("slot1")
# ... persist save_dict (JSON/ConfigFile/etc) ...
session.restore(save_dict) # leaves the session inactive
session.resume()           # re-surfaces the current entry
```

### Rendering with inline links and variables

```gdscript
var raw_text := session.get_current_text(0) # e.g. "You have @Int(Shop.Basket.Count) items. [Pay up](Ogham://Shop.Pay)."
var interpolated := Engine.get_singleton("OghamVariables").interpolate(raw_text, session.get_state())
var bbcode := OghamInlineLinkParser.to_bbcode(interpolated)
rich_text_label.text = bbcode # RichTextLabel with bbcode_enabled = true
```

**C#** — every type above has a matching wrapper class in `Heathen.Ogham` (`OghamEntry`, `OghamOption`, `OghamContentKey`, `OghamStory`, `OghamSession`, `Storyteller`, `OghamInlineLinkParser`, `OghamVariables`) so C# code never touches `Engine.GetSingleton`/`Variant.Call` directly. See the `CSharp/` folder.

---

## Public API reference

### `OghamSession` (`RefCounted`)

| Method | Description |
|--------|-------------|
| `enter(entry_tag)` | Closes any active conversation, enters the given entry (running Fork routing/entry operations as needed) |
| `choose(option_tag)` | Applies the option's operations, records the choice, navigates to its target (or closes if none). `false` only if `option_tag` isn't currently active |
| `close()` | Closes the active conversation |
| `resume()` | Re-surfaces the current entry without re-running entry operations/Fork routing |
| `return_to(entry_tag)` | Navigates back, clearing narrative-state tags recorded for every descendant entry first |
| `get_state()` | The session's `GameplayTagCollection` narrative state (lazily instantiated) |
| `get_current_entry_tag()` / `get_current_content_count()` / `get_current_text(i)` / `get_current_asset(i)` | Current entry content |
| `get_current_options()` / `get_current_all_options()` | Active option tags / every option tag (gated included, for inline-link rendering) |
| `is_option_active(tag)` / `get_option_text(tag)` | Option queries |
| `get_history()` | `Array[Dictionary{entry_id, selected_option}]`, append-only |
| `snapshot(name)` / `restore(dict)` | Save/load |
| `entered(entry_tag)` / `choice(option_tag)` / `closed` *(signals)* | — |

### `Storyteller` (engine singleton)

| Method | Description |
|--------|-------------|
| `open_session(story_tag, story, set_as_main = true)` | Returns the existing session for `story_tag` if one is open, else creates one |
| `get_session(story_tag)` / `has_session(story_tag)` / `forget_session(story_tag)` | Session lookup/lifecycle |
| `get_main_session()` / `set_main_session(story_tag)` | The "current/focused" session — first-opened by default |

### `OghamStory` (`Resource`)

| Method | Description |
|--------|-------------|
| `find_entry(tag)` / `find_entry_by_name(path)` | Entry lookup |
| `collect_descendants(tag)` | Every entry reachable below `tag` via the option graph (BFS, deduplicated) |
| `collect_within_depth(tag, depth)` | `tag` plus every entry reachable within `depth` option-hops |
| `parse_manifest(json_text)` *(static)* | Parses `.ogham` JSON into a new `OghamStory`, registering referenced tags |

### `OghamInlineLinkParser` (static utility)

`is_ogham_link(url)`, `get_tag_path(url)`, `is_pure_link(text)`, `extract_links(text)`, `to_bbcode(text)`, `strip_markup(text)`, `normalise_for_tag(text)`.

### `OghamVariables` (engine singleton)

`interpolate(text, state)` resolves `@Token(Tag.Path[, arg])` spans. Built-in tokens: `@String` (resolves the stored value as a Lexicon key), `@Float`/`@Double` (optional second arg = decimal places), `@Long`/`@Ulong`/`@Int`/`@UInt`. `register_variable(name, callback)`/`unregister_variable(name)` add custom tokens via `Callable(tag_path, extra_arg, state) -> String`.

---

## Editor tooling

Enable the plugin (Project Settings > Plugins) to get a compact `OghamContentKey` Inspector editor: which fields are even shown depends on Type+Mode (only `key_or_value` for Text content; `asset_path`/`asset_sub_name` for Sprite/Image/Audio/PackedScene in Literal/Invariant mode; a Lexicon key picker instead, when Localised and Lexicon Foundation's plugin is also enabled) — mirrors Unity's `OghamEntryDrawer`/`OghamKeyEditWindow` intent that showing every field flat invites authoring mistakes. `OghamOption.target_entry_path` also gets GameplayTags Foundation's tag-picker, when that plugin is enabled.

The full GraphEdit-based node authoring editor (create/edit/delete entries, options, and content keys on a canvas, writing back to `.ogham` JSON directly) lives in the commercial Toolkit for Ogham Storyteller extension, not this Foundation — see that addon's own README for details.

## Not (yet) ported

- **Presentation-layer adapters** — Unity's `OghamTemplateSpawner` (prefab instancing per content key) and `StorytellerInspector` (no-code UnityEvent bridge) are Unity scene/Inspector-specific; the Godot-native equivalents (a Node instancing `PackedScene`s, a Node exposing `signal`s wired in the editor) are a deliberate fresh design, not a code port, and aren't included yet.
- **Windowed asset streaming** — Unity's `OghamAssetStreamer` (reference-counted N-hop-lookahead asset preloading via `LexiconRegistry.AcquireAssetByGuidAsync`) isn't ported. `OghamContentKey.resolve_asset()` loads synchronously via `ResourceLoader` today, same simplification Lexicon Foundation itself makes.
- **Alias pins and inline-link auto-synthesis** (turning a lone `[text](Ogham://Tag)` content key into a real `OghamOption` automatically) are deferred.
- **ECS/Burst state snapshots** — Unity's `#if UNITY_ENTITIES` narrative-state snapshot has no Godot equivalent need, same reasoning as GameplayTags Foundation's `NativeIntervalMap`.

## License

Apache 2.0.
