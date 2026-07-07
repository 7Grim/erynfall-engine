extends PanelContainer

## Inventory panel — 28 slots displayed in a 4×7 grid.
## Phase 4: right-click for equip/unequip/use food.

const ITEM_NAMES := {
	0: "Normal logs", 1: "Oak logs", 2: "Willow logs", 3: "Maple logs",
	4: "Yew logs", 5: "Magic logs",
	10: "Bronze axe", 11: "Iron axe", 12: "Steel axe",
	13: "Mithril axe", 14: "Adamant axe", 15: "Rune axe", 16: "Dragon axe",
	20: "Bronze sword", 21: "Iron sword", 22: "Steel sword",
	30: "Bronze shield", 31: "Iron shield",
	40: "Bronze platebody", 41: "Iron platebody",
	50: "Bronze platelegs", 51: "Iron platelegs",
	60: "Bronze helm", 61: "Iron helm",
	70: "Bronze gloves", 71: "Iron gloves",
	80: "Bronze boots", 81: "Iron boots",
	90: "Amulet of strength", 91: "Amulet of defence",
	110: "Cooked shrimp", 112: "Cooked sardine",
	113: "Cooked trout", 114: "Cooked salmon",
	115: "Cooked lobster", 116: "Cooked swordfish",
	998: "Coins",
}

const ITEM_COLORS := {
	0: Color(0.55, 0.35, 0.15),  # Normal logs — light brown
	1: Color(0.45, 0.28, 0.10),  # Oak — darker brown
	2: Color(0.50, 0.40, 0.18),  # Willow — tan
	3: Color(0.60, 0.35, 0.12),  # Maple — orange-brown
	4: Color(0.35, 0.22, 0.08),  # Yew — dark brown
	5: Color(0.40, 0.18, 0.45),  # Magic — purple
	10: Color(0.72, 0.53, 0.22), # Bronze axe
	11: Color(0.70, 0.70, 0.70), # Iron axe — grey
	12: Color(0.65, 0.65, 0.68), # Steel axe
	13: Color(0.40, 0.60, 0.70), # Mithril axe — blue-ish
	14: Color(0.30, 0.50, 0.30), # Adamant axe — green-ish
	15: Color(0.20, 0.40, 0.65), # Rune axe — blue
	16: Color(0.50, 0.15, 0.15), # Dragon axe — red
	20: Color(0.72, 0.53, 0.22), # Bronze sword
	21: Color(0.70, 0.70, 0.70), # Iron sword
	22: Color(0.65, 0.65, 0.68), # Steel sword
	30: Color(0.72, 0.53, 0.22), # Bronze shield
	31: Color(0.70, 0.70, 0.70), # Iron shield
	40: Color(0.72, 0.53, 0.22), # Bronze platebody
	41: Color(0.70, 0.70, 0.70), # Iron platebody
	50: Color(0.72, 0.53, 0.22), # Bronze platelegs
	51: Color(0.70, 0.70, 0.70), # Iron platelegs
	60: Color(0.72, 0.53, 0.22), # Bronze helm
	61: Color(0.70, 0.70, 0.70), # Iron helm
	70: Color(0.72, 0.53, 0.22), # Bronze gloves
	71: Color(0.70, 0.70, 0.70), # Iron gloves
	80: Color(0.72, 0.53, 0.22), # Bronze boots
	81: Color(0.70, 0.70, 0.70), # Iron boots
	90: Color(0.80, 0.30, 0.30), # Amulet of strength
	91: Color(0.30, 0.50, 0.80), # Amulet of defence
	110: Color(0.6, 0.8, 0.3),     # Cooked shrimp
	112: Color(0.5, 0.7, 0.4),     # Cooked sardine
	113: Color(0.55, 0.75, 0.35), # Cooked trout
	114: Color(0.7, 0.6, 0.3),     # Cooked salmon
	115: Color(0.6, 0.4, 0.3),     # Cooked lobster
	116: Color(0.5, 0.35, 0.25),   # Cooked swordfish
	998: Color(0.85, 0.75, 0.20), # Coins — gold
}

signal action_requested(action: int, slot: int)  # Phase 4: equip(0)/unequip(1)/use(3)

const FOOD_ITEMS = [110, 112, 113, 114, 115, 116]  # Cooked food items
const EQUIP_ITEMS = [10, 11, 12, 13, 14, 15, 16, 20, 21, 22, 30, 31, 40, 41, 50, 51, 60, 61, 70, 71, 80, 81, 90, 91]

var _slots: Array = []  # Array of Panel nodes
var _slot_data: Array = []  # Array of {id, qty}

func _ready() -> void:
	_build_slots()
	# Center-bottom of screen
	anchor_left = 0.5
	anchor_right = 0.5
	anchor_top = 0.0
	anchor_bottom = 0.0
	grow_horizontal = Control.GROW_DIRECTION_BOTH
	position = Vector2(-130, 10)
	size = Vector2(260, 220)

func _build_slots() -> void:
	var grid = GridContainer.new()
	grid.columns = 4
	grid.add_theme_constant_override("h_separation", 2)
	grid.add_theme_constant_override("v_separation", 2)
	
	for i in range(28):
		var slot_panel = Panel.new()
		slot_panel.name = "Slot%d" % i
		slot_panel.custom_minimum_size = Vector2(56, 28)
		slot_panel.add_theme_stylebox_override("panel", _create_slot_style(Color(0.15, 0.15, 0.15, 0.8)))
		
		var label = Label.new()
		label.name = "Label"
		label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		label.add_theme_font_size_override("font_size", 11)
		label.modulate = Color(0.7, 0.7, 0.7)
		label.text = ""
		slot_panel.add_child(label)
		
		var item_rect = ColorRect.new()
		item_rect.name = "ItemRect"
		item_rect.size = Vector2(12, 12)
		item_rect.position = Vector2(4, 8)
		item_rect.color = Color(0, 0, 0, 0)
		slot_panel.add_child(item_rect)
		
		_slots.append(slot_panel)
		_slot_data.append({"id": 0, "qty": 0})
		grid.add_child(slot_panel)
		
		# Phase 4: click handling — left click uses/eats, right click equips
		var idx = i
		slot_panel.gui_input.connect(func(event: InputEvent) -> void:
			if event is InputEventMouseButton and event.pressed:
				var data = _slot_data[idx]
				if data.id == 0:
					return
				if event.button_index == MOUSE_BUTTON_LEFT:
					if data.id in FOOD_ITEMS:
						action_requested.emit(3, idx)
					elif data.id in EQUIP_ITEMS:
						action_requested.emit(0, idx)
				elif event.button_index == MOUSE_BUTTON_RIGHT:
					if data.id in EQUIP_ITEMS:
						action_requested.emit(1, idx)
					elif data.id in FOOD_ITEMS:
						action_requested.emit(3, idx)
		)
	
	# Style the main panel
	add_theme_stylebox_override("panel", _create_bg_style())
	add_child(grid)

func update_inventory(slots: Array) -> void:
	for i in range(min(slots.size(), 28)):
		_slot_data[i] = slots[i]
		_refresh_slot(i)

func _refresh_slot(i: int) -> void:
	var data = _slot_data[i]
	var slot = _slots[i]
	var label = slot.get_node("Label") as Label
	var item_rect = slot.get_node("ItemRect") as ColorRect
	
	if data.id == 0 or data.qty == 0:
		label.text = ""
		item_rect.color = Color(0, 0, 0, 0)
		return
	
	var name = ITEM_NAMES.get(data.id, "???")
	var color = ITEM_COLORS.get(data.id, Color(0.5, 0.5, 0.5))
	item_rect.color = color
	
	if data.qty > 1:
		label.text = "%d×%d" % [data.id, data.qty]
		label.position.x = 20
	else:
		label.text = name
		label.position.x = 20
		label.add_theme_font_size_override("font_size", 9)

func _create_slot_style(bg: Color) -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = bg
	style.border_width_left = 1
	style.border_width_top = 1
	style.border_width_right = 1
	style.border_width_bottom = 1
	style.border_color = Color(0.3, 0.3, 0.3)
	style.set_corner_radius_all(2)
	return style

func _create_bg_style() -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.08, 0.08, 0.85)
	style.border_width_left = 2
	style.border_width_top = 2
	style.border_width_right = 2
	style.border_width_bottom = 2
	style.border_color = Color(0.4, 0.35, 0.2)
	style.set_corner_radius_all(4)
	return style
