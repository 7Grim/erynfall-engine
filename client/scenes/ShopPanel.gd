extends CanvasLayer

## Shop panel — buy/sell interface overlay.
## Phase 4: opens when clicking a shopkeeper NPC.

signal buy_requested(item_id: int)
signal sell_requested(item_id: int)

var _panel: PanelContainer
var _item_list: ItemList
var _gold_label: Label
var _buy_btn: Button
var _sell_btn: Button
var _close_btn: Button
var _items: Array = []
var _player_gold: int = 0

const ITEM_NAMES := {
	20: "Bronze sword", 21: "Iron sword",
	22: "Bronze shield", 23: "Iron shield",
	24: "Bronze platebody", 25: "Iron platebody",
	26: "Bronze platelegs", 27: "Iron platelegs",
	28: "Bronze helm", 29: "Iron helm",
	30: "Bronze gloves", 31: "Iron gloves",
	32: "Bronze boots", 33: "Iron boots",
	34: "Cooked shrimp", 35: "Cooked sardine",
	36: "Bread", 37: "Cooked meat",
	0: "Normal logs", 1: "Oak logs", 2: "Willow logs",
}

const ITEM_COLORS := {
	20: Color(0.72, 0.53, 0.22), 21: Color(0.70, 0.70, 0.70),
	22: Color(0.72, 0.53, 0.22), 23: Color(0.70, 0.70, 0.70),
	24: Color(0.72, 0.53, 0.22), 25: Color(0.70, 0.70, 0.70),
	26: Color(0.72, 0.53, 0.22), 27: Color(0.70, 0.70, 0.70),
	28: Color(0.72, 0.53, 0.22), 29: Color(0.70, 0.70, 0.70),
	30: Color(0.72, 0.53, 0.22), 31: Color(0.70, 0.70, 0.70),
	32: Color(0.72, 0.53, 0.22), 33: Color(0.70, 0.70, 0.70),
	34: Color(0.6, 0.8, 0.3), 35: Color(0.5, 0.7, 0.4),
	36: Color(0.7, 0.6, 0.3), 37: Color(0.6, 0.4, 0.3),
	0: Color(0.55, 0.35, 0.15), 1: Color(0.45, 0.28, 0.10),
	2: Color(0.50, 0.40, 0.18),
}

func _ready() -> void:
	visible = false
	_build_ui()

func _build_ui() -> void:
	# Main panel
	_panel = PanelContainer.new()
	_panel.name = "ShopPanel"
	_panel.anchor_left = 0.5
	_panel.anchor_right = 0.5
	_panel.anchor_top = 0.5
	_panel.anchor_bottom = 0.5
	_panel.offset_left = -160
	_panel.offset_right = 160
	_panel.offset_top = -180
	_panel.offset_bottom = 180
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.06, 0.06, 0.92)
	style.border_width_left = 2
	style.border_width_top = 2
	style.border_width_right = 2
	style.border_width_bottom = 2
	style.border_color = Color(0.5, 0.42, 0.15)
	style.set_corner_radius_all(6)
	_panel.add_theme_stylebox_override("panel", style)
	add_child(_panel)

	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 6)
	_panel.add_child(vbox)

	# Title
	var title = Label.new()
	title.text = "🛒 General Store"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 16)
	title.add_theme_color_override("font_color", Color(0.9, 0.85, 0.5))
	vbox.add_child(title)

	# Gold display
	_gold_label = Label.new()
	_gold_label.text = "Gold: 0"
	_gold_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_gold_label.add_theme_font_size_override("font_size", 13)
	_gold_label.add_theme_color_override("font_color", Color(0.85, 0.75, 0.20))
	vbox.add_child(_gold_label)

	# Item list
	_item_list = ItemList.new()
	_item_list.custom_minimum_size = Vector2(300, 260)
	_item_list.add_theme_font_size_override("font_size", 12)
	vbox.add_child(_item_list)

	# Buttons row
	var btn_row = HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 10)

	_buy_btn = Button.new()
	_buy_btn.text = "Buy 1"
	_buy_btn.custom_minimum_size = Vector2(80, 30)
	_buy_btn.pressed.connect(_on_buy)
	btn_row.add_child(_buy_btn)

	_sell_btn = Button.new()
	_sell_btn.text = "Sell 1"
	_sell_btn.custom_minimum_size = Vector2(80, 30)
	_sell_btn.pressed.connect(_on_sell)
	btn_row.add_child(_sell_btn)

	_close_btn = Button.new()
	_close_btn.text = "Close"
	_close_btn.custom_minimum_size = Vector2(60, 30)
	_close_btn.pressed.connect(_on_close)
	btn_row.add_child(_close_btn)

	vbox.add_child(btn_row)

	# Item selection signal
	_item_list.item_selected.connect(_on_item_selected)

func open_shop(items: Array, gold: int) -> void:
	_items = items
	_player_gold = gold
	_gold_label.text = "Gold: %d" % gold
	_item_list.clear()
	for item in items:
		var name = ITEM_NAMES.get(item.id, "Item #%d" % item.id)
		var qty_str = "∞" if item.qty == 255 else str(item.qty)
		_item_list.add_item("%s  (Buy:%dg  Sell:%dg)  Stock: %s" % [name, item.buy, item.sell, qty_str])
		_item_list.set_item_metadata(_item_list.item_count - 1, item)
	visible = true

func update_gold(gold: int) -> void:
	_player_gold = gold
	_gold_label.text = "Gold: %d" % gold

func close_shop() -> void:
	visible = false

func _on_item_selected(index: int) -> void:
	if _item_list.item_count == 0:
		return
	var item = _item_list.get_item_metadata(index)

func _on_buy() -> void:
	var selected = _item_list.get_selected_items()
	if selected.is_empty():
		return
	var item = _item_list.get_item_metadata(selected[0])
	if item == null:
		return
	if _player_gold < item.buy:
		return
	buy_requested.emit(item.id)

func _on_sell() -> void:
	# Sell requires knowing inventory — handled by Main.gd
	sell_requested.emit(0)

func _on_close() -> void:
	close_shop()
