Bringing the power of a non-linear narrative editor directly into your game engine. We believe crafting intricate, branching stories shouldn’t require a computer science degree or an external toolchain, so we built a tool that lets you author and test rich narratives with absolutely zero coding required. By bringing the writer’s studio right into your workspace, you can seamlessly reference your game assets, prefabs, sounds, and localization files exactly where you need them.

Behind its intuitive design lies a robust powerhouse capable of handling complex interactive storytelling. Ogham Storyteller features a persistent state system, history tracking, and conditional gates to ensure your choices carry real weight. And when you are ready to connect those choices to the rest of your game, our flexible, event-based Storyteller API lets you effortlessly pass data back and forth, allowing your narrative to drive your gameplay and vice versa.

## Tell your story with Heathen’s Ogham Storyteller

### For Writers by Writers!
Ogham Storyteller was created with our writers to deliver an in engine editor designed for your narrative team. You can now write, test, build and deploy a complete working visual novel with rich visuals, sound and text complete dynamic conditions and branching logic all with zero-code.

### Content Editor
Storyteller features an intuitive easy to use story graph for the writer. You can simply click and type and see the story unfold. Our graph is seamlessly integrated with Gamplay Tags and Lexicon Localisation ensuring your stories are natively part of the wider game.

### Voice Over Script
A visual narrative editor built for game. Annotate and export your story as a structured script for Voice Over work right from the editor. You can export in multiple languages and formats and control the formatting of the resulting script.

### Production Ready Tools
Connect your story to your game world. Traditional “Visual Novels” can be authored 100% with zero-code required featuring animations, images, sounds, dynamic state values and of course rich animated text.

Interactive quests, narrative driven RPGs and more can be created using the Storyteller API to connect your game world to the Ogham Storyteller with both read and write support for story state.

## Integrations & Dependencies
### Gameplay Tags
A rich hierarchical tag system built for performance and easy of use. Every node, every option, every stat and item and more can be identified with a human friendly tag. Tags organise by dot path and can be searched, set tested and have values set.

Ogham uses tags to track story state and history and can use tags to “gate” options

## What it does

Ogham has no logic of its own. Every guard on an option is a real GameplayTagCondition, every side effect is a real GameplayTagOperation, and the narrative state a session carries is a real GameplayTagCollection. It is a graph and a state machine wrapped around the GameplayTags gem's own types, nothing more.

Built on:

- OghamEntry: a story graph node with content slots, entry operations, and options.
- OghamOption: a choice on an entry with target, guard conditions, and side-effect operations.
- OghamContentKey: a content slot (text, image, sprite, audio, packed scene), Localised, Literal, or Invariant like Lexicon fields.
- OghamStory: an immutable story definition.
- OghamSession: one playthrough, tracking narrative state, choice history, and the current entry.
- Storyteller: the global entry point, one session per open story tag.

Fork nodes provide silent routing (evaluating conditions and jumping without showing anything to the player), inline links and bold/italic markup render through a dedicated parser, and a token-interpolation system resolves values like character stats directly into dialogue text.

## Editor tooling

- A full GraphEdit-based node authoring editor: create, edit, and delete entries, options, and content keys on a canvas, writing back to plain JSON directly. Includes search and filter, a Tree view alternative to the graph canvas, and automatic layout.
- Fork-cycle validation directly on the graph canvas.
- An in-editor dialogue playtester that runs against your live, unsaved edits.
- Voice-over script export to CSV, Markdown, HTML, or plain text, with per-character filtering.
- A compact content-key Inspector editor, and tag-picker/key-picker integration when GameplayTags and Lexicon are also enabled.

## Works from both GDScript and C#

A matching C# wrapper class exists for every runtime type, so C# code never touches Engine.GetSingleton or Variant.Call directly.

## Requirements

- Godot 4.6 or compatible
- [Godot Game Framework](https://github.com/heathen-engineering/Godot-Game-Framework), [GameplayTags Foundation](https://github.com/heathen-engineering/Godot-GameplayTags-Foundation), and [Lexicon Foundation](https://github.com/heathen-engineering/Godot-Lexicon-Localisation-Foundation), all enabled in the consuming project. If any is missing, enabling this plugin walks you through fetching it automatically.

## Links

- GitHub: [https://github.com/heathen-engineering/Godot-Ogham-Storyteller-Foundation](https://github.com/heathen-engineering/Godot-Ogham-Storyteller-Foundation)
- Support and Discord: [https://discord.gg/xmtRNkW7hW](https://discord.gg/xmtRNkW7hW)
- License: Apache 2.0

## Want more?

Toolkit for Ogham Storyteller, available to GitHub Sponsors, extends Foundation's editor tooling further. Learn more at [https://heathen.group/kb/do-more/](https://heathen.group/kb/do-more/)
