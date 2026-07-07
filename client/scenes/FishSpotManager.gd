extends Node3D

## Manages 3D fishing spot objects on the map.
## Each spot is a flat blue circle on water tiles.

const SPOT_COLORS := {
	0: Color(0.20, 0.50, 0.80),  # Shrimp pool — light blue
	1: Color(0.15, 0.45, 0.75),  # Sardine pool — medium blue
	2: Color(0.10, 0.40, 0.70),  # Trout pool — deeper blue
}

const SPOT_RADIUS := 0.4

var spot_nodes: Dictionary = {}

func add_spot(obj_id: int, spot_type: int, tile_x: int, tile_y: int, grid_size: int, tile_size: float) -> void:
	if spot_nodes.has(obj_id):
		return
	
	var half = float(grid_size) * tile_size / 2.0
	var wx = float(tile_x) - half + tile_size * 0.5
	var wz = float(tile_y) - half + tile_size * 0.5
	
	var spot_color = SPOT_COLORS.get(spot_type, Color(0.20, 0.50, 0.80))
	
	var spot_root = Node3D.new()
	spot_root.name = "FishSpot_%d" % obj_id
	spot_root.position = Vector3(wx, 0, wz)
	
	# Water ripple circle (CylinderMesh flat)
	var ripple = MeshInstance3D.new()
	var ripple_mesh = CylinderMesh.new()
	ripple_mesh.top_radius = SPOT_RADIUS
	ripple_mesh.bottom_radius = SPOT_RADIUS * 0.8
	ripple_mesh.height = 0.05
	var ripple_mat = StandardMaterial3D.new()
	ripple_mat.albedo_color = spot_color
	ripple_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	ripple_mat.albedo_color.a = 0.7
	ripple.mesh = ripple_mesh
	ripple.set_surface_override_material(0, ripple_mat)
	ripple.position.y = 0.03
	spot_root.add_child(ripple)
	
	# Inner brighter dot (where fish are)
	var dot = MeshInstance3D.new()
	var dot_mesh = SphereMesh.new()
	dot_mesh.radius = 0.08
	dot_mesh.height = 0.05
	var dot_mat = StandardMaterial3D.new()
	dot_mat.albedo_color = Color(1.0, 1.0, 1.0, 0.5)
	dot_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	dot.mesh = dot_mesh
	dot.set_surface_override_material(0, dot_mat)
	dot.position.y = 0.06
	spot_root.add_child(dot)
	
	add_child(spot_root)
	spot_nodes[obj_id] = spot_root

func remove_spot(obj_id: int) -> void:
	if spot_nodes.has(obj_id):
		spot_nodes[obj_id].queue_free()
		spot_nodes.erase(obj_id)

func clear_all() -> void:
	for node in spot_nodes.values():
		node.queue_free()
	spot_nodes.clear()
