extends Node3D

## OSRS-style fixed camera.
## Orthographic view from above, slight tilt for depth perception.
## Arrow keys or A/D to rotate camera (like OSRS compass).
## Scroll wheel to zoom.

@export var follow_target: Node3D
@export var zoom_level := 1.0       # Higher = more zoomed in
@export var min_zoom := 0.4
@export var max_zoom := 2.5
@export var base_ortho_size := 8.0  # Orthographic view size at zoom=1.0

var camera: Camera3D
var _rotation := 0.0     # Current camera rotation (radians around Y)
var _smoothed_pos := Vector3.ZERO
var _smooth_speed := 8.0  # Lerp factor for camera follow

func _ready() -> void:
	camera = Camera3D.new()
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.current = true
	add_child(camera)
	_update_camera()

func _process(delta: float) -> void:
	if follow_target:
		# Smooth follow — lerp toward target position
		# Read the target's actual world position (updated by Player.gd)
		var target_pos = follow_target.global_position
		_smoothed_pos = _smoothed_pos.lerp(target_pos, _smooth_speed * delta)
		global_position = _smoothed_pos
	_update_camera()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mb = event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_WHEEL_UP:
			zoom_level = minf(max_zoom, zoom_level + 0.1)
		elif mb.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			zoom_level = maxf(min_zoom, zoom_level - 0.1)
	
	# Arrow keys rotate camera (like OSRS)
	elif event is InputEventKey and event.pressed:
		if event.keycode == KEY_LEFT:
			_rotation += deg_to_rad(45.0)
		elif event.keycode == KEY_RIGHT:
			_rotation -= deg_to_rad(45.0)

func _update_camera() -> void:
	if not camera:
		return
	camera.size = base_ortho_size / zoom_level
	# Position camera above and tilted slightly (OSRS uses ~30° tilt)
	camera.position = Vector3(0, 10.0, 8.0)
	camera.rotation_degrees = Vector3(-51.0, 0, 0)
	# Apply Y rotation to the entire camera pivot
	global_rotation.y = _rotation