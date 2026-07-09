@tool
extends EditorProperty
class_name OghamContentKeyCompactEditor

## One-row compact editor for OghamContentKey — Type | Mode | (key_or_value, shown as
## a Lexicon key picker when Localised, else a plain text field for Text-type content;
## asset_path + asset_sub_name for Sprite/Image/Audio/PackedScene content in
## Literal/Invariant mode). Mirrors OghamEntryDrawer/OghamKeyEditWindow's intent from
## Unity: which fields are even relevant depends entirely on type+mode, and showing
## all of them flat invites authoring mistakes.
##
## Reuses FoundationLexicon's LexiconKeyPickerProperty directly (no compile-time
## dependency — this addon's plugin.cfg lists FoundationLexicon as a soft
## requirement for Localised-mode content the same way OghamInterop looks up
## LexiconRegistry at runtime via Engine.has_singleton()).

const TYPES := ["Text", "Image", "Sprite", "Audio", "PackedScene"]
const MODES := ["Localised", "Literal", "Invariant"]
const TYPE_TEXT := 0

var _type: OptionButton
var _mode: OptionButton
var _key_picker: Control
var _literal_text_edit: LineEdit
var _asset_path_edit: LineEdit
var _asset_sub_edit: LineEdit
var _updating := false

func _init() -> void:
	var row := HBoxContainer.new()

	_type = OptionButton.new()
	for t in TYPES:
		_type.add_item(t)
	_type.item_selected.connect(_on_type_selected)
	row.add_child(_type)

	_mode = OptionButton.new()
	for m in MODES:
		_mode.add_item(m)
	_mode.item_selected.connect(_on_mode_selected)
	row.add_child(_mode)

	_literal_text_edit = LineEdit.new()
	_literal_text_edit.placeholder_text = "Text content..."
	_literal_text_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_literal_text_edit.text_submitted.connect(_on_literal_text_submitted)
	_literal_text_edit.focus_exited.connect(func(): _on_literal_text_submitted(_literal_text_edit.text))
	row.add_child(_literal_text_edit)

	_asset_path_edit = LineEdit.new()
	_asset_path_edit.placeholder_text = "res://path/to/asset"
	_asset_path_edit.custom_minimum_size = Vector2(200, 0)
	_asset_path_edit.text_submitted.connect(_on_asset_path_submitted)
	_asset_path_edit.focus_exited.connect(func(): _on_asset_path_submitted(_asset_path_edit.text))
	row.add_child(_asset_path_edit)

	_asset_sub_edit = LineEdit.new()
	_asset_sub_edit.placeholder_text = "sub-resource"
	_asset_sub_edit.custom_minimum_size = Vector2(100, 0)
	_asset_sub_edit.text_submitted.connect(_on_asset_sub_submitted)
	_asset_sub_edit.focus_exited.connect(func(): _on_asset_sub_submitted(_asset_sub_edit.text))
	row.add_child(_asset_sub_edit)

	# LexiconKeyPickerProperty is only available if FoundationLexicon's plugin is
	# enabled; instantiate it lazily via its global class_name if present, otherwise
	# Localised mode falls back to the plain literal text field for key_or_value.
	if _script_class_exists("LexiconKeyPickerProperty"):
		_key_picker = load("res://addons/FoundationLexicon/editor/LexiconKeyPickerProperty.gd").new()
		_key_picker.custom_minimum_size = Vector2(200, 0)
		_key_picker.property_changed.connect(_relay_property_changed)
		row.add_child(_key_picker)

	add_child(row)
	add_focusable(_type)

func _script_class_exists(name: String) -> bool:
	return ResourceLoader.exists("res://addons/FoundationLexicon/editor/%s.gd" % name)

func _update_property() -> void:
	_updating = true
	var obj := get_edited_object()

	var type_val: int = obj.get("type")
	var mode_val: int = obj.get("mode")
	_type.select(type_val)
	_mode.select(mode_val)

	var is_localised := mode_val == 0
	var is_text := type_val == TYPE_TEXT

	_literal_text_edit.visible = is_text and not is_localised
	if _key_picker != null:
		_key_picker.visible = is_localised
	_asset_path_edit.visible = not is_text and not is_localised
	_asset_sub_edit.visible = not is_text and not is_localised

	if is_localised and _key_picker != null:
		_key_picker.set_object_and_property(obj, "key_or_value")
		_key_picker._update_property()
	elif is_text:
		_literal_text_edit.text = obj.get("key_or_value")
	else:
		_asset_path_edit.text = obj.get("asset_path")
		_asset_sub_edit.text = obj.get("asset_sub_name")

	_updating = false

func _on_type_selected(index: int) -> void:
	if _updating:
		return
	emit_changed("type", index)

func _on_mode_selected(index: int) -> void:
	if _updating:
		return
	emit_changed("mode", index)

func _on_literal_text_submitted(text: String) -> void:
	if _updating:
		return
	emit_changed("key_or_value", text)

func _on_asset_path_submitted(text: String) -> void:
	if _updating:
		return
	emit_changed("asset_path", text)

func _on_asset_sub_submitted(text: String) -> void:
	if _updating:
		return
	emit_changed("asset_sub_name", text)

func _relay_property_changed(property: StringName, value: Variant, field: StringName = "", changing: bool = false) -> void:
	if _updating:
		return
	if property == "key_or_value":
		emit_changed("key_or_value", value, field, changing)
