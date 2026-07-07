extends PanelContainer

## Equipment panel — 10 slots in a vertical column (right side).
## Phase 4: shows equipped gear.

const SLOT_NAMES := [
	"Head", "Cape", "Body", "Legs", "Weapon",
	"Shield", "Gloves", "Boots", "Amulet", "Ring",
]

const ITEM_NAMES := {
	20: "Bronze sword", 21: "Iron sword", 22: "Steel sword",
	30: "Bronze shield", 31: "Iron shield",
	40: "Bronze platebody", 41: "Iron platebody",
	50: "Bronze platelegs", 51: "Iron platelegs",
	60: "Bronze helm", 61: "Iron helm",
	70: "Bronze gloves", 71: "Iron gloves",
	80: "Bronze boots", 81: "Iron boots",
	90: "Amulet of strength", 91: "Amulet of defence",
	10: "Bronze axe", 11: "Iron axe",
}

const ITEM_COLORS := {
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
	10: Color(0.72, 0.53, 0.22), # Bronze axe
	11: Color(0.70, 0.70, 0.70), # Iron axe
}

var _slots: Array = []  # Panel nodes
var _slot_data: Array = []  # {id, qty}
signal unequip_requested(slot_index: int)

func _ready() -> void:
	_build_slots()
	anchor_right = 1.0
	anchor_bottom = 0.5
	offset_left = -150
	offset_top = -180
	custom_minimum_size = Vector2(150, 360)

func _build_slots() -> void:
	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 2)

	for i in range(10):
		var row = HBoxContainer.new()
		# Slot name label
		var name_label = Label.new()
		name_label.text = SLOT_NAMES[i]
		name_label.custom_minimum_size = Vector2(55, 30)
		name_label.add_theme_font_size_override("font_size", 10)
		name_label.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
		row.add_child(name_label)

		# Slot panel
		var slot_panel = Panel.new()
		slot_panel.name = "Slot%d" % i
		slot_panel.custom_minimum_size = Vector2(80, 30)
		slot_panel.add_theme_stylebox_override("panel", _create_slot_style(Color(0.15, 0.15, 0.15, 0.8)))

		var item_label = Label.new()
		item_label.name = "Label"
		item_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		item_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		item_label.add_theme_font_size_override("font_size", 10)
		item_label.modulate = Color(0.8, 0.8, 0.8)
		item_label.text = ""
		slot_panel.add_child(item_label)

		var item_rect = ColorRect.new()
		item_rect.name = "ItemRect"
		item_rect.size = Vector2(10, 10)
		item_rect.position = Vector2(2, 10)
		item_rect.color = Color(0, 0, 0, 0)
		slot_panel.add_child(item_rect)

		_slots.append(slot_panel)
		_slot_data.append({"id": 0, "qty": 0})
		row.add_child(slot_panel)
		vbox.add_child(row)

	add_theme_stylebox_override("panel", _create_bg_style())
	add_child(vbox)

func update_equipment(slots: Array) -> void:
	for i in range(min(slots.size(), 10)):
		_slot_data[i] = slots[i]
		_refresh_slot(i)

func _refresh_slot(i: int) -> void:
	var data = _slot_data[i]
	var slot = _slots[i]
	var label = slot.get_node("Label") as Label
	var item_rect = slot.get_node("ItemRect") as ColorRect

	if data.id == 0 or data.qty == 0:
		label.text = "Empty"
		label.add_theme_color_override("font_color", Color(0.4, 0.4, 0.4))
		item_rect.color = Color(0, 0, 0, 0)
		return

	var name = ITEM_NAMES.get(data.id, "???")
	var color = ITEM_COLORS.get(data.id, Color(0.5, 0.5, 0.5))
	label.text = name
	label.add_theme_color_override("font_color", Color(0.9, 0.85, 0.7))
	item_rect.color = color

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
	style.border_color = Color(0.35, 0.30, 0.15)
	style.set_corner_radius_all(4)
	return style
