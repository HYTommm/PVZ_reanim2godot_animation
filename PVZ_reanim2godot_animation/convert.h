#pragma once
#include <stdbool.h>
#include "PvzReanim.h"




int FPS;

typedef struct Key
{
	float times[MAX_TIMES_NUM];
	float transitions[MAX_TIMES_NUM];
	UpdateMode update;
	char values[MAX_TIMES_NUM][NAME_LENTH];
}Key;

typedef struct Tracks
{
	int num;
	char type[NAME_LENTH];
	bool imported;
	bool enabled;
	char path[PATH_LENTH];
	int interp;//插值模式
	bool loop_wrap;
	Key key;

}Tracks;

typedef struct PVZTracks
{
	char name[NAME_LENTH];
	float length;
	Tracks* tracks_vis;
	Tracks* tracks_pos;
	Tracks* tracks_rot;
	Tracks* tracks_scale;
	Tracks* tracks_skew;
	Tracks* tracks_texture;
	Tracks* tracks_alpha;
	Tracks* tracks_blendmode;
	
}PVZTracks;

typedef struct PVZAnimation
{
	char anim_name[NAME_LENTH];
	int start_frame_time;
	int end_frame_time;
	int all_tracks_num;
	PVZTracks* tracks;
	int current_frame_time_num;
	int current_tracks_num              ;
	int current_tracks_vis_key_times    ;
	int current_tracks_pos_key_times    ;
	int current_tracks_rot_key_times    ;
	int current_tracks_scale_key_times  ;
	int current_tracks_skew_key_times   ;
	int current_tracks_texture_key_times;
	int current_tracks_alpha_key_times;
	int current_tracks_blendmode_key_times;
	bool flag_x;
	bool flag_sx;
	bool flag_kx;
	bool flag_kx2;
	bool flag_ky;
	bool flag_ky2;
	bool tap_is_t;
	/// <summary>输出后缀</summary>
	char output_file_extension[EXT_LENTH];

	/// <summary>输出文件名</summary>
	char str_output[NAME_LENTH];

	/// <summary>第一个输出文件名（导入）</summary>
	char str_first_output_ext[NAME_LENTH];

	/// <summary>第二个输出文件名（动画）</summary>
	char str_second_output_anim[NAME_LENTH];

	/// <summary>第三个输出文件名（轨道）</summary>
	char str_third_output_track[NAME_LENTH];

	/// <summary>第四个输出文件名（节点）</summary>
	char str_forth_output_node[NAME_LENTH];

	/// <summary>输出文件指针</summary>
	FILE* fp_output;

	/// <summary>第一个输出文件指针（导入）</summary>
	FILE* fp_first_output_ext;

	/// <summary>第二个输出文件指针（动画）</summary>
	FILE* fp_second_output_anim;

	/// <summary>第三个输出文件指针（轨道）</summary>
	FILE* fp_third_output_track;

	/// <summary>第四个输出文件指针（节点）</summary>
	FILE* fp_forth_output_node;
	
	char filename_fuck[MAX_TEXTURE_NUM][NAME_LENTH];
	int  filename_fuck_times;
	char track_name[MAX_TRACKS_NUM][NAME_LENTH];
	char ResName[NAME_LENTH];
}PVZAnimation;

void InitTracks(Tracks* track, R2GAStartParam* start_param);
void InitPVZTracks(PVZTracks* pvz_track, R2GAStartParam* start_param);
void InitPVZAnimation(PVZAnimation* pvz_animation, const char* name, R2GAStartParam* start_param);
void OpenOutputFiles(PVZAnimation* pvz_animation, const char* output_file_path,const char* output_file);
void FreePVZAnimation(PVZAnimation* pvz_animation, bool is_remove_output_files);

