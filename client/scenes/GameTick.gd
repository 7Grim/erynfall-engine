extends Node

## Game tick — 600ms heartbeat matching the server.
## All OSRS-style gameplay (movement, skills, combat) ticks here.

signal tick()

const TICK_MS := 600.0

var _accumulator := 0.0

func _process(delta: float) -> void:
	_accumulator += delta * 1000.0
	var ticks_to_fire := int(_accumulator / TICK_MS)
	if ticks_to_fire > 0:
		_accumulator -= ticks_to_fire * TICK_MS
		for _i in range(ticks_to_fire):
			tick.emit()
