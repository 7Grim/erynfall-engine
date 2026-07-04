extends MeshInstance3D

## Tile hover highlight — white translucent overlay on hovered tile.

const GRID_SIZE := 30
const TILE_SIZE := 1.0

func _ready() -> void:
	_create_highlight_mesh()
	visible = false

func _create_highlight_mesh() -> void:
	var mesh = ArrayMesh.new()
	var half = TILE_SIZE * 0.5
	var y = 0.02  # Slightly above ground
	
	var verts = PackedVector3Array([
		Vector3(-half, y, -half),
		Vector3( half, y, -half),
		Vector3( half, y,  half),
		Vector3(-half, y,  half),
	])
	
	var indices = PackedInt32Array([0, 1, 2, 0, 2, 3])
	var arrays = []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_INDEX] = indices
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(1, 1, 1, 0.3)
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	set_mesh(mesh)
	set_surface_override_material(0, mat)

## Change highlight color (e.g. orange for trees, white for walkable)
func set_highlight_color(color: Color) -> void:
	var mat = get_surface_override_material(0) as StandardMaterial3D
	if mat:
		mat.albedo_color = color

func update_position(world_pos: Vector3, grid_size: int, tile_size: float) -> void:
	var offset = float(grid_size) * tile_size / 2.0
	var tx = clampi(int(floor(world_pos.x + offset)), 0, grid_size - 1)
	var ty = clampi(int(floor(world_pos.z + offset)), 0, grid_size - 1)
	
	var cx = float(tx) - offset + tile_size * 0.5
	var cz = float(ty) - offset + tile_size * 0.5
	position = Vector3(cx, 0.02, cz)
	visible = true
