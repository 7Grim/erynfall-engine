extends Node3D

## Player entity — position, movement, rendering.
## Moves 1 tile per 600ms tick (OSRS-accurate).

signal moved(from_pos: Vector2i, to_pos: Vector2i)
signal destination_changed(tile: Vector2i)

const TILE_SIZE := 1.0
const GRID_SIZE := 30

## Tile position (integer grid coords)
var tile_pos := Vector2i(0, 0):
	set(value):
		var old = tile_pos
		tile_pos = value
		moved.emit(old, value)
		_update_visual_position()

## Target tile we're walking toward
var target_tile := Vector2i(0, 0):
	set(value):
		if value != target_tile:
			target_tile = value
			destination_changed.emit(value)

## Are we currently walking?
var is_walking := false

## Reference to visual mesh
var mesh_node: MeshInstance3D

func _ready() -> void:
	_create_player_mesh()
	tile_pos = Vector2i(15, 15)  # Start center of map
	target_tile = tile_pos

func _create_player_mesh() -> void:
	mesh_node = MeshInstance3D.new()
	var box = BoxMesh.new()
	box.size = Vector3(0.8, 1.4, 0.8)
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(0.85, 0.18, 0.18)
	mesh_node.mesh = box
	mesh_node.set_surface_override_material(0, mat)
	add_child(mesh_node)
	_update_visual_position()

func _update_visual_position() -> void:
	if mesh_node:
		var world_pos = tile_to_world(tile_pos)
		mesh_node.position = Vector3(world_pos.x, 0.7, world_pos.y)

## Called every tick. Move 1 step toward target.
func tick_move() -> void:
	if not is_walking:
		return

	var diff = target_tile - tile_pos

	# Snap if we're within 1 tile
	if abs(diff.x) <= 1 and abs(diff.y) <= 1:
		tile_pos = target_tile
		is_walking = false
		return

	# Move 1 step — prioritize the axis with larger distance
	var step = Vector2i(0, 0)
	if abs(diff.x) >= abs(diff.y):
		step.x = sign(diff.x)
	else:
		step.y = sign(diff.y)

	var new_pos = tile_pos + step
	tile_pos = new_pos

	# Check if we reached target
	if tile_pos == target_tile:
		is_walking = false

## Request to walk to a tile. Returns false if blocked.
func walk_to(tile: Vector2i) -> bool:
	target_tile = tile
	is_walking = true
	return true

## Convert tile grid coord → world position (center of tile)
func tile_to_world(tile: Vector2i) -> Vector2:
	return Vector2(
		float(tile.x) - float(GRID_SIZE) / 2.0 + TILE_SIZE * 0.5,
		float(tile.y) - float(GRID_SIZE) / 2.0 + TILE_SIZE * 0.5
	)

## Rotate player mesh to face a tile
func face_toward(tile: Vector2i) -> void:
	if mesh_node:
		var target_world = tile_to_world(tile)
		var my_world = tile_to_world(tile_pos)
		var dir = target_world - my_world
		if dir.length() > 0.01:
			mesh_node.rotation.y = atan2(-dir.x, -dir.y)

## Get current tile position (for distance checks)
func tile_position() -> Vector2i:
	return tile_pos

## Convert world position → tile coord
static func world_to_tile(world_pos: Vector3, grid_size: int, tile_size: float) -> Vector2i:
	var offset = float(grid_size) * tile_size / 2.0
	var tx = int(floor(world_pos.x + offset))
	var ty = int(floor(world_pos.z + offset))
	return Vector2i(tx, ty)
