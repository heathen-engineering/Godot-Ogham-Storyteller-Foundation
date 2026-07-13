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
##
## Deliberately uses object.is_class("OghamContentKey"/"OghamOption") instead of
## `object is OghamContentKey` — those are native gdextension classes that stay inert
## until Extension Resolver unlocks the gate. `is Type` is a static identifier GDScript
## must resolve at PARSE time, and this script's own class_name makes it a target of
## Godot's eager project-wide class scan on editor startup, independent of when anyone
## actually load()s it — same bug class found and fixed in FoundationGameplayTags'
## GameplayTagsInspectorPlugin.gd this session. is_class() is a plain method call with
## a String literal, so the parser never needs the type to exist.

const CONTENT_KEY_PROPERTIES := ["type", "mode", "key_or_value", "asset_path", "asset_sub_name"]

func _can_handle(object: Object) -> bool:
	return true

func _parse_begin(object: Object) -> void:
	if object.is_class("OghamContentKey"):
		add_property_editor_for_multiple_properties("Content", CONTENT_KEY_PROPERTIES, OghamContentKeyCompactEditor.new())

func _parse_property(object: Object, type: int, name: String, hint_type: int, hint_string: String, usage_flags: int, wide: bool) -> bool:
	if object.is_class("OghamContentKey") and CONTENT_KEY_PROPERTIES.has(name):
		return true

	if object.is_class("OghamOption") and name == "target_entry_path" and _script_class_exists("GameplayTagPickerProperty"):
		add_property_editor(name, load("res://addons/FoundationGameplayTags/editor/GameplayTagPickerProperty.gd").new())
		return true

	return false

func _script_class_exists(script_name: String) -> bool:
	return ResourceLoader.exists("res://addons/FoundationGameplayTags/editor/%s.gd" % script_name)
