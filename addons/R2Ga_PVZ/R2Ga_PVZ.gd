@tool
extends EditorPlugin

const MY_PANEL_MAIN_PANEL = preload("res://addons/R2Ga_PVZ/ui/main_panel.tscn")
var R2Ga_panel : MyPanelMainContainer


func _enter_tree():
	R2Ga_panel = MY_PANEL_MAIN_PANEL.instantiate()
	R2Ga_panel.plugin_interface = self
	add_control_to_bottom_panel(R2Ga_panel, "R2Ga_PVZ")

func _exit_tree():
	remove_control_from_bottom_panel(R2Ga_panel)

func refresh_resources():
	get_editor_interface().get_resource_filesystem().scan()
