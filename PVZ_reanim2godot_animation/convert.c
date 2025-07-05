#include <malloc.h>
#include <stdio.h>
#include <ctype.h>

#include <string.h>
#include <stdlib.h>
#include "FileIO.h"
#include "convert.h"
#include "PvzReanim.h"

void InitTracks(Tracks* track, R2GAStartParam* start_param)
{

	//Init num
	track->num = 0;

	//Init type
	for (int i = 0; i < NAME_LENTH; i++)
	{
		track->type[i] = '\0';
	}
	sprintf_s(track->type, NAME_LENTH, "value");

	//Init imported
	track->imported = false;

	//Init enabled
	track->enabled = true;

	//Init path
	for (int i = 0; i < PATH_LENTH; i++)
	{
		track->path[i] = '\0';
	}

	//Init interp
	track->interp = start_param->interpolationMode;

	//Init loop_wrap
	track->loop_wrap = true;

	//Init key
	for (int i = 0; i < MAX_TIMES_NUM; i++)
	{
		track->key.times[i] = 0;
		track->key.transitions[i] = 1.0;
		for (int j = 0; j < NAME_LENTH; j++)
		{
			track->key.values[i][j] = '\0';
		}
	}
	track->key.update = start_param->updateMode;
}

void InitPVZTracks(PVZTracks* pvz_track, R2GAStartParam* start_param)
{
	//Init anim_name
	for (int i = 0; i < NAME_LENTH; i++)
	{
		pvz_track->name[i] = '\0';
	}
	//Init length
	pvz_track->length = 0;
	//Init tracks_vis, tracks_pos, tracks_rot, tracks_scale, tracks_skew, tracks_texture
	if ((pvz_track->tracks_vis = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_pos = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_rot = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_scale = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_skew = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_texture = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_alpha = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	if ((pvz_track->tracks_blendmode = (Tracks*)malloc(sizeof(Tracks))) == NULL) return;
	InitTracks(pvz_track->tracks_vis, start_param);
	pvz_track->tracks_vis->key.update = UPDATE_MODE_CONTINUOUS;
	InitTracks(pvz_track->tracks_pos, start_param);
	InitTracks(pvz_track->tracks_rot, start_param);
	InitTracks(pvz_track->tracks_scale, start_param);
	InitTracks(pvz_track->tracks_skew, start_param);
	InitTracks(pvz_track->tracks_texture, start_param);
	InitTracks(pvz_track->tracks_alpha, start_param);
	InitTracks(pvz_track->tracks_blendmode, start_param);
}

void InitPVZAnimation(PVZAnimation* pvz_animation, const char* name, R2GAStartParam* start_param)
{
	//Init anim_name
	sprintf_s(pvz_animation->anim_name, NAME_LENTH, "%s", name);
	// Init start_frame and end_frame
	pvz_animation->start_frame_time = 0;
	pvz_animation->end_frame_time = 0;
	//Init all_tracks_num
	pvz_animation->all_tracks_num = 0;
	//Init tracks
	if ((pvz_animation->tracks = (PVZTracks*)malloc(sizeof(PVZTracks))) == NULL) return;
		InitPVZTracks(pvz_animation->tracks, start_param);
	// EN:Init now_time_num, now_tracks_num, now_tracks_vis_key_times, now_tracks_pos_key_times, now_tracks_rot_key_times, now_tracks_scale_key_times, now_tracks_skew_key_times, now_tracks_texture_key_times
	pvz_animation->current_frame_time_num = 0;
	pvz_animation->current_tracks_num = 0;
	pvz_animation->current_tracks_vis_key_times = 0;
	pvz_animation->current_tracks_pos_key_times = 0;
	pvz_animation->current_tracks_rot_key_times = 0;
	pvz_animation->current_tracks_scale_key_times = 0;
	pvz_animation->current_tracks_skew_key_times = 0;
	pvz_animation->current_tracks_texture_key_times = 0;
	pvz_animation->current_tracks_alpha_key_times = 0;
	pvz_animation->current_tracks_blendmode_key_times = 0;

	pvz_animation->flag_x = false;
	pvz_animation->flag_sx = false;
	pvz_animation->flag_kx = false;
	pvz_animation->flag_kx2 = false;
	pvz_animation->flag_ky = false;
	pvz_animation->flag_ky2 = false;
	pvz_animation->tap_is_t = false;

	pvz_animation->fp_output = NULL;
	pvz_animation->fp_first_output_ext = NULL;
	pvz_animation->fp_second_output_anim = NULL;
	pvz_animation->fp_third_output_track = NULL;
	pvz_animation->fp_forth_output_node = NULL;

	sprintf_s(pvz_animation->output_file_extension, EXT_LENTH, "tscn");

	pvz_animation->filename_fuck_times = 0;

}

void OpenOutputFiles(PVZAnimation* pvz_animation,const char* output_file_path,const char* output_file_name)
{
	sprintf_s(pvz_animation->str_output, NAME_LENTH, "%s%s.%s",output_file_path, output_file_name, pvz_animation->output_file_extension);
	printf("debug: OutputFiles:str_output--%s\n", pvz_animation->str_output);
	// 将argv[2]和"first"写入str_first_output_ext
	sprintf_s(pvz_animation->str_first_output_ext, NAME_LENTH, "%s%s_first_ext.jiema", output_file_path, output_file_name);
	// 将argv[2]和"second"写入str_second_output_anim					  
	sprintf_s(pvz_animation->str_second_output_anim, NAME_LENTH, "%s%s_second_anim.jiema", output_file_path, output_file_name);
	// 将argv[2]和"third"写入str_third_output_track						  
	sprintf_s(pvz_animation->str_third_output_track, NAME_LENTH, "%s%s_third_track.jiema", output_file_path, output_file_name);
	// 将argv[2]和"forth"写入str_forth_output_node						  
	sprintf_s(pvz_animation->str_forth_output_node, NAME_LENTH, "%s%s_forth_node.jiema", output_file_path, output_file_name);

	// 打开输出文件
	FileOpen(&(pvz_animation->fp_output), pvz_animation->str_output, "w", ErrorCode_CannotOpenOutputFile);
	// 打开ext输出文件
	FileOpen(&(pvz_animation->fp_first_output_ext), pvz_animation->str_first_output_ext, "w+", ErrorCode_CannotOpenExtOutputFile);
	// 打开anim输出文件
	FileOpen(&(pvz_animation->fp_second_output_anim), pvz_animation->str_second_output_anim, "w+", ErrorCode_CannotOpenAnimOutputFile);
	// 打开track输出文件
	FileOpen(&(pvz_animation->fp_third_output_track), pvz_animation->str_third_output_track, "w+", ErrorCode_CannotOpenTrackOutputFile);
	// 打开node输出文件
	FileOpen(&(pvz_animation->fp_forth_output_node), pvz_animation->str_forth_output_node, "w+", ErrorCode_CannotOpenNodeOutputFile);
}

// 释放PVZAnimation
void FreePVZAnimation(PVZAnimation* pvz_animation, bool is_remove_output_files)
{
	if (pvz_animation == NULL) return;
	if (strcmp(pvz_animation->anim_name, "null") != 0)
	{
		//printf("debug: FreePVZAnimation--%s\n", pvz_animation->anim_name);
	}
		
	// 关闭输出文件
	FileClose(pvz_animation->fp_output);
	FileClose(pvz_animation->fp_first_output_ext);
	FileClose(pvz_animation->fp_second_output_anim);
	FileClose(pvz_animation->fp_third_output_track);
	FileClose(pvz_animation->fp_forth_output_node);
	// 释放PVZTracks
	free(pvz_animation->tracks->tracks_vis);
	free(pvz_animation->tracks->tracks_pos);
	free(pvz_animation->tracks->tracks_rot);
	free(pvz_animation->tracks->tracks_scale);
	free(pvz_animation->tracks->tracks_skew);
	free(pvz_animation->tracks->tracks_texture);
	free(pvz_animation->tracks->tracks_alpha);
	free(pvz_animation->tracks->tracks_blendmode);
	free(pvz_animation->tracks);
	

	// 删除输出文件
	if (is_remove_output_files)
	{
		remove(pvz_animation->str_output);
	}
	remove(pvz_animation->str_first_output_ext);
	remove(pvz_animation->str_second_output_anim);
	remove(pvz_animation->str_third_output_track);
	remove(pvz_animation->str_forth_output_node);
	free(pvz_animation);
}


