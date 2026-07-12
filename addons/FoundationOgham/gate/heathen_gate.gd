@tool
extends RefCounted

## Heathen Dependency Gate — COPY THIS FILE VERBATIM into every Heathen asset's
## own addons/<Name>/gate/heathen_gate.gd. Deliberately self-contained: no
## class_name (avoids a "class already in use" collision once multiple copies
## of this same file exist in one project), and no reference whatsoever to any
## other Heathen addon's classes — its entire job is to work correctly BEFORE
## any dependency can be assumed present, so it can't lean on one itself.
##
## What this solves: a Heathen asset with a real GDExtension dependency (e.g.
## FoundationGameplayTags depending on Godot-Game-Framework's shared runtime)
## fails to load its native library AT ALL if that dependency is missing —
## not a soft/null-checkable failure, a hard OS-level dynamic-link failure,
## surfaced as a cryptic "Can't open shared object file" error pointing at a
## file the user has never heard of. This gate prevents Godot from ever
## attempting that load until the dependency is confirmed present.
##
## How: the asset's real, fully-dependent extension ships inert — named
## "<addon_id>.gdextension.available" instead of "<addon_id>.gdextension", so
## Godot's own boot-time project scan (which auto-loads every *.gdextension
## file it finds, well before any plugin script runs) never touches it. This
## gate checks the asset's own heathen_manifest.json; once every declared
## dependency is confirmed installed (each identified by the same convention:
## res://addons/<id>/<id>.gdextension existing), it renames the inert file to
## its real name (permanent — this only happens once, ever, per project),
## triggers a filesystem rescan, and calls GDExtensionManager.load_extension()
## so the tool is live in this same editor session, not just after a restart.
## Verified empirically: GDExtensionManager.load_extension() on a freshly-
## unlocked res://addons/X/X.gdextension returns LOAD_STATUS_OK (not
## LOAD_STATUS_NEEDS_RESTART) and the resulting Engine singleton is
## immediately usable, no restart required.
##
## Exported/shipped games never run this at all — plugin.cfg-driven
## EditorPlugin scripts are an editor-only construct, never included in an
## export. By export time the developer's own project has already been
## through a successful gate pass during development, so the real
## .gdextension is already in place and loads completely normally, same as
## any other addon — zero runtime gating cost in a shipped build.
##
## If a user manually deletes an already-unlocked dependency afterward, the
## next load hard-fails again (the .gdextension is already real, not inert,
## so this gate never re-runs to catch it). Accepted, deliberate: that's the
## user manually breaking an already-working install, not a gap this gate is
## meant to cover.

const MANIFEST_FILENAME := "heathen_manifest.json"
const GITHUB_API_ROOT := "https://api.github.com/repos/"

## Call from the asset's own EditorPlugin._enter_tree(). Returns true if the
## gate is already open (the real extension is already in place from a prior
## session) — the caller can proceed immediately, synchronously. Returns
## false if the gate isn't open yet; on_unlocked (a zero-argument Callable)
## is invoked later, possibly asynchronously (after a fetch completes, or
## after the user retries), once it does open. host is any Node currently in
## the tree — used only as a parent for the confirmation dialog.
static func ensure_unlocked(host: Node, addon_id: String, on_unlocked: Callable) -> bool:
    var addon_dir := "res://addons/%s" % addon_id
    var real_path := "%s/%s.gdextension" % [addon_dir, addon_id]

    if FileAccess.file_exists(real_path):
        return true

    _run_check(host, addon_dir, addon_id, real_path, on_unlocked)
    return false

static func _run_check(host: Node, addon_dir: String, addon_id: String, real_path: String, on_unlocked: Callable) -> void:
    var manifest := _read_manifest("%s/%s" % [addon_dir, MANIFEST_FILENAME])
    if manifest == null:
        push_warning("heathen_gate: %s has no heathen_manifest.json — nothing to check, unlocking directly." % addon_id)
        _unlock(addon_dir, real_path, on_unlocked)
        return

    var missing := _find_missing(manifest.get("dependencies", []))
    if missing.is_empty():
        _unlock(addon_dir, real_path, on_unlocked)
        return

    _show_dialog(host, addon_id, missing, func():
        _run_check(host, addon_dir, addon_id, real_path, on_unlocked)
    )

static func _read_manifest(path: String) -> Variant:
    if not FileAccess.file_exists(path):
        return null
    var file := FileAccess.open(path, FileAccess.READ)
    if file == null:
        return null
    var parsed: Variant = JSON.parse_string(file.get_as_text())
    return parsed if typeof(parsed) == TYPE_DICTIONARY else null

static func _find_missing(dependencies: Array) -> Array:
    var missing: Array = []
    for dep in dependencies:
        if typeof(dep) != TYPE_DICTIONARY or not dep.has("id"):
            continue
        var dep_id: String = dep["id"]
        var dep_real_path := "res://addons/%s/%s.gdextension" % [dep_id, dep_id]
        if not FileAccess.file_exists(dep_real_path):
            missing.append(dep)
    return missing

static func _unlock(addon_dir: String, real_path: String, on_unlocked: Callable) -> void:
    var inert_path := "%s.available" % real_path
    if FileAccess.file_exists(inert_path):
        var ok := DirAccess.rename_absolute(
            ProjectSettings.globalize_path(inert_path),
            ProjectSettings.globalize_path(real_path)
        )
        if ok != OK:
            push_error("heathen_gate: failed to rename %s -> %s (error %d)." % [inert_path, real_path, ok])
            return

    var mgr = Engine.get_singleton("GDExtensionManager") if Engine.has_singleton("GDExtensionManager") else null
    if mgr != null:
        mgr.load_extension(real_path)

    on_unlocked.call()

    # Deferred, not called synchronously above: triggering a full filesystem
    # rescan from inside another plugin's still-running _enter_tree() made
    # Godot re-enter its own plugin-activation bookkeeping for THIS plugin
    # while it was still mid-activation — "Condition 'p_enabled &&
    # addon_name_to_plugin.has(addon_path)' is true", repeatedly. Deferring
    # until the current call stack (this plugin's own activation) has fully
    # unwound avoids the re-entrancy. Only needed so the renamed file/updated
    # class list are reflected in Godot's own project bookkeeping for next
    # launch; GDExtensionManager.load_extension() above already makes
    # everything usable in THIS session regardless of whether this runs.
    if Engine.is_editor_hint():
        EditorInterface.get_resource_filesystem().call_deferred("scan")

## ── Confirm-and-fetch dialog ─────────────────────────────────────────────

static func _show_dialog(host: Node, addon_id: String, missing: Array, retry: Callable) -> void:
    var dialog := AcceptDialog.new()
    dialog.title = "%s — Missing Dependencies" % addon_id
    dialog.dialog_hide_on_ok = false

    var vbox := VBoxContainer.new()
    var label := Label.new()
    var lines: PackedStringArray = []
    for dep in missing:
        lines.append("- %s" % dep.get("id", "?"))
    label.text = (
        "%s needs the following, not currently installed:\n\n%s\n\n" % [addon_id, "\n".join(lines)]
        + "Fetch them automatically from GitHub, or install them yourself "
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
            await _fetch_all(dialog, missing, status_label)
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
    dialog.popup_centered(Vector2i(480, 340))

static func _fetch_all(host: Node, missing: Array, status_label: Label) -> void:
    for dep in missing:
        status_label.text = "Fetching %s..." % dep.get("id", "?")
        var ok := await _fetch_one(host, dep, status_label)
        if not ok:
            status_label.text = "Failed to fetch %s — install it manually." % dep.get("id", "?")

static func _fetch_one(host: Node, dep: Dictionary, status_label: Label) -> bool:
    var repo: String = dep.get("repo", "")
    var id: String = dep.get("id", "")
    var asset_pattern: String = dep.get("release_asset_pattern", "")
    if repo.is_empty() or id.is_empty():
        return false

    var release := await _get_json(host, GITHUB_API_ROOT + repo + "/releases/latest")
    if release == null:
        return false

    var version: String = String(release.get("tag_name", "")).trim_prefix("v")
    var wanted_name := asset_pattern.replace("{version}", version) if not asset_pattern.is_empty() else ""
    var download_url := ""
    for asset in release.get("assets", []):
        if typeof(asset) != TYPE_DICTIONARY:
            continue
        var name: String = asset.get("name", "")
        if (not wanted_name.is_empty() and name == wanted_name) or (wanted_name.is_empty() and name.begins_with(id)):
            download_url = asset.get("browser_download_url", "")
            break
    if download_url.is_empty():
        return false

    status_label.text = "Downloading %s..." % download_url.get_file()
    var body := await _get_bytes(host, download_url)
    if body.is_empty():
        return false

    return _extract_zip(body, id)

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

    for entry_path in reader.get_files():
        if entry_path.ends_with("/"):
            continue
        var relative_path := entry_path
        var first_slash := relative_path.find("/")
        if first_slash != -1:
            relative_path = relative_path.substr(first_slash + 1)
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
