@tool
extends RefCounted

## Extension Resolver Gate — COPY THIS FILE VERBATIM into every participating
## extension's own addons/<Name>/gate/extension_resolver_gate.gd, replacing
## the old heathen_gate.gd. Deliberately self-contained: no class_name
## (avoids a "class already in use" collision once multiple copies of this
## same file exist in one project), and no reference whatsoever to
## ExtensionResolverCore/ExtensionManifestReader/etc. — its entire job is to
## work correctly BEFORE Extension Resolver itself can be assumed present,
## so it can't lean on any of that.
##
## What changed from heathen_gate.gd: that script understood an addon's full
## dependency list itself (fetch mechanics, version-checking — well, no
## version-checking, that was the whole gap this tool exists to close) and
## had to be copy-pasted with all of that logic duplicated per addon. This
## script understands exactly ONE dependency — Extension Resolver itself —
## and delegates everything else (the real dependency graph, version
## guarding, fetching) to it once it's confirmed present. Every addon's copy
## of this file is now doing meaningfully less than heathen_gate.gd did.
##
## How: same inert-.gdextension-until-ready precedent doesn't apply to
## Extension Resolver itself (it's pure GDScript, no native binary, nothing
## that can hard-crash on a missing dependency) — so there's nothing to
## rename here. Once Extension Resolver is confirmed installed and enabled,
## this hands off to Engine.get_singleton("ExtensionResolver").resolve(...),
## which performs ITS caller's own gating unlock (the .gdextension.available
## dance) if that caller's manifest declares "gated": true. This script's
## own job stops at "is Extension Resolver here yet."

const EXTENSION_RESOLVER_ID := "ExtensionResolver"
const EXTENSION_RESOLVER_REPO := "heathen-engineering/Godot-Extension-Resolver"
const GITHUB_API_ROOT := "https://api.github.com/repos/"

## Call from the asset's own EditorPlugin._enter_tree(). Returns true if
## Extension Resolver is already present and this addon_id's own
## dependencies are already resolved — caller can proceed immediately,
## synchronously. Returns false otherwise; on_unlocked (a zero-argument
## Callable) is invoked later, once resolution completes (possibly after
## fetching Extension Resolver itself, then after fetching addon_id's own
## dependencies). host is any Node currently in the tree, used only as a
## parent for dialogs/HTTPRequests.
static func ensure_unlocked(host: Node, addon_id: String, on_unlocked: Callable) -> bool:
    if Engine.has_singleton(EXTENSION_RESOLVER_ID):
        return Engine.get_singleton(EXTENSION_RESOLVER_ID).resolve(host, addon_id, on_unlocked)

    if _is_installed_but_not_enabled():
        EditorInterface.set_plugin_enabled(EXTENSION_RESOLVER_ID, true)
        # set_plugin_enabled() synchronously runs the target plugin's
        # _enter_tree() (confirmed empirically while building this — same
        # way GDExtensionManager.load_extension() makes a freshly-unlocked
        # GDExtension usable in the current session without a restart, just
        # via Godot's own EditorPlugin activation path instead), so the
        # singleton should exist immediately here.
        if Engine.has_singleton(EXTENSION_RESOLVER_ID):
            return Engine.get_singleton(EXTENSION_RESOLVER_ID).resolve(host, addon_id, on_unlocked)

    _show_dialog(host, addon_id, func():
        ensure_unlocked(host, addon_id, on_unlocked)
    )
    return false

static func _is_installed_but_not_enabled() -> bool:
    return FileAccess.file_exists("res://addons/%s/plugin.cfg" % EXTENSION_RESOLVER_ID)

## ── Confirm-and-fetch dialog ─────────────────────────────────────────────

static func _show_dialog(host: Node, addon_id: String, retry: Callable) -> void:
    var dialog := AcceptDialog.new()
    dialog.title = "%s — Extension Resolver Required" % addon_id
    dialog.dialog_hide_on_ok = false

    var vbox := VBoxContainer.new()
    var label := Label.new()
    label.text = (
        "%s depends on Extension Resolver for Godot, which is not currently installed.\n\n" % addon_id
        + "Fetch it automatically from GitHub, or install it yourself "
        + "(copy the addon folder into res://addons/) and press Recheck."
    )
    label.autowrap_mode = TextServer.AUTOWRAP_WORD
    label.custom_minimum_size = Vector2(420, 0)
    vbox.add_child(label)

    var status_label := Label.new()
    vbox.add_child(status_label)
    dialog.add_child(vbox)

    var fetch_button := dialog.add_button("Fetch Automatically", true, "fetch")
    var recheck_button := dialog.add_button("Recheck", false, "recheck")

    dialog.custom_action.connect(func(action: StringName):
        if action == "fetch":
            fetch_button.disabled = true
            recheck_button.disabled = true
            var ok := await _fetch_and_enable(host, status_label)
            if not ok:
                status_label.text = "Failed to fetch Extension Resolver — install it manually."
                fetch_button.disabled = false
                recheck_button.disabled = false
                return
            dialog.hide()
            retry.call()
        elif action == "recheck":
            dialog.hide()
            retry.call()
    )

    if Engine.is_editor_hint():
        EditorInterface.get_base_control().add_child(dialog)
    else:
        host.add_child(dialog)
    dialog.popup_centered(Vector2i(480, 300))

static func _fetch_and_enable(host: Node, status_label: Label) -> bool:
    status_label.text = "Looking up latest release of Extension Resolver..."
    var release := await _get_json(host, GITHUB_API_ROOT + EXTENSION_RESOLVER_REPO + "/releases/latest")
    if release == null:
        return false

    var assets: Array = release.get("assets", [])
    var download_url := ""
    if assets.size() == 1 and typeof(assets[0]) == TYPE_DICTIONARY:
        download_url = assets[0].get("browser_download_url", "")
    else:
        for asset in assets:
            if typeof(asset) == TYPE_DICTIONARY and String(asset.get("name", "")).begins_with(EXTENSION_RESOLVER_ID):
                download_url = asset.get("browser_download_url", "")
                break
    if download_url.is_empty():
        return false

    status_label.text = "Downloading %s..." % download_url.get_file()
    var body := await _get_bytes(host, download_url)
    if body.is_empty():
        return false

    status_label.text = "Extracting into res://addons/%s..." % EXTENSION_RESOLVER_ID
    if not _extract_zip(body, EXTENSION_RESOLVER_ID):
        return false

    status_label.text = "Enabling Extension Resolver..."
    if Engine.is_editor_hint():
        EditorInterface.get_resource_filesystem().scan()
        EditorInterface.set_plugin_enabled(EXTENSION_RESOLVER_ID, true)

    return true

static func _get_json(host: Node, url: String) -> Variant:
    var body := await _get_bytes(host, url, ["Accept: application/vnd.github+json"])
    if body.is_empty():
        return null
    var parsed: Variant = JSON.parse_string(body.get_string_from_utf8())
    return parsed if typeof(parsed) == TYPE_DICTIONARY else null

static func _get_bytes(host: Node, url: String, headers: PackedStringArray = PackedStringArray()) -> PackedByteArray:
    var http := HTTPRequest.new()
    host.add_child(http)
    var err := http.request(url, headers)
    if err != OK:
        http.queue_free()
        return PackedByteArray()
    var result: Array = await http.request_completed
    http.queue_free()
    var response_code: int = result[1]
    var body: PackedByteArray = result[3]
    return body if response_code == 200 else PackedByteArray()

static func _extract_zip(zip_bytes: PackedByteArray, dependency_id: String) -> bool:
    var tmp_path := "user://%s_gate_download.zip" % dependency_id
    var tmp_file := FileAccess.open(tmp_path, FileAccess.WRITE)
    if tmp_file == null:
        return false
    tmp_file.store_buffer(zip_bytes)
    tmp_file.close()

    var reader := ZIPReader.new()
    if reader.open(tmp_path) != OK:
        DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_path))
        return false

    var dest_root := "res://addons/%s" % dependency_id
    DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(dest_root))

    # See ExtensionSourceGithubRelease._extract_zip()'s matching comment in
    # the main addon — locates the "<id>/" segment itself rather than
    # assuming a fixed nesting depth, since real release zips built by this
    # project's own CI are rooted two levels deep ("addons/<id>/..."), not
    # one, and this script has to duplicate that logic rather than call the
    # shared implementation (it must work before Extension Resolver itself
    # is known to be present).
    var marker := "%s/" % dependency_id
    for entry_path in reader.get_files():
        if entry_path.ends_with("/"):
            continue
        var marker_at := entry_path.find(marker)
        if marker_at == -1:
            continue
        var relative_path := entry_path.substr(marker_at + marker.length())
        if relative_path.is_empty():
            continue

        var dest_path := "%s/%s" % [dest_root, relative_path]
        var dest_dir := dest_path.get_base_dir()
        var globalized_dir := ProjectSettings.globalize_path(dest_dir)
        if not DirAccess.dir_exists_absolute(globalized_dir):
            DirAccess.make_dir_recursive_absolute(globalized_dir)

        var out_file := FileAccess.open(dest_path, FileAccess.WRITE)
        if out_file == null:
            reader.close()
            DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_path))
            return false
        out_file.store_buffer(reader.read_file(entry_path))
        out_file.close()

    reader.close()
    DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_path))
    if Engine.is_editor_hint():
        EditorInterface.get_resource_filesystem().scan()
    return true
