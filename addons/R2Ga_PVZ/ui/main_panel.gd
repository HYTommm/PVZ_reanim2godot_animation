@tool
extends PanelContainer
class_name MyPanelMainContainer

const MAX_HISTORY = 5
## R2Ga插件存档
const CONFIG_PATH = "user://R2Ga_save_data.cfg"

## 必选参数
## 输入文件路径
@onready var file_path_option = %file_path_option
@onready var file_select_button = %file_select_button
## 输出文件路径
@onready var anim_path_option = %anim_path_option
@onready var anim_select_button = %anim_select_button
## 素材文件夹路径
@onready var asset_path_option = %asset_path_option
@onready var asset_select_button = %asset_select_button
## 运行按钮
@onready var run_button = %run_button

## 可选参数
## 配置文件参数
@onready var config_file_path_option: OptionButton = %config_file_path_option
@onready var config_file_select_button: Button = %config_file_select_button
## 输出模式（auto\tscn_by_anim\anim_tres）
@onready var output_mode_option_button: OptionButton = %OutputModeOptionButton
## 插值模式（linear\nearest\cubic）
@onready var interpolation_mode_option_button: OptionButton = %InterpolationModeOptionButton
## 是否开启混合模式
@onready var blend_mode_check_button: CheckButton = %BlendModeCheckButton
## 帧模式（inherit\keyframe）
@onready var frame_mode_option_button: OptionButton = %FrameModeOptionButton
## 轨道模式（separate\transform）
@onready var track_mode_option_button: OptionButton = %TrackModeOptionButton

## 输出模式
var output_mode_id :int= 0
## 插值模式
var interpolation_mode_id :int= 0
## 混合模式
var is_blend_mode :bool= false
## 帧模式
var frame_mode_id :int= 0
## 轨道模式
var track_mode_id :int= 0

var plugin_interface: EditorPlugin

## R2Ga可执行文件路径
var exe_res_path = "res://addons/R2Ga_PVZ/PVZ_reanim2godot_animation_v4.1_714.exe"


func _ready():
	_load_history_list("file_path", file_path_option)
	_load_history_list("anim_path", anim_path_option)
	_load_history_list("asset_path", asset_path_option)
	_load_history_list("config_path", config_file_path_option)

	file_select_button.pressed.connect(_on_select_file)
	anim_select_button.pressed.connect(_on_select_anim_folder)
	asset_select_button.pressed.connect(_on_select_asset_folder)
	config_file_select_button.pressed.connect(_on_select_config_file)
	run_button.pressed.connect(_on_run_exe)

	## 初始化可选参数
	init_choose_para()


#region 选择 输入文件、输出目录、资产目录、配置文件
## 当选择输入文件
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

## 当选择输出文件夹
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

## 当选择资产（素材）文件夹
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

## 当选择配置文件
func _on_select_config_file():
	var dialog = FileDialog.new()
	dialog.access = FileDialog.ACCESS_FILESYSTEM
	dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	dialog.filters = ["*.txt"]
	# 从file_path_option获取路径
	var full_path = config_file_path_option.text
	dialog.current_path = full_path

	dialog.file_selected.connect(func(path):
		_add_to_history("config_path", path, config_file_path_option)
	)
	add_child(dialog)
	dialog.popup_centered()

## 添加存档
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

#endregion

#region 初始化可选参数
## 初始化选参数，若存档有数据，则更新，每次运行程序时更新数据
func init_choose_para():
	var cfg = ConfigFile.new()
	cfg.load(CONFIG_PATH)
	## 输出模式
	output_mode_id = cfg.get_value("choose_para", "output_mode", output_mode_id)
	## 插值模式
	interpolation_mode_id = cfg.get_value("choose_para", "interpolation_mode", interpolation_mode_id)
	## 混合模式
	is_blend_mode = cfg.get_value("choose_para", "is_blend_mode", is_blend_mode)
	## 帧模式
	frame_mode_id = cfg.get_value("choose_para", "frame_mode", frame_mode_id)
	## 轨道模式
	track_mode_id = cfg.get_value("choose_para", "track_mode", track_mode_id)

	output_mode_option_button.select(output_mode_id)
	interpolation_mode_option_button.select(interpolation_mode_id)
	blend_mode_check_button.button_pressed = is_blend_mode
	frame_mode_option_button.select(frame_mode_id)
	track_mode_option_button.select(track_mode_id)

	#print("可选参数：")
	#print("输出模式：", output_mode_option_button.get_item_text(output_mode_id))
	#print("插值模式：", interpolation_mode_option_button.get_item_text(interpolation_mode_id))
	#print("混合模式：", blend_mode_check_button.button_pressed)


## 获取当前ui的可选参数
func get_choose_para():
	output_mode_id = output_mode_option_button.selected
	interpolation_mode_id = interpolation_mode_option_button.selected
	is_blend_mode = blend_mode_check_button.button_pressed
	frame_mode_id = frame_mode_option_button.selected
	track_mode_id = track_mode_option_button.selected

## 保存可选参数
func save_choose_para():
	var cfg = ConfigFile.new()
	cfg.load(CONFIG_PATH)
	# 保存该 key 的更新数据
	cfg.set_value("choose_para", "output_mode", output_mode_id)
	cfg.set_value("choose_para", "interpolation_mode", interpolation_mode_id)
	cfg.set_value("choose_para", "is_blend_mode", is_blend_mode)
	cfg.set_value("choose_para", "frame_mode", frame_mode_id)
	cfg.set_value("choose_para", "track_mode", track_mode_id)

	cfg.save(CONFIG_PATH)


#endregion


#region 运行程序
func _on_run_exe():

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

	## 获取ui的可选参数
	get_choose_para()

	var args = [file_arg, anim_arg, asset_arg,\
				"-of", ProjectSettings.globalize_path(anim_arg), \
				"-om", output_mode_option_button.get_item_text(output_mode_id), \
				"-im", interpolation_mode_option_button.get_item_text(interpolation_mode_id), \
				"-fm", frame_mode_option_button.get_item_text(frame_mode_id), \
				"-tm", track_mode_option_button.get_item_text(track_mode_id)
				]


	if config_file_path_option.selected >= 0 and not config_file_path_option.get_item_text(config_file_path_option.get_selected_id()).is_empty():
		args.append_array(["-cf", config_file_path_option.get_item_text(config_file_path_option.get_selected_id()).strip_edges()])
	if is_blend_mode:
		args.append("-bm")
	else:
		args.append("-nbm")

	var cmd_line = exe_path + " " + String(" ").join(args)  # 定义 cmd_line 变量

	print("运行命令：", cmd_line)

	var output = []
	var exit_code = OS.execute(exe_path, args, output)  # 参数4是数组，接收输出

	for line in output:
		print(line)

	if exit_code != 0:
		push_error("运行失败，错误码：" + str(exit_code))
	else:
		print("✅ 成功运行, 已存档当前可选参数")
		save_choose_para()

	## 更新全局文件
	if plugin_interface:
		plugin_interface.refresh_resources()
#endregion
