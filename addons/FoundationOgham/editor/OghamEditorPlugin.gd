@tool
extends EditorPlugin

## Activates the Ogham editor tooling: the compact OghamContentKey inspector plugin
## (plus reusing FoundationGameplayTags/FoundationLexicon's pickers when those
## addons are enabled). Enable via Project Settings > Plugins (this addon's
## plugin.cfg now points here).
##
## Gated: FoundationOgham.gdextension (the native library everything here
## ultimately depends on — Storyteller, OghamVariables, the Subsystem
## integration, all of it) ships inert until heathen_gate confirms
## Godot-Game-Framework is actually installed. See gate/heathen_gate.gd and
## Godot-GameplayTags-Foundation's GameplayTagsEditorPlugin.gd (the
## reference implementation this mirrors) for the full mechanism and why.
##
## OghamInspectorPlugin is loaded with a runtime load(), not a static
## type/preload — it does "is OghamContentKey"/"is OghamOption" checks
## internally, and GDScript resolves type identifiers at COMPILE time of
## whatever file references them. A plain typed var here would force
## GDScript to parse that whole file (and therefore resolve the native
## types) before the gate has a chance to run.

const HeathenGate = preload("res://addons/FoundationOgham/gate/heathen_gate.gd")

var _inspector_plugin: Object

func _enter_tree() -> void:
	if HeathenGate.ensure_unlocked(self, "FoundationOgham", _activate_tooling):
		_activate_tooling()

# NOT named _build() — EditorPlugin already declares a virtual _build() -> bool
# (asks whether a custom pre-run build step should block "Run Project"); naming
# this the same collides with it and breaks script parsing entirely.
func _activate_tooling() -> void:
	var inspector_script: GDScript = load("res://addons/FoundationOgham/editor/OghamInspectorPlugin.gd")
	_inspector_plugin = inspector_script.new()
	add_inspector_plugin(_inspector_plugin)

func _exit_tree() -> void:
	if _inspector_plugin != null:
		remove_inspector_plugin(_inspector_plugin)
		_inspector_plugin = null
