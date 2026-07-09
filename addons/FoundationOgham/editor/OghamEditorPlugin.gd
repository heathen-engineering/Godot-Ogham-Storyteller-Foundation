@tool
extends EditorPlugin

## Activates the Ogham editor tooling: the compact OghamContentKey inspector plugin
## (plus reusing FoundationGameplayTags/FoundationLexicon's pickers when those
## addons are enabled). Enable via Project Settings > Plugins (this addon's
## plugin.cfg now points here).

var _inspector_plugin: OghamInspectorPlugin

func _enter_tree() -> void:
	_inspector_plugin = OghamInspectorPlugin.new()
	add_inspector_plugin(_inspector_plugin)

func _exit_tree() -> void:
	if _inspector_plugin != null:
		remove_inspector_plugin(_inspector_plugin)
		_inspector_plugin = null
