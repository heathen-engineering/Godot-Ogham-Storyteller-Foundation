@tool
extends Control
class_name OghamSettingsPanel

## Ogham Storyteller's settings page — handed to the unified Project Settings
## Subsystems tab (see SubsystemManagerBridge::register_settings_panel()).
## Scoped exactly like the reference Unity OghamStorytellerSettingsProvider:
## the VO-export Content Labels (an ordered, reorderable list of row-header
## names — Stage, Narrator, Speaker Name, etc. — stored as the
## "ogham/editor/key_labels" ProjectSettings entry, the same key
## OghamKeyLabelsNative/OghamKeyLabelsPopup already read/write natively) plus
## a button to open the graph editor tool window. The graph content itself
## (stories, entries, options) is never touched from here — same split Unity
## uses, where Content Labels are a project-wide setting but story authoring
## is a separate tool window.
##
## Unity's provider also exposes "Default Link Colour"/"Default Link
## Underline" graph-editor preferences — deliberately NOT ported here, since
## the Godot OghamGraphView has no such concept yet (grepped: no
## link-colour/underline handling anywhere in the port). Adding settings UI
## for a preference the graph editor can't actually consume would be dead
## control, not a real feature.

const SETTING_PATH := "ogham/editor/key_labels"

var _list_box: VBoxContainer
var _new_name_edit: LineEdit

func _ready() -> void:
	_ensure_built()

func _ensure_built() -> void:
	set_h_size_flags(Control.SIZE_EXPAND_FILL)
	set_v_size_flags(Control.SIZE_EXPAND_FILL)

	var root_vbox := VBoxContainer.new()
	root_vbox.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(root_vbox)

	var title := Label.new()
	title.text = "Ogham Storyteller — VO Export Content Labels"
	title.add_theme_font_size_override("font_size", 14)
	root_vbox.add_child(title)

	var help := Label.new()
	help.text = "Content Labels define the row header for each content key slot in exported VO scripts.\nLabel 1 -> Content Key 0, Label 2 -> Content Key 1, and so on."
	help.autowrap_mode = TextServer.AUTOWRAP_WORD
	root_vbox.add_child(help)
	root_vbox.add_child(HSeparator.new())

	_list_box = VBoxContainer.new()
	root_vbox.add_child(_list_box)

	var add_row := HBoxContainer.new()
	_new_name_edit = LineEdit.new()
	_new_name_edit.placeholder_text = "New label..."
	_new_name_edit.set_h_size_flags(Control.SIZE_EXPAND_FILL)
	_new_name_edit.text_submitted.connect(func(_text: String): _on_add())
	add_row.add_child(_new_name_edit)
	var add_btn := Button.new()
	add_btn.text = "Add Label"
	add_btn.pressed.connect(_on_add)
	add_row.add_child(add_btn)
	root_vbox.add_child(add_row)

	root_vbox.add_child(HSeparator.new())

	var open_btn := Button.new()
	open_btn.text = "Open Ogham Storyteller Graph Editor"
	open_btn.pressed.connect(_on_open_graph_editor)
	root_vbox.add_child(open_btn)

	_rebuild_list()

func _get_labels() -> PackedStringArray:
	if not ProjectSettings.has_setting(SETTING_PATH):
		return PackedStringArray()
	var value = ProjectSettings.get_setting(SETTING_PATH)
	if value is PackedStringArray:
		return value
	if value is Array:
		return PackedStringArray(value)
	return PackedStringArray()

func _set_labels(labels: PackedStringArray) -> void:
	ProjectSettings.set_setting(SETTING_PATH, labels)
	ProjectSettings.set_initial_value(SETTING_PATH, PackedStringArray())
	ProjectSettings.save()
	_rebuild_list()

func _rebuild_list() -> void:
	for child in _list_box.get_children():
		child.queue_free()

	var labels := _get_labels()
	for i in range(labels.size()):
		var row := HBoxContainer.new()

		var index_label := Label.new()
		index_label.text = "%d." % (i + 1)
		index_label.custom_minimum_size = Vector2(22, 0)
		row.add_child(index_label)

		var name_edit := LineEdit.new()
		name_edit.text = labels[i]
		name_edit.set_h_size_flags(Control.SIZE_EXPAND_FILL)
		name_edit.text_submitted.connect(_rename.bind(i))
		name_edit.focus_exited.connect(func(): _rename(name_edit.text, i))
		row.add_child(name_edit)

		var up_btn := Button.new()
		up_btn.text = "^"
		up_btn.disabled = i == 0
		up_btn.pressed.connect(_move.bind(i, -1))
		row.add_child(up_btn)

		var down_btn := Button.new()
		down_btn.text = "v"
		down_btn.disabled = i == labels.size() - 1
		down_btn.pressed.connect(_move.bind(i, 1))
		row.add_child(down_btn)

		var delete_btn := Button.new()
		delete_btn.text = "X"
		delete_btn.pressed.connect(_delete.bind(i))
		row.add_child(delete_btn)

		_list_box.add_child(row)

func _rename(text: String, index: int) -> void:
	var labels := _get_labels()
	if index < 0 or index >= labels.size() or labels[index] == text:
		return
	labels[index] = text
	_set_labels(labels)

func _move(index: int, delta: int) -> void:
	var labels := _get_labels()
	var target := index + delta
	if index < 0 or index >= labels.size() or target < 0 or target >= labels.size():
		return
	var tmp := labels[index]
	labels[index] = labels[target]
	labels[target] = tmp
	_set_labels(labels)

func _delete(index: int) -> void:
	var labels := _get_labels()
	if index < 0 or index >= labels.size():
		return
	labels.remove_at(index)
	_set_labels(labels)

func _on_add() -> void:
	var name_text := _new_name_edit.text.strip_edges()
	if name_text.is_empty():
		return
	var labels := _get_labels()
	labels.append(name_text)
	_new_name_edit.text = ""
	_set_labels(labels)

func _on_open_graph_editor() -> void:
	var bridge = Engine.get_singleton("SubsystemManagerBridge")
	if bridge != null and bridge.has_tool_window("Ogham"):
		bridge.open_tool_window("Ogham")
