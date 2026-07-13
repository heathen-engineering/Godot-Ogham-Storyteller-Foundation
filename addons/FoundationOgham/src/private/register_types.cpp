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

#include "register_types.h"
#include "OghamContentKey.h"
#include "OghamEntry.h"
#include "OghamInlineLinkParser.h"
#include "OghamOption.h"
#include "OghamSession.h"
#include "OghamStory.h"
#include "OghamSubsystem.h"
#include "OghamVariables.h"
#include "Storyteller.h"
#include "editor/OghamContentKeyCompactEditor.h"
#include "editor/OghamContentKeyPopup.h"
#include "editor/OghamDocument.h"
#include "editor/OghamGraphNode.h"
#include "editor/OghamGraphView.h"
#include "editor/OghamKeyLabelsPopup.h"
#include "editor/OghamLabelPickerPopup.h"
#include "editor/OghamManifestIO.h"
#include "editor/OghamOperationPopup.h"
#include "editor/OghamOptionPopup.h"
#include "editor/OghamPopupBase.h"
#include "editor/OghamTargetPickerPopup.h"

#include <gameframework/SubsystemManager.h>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

static OghamVariables *ogham_variables_singleton = nullptr;
static Storyteller *storyteller_singleton = nullptr;

void initialize_foundation_ogham_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        ClassDB::register_class<OghamContentKey>();
        ClassDB::register_class<OghamOption>();
        ClassDB::register_class<OghamEntry>();
        ClassDB::register_class<OghamStory>();
        ClassDB::register_class<OghamSession>();
        ClassDB::register_class<OghamInlineLinkParser>();
        ClassDB::register_class<OghamVariables>();
        ClassDB::register_class<Storyteller>();

        ogham_variables_singleton = memnew(OghamVariables);
        Engine::get_singleton()->register_singleton("OghamVariables", OghamVariables::get_singleton());

        storyteller_singleton = memnew(Storyteller);
        Engine::get_singleton()->register_singleton("Storyteller", Storyteller::get_singleton());

        // Real gameframework::Subsystem registration — see Godot-Game-Framework's
        // README, "The linking model", and FoundationGameplayTags' register_types.cpp
        // for the reference implementation this mirrors.
        gameframework::SubsystemManager::instance().register_subsystem<OghamSubsystem>();
    }
    else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // Editor-tools-only classes — the Ogham Storyteller document model and
        // node-graph editor. Godot never initializes
        // MODULE_INITIALIZATION_LEVEL_EDITOR in exported game builds, so this
        // stays entirely out of shipped games automatically. Deliberately
        // excludes OghamInspectorPlugin — the GDScript
        // editor/OghamInspectorPlugin.gd is what's actually wired up; the C++
        // port here was never instantiated by anything, so it isn't promoted.
        ClassDB::register_class<OghamDocument>();
        ClassDB::register_class<OghamManifestIO>();
        ClassDB::register_class<OghamGraphNode>();
        ClassDB::register_class<OghamPopupBase>();
        ClassDB::register_class<OghamOperationPopup>();
        ClassDB::register_class<OghamContentKeyPopup>();
        ClassDB::register_class<OghamOptionPopup>();
        ClassDB::register_class<OghamTargetPickerPopup>();
        ClassDB::register_class<OghamKeyLabelsPopup>();
        ClassDB::register_class<OghamLabelPickerPopup>();
        ClassDB::register_class<OghamGraphView>();
        ClassDB::register_class<OghamContentKeyCompactEditor>();
    }
}

void uninitialize_foundation_ogham_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
    {
        // OghamGraphView::_swatch_icon_cache holds static-duration
        // Ref<ImageTexture> — must be cleared before this extension unloads
        // and RenderingServer tears down, or the cached Refs' destructors
        // run against an already-gone RenderingServer ("Parameter
        // RenderingServer::get_singleton() is null", confirmed via a
        // headless --editor --quit run).
        OghamGraphView::clear_swatch_icon_cache();
        return;
    }

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;

    if (storyteller_singleton != nullptr)
    {
        Engine::get_singleton()->unregister_singleton("Storyteller");
        memdelete(storyteller_singleton);
        storyteller_singleton = nullptr;
    }

    if (ogham_variables_singleton != nullptr)
    {
        Engine::get_singleton()->unregister_singleton("OghamVariables");
        memdelete(ogham_variables_singleton);
        ogham_variables_singleton = nullptr;
    }
}

extern "C"
{
    GDE_EXPORT GDExtensionBool foundation_ogham_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization)
    {
        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_foundation_ogham_module);
        init_obj.register_terminator(uninitialize_foundation_ogham_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
