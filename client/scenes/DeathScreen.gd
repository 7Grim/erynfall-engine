extends CanvasLayer

## Death screen overlay — "Oh dear, you are dead!" message with respawn countdown.

var _panel: PanelContainer
var _label: Label
var _timer_label: Label

var _active := false
var _countdown := 5.0

func _ready() -> void:
	pass  # Children added after script set

func _deferred_init() -> void:
	_panel = get_node_or_null("Panel") as PanelContainer
	_label = get_node_or_null("Panel/Label") as Label
	_timer_label = get_node_or_null("Panel/TimerLabel") as Label

func show_death() -> void:
	if not _panel:
		_deferred_init()
	_active = true
	_countdown = 5.0
	if _label:
		_label.text = "Oh dear, you are dead!"
	if _timer_label:
		_timer_label.text = "Respawning in %d..." % ceil(_countdown)
	visible = true

func hide_death() -> void:
	_active = false
	visible = false

func _process(delta: float) -> void:
	if not _active:
		return
	_countdown -= delta
	if _countdown <= 0:
		hide_death()
		return
	if _timer_label:
		_timer_label.text = "Respawning in %d..." % ceil(_countdown)
