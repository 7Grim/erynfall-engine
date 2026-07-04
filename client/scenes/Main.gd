extends Node3D

## Main game scene — Phase 1: The Grid.
## Tile map, player movement, orbital camera, tile highlight.

const GRID_SIZE := 30
const TILE_SIZE := 1.0

@onready var game_tick = $GameTick
@onready var player: Node = $Player
@onready var camera: Node = $OrbitalCamera
@onready var highlight: MeshInstance3D = $TileHighlight
@onready var ground: MeshInstance3D = $Ground

## Tile data: 0 = grass (walkable), 1 = water (blocked), 2 = dirt (walkable)
var tiles: Dictionary = {}  # Vector2i -> int

func _ready() -> void:
	_generate_map()
	_connect_signals()
	print("[Main] Phase 1 initialized - grid, click-to-move, camera, highlight")

func _generate_map() -> void:
	# Create tile data
	for x in range(GRID_SIZE):
		for z in range(GRID_SIZE):
			tiles[Vector2i(x, z)] = 0  # grass
	
	# Water pond
	for x in range(10, 14):
		for z in range(20, 25):
			tiles[Vector2i(x, z)] = 1
	
	# Dirt path horizontal
	for x in range(0, GRID_SIZE):
		tiles[Vector2i(x, 14)] = 2
		tiles[Vector2i(x, 15)] = 2
	
	_build_ground_mesh()

func _build_ground_mesh() -> void:
	var half = float(GRID_SIZE) * TILE_SIZE / 2.0
	
	# Collect vertices per tile type
	var grass_quads = []
	var water_quads = []
	var dirt_quads = []
	
	for x in range(GRID_SIZE):
		for z in range(GRID_SIZE):
			var wx = float(x) - half + TILE_SIZE * 0.5
			var wz = float(z) - half + TILE_SIZE * 0.5
			var h = TILE_SIZE * 0.5
			
			var quad = [
				Vector3(wx - h, 0, wz - h),
				Vector3(wx + h, 0, wz - h),
				Vector3(wx + h, 0, wz + h),
				Vector3(wx - h, 0, wz + h),
			]
			
			var type = tiles.get(Vector2i(x, z), 0)
			match type:
				0: grass_quads.append(quad)
				1: water_quads.append(quad)
				2: dirt_quads.append(quad)
	
	# Build mesh with multiple surfaces (one per type)
	var array_mesh = ArrayMesh.new()
	var surface_idx = 0
	
	var type_colors = [
		Color(0.32, 0.68, 0.32),  # grass
		Color(0.2, 0.4, 0.8),     # water
		Color(0.5, 0.38, 0.22),   # dirt
	]
	
	for type_quads in [grass_quads, water_quads, dirt_quads]:
		if type_quads.is_empty():
			continue
		
		var verts = PackedVector3Array()
		var normals = PackedVector3Array()
		var indices = PackedInt32Array()
		
		for quad in type_quads:
			var base = verts.size()
			verts.append(quad[0])
			verts.append(quad[1])
			verts.append(quad[2])
			verts.append(quad[3])
			normals.append(Vector3.UP)
			normals.append(Vector3.UP)
			normals.append(Vector3.UP)
			normals.append(Vector3.UP)
			indices.append(base)
			indices.append(base + 2)
			indices.append(base + 1)
			indices.append(base)
			indices.append(base + 3)
			indices.append(base + 2)
		
		var arrays = []
		arrays.resize(Mesh.ARRAY_MAX)
		arrays[Mesh.ARRAY_VERTEX] = verts
		arrays[Mesh.ARRAY_NORMAL] = normals
		arrays[Mesh.ARRAY_INDEX] = indices
		array_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
		surface_idx += 1
	
	# Set mesh first, then assign materials per surface
	ground.mesh = array_mesh
	for i in range(array_mesh.get_surface_count()):
		var mat = StandardMaterial3D.new()
		mat.albedo_color = type_colors[i]
		ground.set_surface_override_material(i, mat)

func _connect_signals() -> void:
	game_tick.tick.connect(_on_tick)

func _on_tick() -> void:
	player.tick_move()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mb = event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_LEFT and mb.pressed:
			_handle_click()
	
	if event is InputEventMouseMotion:
		_handle_hover()

func _world_to_tile(world_pos: Vector3) -> Vector2i:
	var offset = float(GRID_SIZE) * TILE_SIZE / 2.0
	var tx = int(floor(world_pos.x + offset))
	var ty = int(floor(world_pos.z + offset))
	return Vector2i(tx, ty)

func _handle_click() -> void:
	var camera_3d = get_viewport().get_camera_3d()
	if not camera_3d:
		return
	
	var mouse_pos = get_viewport().get_mouse_position()
	var from = camera_3d.project_ray_origin(mouse_pos)
	var dir = camera_3d.project_ray_normal(mouse_pos)
	
	var plane = Plane(Vector3.UP, 0)
	var hit = plane.intersects_ray(from, dir)
	if not hit:
		return
	
	var tile = _world_to_tile(hit)
	
	if tile.x < 0 or tile.x >= GRID_SIZE or tile.y < 0 or tile.y >= GRID_SIZE:
		return
	
	var type = tiles.get(tile, 0)
	if type == 1:
		return  # water — blocked
	
	player.walk_to(tile)

func _handle_hover() -> void:
	var camera_3d = get_viewport().get_camera_3d()
	if not camera_3d:
		highlight.visible = false
		return
	
	var mouse_pos = get_viewport().get_mouse_position()
	var from = camera_3d.project_ray_origin(mouse_pos)
	var dir = camera_3d.project_ray_normal(mouse_pos)
	
	var plane = Plane(Vector3.UP, 0)
	var hit = plane.intersects_ray(from, dir)
	if not hit:
		highlight.visible = false
		return
	
	highlight.update_position(hit, GRID_SIZE, TILE_SIZE)
