extends Node

## Network client — connects to Erynfall server.
## Handles binary protocol: packet framing, parsing, dispatching.

signal connected()
signal disconnected()
signal system_message(msg: String)
signal position_update(x: int, y: int)
signal inventory_sync(slots: Array)  # Array of {id, qty}
signal skill_update(skill_id: int, level: int, xp: int)
# For single-skill updates (6 bytes) vs full sync (115 bytes)
signal skills_sync(skills: Array)  # Array of {id, level, xp}
signal animation(anim_id: int, target_x: int, target_y: int)
signal npc_update(npc_id: int, npc_type: int, x: int, y: int, hp: int, max_hp: int, alive: bool)
signal tree_added(obj_id: int, tree_type: int, x: int, y: int)
signal tree_removed(obj_id: int)
signal equipment_sync(slots: Array)  # Phase 4: Array of {id, qty}
signal gold_update(gold: int)         # Phase 4: player gold balance
signal health_update(current_hp: int, max_hp: int)  # Phase 4: HP
signal shop_open(items: Array)        # Phase 4: shop stock

var _socket := StreamPeerTCP.new()
var _connected := false
var _recv_buffer := PackedByteArray()
var _server_host := "127.0.0.1"
var _server_port := 43594

func _ready() -> void:
	_connect_to_server()

func _process(_delta: float) -> void:
	if not _connected:
		return
	
	_socket.poll()
	var available = _socket.get_available_bytes()
	if available <= 0:
		return
	
	var chunk = _socket.get_data(available)
	if chunk[0] != OK:
		return
	
	var data = chunk[1]
	_recv_buffer.append_array(data)
	_parse_packets()

func _connect_to_server() -> void:
	print("[Network] Connecting to %s:%d..." % [_server_host, _server_port])
	var err = _socket.connect_to_host(_server_host, _server_port)
	if err != OK:
		print("[Network] Failed to connect")
		return
	
	# Wait briefly for connection
	var timeout_ms := 3000
	var start := Time.get_ticks_msec()
	while _socket.get_status() != StreamPeerTCP.STATUS_CONNECTED:
		_socket.poll()
		if Time.get_ticks_msec() - start > timeout_ms:
			print("[Network] Connection timed out")
			return
		OS.delay_msec(10)
	
	_connected = true
	print("[Network] Connected to server!")
	connected.emit()

func disconnect_from_server() -> void:
	_connected = false
	_socket.disconnect_from_host()
	disconnected.emit()

func send_walk_tile(x: int, y: int) -> void:
	_send_packet(0x02, PackedByteArray([x, y]))

func send_action_object(action_type: int, x: int, y: int) -> void:
	_send_packet(0x03, PackedByteArray([action_type, x, y]))

func send_action_npc(action_type: int, x: int, y: int) -> void:
	_send_packet(0x04, PackedByteArray([action_type, x, y]))

func send_action_ground_item(x: int, y: int) -> void:
	_send_packet(0x05, PackedByteArray([0, x, y]))

func send_keepalive() -> void:
	_send_packet(0x09, PackedByteArray())

func send_inventory_action(action: int, slot: int) -> void:
	_send_packet(0x06, PackedByteArray([action, slot]))

func send_shop_action(shop_action: int, item_id: int) -> void:
	_send_packet(0x0A, PackedByteArray([shop_action, item_id & 0xFF, (item_id >> 8) & 0xFF]))

func _send_packet(opcode: int, payload: PackedByteArray) -> void:
	if not _connected:
		return
	var pkt_len := 1 + payload.size()  # opcode + payload
	var packet := PackedByteArray()
	packet.append(pkt_len)  # length prefix (1 byte)
	packet.append(opcode)
	packet.append_array(payload)
	_socket.put_data(packet)

func _parse_packets() -> void:
	while _recv_buffer.size() >= 2:
		var pkt_len := _recv_buffer[0]
		if _recv_buffer.size() < 1 + pkt_len:
			return  # Incomplete
		
		var opcode := _recv_buffer[1]
		var payload := _recv_buffer.slice(2, 1 + pkt_len)
		
		_handle_packet(opcode, payload)
		_recv_buffer = _recv_buffer.slice(1 + pkt_len)

func _handle_packet(opcode: int, payload: PackedByteArray) -> void:
	match opcode:
		0x81:  # LoginResponse
			pass  # Not used yet
		
		0x83:  # PlayerUpdate
			if payload.size() >= 2:
				position_update.emit(payload[0], payload[1])
		
		0x84:  # InventorySync
			_parse_inventory(payload)
		
		0x86:  # SkillUpdate
			if payload.size() == 6:
				# Single skill update
				var skill_id := payload[0]
				var level := payload[1]
				var xp := payload[2] | (payload[3] << 8) | (payload[4] << 16) | (payload[5] << 24)
				skill_update.emit(skill_id, level, xp)
			elif payload.size() == 115:
				# Full skills sync (23 × 5 bytes)
				_parse_skills_full(payload)
		
		0x87:  # Animation
			if payload.size() >= 5:
				var anim_id := payload[0]
				var tx := payload[1] | (payload[2] << 8)
				var ty := payload[3] | (payload[4] << 8)
				animation.emit(anim_id, tx, ty)
		
		0x88:  # GroundItemAdd (repurposed for tree sync)
			if payload.size() >= 7:
				var action := payload[0]  # 0=remove, 1=add
				var obj_id := payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24)
				var tree_type := payload[5]
				# For now we don't have tile coords in tree sync — need to add
				# The tree positions are hardcoded on client to match server
				if action == 1:
					tree_added.emit(obj_id, tree_type, 0, 0)
				else:
					tree_removed.emit(obj_id)
		
		0x90:  # SystemMessage
			if payload.size() >= 2:
				var msg_len := payload[0] | (payload[1] << 8)
				if payload.size() >= 2 + msg_len:
					var msg := payload.slice(2, 2 + msg_len).get_string_from_utf8()
					system_message.emit(msg)
		
		0x93:  # NPCUpdate
			if payload.size() >= 13:
				var npc_type := payload[0]
				var npc_id := payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24)
				var nx := payload[5] | (payload[6] << 8)
				var ny := payload[7] | (payload[8] << 8)
				var hp := payload[9] | (payload[10] << 8)
				var max_hp := payload[11]
				var alive := payload[12] == 1
				npc_update.emit(npc_id, npc_type, nx, ny, hp, max_hp, alive)

		0x85:  # EquipmentSync (Phase 4)
			_parse_equipment(payload)

		0x96:  # ShopOpen (Phase 4)
			_parse_shop(payload)

		0x97:  # GoldUpdate (Phase 4)
			if payload.size() >= 4:
				var gold := payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24)
				gold_update.emit(gold)

		0x98:  # HealthUpdate (Phase 4)
			if payload.size() >= 2:
				health_update.emit(payload[0], payload[1])
		
		0x94:  # KeepAliveResponse
			pass
		
		_:
			pass  # Unknown opcode

func _parse_inventory(payload: PackedByteArray) -> void:
	var slots := []
	for i in range(28):
		var offset := i * 4
		if payload.size() < offset + 4:
			slots.append({"id": 0, "qty": 0})
			continue
		var item_id := payload[offset] | (payload[offset + 1] << 8)
		var qty := payload[offset + 2] | (payload[offset + 3] << 8)
		slots.append({"id": item_id, "qty": qty})
	inventory_sync.emit(slots)

func _parse_skills_full(payload: PackedByteArray) -> void:
	var skills := []
	for i in range(23):
		var offset := i * 5
		if payload.size() < offset + 5:
			break
		var skill_id := i
		var level := payload[offset]
		var xp := payload[offset + 1] | (payload[offset + 2] << 8) | (payload[offset + 3] << 16) | (payload[offset + 4] << 24)
		skills.append({"id": skill_id, "level": level, "xp": xp})
	skills_sync.emit(skills)

func _parse_equipment(payload: PackedByteArray) -> void:
	var slots := []
	for i in range(10):
		var offset := i * 4
		if payload.size() < offset + 4:
			slots.append({"id": 0, "qty": 0})
			continue
		var item_id := payload[offset] | (payload[offset + 1] << 8)
		var qty := payload[offset + 2] | (payload[offset + 3] << 8)
		slots.append({"id": item_id, "qty": qty})
	equipment_sync.emit(slots)

func _parse_shop(payload: PackedByteArray) -> void:
	if payload.size() < 1:
		return
	var count := payload[0]
	var items := []
	var offset := 1
	for i in range(count):
		if payload.size() < offset + 7:
			break
		var item_id := payload[offset] | (payload[offset + 1] << 8)
		var buy_price := payload[offset + 2] | (payload[offset + 3] << 8)
		var sell_price := payload[offset + 4] | (payload[offset + 5] << 8)
		var qty := payload[offset + 6]  # 0xFF = infinite
		items.append({"id": item_id, "buy": buy_price, "sell": sell_price, "qty": qty})
		offset += 7
	shop_open.emit(items)
