#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

using namespace godot;

namespace erynfall {

class TileMap : public Node3D {
    GDCLASS(TileMap, Node3D)

public:
    static void _bind_methods();

    void _ready() override;
    void _process(double delta) override;

    // Generate a flat tile grid mesh
    void generate(int width, int height, float tile_size);
    void clear();

    // Get/set tile type at position
    int get_tile(int x, int y) const;
    void set_tile(int x, int y, int type);

private:
    int width_ = 0;
    int height_ = 0;
    float tile_size_ = 1.0f;
    Array mesh_instances_;
    Ref<StandardMaterial3D> grass_material_;
    Ref<StandardMaterial3D> water_material_;
    Ref<StandardMaterial3D> dirt_material_;
    std::vector<int8_t> tiles_;

    void create_materials();
    MeshInstance3D* create_tile_quad(float x, float z, float size, Ref<Material> material);
};

} // namespace erynfall
