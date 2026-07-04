extends Node3D

## Orbital camera that follows a target (player).
## Right-drag to orbit, scroll to zoom.

@export var follow_target: Node3D
@export var default_distance := 12.0
@export var default_pitch := 0.65
@export var min_distance := 3.0
@export var max_distance := 30.0
@export var min_pitch := 0.1
@export var max_pitch := 1.45

var camera: Camera3D
var angle := 0.0
var pitch := default_pitch
var distance := default_distance
var dragging := false

func _ready() -> void:
	camera = Camera3D.new()
	camera.current = true
	add_child(camera)
	_update_camera()

func _process(_delta: float) -> void:
	if follow_target:
		global_position = follow_target.global_position
		_update_camera()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mb = event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_RIGHT:
			dragging = mb.pressed
		elif mb.button_index == MOUSE_BUTTON_WHEEL_UP:
			distance = maxf(min_distance, distance - 1.0)
			_update_camera()
		elif mb.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			distance = minf(max_distance, distance + 1.0)
			_update_camera()
	
	elif event is InputEventMouseMotion and dragging:
		angle += event.relative.x * 0.01
		pitch = clampf(pitch - event.relative.y * 0.01, min_pitch, max_pitch)
		_update_camera()

func _update_camera() -> void:
	if not camera:
		return
	var x = distance * sin(angle) * cos(pitch)
	var y = distance * sin(pitch)
	var z = distance * cos(angle) * cos(pitch)
	camera.position = Vector3(x, y, z)
	camera.look_at(Vector3(0, 0.8, 0))
