@tool
extends PanelContainer
class_name MyPanelMainContainer

const MAX_HISTORY = 5
const CONFIG_PATH = "user://selected_paths.cfg"

@onready var file_path_option = %file_path_option
@onready var file_select_button = %file_select_button
@onready var anim_path_option = %anim_path_option
@onready var anim_select_button = %anim_select_button
@onready var asset_path_option = %asset_path_option
@onready var asset_select_button = %asset_select_button
@onready var run_button = %run_button

var plugin_interface: EditorPlugin

func _ready():
	_load_history_list("file_path", file_path_option)
	_load_history_list("anim_path", anim_path_option)
	_load_history_list("asset_path", asset_path_option)

	file_select_button.pressed.connect(_on_select_file)
	anim_select_button.pressed.connect(_on_select_anim_folder)
	asset_select_button.pressed.connect(_on_select_asset_folder)
	run_button.pressed.connect(_on_run_exe)

func _on_select_file():
	var dialog = FileDialog.new()
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	dialog.filters = ["*.reanim"]
	# 从file_path_option获取路径
	var full_path = file_path_option.text
	dialog.current_path = full_path

	dialog.file_selected.connect(func(path):
		_add_to_history("file_path", path, file_path_option)
	)
	add_child(dialog)
	dialog.popup_centered()

func _on_select_anim_folder():
	var dialog = FileDialog.new()
	dialog.access = FileDialog.ACCESS_RESOURCES  
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_DIR
	dialog.dir_selected.connect(func(path):
		var fixed_path = path
		if not fixed_path.ends_with("/"):
			fixed_path += "/"
		_add_to_history("anim_path", fixed_path, anim_path_option)
	)
	add_child(dialog)
	dialog.popup_centered()

func _on_select_asset_folder():
	var dialog = FileDialog.new()
	dialog.access = FileDialog.ACCESS_RESOURCES  
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_DIR
	dialog.dir_selected.connect(func(path):
		var fixed_path = path
		if not fixed_path.ends_with("/"):
			fixed_path += "/"
		_add_to_history("asset_path", fixed_path, asset_path_option)
	)
	add_child(dialog)
	dialog.popup_centered()

func _add_to_history(key: String, path: String, option: OptionButton):
	var cfg = ConfigFile.new()
	cfg.load(CONFIG_PATH)

	var history := cfg.get_value("paths", key + "_history", [])
	

	# 移除重复项
	if path in history:
		history.erase(path)
	# 添加新路径到最前
	history.insert(0, path)
	# 保留最多 MAX_HISTORY 条
	history = history.slice(0, MAX_HISTORY)

	# 保存该 key 的更新数据
	cfg.set_value("paths", key + "_history", history)
	cfg.save(CONFIG_PATH)

	# 刷新 UI 下拉列表
	option.clear()
	for item in history:
		option.add_item(item)

	option.tooltip_text = history[0]
	option.select(0)

func _load_history_list(key: String, option: OptionButton):
	var cfg = ConfigFile.new()
	var err = cfg.load(CONFIG_PATH)
	if err != OK:
		return

	var history := cfg.get_value("paths", key + "_history", [])

	# 只保留最多 MAX_HISTORY 条
	history = history.slice(0, MAX_HISTORY)

	option.clear()
	for item in history:
		option.add_item(item)
	if history.size() > 0:
		option.select(0)
		option.tooltip_text = history[0]

func _on_run_exe():
	var exe_res_path = "res://addons/R2Ga_PVZ/PVZ_reanim2godot_animation_x86_x64.exe"
	var exe_path = ProjectSettings.globalize_path(exe_res_path)

	if not FileAccess.file_exists(exe_path):
		push_error("EXE 文件不存在：" + exe_path)
		return

	var file_arg = file_path_option.get_item_text(file_path_option.get_selected_id()).strip_edges()
	var anim_arg = anim_path_option.get_item_text(anim_path_option.get_selected_id()).strip_edges()
	var asset_arg = asset_path_option.get_item_text(asset_path_option.get_selected_id()).strip_edges()

	if file_arg == "" or anim_arg == "" or asset_arg == "":
		push_error("请填写完整的参数路径")
		return

	var args = [file_arg, anim_arg, asset_arg, "auto"]
	var cmd_line = exe_path + " " + String(" ").join(args)  # 定义 cmd_line 变量

	print("运行命令：", cmd_line)

	var output = []
	var exit_code = OS.execute(exe_path, args, output)  # 参数4是数组，接收输出
	
	for line in output:
		print(line)
		
	if exit_code != 0:
		push_error("运行失败，错误码：" + str(exit_code))
	else:
		print("✅ 成功运行")

	## 更新全局文件
	if plugin_interface:
		plugin_interface.refresh_resources()
