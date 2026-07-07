extends Node3D

## Main game scene — Phase 4: The Gear.
## Trees, woodcutting, inventory, skills, NPCs, combat, death.
## Equipment, shops, food, gold, HP system.
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
var npc_manager: Node
var network: Node
var inventory_panel: Control
var skills_panel: Control
var level_up_notif: CanvasLayer
var chop_label: Label
var msg_log: Label
var combat_overlay: CanvasLayer
var death_screen: CanvasLayer
var equipment_panel: PanelContainer  # Phase 4
var shop_panel: CanvasLayer  # Phase 4
var _shop_visible := false  # Phase 4

var tiles: Dictionary = {}
var tree_tiles: Dictionary = {}  # Vector2i -> [obj_id, tree_type]
var npc_tiles: Dictionary = {}   # Vector2i -> npc_id
var _chopping := false
var _chop_target := Vector2i()
var _chop_timer := 0.0
var _in_combat := false
var _combat_target := ""
var _dead := false
var _messages: Array = []
var _cached_levels: Dictionary = {}
var _player_hp: int = 10
var _player_max_hp: int = 10

func _ready() -> void:
	_bootstrap_scene()
	_generate_map()
	_spawn_trees()
	_connect_signals()
	print("[Main] Phase 4 initialized - gear, shops, food, gold")

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
	
	# NPC Manager
	npc_manager = Node3D.new()
	npc_manager.name = "NPCManager"
	npc_manager.set_script(load("res://scenes/NPCManager.gd"))
	add_child(npc_manager)
	
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
	
	# Combat Overlay (HP bar, hit messages)
	combat_overlay = CanvasLayer.new()
	combat_overlay.name = "CombatOverlay"
	combat_overlay.set_script(load("res://scenes/CombatOverlay.gd"))
	ui.add_child(combat_overlay)
	# HP Bar panel — top center
	var hp_panel = PanelContainer.new()
	hp_panel.name = "HPPanel"
	hp_panel.anchor_left = 0.5
	hp_panel.anchor_right = 0.5
	hp_panel.anchor_top = 0.0
	hp_panel.offset_left = -120
	hp_panel.offset_right = 120
	hp_panel.offset_top = 10
	hp_panel.offset_bottom = 50
	combat_overlay.add_child(hp_panel)
	var hp_bar = ProgressBar.new()
	hp_bar.name = "HPBar"
	hp_bar.max_value = 10
	hp_bar.value = 10
	hp_bar.show_percentage = false
	hp_bar.custom_minimum_size = Vector2(200, 20)
	hp_bar.add_theme_color_override("font_color", Color.WHITE)
	hp_panel.add_child(hp_bar)
	var hp_label = Label.new()
	hp_label.name = "HPLabel"
	hp_label.text = "10/10"
	hp_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hp_bar.add_child(hp_label)
	# Phase 4: Gold label
	var gold_label = Label.new()
	gold_label.name = "GoldLabel"
	gold_label.text = "Gold: 0"
	gold_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	gold_label.add_theme_font_size_override("font_size", 11)
	gold_label.add_theme_color_override("font_color", Color(0.85, 0.75, 0.20))
	hp_panel.add_child(gold_label)
	# Combat label — below HP bar
	var combat_label = Label.new()
	combat_label.name = "CombatLabel"
	combat_label.text = "Fighting: ..."
	combat_label.anchor_left = 0.5
	combat_label.anchor_right = 0.5
	combat_label.anchor_top = 0.0
	combat_label.offset_top = 55
	combat_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	combat_label.add_theme_font_size_override("font_size", 14)
	combat_label.add_theme_color_override("font_color", Color(1, 0.3, 0.3))
	combat_label.visible = false
	combat_overlay.add_child(combat_label)
	# Hit message container — top right
	var hit_container = VBoxContainer.new()
	hit_container.name = "HitContainer"
	hit_container.anchor_right = 1.0
	hit_container.anchor_top = 0.0
	hit_container.offset_left = -280
	hit_container.offset_right = -20
	hit_container.offset_top = 80
	hit_container.visible = false
	combat_overlay.add_child(hit_container)
	
	# Death Screen
	death_screen = CanvasLayer.new()
	death_screen.name = "DeathScreen"
	death_screen.set_script(load("res://scenes/DeathScreen.gd"))
	ui.add_child(death_screen)
	var death_panel = PanelContainer.new()
	death_panel.name = "Panel"
	death_panel.anchor_left = 0.5
	death_panel.anchor_right = 0.5
	death_panel.anchor_top = 0.5
	death_panel.anchor_bottom = 0.5
	death_panel.offset_left = -150
	death_panel.offset_right = 150
	death_panel.offset_top = -60
	death_panel.offset_bottom = 60
	death_screen.add_child(death_panel)
	var death_label = Label.new()
	death_label.name = "Label"
	death_label.text = "Oh dear, you are dead!"
	death_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	death_label.add_theme_font_size_override("font_size", 20)
	death_label.add_theme_color_override("font_color", Color.RED)
	death_panel.add_child(death_label)
	var timer_label = Label.new()
	timer_label.name = "TimerLabel"
	timer_label.text = "Respawning in 5..."
	timer_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	death_panel.add_child(timer_label)

	# Phase 4: Equipment Panel (right side)
	equipment_panel = PanelContainer.new()
	equipment_panel.name = "EquipmentPanel"
	equipment_panel.set_script(load("res://scenes/EquipmentPanel.gd"))
	ui.add_child(equipment_panel)

	# Phase 4: Shop Panel (overlay)
	shop_panel = CanvasLayer.new()
	shop_panel.name = "ShopPanel"
	shop_panel.set_script(load("res://scenes/ShopPanel.gd"))
	ui.add_child(shop_panel)

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
	# Phase 4: inventory equip/use/eat actions
	inventory_panel.action_requested.connect(_on_inventory_action)
	network.skill_update.connect(_on_skill_update)
	network.skills_sync.connect(_on_skills_sync)
	network.animation.connect(_on_animation)
	network.npc_update.connect(_on_npc_update)
	# Phase 4: equipment, gold, health, shop
	network.equipment_sync.connect(_on_equipment_sync)
	network.gold_update.connect(_on_gold_update)
	network.health_update.connect(_on_health_update)
	network.shop_open.connect(_on_shop_open)
	shop_panel.buy_requested.connect(_on_buy_requested)
	shop_panel.sell_requested.connect(_on_sell_requested)

func _on_tick() -> void:
	player.tick_move()
	network.send_keepalive()
	if _chopping:
		_chop_timer -= 0.6
		if _chop_timer <= 0:
			_chopping = false
			chop_label.visible = false

func _unhandled_input(event: InputEvent) -> void:
	# Phase 4: Escape closes shop
	if event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
		if _shop_visible:
			shop_panel.close_shop()
			_shop_visible = false
			return
	if event is InputEventMouseButton:
		var mb = event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_LEFT and mb.pressed:
			_handle_click()
		elif mb.button_index == MOUSE_BUTTON_RIGHT and mb.pressed:
			_handle_right_click()

func _process(_delta: float) -> void:
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
	# Color highlight based on what's on the tile
	if npc_tiles.has(tile):
		highlight.set_highlight_color(Color(1.0, 0.2, 0.2, 0.5))  # Red for NPCs
	elif tree_tiles.has(tile):
		highlight.set_highlight_color(Color(0.8, 0.6, 0.2, 0.4))  # Orange for trees
	else:
		highlight.set_highlight_color(Color(1, 1, 1, 0.3))  # White for walkable

func _handle_click() -> void:
	if _dead:
		return
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
	
	# NPC click? (attack)
	if npc_tiles.has(tile):
		var player_tile = player.tile_position()
		var dist = abs(player_tile.x - tile.x) + abs(player_tile.y - tile.y)
		if dist > 2:
			_add_message("Too far away to attack.")
			return
		network.send_action_npc(0, tile.x, tile.y)
		_add_message("You attack the NPC!")
		player.face_toward(tile)
		return
	
	# Tree click? (chop)
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

func _handle_right_click() -> void:
	# Phase 4: right-click could open context menus
	# For now, eating food and equipping is via inventory panel clicks
	pass

func _world_to_tile(world_pos: Vector3) -> Vector2i:
	var offset = float(GRID_SIZE) * TILE_SIZE / 2.0
	return Vector2i(
		int(floor(world_pos.x + offset)),
		int(floor(world_pos.z + offset))
	)

# --- Network handlers ---

func _on_system_message(msg: String) -> void:
	_add_message(msg)
	# Parse combat messages
	if msg.begins_with("You hit the "):
		_in_combat = true
		combat_overlay.show_combat(msg.split(" for ")[0].replace("You hit the ", ""))
		combat_overlay.add_hit_message(msg)
	elif msg.begins_with("The ") and msg.find(" hits you") >= 0:
		combat_overlay.add_hit_message("[DAMAGE] " + msg)
	elif msg.find(" is dead!") >= 0:
		_in_combat = false
		combat_overlay.hide_combat()
		combat_overlay.add_hit_message("[KILL] " + msg)
	elif msg == "You respawn at home.":
		_dead = false
		death_screen.hide_death()

func _on_position_update(x: int, y: int) -> void:
	pass  # Movement is local-only

func _on_inventory_sync(slots: Array) -> void:
	inventory_panel.update_inventory(slots)

func _on_inventory_action(action: int, slot: int) -> void:
	network.send_inventory_action(action, slot)

func _on_skill_update(skill_id: int, level: int, xp: int) -> void:
	skills_panel.update_skill(skill_id, level, xp)
	var old_level = _cached_levels.get(skill_id, 1)
	if level > old_level:
		_on_level_up(skill_id, level)
	_cached_levels[skill_id] = level
	# Update HP overlay if hitpoints skill changed
	if skill_id == 3:  # Hitpoints
		_player_max_hp = level
		combat_overlay.update_hp(_player_hp, _player_max_hp)

func _on_skills_sync(skills: Array) -> void:
	skills_panel.update_skills(skills)
	for s in skills:
		_cached_levels[s.id] = s.level
		if s.id == 3:  # Hitpoints
			_player_max_hp = s.level
			_player_hp = s.level  # Full HP on connect

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
	elif anim_id == 2:
		# Attack animation
		player.play_attack()

func _on_npc_update(npc_id: int, npc_type: int, x: int, y: int, hp: int, max_hp: int, alive: bool) -> void:
	# Set up NPC manager with grid reference on first call
	if not npc_manager.has_method("setup"):
		return
	npc_manager.setup(ground, TILE_SIZE, GRID_SIZE)
	npc_manager.sync_npc(npc_id, npc_type, x, y, hp, max_hp, alive)
	
	# Track NPC tile positions
	if alive:
		npc_tiles[Vector2i(x, y)] = npc_id

# Phase 4: Equipment sync
func _on_equipment_sync(slots: Array) -> void:
	equipment_panel.update_equipment(slots)

# Phase 4: Gold update
func _on_gold_update(gold: int) -> void:
	if _shop_visible:
		shop_panel.update_gold(gold)
	var gold_label = combat_overlay.get_node_or_null("HPPanel/GoldLabel") as Label
	if gold_label:
		gold_label.text = "Gold: %d" % gold

# Phase 4: Health update
func _on_health_update(current_hp: int, max_hp: int) -> void:
	_player_hp = current_hp
	_player_max_hp = max_hp
	combat_overlay.update_hp(current_hp, max_hp)

# Phase 4: Shop opened
func _on_shop_open(items: Array) -> void:
	_shop_visible = true
	shop_panel.open_shop(items, 0)  # Gold will come from next gold_update

# Phase 4: Buy from shop
func _on_buy_requested(item_id: int) -> void:
	network.send_shop_action(0, item_id)  # ShopActionType::Buy = 0

# Phase 4: Sell to shop (sell selected inventory item)
func _on_sell_requested(_item_id: int) -> void:
	# For now, sell the first item in inventory
	# TODO: proper inventory selection UI
	network.send_shop_action(1, 0)  # ShopActionType::Sell = 1

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
