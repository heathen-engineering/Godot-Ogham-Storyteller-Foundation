@tool
extends EditorPlugin

## Activates the Ogham editor tooling: the compact OghamContentKey inspector plugin
## (plus reusing FoundationGameplayTags/FoundationLexicon's pickers when those
## addons are enabled), and the Ogham Storyteller node-graph editor — a real,
## floating-capable EditorDock (not the old bottom-panel API, which cannot
## float; see Godot-Game-Framework's plan doc, "Editor UX Unification", for
## why). Enable via Project Settings > Plugins (this addon's plugin.cfg now
## points here).
##
## Gated: FoundationOgham.gdextension (the native library everything here
## ultimately depends on — Storyteller, OghamVariables, the Subsystem
## integration, all of it) ships inert until Extension Resolver confirms
## Godot-Game-Framework is actually installed (at a satisfying version —
## real version-guarding, not just presence, since the migration off
## heathen_gate.gd). See gate/extension_resolver_gate.gd and
## Godot-GameplayTags-Foundation's GameplayTagsEditorPlugin.gd (the
## reference implementation this mirrors) for the full mechanism and why.
##
## OghamInspectorPlugin is loaded with a runtime load(), not a static
## type/preload — it does "is OghamContentKey"/"is OghamOption" checks
## internally, and GDScript resolves type identifiers at COMPILE time of
## whatever file references them. A plain typed var here would force
## GDScript to parse that whole file (and therefore resolve the native
## types) before the gate has a chance to run. OghamGraphView is instantiated
## via ClassDB.instantiate() below for the same reason — it's a native class
## reached by string, never a static GDScript type reference in this file.

const Gate = preload("res://addons/FoundationOgham/gate/extension_resolver_gate.gd")

var _inspector_plugin: Object
var _graph_dock: EditorDock

func _enter_tree() -> void:
	if Gate.ensure_unlocked(self, "FoundationOgham", _activate_tooling):
		_activate_tooling()

# NOT named _build() — EditorPlugin already declares a virtual _build() -> bool
# (asks whether a custom pre-run build step should block "Run Project"); naming
# this the same collides with it and breaks script parsing entirely.
func _activate_tooling() -> void:
	var inspector_script: GDScript = load("res://addons/FoundationOgham/editor/OghamInspectorPlugin.gd")
	_inspector_plugin = inspector_script.new()
	add_inspector_plugin(_inspector_plugin)

	_graph_dock = EditorDock.new()
	_graph_dock.title = "Ogham Storyteller"
	_graph_dock.set_default_slot(EditorDock.DOCK_SLOT_BOTTOM)
	_graph_dock.set_available_layouts(EditorDock.DOCK_LAYOUT_ALL)
	_graph_dock.set_closable(true)
	var graph_view: Control = ClassDB.instantiate("OghamGraphView")
	_graph_dock.add_child(graph_view)
	add_dock(_graph_dock)

	var bridge = Engine.get_singleton("SubsystemManagerBridge")
	if bridge != null:
		bridge.register_tool_window_opener("Ogham", Callable(self, "_open_graph_dock"))
		bridge.register_settings_panel("Ogham", Callable(self, "_build_settings_panel"))

func _open_graph_dock() -> void:
	if _graph_dock != null:
		_graph_dock.open()

func _build_settings_panel() -> Control:
	return OghamSettingsPanel.new()

func _exit_tree() -> void:
	if _inspector_plugin != null:
		remove_inspector_plugin(_inspector_plugin)
		_inspector_plugin = null
	if _graph_dock != null:
		remove_dock(_graph_dock)
		_graph_dock.queue_free()
		_graph_dock = null
