extends Node3D

## Main game scene — Phase 2: The Loop.
## Trees, woodcutting, inventory, skills, server connection.
## GDScript bootstrap — creates all nodes at runtime.

const GRID_SIZE := 30
const TILE_SIZE := 1.0

# Tree positions matching server (32 trees)
const TREE_DATA := [
	[2, 2, 0], [4, 1, 0], [1, 4, 0], [5, 3, 0], [3, 6, 0], [6, 5, 0],
	[2, 8, 0], [7, 2, 0], [5, 7, 0], [1, 9, 0], [8, 4, 0], [4, 10, 0],
	[3, 3, 0], [7, 7, 0], [9, 1, 0], [3, 12, 0], [7, 13, 0], [12, 10, 0],
	[17, 8, 0], [22, 12, 0], [25, 11, 0], [5, 17, 0], [20, 5, 0],
	[26, 7, 0], [15, 4, 0], [18, 17, 0],
	[8, 8, 1], [14, 6, 1], [20, 3, 1], [27, 9, 1],
	[2, 16, 2], [25, 5, 2],
]

var game_tick: Node
var player: Node
var camera: Node3D
var highlight: MeshInstance3D
var ground: MeshInstance3D
var tree_manager: Node
var network: Node
var inventory_panel: Control
var skills_panel: Control
var level_up_notif: CanvasLayer
var chop_label: Label
var msg_log: Label

var tiles: Dictionary = {}
var tree_tiles: Dictionary = {}  # Vector2i -> [obj_id, tree_type]
var _chopping := false
var _chop_target := Vector2i()
var _chop_timer := 0.0
var _messages: Array = []
var _cached_levels: Dictionary = {}

func _ready() -> void:
	_bootstrap_scene()
	_generate_map()
	_spawn_trees()
	_connect_signals()
	print("[Main] Phase 2 initialized - trees, woodcutting, inventory, skills")

func _bootstrap_scene() -> void:
	# GameTick
	game_tick = Node.new()
	game_tick.name = "GameTick"
	game_tick.set_script(load("res://scenes/GameTick.gd"))
	add_child(game_tick)
	
	# Player
	player = Node3D.new()
	player.name = "Player"
	player.set_script(load("res://scenes/Player.gd"))
	add_child(player)
	
	# Orbital Camera
	camera = Node3D.new()
	camera.name = "OrbitalCamera"
	camera.set_script(load("res://scenes/OrbitalCamera.gd"))
	add_child(camera)
	# Camera needs player reference
	camera.set("follow_target", player)
	
	# Tile Highlight
	highlight = MeshInstance3D.new()
	highlight.name = "TileHighlight"
	highlight.set_script(load("res://scenes/TileHighlight.gd"))
	add_child(highlight)
	
	# Ground mesh
	ground = MeshInstance3D.new()
	ground.name = "Ground"
	add_child(ground)
	
	# Tree Manager
	tree_manager = Node3D.new()
	tree_manager.name = "TreeManager"
	tree_manager.set_script(load("res://scenes/TreeManager.gd"))
	add_child(tree_manager)
	
	# Network Client
	network = Node.new()
	network.name = "NetworkClient"
	network.set_script(load("res://scenes/NetworkClient.gd"))
	add_child(network)
	
	# Directional Light
	var light = DirectionalLight3D.new()
	light.name = "DirectionalLight3D"
	light.light_color = Color(1, 0.95, 0.85)
	light.light_energy = 1.2
	light.rotation.x = -0.8
	light.rotation.z = -0.3
	add_child(light)
	
	# UI CanvasLayer
	var ui = CanvasLayer.new()
	ui.name = "UI"
	add_child(ui)
	
	# Inventory Panel
	inventory_panel = PanelContainer.new()
	inventory_panel.name = "InventoryPanel"
	inventory_panel.set_script(load("res://scenes/InventoryPanel.gd"))
	ui.add_child(inventory_panel)
	
	# Skills Panel
	skills_panel = PanelContainer.new()
	skills_panel.name = "SkillsPanel"
	skills_panel.set_script(load("res://scenes/SkillsPanel.gd"))
	ui.add_child(skills_panel)
	
	# Level-Up Notification
	level_up_notif = CanvasLayer.new()
	level_up_notif.name = "LevelUpNotification"
	level_up_notif.set_script(load("res://scenes/LevelUpNotification.gd"))
	ui.add_child(level_up_notif)
	
	# Level-up label (inside the CanvasLayer)
	var lvl_label = Label.new()
	lvl_label.name = "Label"
	lvl_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lvl_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	lvl_label.position = Vector2(400, 200)
	lvl_label.size = Vector2(300, 60)
	lvl_label.add_theme_font_size_override("font_size", 18)
	lvl_label.add_theme_color_override("font_color", Color(1, 0.85, 0.2))
	lvl_label.add_theme_color_override("font_shadow_color", Color(0, 0, 0))
	level_up_notif.add_child(lvl_label)
	
	# Chop label
	chop_label = Label.new()
	chop_label.name = "ChopLabel"
	chop_label.text = "Chopping..."
	chop_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	chop_label.visible = false
	chop_label.add_theme_font_size_override("font_size", 14)
	chop_label.add_theme_color_override("font_color", Color(0.9, 0.8, 0.3))
	ui.add_child(chop_label)
	
	# Message log
	msg_log = Label.new()
	msg_log.name = "MessageLog"
	msg_log.vertical_alignment = VERTICAL_ALIGNMENT_BOTTOM
	msg_log.add_theme_font_size_override("font_size", 12)
	msg_log.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))
	ui.add_child(msg_log)

func _generate_map() -> void:
	for x in range(GRID_SIZE):
		for z in range(GRID_SIZE):
			tiles[Vector2i(x, z)] = 0
	# Water pond
	for x in range(10, 14):
		for z in range(20, 25):
			tiles[Vector2i(x, z)] = 1
	# Dirt path
	for x in range(GRID_SIZE):
		tiles[Vector2i(x, 14)] = 2
		tiles[Vector2i(x, 15)] = 2
	_build_ground_mesh()

func _spawn_trees() -> void:
	tree_manager.clear_all()
	tree_tiles.clear()
	for i in range(TREE_DATA.size()):
		var tx = TREE_DATA[i][0]
		var ty = TREE_DATA[i][1]
		var ttype = TREE_DATA[i][2]
		var obj_id = i + 1
		tree_manager.add_tree(obj_id, ttype, tx, ty, GRID_SIZE, TILE_SIZE)
		tree_tiles[Vector2i(tx, ty)] = [obj_id, ttype]

func _build_ground_mesh() -> void:
	var half = float(GRID_SIZE) * TILE_SIZE / 2.0
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
	
	var array_mesh = ArrayMesh.new()
	var type_colors = [
		Color(0.32, 0.68, 0.32),
		Color(0.2, 0.4, 0.8),
		Color(0.5, 0.38, 0.22),
	]
	
	for type_quads in [grass_quads, water_quads, dirt_quads]:
		if type_quads.is_empty():
			continue
		var verts = PackedVector3Array()
		var normals = PackedVector3Array()
		var indices = PackedInt32Array()
		for quad in type_quads:
			var base = verts.size()
			verts.append_array(PackedVector3Array(quad))
			for _j in range(4):
				normals.append(Vector3.UP)
			indices.append_array(PackedInt32Array([
				base, base + 2, base + 1, base, base + 3, base + 2
			]))
		var arrays = []
		arrays.resize(Mesh.ARRAY_MAX)
		arrays[Mesh.ARRAY_VERTEX] = verts
		arrays[Mesh.ARRAY_NORMAL] = normals
		arrays[Mesh.ARRAY_INDEX] = indices
		array_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	
	ground.mesh = array_mesh
	for i in range(array_mesh.get_surface_count()):
		var mat = StandardMaterial3D.new()
		mat.albedo_color = type_colors[i]
		ground.set_surface_override_material(i, mat)

func _connect_signals() -> void:
	game_tick.tick.connect(_on_tick)
	network.system_message.connect(_on_system_message)
	network.position_update.connect(_on_position_update)
	network.inventory_sync.connect(_on_inventory_sync)
	network.skill_update.connect(_on_skill_update)
	network.skills_sync.connect(_on_skills_sync)
	network.animation.connect(_on_animation)

func _on_tick() -> void:
	player.tick_move()
	network.send_keepalive()
	if _chopping:
		_chop_timer -= 0.6
		if _chop_timer <= 0:
			_chopping = false
			chop_label.visible = false

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mb = event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_LEFT and mb.pressed:
			_handle_click()

func _process(_delta: float) -> void:
	# Update hover highlight every frame
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
	var tile = _world_to_tile(hit)
	if tree_tiles.has(tile):
		highlight.set_highlight_color(Color(0.8, 0.6, 0.2, 0.4))
	else:
		highlight.set_highlight_color(Color(1, 1, 1, 0.3))

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
	
	# Tree click?
	if tree_tiles.has(tile):
		var player_tile = player.tile_position()
		var dist = abs(player_tile.x - tile.x) + abs(player_tile.y - tile.y)
		if dist > 2:
			_add_message("Too far away.")
			return
		network.send_action_object(0, tile.x, tile.y)
		_add_message("You swing your axe at the tree...")
		_chopping = true
		_chop_target = tile
		_chop_timer = 3.0
		chop_label.visible = true
		chop_label.text = "Chopping..."
		player.face_toward(tile)
		return
	
	# Walk
	var type = tiles.get(tile, 0)
	if type == 1:
		return
	if _chopping:
		_chopping = false
		chop_label.visible = false
	player.walk_to(tile)

func _world_to_tile(world_pos: Vector3) -> Vector2i:
	var offset = float(GRID_SIZE) * TILE_SIZE / 2.0
	return Vector2i(
		int(floor(world_pos.x + offset)),
		int(floor(world_pos.z + offset))
	)

# --- Network handlers ---

func _on_system_message(msg: String) -> void:
	_add_message(msg)

func _on_position_update(x: int, y: int) -> void:
	pass  # Movement is local-only in Phase 2

func _on_inventory_sync(slots: Array) -> void:
	inventory_panel.update_inventory(slots)

func _on_skill_update(skill_id: int, level: int, xp: int) -> void:
	skills_panel.update_skill(skill_id, level, xp)
	var old_level = _cached_levels.get(skill_id, 1)
	if level > old_level:
		_on_level_up(skill_id, level)
	_cached_levels[skill_id] = level

func _on_skills_sync(skills: Array) -> void:
	skills_panel.update_skills(skills)
	for s in skills:
		_cached_levels[s.id] = s.level

func _on_animation(anim_id: int, target_x: int, target_y: int) -> void:
	if anim_id == 1:
		_chopping = true
		_chop_target = Vector2i(target_x, target_y)
		var tile = _chop_target
		if tree_tiles.has(tile):
			var tree_type = tree_tiles[tile][1]
			match tree_type:
				0: _chop_timer = 2.4
				1: _chop_timer = 4.0
				2: _chop_timer = 5.0
				_: _chop_timer = 3.0
		chop_label.visible = true
		chop_label.text = "Chopping..."

func _on_level_up(skill_id: int, level: int) -> void:
	var skill_names := [
		"Attack", "Strength", "Defence", "Hitpoints", "Ranged", "Prayer",
		"Magic", "Cooking", "Woodcutting", "Fletching", "Fishing", "Firemaking",
		"Crafting", "Smithing", "Mining", "Herblore", "Agility", "Thieving",
		"Slayer", "Farming", "Runecrafting", "Hunter", "Construction",
	]
	var name = skill_names[skill_id] if skill_id < skill_names.size() else "???"
	level_up_notif.show_level_up(name, level)
	_add_message("Congratulations! Your %s level is now %d!" % [name, level])

func _add_message(msg: String) -> void:
	_messages.append(msg)
	if _messages.size() > 8:
		_messages.pop_front()
	msg_log.text = "\n".join(_messages)
