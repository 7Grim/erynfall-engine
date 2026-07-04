#include "world/tile_map.h"
#include "input/player_controller.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;
using namespace erynfall;

void erynfall_client_init(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<TileMap>();
    ClassDB::register_class<PlayerController>();
}

void erynfall_client_uninit(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
// Initialization entry point
GDExtensionBool GDExtensionInitializer(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* p_initialization) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_initialization);

    init_obj.register_initializer(erynfall_client_init);
    init_obj.register_terminator(erynfall_client_uninit);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
