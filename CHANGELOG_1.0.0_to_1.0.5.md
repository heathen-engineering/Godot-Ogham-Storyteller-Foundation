# Changelog: v1.0.0 to v1.0.5

## New

- **Fork-cycle validation**: a Fork node participating in a routing cycle is now flagged directly on the graph canvas (a colored node stripe), instead of only failing at runtime.
- **In-editor dialogue playtester**: run a story from any entry against your live, unsaved edits, without needing to launch the game.
- **Voice-over script export**: export dialogue as CSV, Markdown, HTML, or plain text, with real Content-Label field filtering so a specific character's lines can be exported on their own. Includes a new Director Notes editor.

## Fixes

- Removed a stale `OghamContentKeyCompactEditor.gd` script left behind after that editor was ported to native C++. The leftover file could shadow the compiled version depending on load order.

## Other

- Publisher metadata standardized, and documentation/support links updated to point at the Heathen Group Knowledge Base and Discord.
- CI build workflow improvements for reliability and packaging correctness.
