extends Node3D

## Manages 3D rock objects on the map.
## Each rock is a box/irregular shape with color by type.

const ROCK_COLORS := {
	0: Color(0.55, 0.35, 0.25),  # Copper — brownish
	1: Color(0.45, 0.45, 0.45),  # Iron — gray
	2: Color(0.80, 0.70, 0.20),  # Gold — yellow
	3: Color(0.35, 0.40, 0.55),  # Mithril — blue-gray
}

const ROCK_VEIN_COLORS := {
	0: Color(0.85, 0.55, 0.25),  # Copper vein — orange-brown
	1: Color(0.65, 0.60, 0.55),  # Iron vein — light gray
	2: Color(0.95, 0.85, 0.30),  # Gold vein — bright yellow
	3: Color(0.55, 0.60, 0.80),  # Mithril vein — blue
}

const ROCK_HEIGHT := 0.8
const ROCK_SIZE := 0.7

var rock_nodes: Dictionary = {}

func add_rock(obj_id: int, rock_type: int, tile_x: int, tile_y: int, grid_size: int, tile_size: float) -> void:
	if rock_nodes.has(obj_id):
		return
	
	var half = float(grid_size) * tile_size / 2.0
	var wx = float(tile_x) - half + tile_size * 0.5
	var wz = float(tile_y) - half + tile_size * 0.5
	
	var base_color = ROCK_COLORS.get(rock_type, Color(0.45, 0.45, 0.45))
	var vein_color = ROCK_VEIN_COLORS.get(rock_type, Color(0.60, 0.60, 0.60))
	
	var rock_root = Node3D.new()
	rock_root.name = "Rock_%d" % obj_id
	rock_root.position = Vector3(wx, 0, wz)
	
	# Main rock body (box)
	var body = MeshInstance3D.new()
	var body_mesh = BoxMesh.new()
	body_mesh.size = Vector3(ROCK_SIZE, ROCK_HEIGHT, ROCK_SIZE * 0.8)
	var body_mat = StandardMaterial3D.new()
	body_mat.albedo_color = base_color
	body.mesh = body_mesh
	body.set_surface_override_material(0, body_mat)
	body.position.y = ROCK_HEIGHT * 0.4
	rock_root.add_child(body)
	
	# Ore vein highlight (smaller box on top)
	var vein = MeshInstance3D.new()
	var vein_mesh = BoxMesh.new()
	vein_mesh.size = Vector3(ROCK_SIZE * 0.5, 0.15, ROCK_SIZE * 0.4)
	var vein_mat = StandardMaterial3D.new()
	vein_mat.albedo_color = vein_color
	vein.mesh = vein_mesh
	vein.set_surface_override_material(0, vein_mat)
	vein.position = Vector3(0, ROCK_HEIGHT * 0.7, 0.05)
	rock_root.add_child(vein)
	
	add_child(rock_root)
	rock_nodes[obj_id] = rock_root

func remove_rock(obj_id: int) -> void:
	if rock_nodes.has(obj_id):
		rock_nodes[obj_id].queue_free()
		rock_nodes.erase(obj_id)

func clear_all() -> void:
	for node in rock_nodes.values():
		node.queue_free()
	rock_nodes.clear()
