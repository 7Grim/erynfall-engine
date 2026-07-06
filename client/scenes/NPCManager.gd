extends Node3D

## Manages 3D NPC objects on the map.
## Creates colored box NPCs with HP bars.

const NPC_COLORS := {
	0: Color(0.2, 0.7, 0.2),   # Goblin — green
	1: Color(0.7, 0.55, 0.3),   # Cow — brown
	2: Color(0.9, 0.85, 0.7),   # Chicken — white/cream
}

const NPC_NAMES := {
	0: "Goblin",
	1: "Cow",
	2: "Chicken",
}

const NPC_SIZES := {
	0: Vector3(0.6, 0.8, 0.6),   # Goblin — small
	1: Vector3(0.9, 0.7, 0.5),   # Cow — medium
	2: Vector3(0.3, 0.3, 0.3),   # Chicken — tiny
}

var _npcs: Dictionary = {}  # npcId -> NPCVisual
var _grid: Node3D
var _tile_size: float
var _grid_size: int = 30
var _setup_done := false

func setup(grid: Node3D, tile_size: float, grid_size: int = 30) -> void:
	_grid = grid
	_tile_size = tile_size
	_grid_size = grid_size
	_setup_done = true

## Add/update an NPC on the map
func sync_npc(npc_id: int, npc_type: int, x: int, y: int, hp: int, max_hp: int, alive: bool) -> void:
	if not _setup_done:
		return
	if _npcs.has(npc_id):
		if not alive:
			# Remove dead NPC
			var vis = _npcs[npc_id]
			if vis.body and vis.body.is_inside_tree():
				vis.body.queue_free()
			if vis.hp_bar and vis.hp_bar.is_inside_tree():
				vis.hp_bar.queue_free()
			if vis.name_label and vis.name_label.is_inside_tree():
				vis.name_label.queue_free()
			_npcs.erase(npc_id)
			return
		
		# Update existing NPC
		var vis = _npcs[npc_id]
		var wpos = _tile_to_world(x, y)
		vis.body.global_position = wpos + Vector3(0, NPC_SIZES.get(npc_type, Vector3(0.5,0.5,0.5)).y * 0.5, 0)
		_update_hp_bar(vis, hp, max_hp)
		return
	
	if not alive:
		return
	
	# Create new NPC
	var color = NPC_COLORS.get(npc_type, Color(0.5, 0.5, 0.5))
	var size = NPC_SIZES.get(npc_type, Vector3(0.5, 0.5, 0.5))
	
	var body = MeshInstance3D.new()
	var mesh = BoxMesh.new()
	mesh.size = size
	body.mesh = mesh
	var mat = StandardMaterial3D.new()
	mat.albedo_color = color
	body.material_override = mat
	body.name = "NPC_%d" % npc_id
	
	var wpos = _tile_to_world(x, y)
	body.global_position = wpos + Vector3(0, size.y * 0.5, 0)
	_grid.add_child(body)
	
	# HP bar above NPC
	var hp_bar := _create_hp_bar()
	hp_bar.global_position = wpos + Vector3(0, size.y + 0.3, 0)
	_grid.add_child(hp_bar)
	_update_hp_bar_from_nodes(body, hp_bar, hp, max_hp)
	
	# Name label
	var name_label := _create_name_label(NPC_NAMES.get(npc_type, "NPC"))
	name_label.global_position = wpos + Vector3(0, size.y + 0.55, 0)
	_grid.add_child(name_label)
	
	_npcs[npc_id] = {
		"body": body,
		"hp_bar": hp_bar,
		"name_label": name_label,
	}

func _create_hp_bar() -> MeshInstance3D:
	var bg = MeshInstance3D.new()
	var bg_mesh = BoxMesh.new()
	bg_mesh.size = Vector3(0.8, 0.08, 0.08)
	bg.mesh = bg_mesh
	var bg_mat = StandardMaterial3D.new()
	bg_mat.albedo_color = Color(0.8, 0.1, 0.1)
	bg.material_override = bg_mat
	
	var fg = MeshInstance3D.new()
	var fg_mesh = BoxMesh.new()
	fg_mesh.size = Vector3(0.8, 0.06, 0.09)
	fg.mesh = fg_mesh
	var fg_mat = StandardMaterial3D.new()
	fg_mat.albedo_color = Color(0.1, 0.8, 0.1)
	fg.material_override = fg_mat
	fg.position.z = 0.01
	fg.name = "HPFill"
	bg.add_child(fg)
	
	return bg

func _update_hp_bar(vis: Dictionary, hp: int, max_hp: int) -> void:
	_update_hp_bar_from_nodes(vis.body, vis.hp_bar, hp, max_hp)

func _update_hp_bar_from_nodes(body: Node3D, hp_bar: Node3D, hp: int, max_hp: int) -> void:
	var fill = hp_bar.get_node_or_null("HPFill") as MeshInstance3D
	if fill:
		var ratio = clampf(float(hp) / float(max(max_hp, 1)), 0.0, 1.0)
		fill.scale.x = ratio
		fill.position.x = -0.4 * (1.0 - ratio)
		if ratio < 0.3:
			(fill.material_override as StandardMaterial3D).albedo_color = Color(0.8, 0.1, 0.1)
		elif ratio < 0.6:
			(fill.material_override as StandardMaterial3D).albedo_color = Color(0.8, 0.7, 0.1)
		else:
			(fill.material_override as StandardMaterial3D).albedo_color = Color(0.1, 0.8, 0.1)

func _create_name_label(text: String) -> Label3D:
	var label = Label3D.new()
	label.text = text
	label.font_size = 48
	label.pixel_size = 0.02
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.outline_size = 4
	label.outline_modulate = Color(0, 0, 0)
	return label

func _tile_to_world(x: int, y: int) -> Vector3:
	var offset = float(_grid_size) * _tile_size * 0.5
	return Vector3(x * _tile_size - offset, 0, y * _tile_size - offset)

## Check if an NPC exists at a tile position
func npc_at_tile(x: int, y: int) -> int:
	for npc_id in _npcs:
		return npc_id
	return -1

## Remove all NPCs
func clear() -> void:
	for npc_id in _npcs:
		var vis = _npcs[npc_id]
		if vis.body and vis.body.is_inside_tree():
			vis.body.queue_free()
		if vis.hp_bar and vis.hp_bar.is_inside_tree():
			vis.hp_bar.queue_free()
		if vis.name_label and vis.name_label.is_inside_tree():
			vis.name_label.queue_free()
	_npcs.clear()
