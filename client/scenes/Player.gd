extends Node3D

## Player entity — position, movement, rendering.
## Moves 1 tile per 600ms tick with smooth animation.
## OSRS-accurate: player node position tracks tile for camera follow.

signal moved(from_pos: Vector2i, to_pos: Vector2i)
signal destination_changed(tile: Vector2i)

const TILE_SIZE := 1.0
const GRID_SIZE := 30
const WALK_DURATION := 0.6  # Seconds per tile (matches server tick)

## Tile position (integer grid coords)
var tile_pos := Vector2i(0, 0):
	set(value):
		var old = tile_pos
		tile_pos = value
		moved.emit(old, value)

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
var _walk_tween: Tween

func _ready() -> void:
	_create_player_mesh()
	tile_pos = Vector2i(15, 15)  # Start center of map
	target_tile = tile_pos
	# Set node position to match tile (so camera follows)
	_snap_to_tile(tile_pos)

func _create_player_mesh() -> void:
	mesh_node = MeshInstance3D.new()
	var box = BoxMesh.new()
	box.size = Vector3(0.8, 1.4, 0.8)
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(0.85, 0.18, 0.18)
	mesh_node.mesh = box
	mesh_node.set_surface_override_material(0, mat)
	add_child(mesh_node)

## Snap player node + mesh to a tile instantly (for login, respawn)
func _snap_to_tile(tile: Vector2i) -> void:
	var world_pos = tile_to_world(tile)
	position = Vector3(world_pos.x, 0, world_pos.y)
	if mesh_node:
		mesh_node.position = Vector3(0, 0.7, 0)  # Local: mesh is centered on node

## Called every tick. Move 1 step toward target.
func tick_move() -> void:
	if not is_walking:
		return

	var diff = target_tile - tile_pos

	# Snap if we're within 1 tile
	if abs(diff.x) <= 1 and abs(diff.y) <= 1:
		_animate_step(target_tile)
		return

	# Move 1 step — prioritize the axis with larger distance
	var step = Vector2i(0, 0)
	if abs(diff.x) >= abs(diff.y):
		step.x = sign(diff.x)
	else:
		step.y = sign(diff.y)

	var new_pos = tile_pos + step
	_animate_step(new_pos)

	# Check if we reached target
	if tile_pos == target_tile:
		is_walking = false

## Animate smooth movement from current visual position to a new tile
func _animate_step(new_tile: Vector2i) -> void:
	var world_pos = tile_to_world(new_tile)
	
	# Update logical tile position (emits signal)
	tile_pos = new_tile
	
	# Cancel any running walk tween
	if _walk_tween and _walk_tween.is_running():
		_walk_tween.kill()
	
	# Animate both the node position (for camera) and mesh (visual)
	var target_pos = Vector3(world_pos.x, 0, world_pos.y)
	_walk_tween = create_tween()
	_walk_tween.set_parallel(true)
	_walk_tween.tween_property(self, "position", target_pos, WALK_DURATION)
	# No need to tween mesh since it's local and node moves

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

## Attack animation — simple bob forward and back
func play_attack() -> void:
	if mesh_node:
		var tween = create_tween()
		var dir := Vector3.FORWARD.rotated(Vector3.UP, mesh_node.rotation.y)
		tween.tween_property(mesh_node, "position", mesh_node.position + dir * 0.3, 0.1)
		tween.tween_property(mesh_node, "position", mesh_node.position, 0.2)

## Snap to tile instantly (for teleport/respawn, no animation)
func snap_to_tile(tile: Vector2i) -> void:
	if _walk_tween and _walk_tween.is_running():
		_walk_tween.kill()
	is_walking = false
	tile_pos = tile
	target_tile = tile
	_snap_to_tile(tile)