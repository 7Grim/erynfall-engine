#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

using namespace godot;

namespace erynfall {

class PlayerController : public Node3D {
    GDCLASS(PlayerController, Node3D)

public:
    static void _bind_methods();

    void _ready() override;
    void _process(double delta) override;
    void _input(const Ref<InputEvent>& event) override;

    // Camera orbit
    void set_camera_distance(float dist);
    float get_camera_distance() const;

    void set_camera_angle(float angle);
    float get_camera_angle() const;

    void set_camera_pitch(float pitch);
    float get_camera_pitch() const;

private:
    Camera3D* camera_ = nullptr;
    Node3D* player_mesh_ = nullptr;

    // Camera orbit state
    float camera_distance_ = 10.0f;
    float camera_angle_ = 0.0f;
    float camera_pitch_ = 0.6f;
    bool dragging_camera_ = false;

    void update_camera_position();
    void handle_camera_input(const Ref<InputEvent>& event);
};

} // namespace erynfall
