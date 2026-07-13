/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "editor/OghamManifestIO.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace
{
    void scan_dir(const String &path, PackedStringArray &out_files)
    {
        Ref<DirAccess> dir = DirAccess::open(path);
        if (dir.is_null())
            return;
        dir->list_dir_begin();
        String entry = dir->get_next();
        while (!entry.is_empty())
        {
            if (entry == "." || entry == "..")
            {
                entry = dir->get_next();
                continue;
            }
            const String full_path = path.path_join(entry);
            if (dir->current_is_dir())
            {
                if (entry != ".godot")
                    scan_dir(full_path, out_files);
            }
            else if (entry.ends_with(".ogham"))
            {
                out_files.push_back(full_path);
            }
            entry = dir->get_next();
        }
        dir->list_dir_end();
    }
}

Ref<OghamDocument> OghamManifestIO::load_manifest(const String &path)
{
    Ref<OghamDocument> doc;
    doc.instantiate();

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null())
        return doc;

    doc->load_from_json(file->get_as_text());
    return doc;
}

PackedStringArray OghamManifestIO::find_ogham_files(const String &root)
{
    PackedStringArray result;
    scan_dir(root, result);
    return result;
}

void OghamManifestIO::_bind_methods()
{
    ClassDB::bind_static_method("OghamManifestIO", D_METHOD("load_manifest", "path"), &OghamManifestIO::load_manifest);
    ClassDB::bind_static_method("OghamManifestIO", D_METHOD("find_ogham_files", "root"), &OghamManifestIO::find_ogham_files, DEFVAL(String("res://")));
}
