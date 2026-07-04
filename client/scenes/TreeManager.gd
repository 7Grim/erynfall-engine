extends Node3D

## Manages 3D tree objects on the map.
## Each tree is a cylinder (trunk) + cone (canopy) with color by type.

const TREE_COLORS := {
	0: Color(0.35, 0.22, 0.08),  # Normal — brown trunk, green canopy
	1: Color(0.28, 0.18, 0.06),  # Oak — darker
	2: Color(0.30, 0.20, 0.07),  # Willow — medium
	3: Color(0.45, 0.25, 0.10),  # Maple — reddish
	4: Color(0.22, 0.14, 0.05),  # Yew — dark
	5: Color(0.50, 0.30, 0.12),  # Magic — purple-ish
}

const CANOPY_COLORS := {
	0: Color(0.15, 0.50, 0.15),  # Normal — green
	1: Color(0.10, 0.40, 0.10),  # Oak — dark green
	2: Color(0.20, 0.55, 0.20),  # Willow — light green
	3: Color(0.45, 0.30, 0.15),  # Maple — orange-ish
	4: Color(0.08, 0.35, 0.08),  # Yew — very dark green
	5: Color(0.30, 0.15, 0.50),  # Magic — purple
}

const TREE_HEIGHT := 1.8
const CANOPY_RADIUS := 0.6
const TRUNK_RADIUS := 0.12

# Map: objectId -> Node3D
var tree_nodes: Dictionary = {}

func add_tree(obj_id: int, tree_type: int, tile_x: int, tile_y: int, grid_size: int, tile_size: float) -> void:
	if tree_nodes.has(obj_id):
		return
	
	var half = float(grid_size) * tile_size / 2.0
	var wx = float(tile_x) - half + tile_size * 0.5
	var wz = float(tile_y) - half + tile_size * 0.5
	
	var trunk_color = TREE_COLORS.get(tree_type, Color(0.35, 0.22, 0.08))
	var canopy_color = CANOPY_COLORS.get(tree_type, Color(0.15, 0.50, 0.15))
	
	var tree_root = Node3D.new()
	tree_root.name = "Tree_%d" % obj_id
	tree_root.position = Vector3(wx, 0, wz)
	
	# Trunk (cylinder)
	var trunk = MeshInstance3D.new()
	var trunk_mesh = CylinderMesh.new()
	trunk_mesh.top_radius = TRUNK_RADIUS * 0.7
	trunk_mesh.bottom_radius = TRUNK_RADIUS
	trunk_mesh.height = TREE_HEIGHT
	var trunk_mat = StandardMaterial3D.new()
	trunk_mat.albedo_color = trunk_color
	trunk.mesh = trunk_mesh
	trunk.set_surface_override_material(0, trunk_mat)
	trunk.position.y = TREE_HEIGHT / 2.0
	tree_root.add_child(trunk)
	
	# Canopy (cone — use CylinderMesh with top_radius=0)
	var canopy = MeshInstance3D.new()
	var canopy_mesh = CylinderMesh.new()
	canopy_mesh.top_radius = 0.0
	canopy_mesh.bottom_radius = CANOPY_RADIUS
	canopy_mesh.height = TREE_HEIGHT * 0.7
	var canopy_mat = StandardMaterial3D.new()
	canopy_mat.albedo_color = canopy_color
	canopy.mesh = canopy_mesh
	canopy.set_surface_override_material(0, canopy_mat)
	canopy.position.y = TREE_HEIGHT + TREE_HEIGHT * 0.35
	tree_root.add_child(canopy)
	
	add_child(tree_root)
	tree_nodes[obj_id] = tree_root

func remove_tree(obj_id: int) -> void:
	if tree_nodes.has(obj_id):
		tree_nodes[obj_id].queue_free()
		tree_nodes.erase(obj_id)

func clear_all() -> void:
	for node in tree_nodes.values():
		node.queue_free()
	tree_nodes.clear()
