extends Node3D

## Main game scene — Phase 0 bootstrap.

func _ready():
	var grid_size = 30
	var half = grid_size / 2.0
	
	# --- Tile Grid ---
	var mesh_instance = MeshInstance3D.new()
	var array_mesh = ArrayMesh.new()
	var vertices = PackedVector3Array()
	var normals = PackedVector3Array()
	var uvs = PackedVector2Array()
	var indices = PackedInt32Array()
	
	for x in range(grid_size):
		for z in range(grid_size):
			var idx = x * grid_size + z
			var wx = x - half
			var wz = z - half
			vertices.append_array([
				Vector3(wx, 0, wz),
				Vector3(wx + 1, 0, wz),
				Vector3(wx + 1, 0, wz + 1),
				Vector3(wx, 0, wz + 1),
			])
			normals.append_array([Vector3.UP, Vector3.UP, Vector3.UP, Vector3.UP])
			uvs.append_array([Vector2(0,0), Vector2(1,0), Vector2(1,1), Vector2(0,1)])
			indices.append_array([idx*4, idx*4+2, idx*4+1, idx*4, idx*4+3, idx*4+2])
	
	var arrays = []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = vertices
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_INDEX] = indices
	array_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(0.3, 0.7, 0.3)
	mesh_instance.mesh = array_mesh
	mesh_instance.set_surface_override_material(0, mat)
	add_child(mesh_instance)
	
	# --- Player Cube (red) ---
	var player = MeshInstance3D.new()
	var box = BoxMesh.new()
	box.size = Vector3(0.8, 0.8, 0.8)
	var player_mat = StandardMaterial3D.new()
	player_mat.albedo_color = Color(0.8, 0.2, 0.2)
	player.mesh = box
	player.set_surface_override_material(0, player_mat)
	player.position = Vector3(0, 0.4, 0)
	add_child(player)
	
	# --- Orbital Camera ---
	var camera = Camera3D.new()
	camera.current = true
	camera.position = Vector3(0, 10, 12)
	add_child(camera)
	camera.look_at(Vector3(0, 0, 0))
	
	print("[Main] Phase 0 scene initialized - 30x30 grid, player cube, camera")
