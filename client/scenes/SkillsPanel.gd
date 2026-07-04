extends PanelContainer

## Skills panel — shows current skill levels.
## Focuses on Woodcutting for Phase 2, expandable later.

const SKILL_NAMES := [
	"Attack", "Strength", "Defence", "Hitpoints", "Ranged", "Prayer",
	"Magic", "Cooking", "Woodcutting", "Fletching", "Fishing", "Firemaking",
	"Crafting", "Smithing", "Mining", "Herblore", "Agility", "Thieving",
	"Slayer", "Farming", "Runecrafting", "Hunter", "Construction",
]

const SKILL_COLORS := {
	0: Color(0.8, 0.2, 0.2),   # Attack — red
	1: Color(0.8, 0.6, 0.2),   # Strength — orange
	2: Color(0.2, 0.5, 0.8),   # Defence — blue
	3: Color(0.8, 0.2, 0.2),   # Hitpoints — red
	8: Color(0.2, 0.7, 0.2),   # Woodcutting — green (highlighted)
}

var _skill_labels: Dictionary = {}
var _skill_xp_bars: Dictionary = {}
var _total_label: Label

func _ready() -> void:
	_build_panel()
	anchor_right = 0.0
	anchor_bottom = 0.0
	anchor_left = 0.0
	anchor_top = 0.0
	grow_horizontal = Control.GROW_DIRECTION_BEGIN
	position = Vector2(10, 10)
	size = Vector2(180, 340)

func _build_panel() -> void:
	var vbox = VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 2)
	
	var title = Label.new()
	title.text = "Skills"
	title.add_theme_font_size_override("font_size", 14)
	title.add_theme_color_override("font_color", Color(0.9, 0.85, 0.6))
	vbox.add_child(title)
	
	_total_label = Label.new()
	_total_label.text = "Total: 10"
	_total_label.add_theme_font_size_override("font_size", 11)
	_total_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.7))
	vbox.add_child(_total_label)
	
	var separator = HSeparator.new()
	vbox.add_child(separator)
	
	# Show all 23 skills
	for i in range(23):
		var hbox = HBoxContainer.new()
		hbox.add_theme_constant_override("separation", 4)
		
		var name_label = Label.new()
		name_label.text = SKILL_NAMES[i]
		name_label.custom_minimum_size.x = 80
		var skill_color = SKILL_COLORS.get(i, Color(0.6, 0.6, 0.6))
		name_label.add_theme_color_override("font_color", skill_color)
		name_label.add_theme_font_size_override("font_size", 11)
		hbox.add_child(name_label)
		
		var level_label = Label.new()
		level_label.text = "1"
		level_label.custom_minimum_size.x = 25
		level_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		level_label.add_theme_color_override("font_color", Color(0.9, 0.9, 0.9))
		level_label.add_theme_font_size_override("font_size", 11)
		hbox.add_child(level_label)
		
		# XP bar (progress from current level to next)
		var progress = ProgressBar.new()
		progress.custom_minimum_size.x = 40
		progress.custom_minimum_size.y = 12
		progress.max_value = 100
		progress.value = 0
		progress.show_percentage = false
		var prog_style = StyleBoxFlat.new()
		prog_style.bg_color = Color(0.15, 0.15, 0.15)
		prog_style.set_corner_radius_all(2)
		progress.add_theme_stylebox_override("background", prog_style)
		var fill_style = StyleBoxFlat.new()
		fill_style.bg_color = skill_color
		fill_style.set_corner_radius_all(2)
		progress.add_theme_stylebox_override("fill", fill_style)
		hbox.add_child(progress)
		
		_skill_labels[i] = level_label
		_skill_xp_bars[i] = progress
		vbox.add_child(hbox)
	
	add_theme_stylebox_override("panel", _create_bg_style())
	add_child(vbox)

func update_skills(skills: Array) -> void:
	var total := 0
	for skill_data in skills:
		var id: int = skill_data.id
		var level: int = skill_data.level
		var xp: int = skill_data.xp
		total += level
		
		if _skill_labels.has(id):
			_skill_labels[id].text = str(level)
		if _skill_xp_bars.has(id):
			# Calculate XP progress toward next level
			var progress := _xp_progress(xp, level)
			_skill_xp_bars[id].value = progress
	
	_total_label.text = "Total: %d" % total

func update_skill(skill_id: int, level: int, xp: int) -> void:
	if _skill_labels.has(skill_id):
		_skill_labels[skill_id].text = str(level)
	if _skill_xp_bars.has(skill_id):
		var progress := _xp_progress(xp, level)
		_skill_xp_bars[skill_id].value = progress

func _xp_progress(xp: int, level: int) -> float:
	# Calculate percentage toward next level
	if level >= 99:
		return 100.0
	var current_level_xp := _xp_for_level(level)
	var next_level_xp := _xp_for_level(level + 1)
	if next_level_xp == current_level_xp:
		return 100.0
	var progress := float(xp - current_level_xp) / float(next_level_xp - current_level_xp)
	return clampf(progress * 100.0, 0.0, 100.0)

# OSRS XP formula (mirrors server xp_table.h)
func _xp_for_level(level: int) -> int:
	if level <= 1:
		return 0
	var total := 0.0
	for i in range(1, level):
		total += floor(float(i) + 300.0 * pow(2.0, float(i) / 7.0))
	return int(floor(total / 4.0))

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
