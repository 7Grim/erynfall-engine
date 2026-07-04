#include "world/tile_map.h"

namespace erynfall {

void TileMap::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "width", "height", "tile_size"), &TileMap::generate);
    ClassDB::bind_method(D_METHOD("clear"), &TileMap::clear);
    ClassDB::bind_method(D_METHOD("get_tile", "x", "y"), &TileMap::get_tile);
    ClassDB::bind_method(D_METHOD("set_tile", "x", "y", "type"), &TileMap::set_tile);
}

void TileMap::_ready() {
    create_materials();
}

void TileMap::_process(double /*delta*/) {
    // Phase 1: add camera follow, entity rendering, etc.
}

void TileMap::create_materials() {
    grass_material_.instantiate();
    grass_material_->set_albedo(Color(0.3f, 0.7f, 0.3f));

    water_material_.instantiate();
    water_material_->set_albedo(Color(0.2f, 0.4f, 0.8f));

    dirt_material_.instantiate();
    dirt_material_->set_albedo(Color(0.5f, 0.35f, 0.2f));
}

MeshInstance3D* TileMap::create_tile_quad(float x, float z, float size, Ref<Material> material) {
    Ref<ArrayMesh> mesh;
    mesh.instantiate();

    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedInt32Array indices;

    float half = size * 0.5f;

    vertices.push_back(Vector3(x - half, 0, z - half));
    vertices.push_back(Vector3(x + half, 0, z - half));
    vertices.push_back(Vector3(x + half, 0, z + half));
    vertices.push_back(Vector3(x - half, 0, z + half));

    for (int i = 0; i < 4; ++i) {
        normals.push_back(Vector3(0, 1, 0));
    }

    uvs.push_back(Vector2(0, 0));
    uvs.push_back(Vector2(1, 0));
    uvs.push_back(Vector2(1, 1));
    uvs.push_back(Vector2(0, 1));

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(1);
    indices.push_back(0);
    indices.push_back(3);
    indices.push_back(2);

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_INDEX] = indices;

    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    auto* instance = memnew(MeshInstance3D);
    instance->set_mesh(mesh);
    instance->set_surface_override_material(0, material);
    add_child(instance);

    return instance;
}

void TileMap::generate(int width, int height, float tile_size) {
    clear();

    width_ = width;
    height_ = height;
    tile_size_ = tile_size;
    tiles_.resize(width * height, 0);  // All grass (type 0)

    float offset_x = -(width * tile_size) * 0.5f + tile_size * 0.5f;
    float offset_z = -(height * tile_size) * 0.5f + tile_size * 0.5f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float wx = offset_x + x * tile_size;
            float wz = offset_z + y * tile_size;

            int type = tiles_[y * width + x];
            Ref<Material> mat = grass_material_;

            if (type == 1) mat = water_material_;
            else if (type == 2) mat = dirt_material_;

            create_tile_quad(wx, wz, tile_size, mat);
        }
    }

    UtilityFunctions::print("[TileMap] Generated " + String::num_int64(width) + "x"
        + String::num_int64(height) + " map (" + String::num_int64(width * height) + " tiles)");
}

void TileMap::clear() {
    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node* child = get_child(i);
        remove_child(child);
        if (child) {
            child->queue_free();
        }
    }
    mesh_instances_.clear();
    tiles_.clear();
}

int TileMap::get_tile(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return -1;
    return tiles_[y * width_ + x];
}

void TileMap::set_tile(int x, int y, int type) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    tiles_[y * width_ + x] = static_cast<int8_t>(type);
}

} // namespace erynfall
