extends CanvasLayer

## Level-up notification — floating text that fades out.

var _timer := 0.0

func _ready() -> void:
	# Label child is added by Main.gd after this node is in tree
	# We'll find it dynamically
	pass

func _get_label() -> Label:
	var label = get_node_or_null("Label") as Label
	if not label:
		# Create it if missing
		label = Label.new()
		label.name = "Label"
		label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		label.position = Vector2(400, 200)
		label.size = Vector2(300, 60)
		label.add_theme_font_size_override("font_size", 18)
		label.add_theme_color_override("font_color", Color(1, 0.85, 0.2))
		add_child(label)
	return label

func show_level_up(skill_name: String, new_level: int) -> void:
	var label = _get_label()
	label.text = "Level up!\n%s is now %d!" % [skill_name, new_level]
	label.modulate.a = 1.0
	label.visible = true
	_timer = 4.0

func _process(delta: float) -> void:
	if _timer > 0:
		_timer -= delta
		var label = get_node_or_null("Label") as Label
		if label:
			if _timer <= 1.0:
				label.modulate.a = _timer
			if _timer <= 0:
				label.visible = false
