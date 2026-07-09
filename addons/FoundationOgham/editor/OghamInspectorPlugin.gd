@tool
extends EditorInspectorPlugin
class_name OghamInspectorPlugin

## Registers the compact OghamContentKey editor, and swaps FoundationGameplayTags'
## tag-picker in for OghamOption's "target_entry_path" (the target entry's dot-path
## tag — semantically a GameplayTag reference even though it isn't named "*_tag_name",
## so it doesn't already get picked up by FoundationGameplayTags' own naming
## convention). "tag_name" fields on OghamEntry/OghamOption already get the picker
## for free via that convention as long as FoundationGameplayTags is enabled — no
## extra work needed here for those.

const CONTENT_KEY_PROPERTIES := ["type", "mode", "key_or_value", "asset_path", "asset_sub_name"]

func _can_handle(object: Object) -> bool:
	return true

func _parse_begin(object: Object) -> void:
	if object is OghamContentKey:
		add_property_editor_for_multiple_properties("Content", CONTENT_KEY_PROPERTIES, OghamContentKeyCompactEditor.new())

func _parse_property(object: Object, type: int, name: String, hint_type: int, hint_string: String, usage_flags: int, wide: bool) -> bool:
	if object is OghamContentKey and CONTENT_KEY_PROPERTIES.has(name):
		return true

	if object is OghamOption and name == "target_entry_path" and _script_class_exists("GameplayTagPickerProperty"):
		add_property_editor(name, load("res://addons/FoundationGameplayTags/editor/GameplayTagPickerProperty.gd").new())
		return true

	return false

func _script_class_exists(script_name: String) -> bool:
	return ResourceLoader.exists("res://addons/FoundationGameplayTags/editor/%s.gd" % script_name)
