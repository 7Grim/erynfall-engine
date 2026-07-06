extends CanvasLayer

## Combat overlay — shows player HP bar, combat state, hit messages.

var _hp_bar: ProgressBar
var _hp_label: Label
var _combat_label: Label
var _hit_container: VBoxContainer

var _max_hp: int = 10
var _current_hp: int = 10
var _hit_messages: Array[String] = []
var _hit_timers: Array[float] = []

func _ready() -> void:
	# Children are added after this script is set — use deferred init
	pass

func _deferred_init() -> void:
	_hp_bar = get_node_or_null("HPPanel/HPBar") as ProgressBar
	_hp_label = get_node_or_null("HPBar/HPLabel") as Label
	_combat_label = get_node_or_null("CombatLabel") as Label
	_hit_container = get_node_or_null("HitContainer") as VBoxContainer
	if _hp_bar:
		_hp_bar.max_value = _max_hp
		_hp_bar.value = _current_hp
	if _hp_label:
		_hp_label.text = "%d/%d" % [_current_hp, _max_hp]
	if _combat_label:
		_combat_label.visible = false
	if _hit_container:
		_hit_container.visible = false

func _process(delta: float) -> void:
	if not _hp_bar:
		_deferred_init()
		return
	var i := _hit_timers.size() - 1
	while i >= 0:
		_hit_timers[i] -= delta
		if _hit_timers[i] <= 0:
			_hit_timers.remove_at(i)
			_hit_messages.remove_at(i)
			_refresh_hits()
		i -= 1

func update_hp(current: int, maximum: int) -> void:
	_current_hp = current
	_max_hp = max(maximum, 1)
	if not _hp_bar:
		return
	_hp_bar.max_value = _max_hp
	_hp_bar.value = _current_hp
	if _hp_label:
		_hp_label.text = "%d/%d" % [_current_hp, _max_hp]
	var ratio := float(_current_hp) / float(_max_hp)
	if ratio < 0.3:
		_hp_bar.modulate = Color.RED
	elif ratio < 0.6:
		_hp_bar.modulate = Color.YELLOW
	else:
		_hp_bar.modulate = Color.GREEN

func show_combat(target_name: String) -> void:
	if _combat_label:
		_combat_label.text = "Fighting: %s" % target_name
		_combat_label.visible = true

func hide_combat() -> void:
	if _combat_label:
		_combat_label.visible = false

func add_hit_message(msg: String, duration: float = 3.0) -> void:
	_hit_messages.append(msg)
	_hit_timers.append(duration)
	_refresh_hits()

func _refresh_hits() -> void:
	if not _hit_container:
		return
	for child in _hit_container.get_children():
		child.queue_free()
	_hit_container.visible = _hit_messages.size() > 0
	for msg in _hit_messages:
		var label := Label.new()
		label.text = msg
		label.add_theme_font_size_override("font_size", 16)
		_hit_container.add_child(label)
