#include "input/player_controller.h"

namespace erynfall {

void PlayerController::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_camera_distance", "dist"), &PlayerController::set_camera_distance);
    ClassDB::bind_method(D_METHOD("get_camera_distance"), &PlayerController::get_camera_distance);
    ClassDB::bind_method(D_METHOD("set_camera_angle", "angle"), &PlayerController::set_camera_angle);
    ClassDB::bind_method(D_METHOD("get_camera_angle"), &PlayerController::get_camera_angle);
    ClassDB::bind_method(D_METHOD("set_camera_pitch", "pitch"), &PlayerController::set_camera_pitch);
    ClassDB::bind_method(D_METHOD("get_camera_pitch"), &PlayerController::get_camera_pitch);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "camera_distance"), "set_camera_distance", "get_camera_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "camera_angle"), "set_camera_angle", "get_camera_angle");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "camera_pitch"), "set_camera_pitch", "get_camera_pitch");
}

void PlayerController::_ready() {
    // Create a placeholder player cube (red box)
    auto* box = memnew(MeshInstance3D);
    Ref<BoxMesh> mesh;
    mesh.instantiate();
    mesh->set_size(Vector3(0.8f, 0.8f, 0.8f));

    Ref<StandardMaterial3D> mat;
    mat.instantiate();
    mat->set_albedo(Color(0.8f, 0.2f, 0.2f));
    box->set_surface_override_material(0, mat);
    box->set_mesh(mesh);
    box->set_position(Vector3(0, 0.4f, 0));
    add_child(box);
    player_mesh_ = box;

    // Create camera
    camera_ = memnew(Camera3D);
    camera_->set_current(true);
    add_child(camera_);

    update_camera_position();

    UtilityFunctions::print("[Player] Controller ready at origin");
}

void PlayerController::_process(double /*delta*/) {
    // Phase 1: movement interpolation
}

void PlayerController::_input(const Ref<InputEvent>& event) {
    handle_camera_input(event);
}

void PlayerController::handle_camera_input(const Ref<InputEvent>& event) {
    auto* mouse_motion = Object::cast_to<InputEventMouseMotion>(event.ptr());
    auto* mouse_button = Object::cast_to<InputEventMouseButton>(event.ptr());

    // Right-click drag to orbit camera
    if (mouse_button) {
        if (mouse_button->get_button_index() == MouseButton::MOUSE_BUTTON_RIGHT) {
            dragging_camera_ = mouse_button->is_pressed();
        }
        // Scroll to zoom
        if (mouse_button->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_UP) {
            camera_distance_ = Math::max(2.0f, camera_distance_ - 1.0f);
            update_camera_position();
        }
        if (mouse_button->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_DOWN) {
            camera_distance_ = Math::min(30.0f, camera_distance_ + 1.0f);
            update_camera_position();
        }
    }

    if (mouse_motion && dragging_camera_) {
        camera_angle_ += mouse_motion->get_relative().x * 0.01f;
        camera_pitch_ = Math::clamp(
            camera_pitch_ - mouse_motion->get_relative().y * 0.01f,
            0.1f, 1.5707f - 0.1f
        );
        update_camera_position();
    }
}

void PlayerController::update_camera_position() {
    if (!camera_) return;

    float x = camera_distance_ * std::sin(camera_angle_) * std::cos(camera_pitch_);
    float y = camera_distance_ * std::sin(camera_pitch_);
    float z = camera_distance_ * std::cos(camera_angle_) * std::cos(camera_pitch_);

    camera_->set_position(Vector3(x, y + 1.0f, z));
    camera_->look_at(Vector3(0, 0.5f, 0));
}

void PlayerController::set_camera_distance(float dist) { camera_distance_ = dist; update_camera_position(); }
float PlayerController::get_camera_distance() const { return camera_distance_; }
void PlayerController::set_camera_angle(float angle) { camera_angle_ = angle; update_camera_position(); }
float PlayerController::get_camera_angle() const { return camera_angle_; }
void PlayerController::set_camera_pitch(float pitch) { camera_pitch_ = pitch; update_camera_position(); }
float PlayerController::get_camera_pitch() const { return camera_pitch_; }

} // namespace erynfall
