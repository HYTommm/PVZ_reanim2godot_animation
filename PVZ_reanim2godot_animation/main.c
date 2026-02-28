#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
//#include <windows.h>

#include "convert.h"
#include "StartParam.h"
#include "FileIO.h"
#include "main.h"
#include "ResourceFile.h"
//#include <malloc.h>

//转换器

int current_frame_time_num = 0;
int anim_nums = 0;
bool is_track_anim = false;
bool is_start_frame_time_select = false;
bool is_blend_mode_enabled = false;

int FPS;

//char input_file[NAME_LENGTH];
//char input_filePath[PATH_LENGTH];
//char output_file[NAME_LENGTH];
//char output_type[NAME_LENGTH];

/// <summary>
/// 通过文件内容判断是否应启用混合模式
/// </summary>
/// <param name="file_text">文件内容字符串</param>
/// <param name="start_param">启动参数</param>
void IsBlendModeEnabled(char* file_text, R2GAStartParam* start_param)
{
    // 遍历file_text，查找"<bm>"关键字，如果找到，则设置is_enabled为true
    char* p = strstr(file_text, "<bm>");
    if (p != NULL && start_param->blendModeTrackEnabledSpecified && start_param->blendModeTrackEnabled)
    {
        start_param->blendModeTrackEnabled = true;
    }
    else
    {
        start_param->blendModeTrackEnabled = false;
    }
}

void arrayprintf_s(char* str, int num, char* array)
{
    for (int i = 0; i < num; i++)
    {
        if (array[i] == '\0')
        {
            return;
        }
        str[i] = array[i];
    }
}

void SetAnimName(PvzAnimation* pvz_animation, char* new_content)
{
    sprintf_s(pvz_animation->anim_name, NAME_LENGTH, "%s", new_content);
}
void SetAnimStartFrameTime(PvzAnimation* pvz_animation, int frame_time_num)
{
    pvz_animation->start_frame_time = frame_time_num;
}
void SetAnimEndFrameTime(PvzAnimation* pvz_animation, int frame_time_num)
{
    pvz_animation->end_frame_time = frame_time_num;
}
void SetAnimResName(PvzAnimation* pvz_animation, char* new_content)
{
    sprintf_s(pvz_animation->res_file_name, NAME_LENGTH, "%s", new_content);
}

void SetFPS(const char* new_content)
{
    FPS = atoi(new_content);
}
void SetAnimKeyTimes(const PvzAnimation* anim, const int num)
{
    //anim->current_tracks_vis_key_times = num;
    //anim->current_tracks_pos_key_times = num;
    //anim->current_tracks_rot_key_times = num;
    //anim->current_tracks_scale_key_times = num;
    //anim->current_tracks_skew_key_times = num;
    //anim->current_tracks_texture_key_times = num;
    //anim->current_tracks_alpha_key_times = num;
    //anim->current_tracks_blendmode_key_times = num;
    const PvzTracks* tracks = anim->tracks;
    tracks->vis->keys.times_num = num;
    tracks->pos->keys.times_num = num;
    tracks->rot->keys.times_num = num;
    tracks->scale->keys.times_num = num;
    tracks->skew->keys.times_num = num;
    tracks->texture->keys.times_num = num;
    tracks->alpha->keys.times_num = num;
    tracks->blend_mode->keys.times_num = num;
}
void SetTrack(PvzAnimation* pvz_animation, char* new_content, const R2GAStartParam* start_param)
{
    static int track_num = 0;
    pvz_animation->current_frame_time_num = 0;
    //pvz_animation->tracks->vis->num = pvz_animation->current_tracks_num *       (7+(is_blend_mode_enabled ? 1 : 0)) + 0;
    //pvz_animation->tracks->pos->num = pvz_animation->current_tracks_num *       (7+(is_blend_mode_enabled ? 1 : 0)) + 1;
    //pvz_animation->tracks->rot->num = pvz_animation->current_tracks_num *       (7+(is_blend_mode_enabled ? 1 : 0)) + 2;
    //pvz_animation->tracks->scale->num = pvz_animation->current_tracks_num *     (7+(is_blend_mode_enabled ? 1 : 0)) + 3;
    //pvz_animation->tracks->skew->num = pvz_animation->current_tracks_num *      (7+(is_blend_mode_enabled ? 1 : 0)) + 4;
    //pvz_animation->tracks->texture->num = pvz_animation->current_tracks_num *   (7+(is_blend_mode_enabled ? 1 : 0)) + 5;
    //pvz_animation->tracks->alpha->num = pvz_animation->current_tracks_num *     (7+(is_blend_mode_enabled ? 1 : 0)) + 6;

    ////blend_mode->num
    //if (is_blend_mode_enabled)
    //	pvz_animation->tracks->blend_mode->num = pvz_animation->current_tracks_num * (7 + 1) + 7;

    if (start_param->visibleTrackEnabled)
    {
        pvz_animation->tracks->vis->num = track_num;
        track_num++;
    }
    pvz_animation->tracks->pos->num = track_num;
    track_num++;
    pvz_animation->tracks->rot->num = track_num;
    track_num++;
    pvz_animation->tracks->scale->num = track_num;
    track_num++;
    pvz_animation->tracks->skew->num = track_num;
    track_num++;
    if (start_param->textureTrackEnabled)
    {
        pvz_animation->tracks->texture->num = track_num;
        track_num++;
    }
    if (start_param->alphaTrackEnabled)
    {
        pvz_animation->tracks->alpha->num = track_num;
        track_num++;
    }
    if (start_param->blendModeTrackEnabled)
    {
        pvz_animation->tracks->blend_mode->num = track_num;
        track_num++;
    }
    SetAnimKeyTimes(pvz_animation, 0);
}
void SetTrackName(PvzAnimation* anim[], const int anim_num, const char* new_content)
{
    const PvzAnimation* first_anim = anim[0];
    PvzAnimation* current_anim = anim[anim_num];
    PvzTracks* current_tracks = anim[anim_num]->tracks;
    sprintf_s(current_tracks->name, NAME_LENGTH, new_content);
    // 注释：使用2进制位操作符来大写第一个字母
    current_tracks->name[0] &= 0b1011111;
    for (int i = 0; i < NAME_LENGTH; i++)
    {
        if (current_tracks->name[i] == '.')
            current_tracks->name[i] = '_';
    }
    int track_str_end_num = 0;
    char temp_name[NAME_LENGTH];
    sprintf_s(temp_name, NAME_LENGTH, "%s", current_tracks->name);
    for (int i = 0; i < first_anim->current_tracks_num; i++)
    {
        // 注释：判断是否已经存在相同的track
        if (track_str_end_num)
        {
            sprintf_s(temp_name, NAME_LENGTH, "%s%d", current_tracks->name, track_str_end_num);
        }
        if (strcmp(temp_name, first_anim->track_name[i]) == 0)
        {
            track_str_end_num++;
        }
    }
    if (track_str_end_num)
    {
        sprintf_s(temp_name, NAME_LENGTH, "%s%d", current_tracks->name, track_str_end_num);
        sprintf_s(current_tracks->name, NAME_LENGTH, "%s", temp_name);
    }
    sprintf_s(current_tracks->vis->path, PATH_LENGTH, "%s:visible", current_tracks->name);
    sprintf_s(current_tracks->pos->path, PATH_LENGTH, "%s:position", current_tracks->name);
    sprintf_s(current_tracks->rot->path, PATH_LENGTH, "%s:rotation", current_tracks->name);
    sprintf_s(current_tracks->scale->path, PATH_LENGTH, "%s:scale", current_tracks->name);
    sprintf_s(current_tracks->skew->path, PATH_LENGTH, "%s:skew", current_tracks->name);
    sprintf_s(current_tracks->texture->path, PATH_LENGTH, "%s:texture", current_tracks->name);
    sprintf_s(current_tracks->alpha->path, PATH_LENGTH, "%s:self_modulate", current_tracks->name);
    sprintf_s(current_tracks->blend_mode->path, PATH_LENGTH, "%s:material", current_tracks->name);
    sprintf_s(current_anim->track_name[current_anim->current_tracks_num], NAME_LENGTH, "%s", current_tracks->name);
}
void PreSetTrackTVis(const PvzAnimation* anim)
{
    BoolKeys* vis_keys = &anim->tracks->vis->keys;
    // 注释：如果当前帧时间为0，则track的visible设置为true
    if (anim->current_frame_time_num == 0)
    {
        if (!strcmp(anim->anim_name, "all"))
        {
            //sprintf_s(anim->tracks->vis->key.values[anim->current_tracks_vis_key_times], NAME_LENGTH, "true");
            vis_keys->values[vis_keys->times_num] = true;
        }
        //anim->tracks->vis->key.times[anim->current_tracks_vis_key_times] = 0;
        vis_keys->times[vis_keys->times_num] = 0;
        vis_keys->times_num++;
    }
}
void PreSetTrackTPos(const PvzAnimation* anim)
{
    Vector2Keys* pos_keys = &anim->tracks->pos->keys;
    // 注释：如果当前帧时间不为0，则track的position设置为上一帧的值，否则设置为0，0
    if (pos_keys->times_num)
    {
        pos_keys->values[pos_keys->times_num] = pos_keys->values[pos_keys->times_num - 1];
    }
    else if (strcmp(anim->output_file_extension, "tscn") == 0)
    {
        pos_keys->values[pos_keys->times_num] = (Vector2){ 0.0f, 0.0f };
    }

    pos_keys->times[pos_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    pos_keys->times_num++;
}
void PreSetTrackTScale(const PvzAnimation* anim)
{
    Vector2Keys* scale_keys = &anim->tracks->scale->keys;
    // 注释：如果当前帧时间不为0，则track的scale设置为上一帧的值，否则设置为1，1
    if (scale_keys->times_num)
    {
        scale_keys->values[scale_keys->times_num] = scale_keys->values[scale_keys->times_num - 1];
    }
    else if (strcmp(anim->output_file_extension, "tscn") == 0)
    {
        scale_keys->values[scale_keys->times_num] = (Vector2){ 1.0f, 1.0f };
    }

    scale_keys->times[scale_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    scale_keys->times_num++;
}
void PreSetTrackTRot(const PvzAnimation* anim)
{
    FloatKeys* rot_keys = &anim->tracks->rot->keys;
    if (rot_keys->times_num)
    {
        rot_keys->values[rot_keys->times_num] = rot_keys->values[rot_keys->times_num - 1];
    }
    else if (strcmp(anim->output_file_extension, "tscn") == 0)
    {
        rot_keys->values[rot_keys->times_num] = 0.0 / 180 * PI;
    }

    rot_keys->times[rot_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    rot_keys->times_num++;
}
void PreSetTrackTSkew(const PvzAnimation* anim)
{
    FloatKeys* skew_keys = &anim->tracks->skew->keys;
    if (skew_keys->times_num)
    {
        skew_keys->values[skew_keys->times_num] = skew_keys->values[skew_keys->times_num - 1];
    }
    else if (strcmp(anim->output_file_extension, "tscn") == 0)
    {
        skew_keys->values[skew_keys->times_num] = 0.0 / 180 * PI;
    }

    skew_keys->times[skew_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    skew_keys->times_num++;
}
void PreSetTrackTTexture(const PvzAnimation* anim)
{
    ExtResourceKeys* texture_keys = &anim->tracks->texture->keys;
    if (!texture_keys->times_num)
    {
        //sprintf_s(anim->tracks->texture->key.values[anim->current_tracks_texture_key_times], NAME_LENGTH, "%s", "null");
        texture_keys->values[texture_keys->times_num] = -1;
        texture_keys->times[texture_keys->times_num] = 0;
        texture_keys->times_num++;
    }
}

void PreSetTrackTAlpha(const PvzAnimation* anim)
{
    ColorKeys* alpha_keys = &anim->tracks->alpha->keys;
    if (alpha_keys->times_num)
    {
        //sprintf_s(anim->tracks->alpha->key.values[alpha_keys->times_num], NAME_LENGTH, anim->tracks->alpha->key.values[alpha_keys->times_num - 1]);
        alpha_keys->values[alpha_keys->times_num] = alpha_keys->values[alpha_keys->times_num - 1];
    }
    else if (strcmp(anim->output_file_extension, "tscn") == 0)
    {
        //sprintf_s(anim->tracks->alpha->key.values[alpha_keys->times_num], NAME_LENGTH, "Color(1, 1, 1, %2.5Lf)", 1.0);
        alpha_keys->values[alpha_keys->times_num] = (Color){ 1, 1, 1, 1 };
    }

    alpha_keys->times[alpha_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    alpha_keys->times_num++;
}

void SetTrackT(PvzAnimation* anim, char* new_content, R2GAStartParam* start_param)
{
    PreSetTrackTVis(anim);

    PreSetTrackTPos(anim);

    PreSetTrackTScale(anim);

    PreSetTrackTRot(anim);

    PreSetTrackTSkew(anim);

    PreSetTrackTTexture(anim);

    PreSetTrackTAlpha(anim);

    text(new_content, &anim, start_param);

    anim->current_frame_time_num++;
}
void SetF(const PvzAnimation* anim, const char* new_content)
{
    BoolKeys* vis_keys = &anim->tracks->vis->keys;
    if (anim->current_frame_time_num == 0)
    {
        vis_keys->times_num--;
    }
    if (vis_keys->times_num != 0)
    {
        //sprintf_s(anim->tracks->vis->key.values[vis_keys->times_num], NAME_LENGTH, "%s", anim->tracks->vis->key.values[vis_keys->times_num - 1]);
        vis_keys->values[vis_keys->times_num] = vis_keys->values[vis_keys->times_num - 1];
        vis_keys->times[vis_keys->times_num] = (float)(1.0 / FPS * (anim->current_frame_time_num - 1));
        vis_keys->times_num++;
    }

    switch (atoi(new_content))
    {
        case -1:    vis_keys->values[vis_keys->times_num] = false;    break;
        case  0:    vis_keys->values[vis_keys->times_num] = true;     break;
        default:    break;
    }
    vis_keys->times[vis_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    vis_keys->times_num++;
}
void SetX(const PvzAnimation* anim, const char* new_content)
{
    Vector2Keys* pos_keys = &anim->tracks->pos->keys;
    if (!pos_keys->times_num)
    {
        printf("pvz_animations->current_tracks_pos_key_times_x = 0\n");

        pos_keys->values[pos_keys->times_num] = (Vector2){ 0.0f, 0.0f };
        pos_keys->times_num++;
    }
    pos_keys->values[pos_keys->times_num - 1].x = atof(new_content);
    pos_keys->times[pos_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}
void SetY(const PvzAnimation* anim, const char* new_content)
{
    Vector2Keys* pos_keys = &anim->tracks->pos->keys;
    if (!pos_keys->times_num)
    {
        printf("pvz_animations->current_tracks_pos_key_times_y = 0\n");
        pos_keys->times_num++;
    }

    pos_keys->values[pos_keys->times_num - 1].y = atof(new_content);
    pos_keys->times[pos_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}
void SetSx(const PvzAnimation* anim, const char* new_content)
{
    Vector2Keys* scale_keys = &anim->tracks->scale->keys;
    if (!scale_keys->times_num)
    {
        printf("tracks_scale_key_times_x = 0\n");
        scale_keys->times_num++;
    }
    scale_keys->values[scale_keys->times_num - 1].x = atof(new_content);
    scale_keys->times[scale_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}
void SetSy(const PvzAnimation* anim, const char* new_content)
{
    Vector2Keys* scale_keys = &anim->tracks->scale->keys;
    if (!scale_keys->times_num)
    {
        printf("tracks_scale_key_times_y = 0\n");
        //sprintf_s(anim->tracks->scale->key.values[scale_keys->times_num], NAME_LENGTH, "Vector2(1.000, %5.3Lf)", atof(new_content));
        scale_keys->times_num++;
    }
    scale_keys->values[scale_keys->times_num - 1].y = atof(new_content);
    scale_keys->times[scale_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}
void SetKx(const PvzAnimation* anim, const char* new_content)
{
    FloatKeys* rot_keys = &anim->tracks->rot->keys;
    FloatKeys* skew_keys = &anim->tracks->skew->keys;

    if (!rot_keys->times_num)
    {
        rot_keys->times_num++;
    }

    if (fabs(atof(new_content)) > 360 && fabs(fmod(atof(new_content), 360.0)) >= 180)
        printf("kx > 360\n处理后 kx = %lf\n", fmod(atof(new_content), 360.0));

    //sprintf_s(anim->tracks->rot->key.values[rot_keys->times_num - 1], NAME_LENGTH, "%10.6Lf", temp);

    rot_keys->values[rot_keys->times_num - 1] = fmod(atof(new_content), 360.0) / 180 * PI;

    rot_keys->times[rot_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);

    float last_skew = 0, last_rot = 0;
    if (skew_keys->times_num != 1)
    {
        last_skew = anim->tracks->skew->keys.values[skew_keys->times_num - 2];
    }
    if (rot_keys->times_num != 1)
    {
        last_rot = rot_keys->values[rot_keys->times_num - 2];
    }
    float temp = last_skew + last_rot
        - rot_keys->values[rot_keys->times_num - 1];

    skew_keys->values[skew_keys->times_num - 1] = temp;
    skew_keys->times[skew_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}
void SetKy(const PvzAnimation* anim, const char* new_content)
{
    const FloatKeys* rot_keys = &anim->tracks->rot->keys;
    FloatKeys* skew_keys = &anim->tracks->skew->keys;

    const float temp = fmod(atof(new_content), 360.0) / 180 * PI
        - rot_keys->values[rot_keys->times_num - 1];

    //sprintf_s(pvz_animation->tracks->skew->key.values[pvz_animation->current_tracks_skew_key_times - 1], NAME_LENGTH, "%10.6Lf", temp);
    skew_keys->values[skew_keys->times_num - 1] = temp;
    skew_keys->times[skew_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}
void SetI(PvzAnimation* anim, const char* new_content)
{
    ExtResourceKeys* texture_keys = &anim->tracks->texture->keys;
    char temp[NAME_LENGTH] = { 0 };
    int res_filename_index = 0;
    // 如果 texture 的 key 数量 已经超过了 当前帧数，则将 texture 的 key 数量设置为 当前帧数
    /*if (pvz_animation->current_tracks_texture_key_times == pvz_animation->current_frame_time_num)
        printf("tracks_texture_key_times = current_frame_time_num\n");*/
    if (texture_keys->times_num > anim->current_frame_time_num)
    {
        texture_keys->times_num = anim->current_frame_time_num;
    }
    if (texture_keys->times_num != 0 &&
        texture_keys->times[texture_keys->times_num] != (float)(1.0 / FPS * (anim->current_frame_time_num - 1)))
    {
        //sprintf_s(anim->tracks->texture->key.values[texture_keys->times_num], NAME_LENGTH, "%s", anim->tracks->texture->key.values[texture_keys->times_num - 1]);
        texture_keys->values[texture_keys->times_num] = texture_keys->values[texture_keys->times_num - 1];
        texture_keys->times[texture_keys->times_num] = (float)(1.0 / FPS * (anim->current_frame_time_num - 1));
        texture_keys->times_num++;
    }
    // 提取 IMAGE_REANIM_XXX 中的 XXX 并将除首字母外的其他字符转为小写字母
    if (strncmp(new_content, "IMAGE_REANIM_", 13) == 0)
    {
        temp[0] = new_content[13]; // 保留首字母
        for (res_filename_index = 14; new_content[res_filename_index] != '\0'; res_filename_index++)
        {
            temp[res_filename_index - 13] = tolower(new_content[res_filename_index]);
        }
        temp[res_filename_index - 13] = '\0';
        strncat_s(temp, NAME_LENGTH, ".png", _TRUNCATE);
    }
    else
    {
        strncpy_s(temp, NAME_LENGTH, new_content, _TRUNCATE);
    }
    // 这里可以使用 temp 进行后续操作
    for (res_filename_index = 0; res_filename_index < anim->texture_filename_times; res_filename_index++)
    {
        if (strcmp(anim->texture_filename[res_filename_index], temp) == 0)
        {
            //sprintf_s(anim->tracks->texture->key.values[texture_keys->times_num], NAME_LENGTH, "ExtResource(\"%d_fuck\")", i);
            //texture_keys->values[texture_keys->times_num] = i;
            break;
        }
    }
    if (res_filename_index == anim->texture_filename_times)
    {
        sprintf_s(anim->texture_filename[res_filename_index], NAME_LENGTH, "%s", temp);
        //sprintf_s(anim->tracks->texture->key.values[texture_keys->times_num], NAME_LENGTH, "ExtResource(\"%d_fuck\")", i);
        //texture_keys->values[texture_keys->times_num] = i;
        anim->texture_filename_times++;
    }
    texture_keys->values[texture_keys->times_num] = res_filename_index;
    texture_keys->times[texture_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);

    texture_keys->times_num++;
}

void SetA(const PvzAnimation* anim, const char* new_content)
{
    ColorKeys* alpha_keys = &anim->tracks->alpha->keys;

    if (alpha_keys->times_num)
    {
        //sprintf_s(anim->tracks->alpha->key.values[alpha_keys->times_num - 1], NAME_LENGTH, "Color(1, 1, 1, %2.5Lf)", atof(new_content));
        alpha_keys->values[alpha_keys->times_num - 1] = (Color){ 1, 1, 1, atof(new_content) };
    }
    else
    {
        printf("tracks_alpha_key_times = 0\n");
        //sprintf_s(anim->tracks->alpha->key.values[alpha_keys->times_num], NAME_LENGTH, "Color(1, 1, 1, %2.5Lf)", atof(new_content));
        alpha_keys->values[alpha_keys->times_num] = (Color){ 1, 1, 1, atof(new_content) };
        alpha_keys->times_num++;
    }
    alpha_keys->times[alpha_keys->times_num - 1] = (float)(1.0 / FPS * anim->current_frame_time_num);
}

void SetBm(const PvzAnimation* anim, const char* new_content)
{
    BlendModeKeys* blend_mode_keys = &anim->tracks->blend_mode->keys;
    if (strcmp(new_content, "normal") == 0)
    {
        //sprintf_s(anim->tracks->blend_mode->key.values[blend_mode_keys->times_num], NAME_LENGTH, "SubResource(\"ShaderMaterial_%s\")", new_content);
        blend_mode_keys->values[blend_mode_keys->times_num] = BLEND_MODE_NORMAL;
    }
    else if (strcmp(new_content, "add") == 0)
    {
        blend_mode_keys->values[blend_mode_keys->times_num] = BLEND_MODE_ADD;
    }
    //anim->tracks->blend_mode->key.times[blend_mode_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    blend_mode_keys->times[blend_mode_keys->times_num] = (float)(1.0 / FPS * anim->current_frame_time_num);
    blend_mode_keys->times_num++;
}

void SetInitValue(PvzAnimation* anims[], const int anim_index)
{
    const PvzTracks* first_tracks = anims[0]->tracks;
    const PvzTracks* current_tracks = anims[anim_index]->tracks;

    current_tracks->vis->keys.values[0] = first_tracks->vis->keys.values[first_tracks->vis->keys.times_num ? first_tracks->vis->keys.times_num - 1 : 0];

    current_tracks->pos->keys.values[0] = first_tracks->pos->keys.values[first_tracks->pos->keys.times_num ? first_tracks->pos->keys.times_num - 1 : 0];

    current_tracks->scale->keys.values[0] = first_tracks->scale->keys.values[first_tracks->scale->keys.times_num ? first_tracks->scale->keys.times_num - 1 : 0];

    current_tracks->rot->keys.values[0] = first_tracks->rot->keys.values[first_tracks->rot->keys.times_num ? first_tracks->rot->keys.times_num - 1 : 0];

    current_tracks->skew->keys.values[0] = first_tracks->skew->keys.values[first_tracks->skew->keys.times_num ? first_tracks->skew->keys.times_num - 1 : 0];

    if (first_tracks->texture->keys.times_num)
    {
        const int zero_texture_index = first_tracks->texture->keys.times_num - 1;
        //sscanf_s(anims[0]->tracks->texture->key.values[zero_texture_index], "ExtResource(\"%d_fuck\")", &zero_fuck_index);
        const int zero_fuck_index = first_tracks->texture->keys.values[zero_texture_index];
        if (zero_fuck_index != -1)
            SetI(anims[anim_index], anims[0]->texture_filename[zero_fuck_index]);
    }

    current_tracks->alpha->keys.values[0] = first_tracks->alpha->keys.values[first_tracks->alpha->keys.times_num ? first_tracks->alpha->keys.times_num - 1 : 0];
}

/// <summary>
///		<para>查找经过偏移后的字符串中第一个Tag的内容及所包含的文本</para>
///		<para>或者说，查找下一个Tag及其内容，因为offset在函数内会自动更新</para>
/// </summary>
/// <param name="input">用于查找的tag的字符串</param>
/// <param name="offset">偏移量的指针</param>
/// <param name="tap_name">tag的名字，这里应传入被修改的字符串</param>
/// <param name="content">tag的内容，这里应传入被修改的字符串</param>
/// <returns></returns>
int tap(char* input, int* offset, char* tap_name, char* content)
{
    char temp;

    for (; (temp = input[*offset]) != '<'; (*offset)++)
    {
        if (temp == '\n' || temp == ' ' || temp == '\t')	continue;
        else if (temp == '\0' || temp == EOF)	return -1;
        else	return 1;
    }
    (*offset)++;
    int i = 0;
    for (i = 0;
        (temp = input[*offset]) != '>';
        i++, (*offset)++)
    {
        tap_name[i] = temp;
    }
    tap_name[i] = '\0';
    (*offset)++;
    int k = 0;
    for (int j = 1; temp = input[*offset]; j++, (*offset)++)
    {
        if (temp == '<')
        {
            k = j;
        }
        if (k && j - k >= 2 && temp != tap_name[j - k - 2])
        {
            if (temp == '>' && '\0' == tap_name[j - k - 2])
            {
                (*offset)++;
                content[k - 1] = '\0';
                return 0;
            }
            else
            {
                k = 0;
            }
        }
        content[j - 1] = temp;
    }
    return 1;
}
/// <summary>
/// 对PVZ动画文件进行预处理，提取出动画信息
/// </summary>
/// <param name="old_content">递归时传入的字符串，用于读取上一层内容，当第一次调用时传入完整文件内容</param>
/// <param name="pvz_animations"></param>
/// <param name="start_param"></param>
void SeekAnim(char* old_content, PvzAnimation* pvz_animations[], R2GAStartParam* start_param)
{
    static bool is_track_anim_finished = false;
    static int anim_index = 0;
    int offset = 0;
    char tap_name[20] = { '\0' };
    char* new_content = (char*)calloc(50000, sizeof(char));
    if (new_content == NULL)
    {
        fprintf(stderr, "SeekAnim_content = NULL\n");
        return;
    }
    while (tap(old_content, &offset, tap_name, new_content) != -1 && is_track_anim_finished == false)
    {
        if (!strcmp(tap_name, dictionary[TRACK_INDEX]/*track*/))
        {
            current_frame_time_num = 0;
            is_track_anim = false;
            is_start_frame_time_select = false;
            SeekAnim(new_content, pvz_animations, start_param);
            if (is_track_anim && pvz_animations[anim_index]->end_frame_time < pvz_animations[anim_index]->start_frame_time)
            {
                pvz_animations[anim_index]->end_frame_time = current_frame_time_num - 1;
            }
            continue;
        }
        if (!strcmp(tap_name, dictionary[NAME_INDEX]/*name*/))
        {
            // 如果new_content中前五个字符为 "anim_", 则认为是动画名称
            if (strncmp(new_content, "anim_", 5) == 0)
            {
                anim_nums++;
                anim_index++;
                is_track_anim = true;
                int anim_str_end_num = 0;
                char anim_name[NAME_LENGTH];
                sprintf_s(anim_name, NAME_LENGTH, "%s", new_content + 5);
                for (int i = 0; i < anim_index; i++)
                {
                    if (anim_str_end_num)
                    {
                        sprintf_s(anim_name, NAME_LENGTH, "%s%d", new_content + 5, anim_str_end_num);
                    }
                    if (strcmp(pvz_animations[i]->anim_name, anim_name) == 0)
                    {
                        anim_str_end_num++;
                    }
                }
                if (anim_str_end_num)
                {
                    sprintf_s(anim_name, NAME_LENGTH, "%s%d", new_content + 5, anim_str_end_num);
                }
                SetAnimName(pvz_animations[anim_index], anim_name);
                char res_file_name[NAME_LENGTH];

                //FileGetFileNameWithoutExt(input_file, res_name);
                strncpy(res_file_name, start_param->inputFileName, NAME_LENGTH - 1);
                res_file_name[NAME_LENGTH - 1] = '\0';
                strcat_s(res_file_name, NAME_LENGTH, "_");
                strcat_s(res_file_name, NAME_LENGTH, anim_name);
                SetAnimResName(pvz_animations[anim_index], res_file_name);
            }
            else
            {
                //is_track_anim_finished = true;
            }
            continue;
        }
        // t
        if (!strcmp(tap_name, dictionary[T_INDEX]/*t*/) && is_track_anim)
        {
            if (current_frame_time_num == 0 && new_content[0] != '\0')
            {
                SetAnimStartFrameTime(pvz_animations[anim_index], current_frame_time_num);
            }
            SeekAnim(new_content, pvz_animations, start_param);
            current_frame_time_num++;
            continue;
        }
        // f
        if (!strcmp(tap_name, dictionary[F_INDEX]/*f*/))
        {
            if (atoi(new_content) == 0 && is_start_frame_time_select == false)
            {
                is_start_frame_time_select = true;
                SetAnimStartFrameTime(pvz_animations[anim_index], current_frame_time_num);
            }
            else if (atoi(new_content) == -1)
            {
                SetAnimEndFrameTime(pvz_animations[anim_index], current_frame_time_num - 1);
            }
            continue;
        }
    }

    free(new_content);
}

/// <summary>
/// 读取文件内容，并解析出动画数据
/// </summary>
/// <param name="old_content">递归时传入的字符串，用于读取上一层内容，当第一次调用时传入完整文件内容</param>
/// <param name="pvz_animations"></param>
/// <param name="start_param"></param>
void text(char* old_content, PvzAnimation* pvz_animations[], R2GAStartParam* start_param)
{
    int anim_index = 0;
    int offset = 0;
    char tap_name[20] = { '\0' };
    char* new_content = (char*)calloc(50000, sizeof(char));

    if (new_content == NULL)
    {
        fprintf(stderr, "text_content = NULL\n");
        return;
    }

    while (tap(old_content, &offset, tap_name, new_content) != -1)
    {
        if (!strcmp(tap_name, dictionary[FPS_INDEX]/*fps*/))
        {
            SetFPS(new_content);
            continue;
        }

        if (!strcmp(tap_name, dictionary[TRACK_INDEX]/*track*/))
        {
            current_frame_time_num = 0;
            for (anim_index = 0; anim_index <= anim_nums; anim_index++)
            {
                SetTrack(pvz_animations[anim_index], new_content, start_param);
            }
            text(new_content, pvz_animations, start_param);
            for (anim_index = 0; anim_index <= anim_nums; anim_index++)
            {
                PvzAnimation* anim = pvz_animations[anim_index];

                ExtResourceKeys* texture_keys = &anim->tracks->texture->keys;
                // 如果当前帧时间大于结束帧时间，则跳过
                if (texture_keys->times_num != 0)
                {
                    // 复制最后一个值到第一个值，防止出现第一个值为空的情况
                    //sprintf_s(pvz_animations[anim_index]->tracks->texture->key.values[pvz_animations[anim_index]->current_tracks_texture_key_times], NAME_LENGTH, "%s", pvz_animations[anim_index]->tracks->texture->key.values[pvz_animations[anim_index]->current_tracks_texture_key_times - 1]);
                    texture_keys->values[texture_keys->times_num] = texture_keys->values[texture_keys->times_num - 1];
                    texture_keys->times[texture_keys->times_num] = (float)(1.0 / FPS * (anim->current_frame_time_num - 1));
                    texture_keys->times_num++;
                }
                BoolKeys* vis_keys = &anim->tracks->vis->keys;
                // 如果当前帧时间大于结束帧时间，则跳过
                if (vis_keys->times_num != 0)
                {
                    // 复制最后一个值到第一个值，防止出现第一个值为空的情况
                    //sprintf_s(pvz_animations[anim_index]->tracks->vis->key.values[pvz_animations[anim_index]->current_tracks_vis_key_times], NAME_LENGTH, "%s", pvz_animations[anim_index]->tracks->vis->key.values[pvz_animations[anim_index]->current_tracks_vis_key_times - 1]);
                    vis_keys->values[vis_keys->times_num] = vis_keys->values[vis_keys->times_num - 1];
                    vis_keys->times[vis_keys->times_num] = (float)(1.0 / FPS * (anim->current_frame_time_num - 1));
                    vis_keys->times_num++;
                }
                //FileWriteTracks(anim, start_param);
                anim->vptr->PrintTracksToFile(anim, start_param);
                anim->current_tracks_num++;
            }
            continue;
        }

        if (!strcmp(tap_name, dictionary[NAME_INDEX]/*name*/))
        {
            for (anim_index = 0; anim_index <= anim_nums; anim_index++)
                SetTrackName(pvz_animations, anim_index, new_content);
            continue;
        }

        if (!strcmp(tap_name, dictionary[T_INDEX]/*t*/))
        {
            for (anim_index = 0; anim_index <= anim_nums; anim_index++)
            {
                // 如果当前帧时间在开始帧时间和结束帧时间之外，则跳过
                if (pvz_animations[anim_index]->start_frame_time > current_frame_time_num ||
                    pvz_animations[anim_index]->end_frame_time < current_frame_time_num)
                    continue;
                // 如果当前帧时间为0，则设置初始值
                if (pvz_animations[anim_index]->current_frame_time_num == 0/* && pvz_animations[anim_index]->start_frame_time != 0*/)
                {
                    // 函数：设置初始值
                    SetInitValue(pvz_animations, anim_index);
                    //pvz_animations[anim_index];
                }
                SetTrackT(pvz_animations[anim_index], new_content, start_param);
            }
            current_frame_time_num++;
            continue;
        }

        if (!strcmp(tap_name, dictionary[F_INDEX]/*f*/))
        {
            SetF(pvz_animations[0], new_content);
            continue;
        }

        if (!strcmp(tap_name, dictionary[X_INDEX]/*x*/))
        {
            SetX(pvz_animations[0], new_content);
            continue;
        }
        if (!strcmp(tap_name, dictionary[Y_INDEX]/*y*/))
        {
            SetY(pvz_animations[0], new_content);
            continue;
        }

        if (!strcmp(tap_name, dictionary[SX_INDEX]/*sx*/))
        {
            SetSx(pvz_animations[0], new_content);
            continue;
        }
        if (!strcmp(tap_name, dictionary[SY_INDEX]/*sy*/))
        {
            SetSy(pvz_animations[0], new_content);
            continue;
        }

        if (!strcmp(tap_name, dictionary[KX_INDEX]/*kx*/))
        {
            SetKx(pvz_animations[0], new_content);
            continue;
        }
        if (!strcmp(tap_name, dictionary[KY_INDEX]/*ky*/))
        {
            SetKy(pvz_animations[0], new_content);
            continue;
        }

        if (!strcmp(tap_name, dictionary[I_INDEX]/*i*/))
        {
            SetI(pvz_animations[0], new_content);
            continue;
        }

        /*a*/
        if (!strcmp(tap_name, dictionary[A_INDEX]/*a*/))
        {
            SetA(pvz_animations[0], new_content);
            continue;
        }

        /*bm*/
        if (!strcmp(tap_name, dictionary[BM_INDEX]/*bm*/))
        {
            SetBm(pvz_animations[0], new_content);
            continue;
        }
    }

    free(new_content);
}

FILE* fp_input;
// 分配50MB的内存用于读取文件内容

static void print_help(char* exe_name)
{
    // 颜色定义

    printf(COL_TITLE "欢迎使用PVZ_reanim2godot_animation(R2GA) v%s" COL_RESET "\n", VERSION);
    printf(COL_CMD "用法: " COL_RESET "%s " COL_OPT "<输入文件> <动画路径> <资源路径>" COL_RESET " [选项]\n\n", exe_name);

    printf(COL_HEADER "必需参数:" COL_RESET "\n");
    printf("  " COL_OPT "<输入文件 input_file>" COL_RESET "                " "输入文件路径 (" COL_VAL ".reanim" COL_RESET "格式)\n");
    printf("  " COL_OPT "<动画路径 anim_godot_path>" COL_RESET "           " "Godot动画资源输出路径 (如: " COL_VAL "res://anim/abc/" COL_RESET ")\n");
    printf("  " COL_OPT "<资源路径 res_godot_path>" COL_RESET "            " "Godot资源文件路径 (如: " COL_VAL "res://art/abc/" COL_RESET ")\n\n");

    printf(COL_HEADER "可选选项:" COL_RESET "\n");
    printf("  " COL_OPT "-of, --output-file" COL_RESET " " COL_VAL "<输出文件>" COL_RESET "        " "输出文件路径 (默认: 与输入文件同名)\n");
    printf("  " COL_OPT "-om, --output-mode" COL_RESET " " COL_VAL "<输出模式>" COL_RESET "        " "设置输出模式 (可选: " COL_VAL "auto, tscn_by_anim, anim_tres" COL_RESET ", 默认: " COL_VAL "auto" COL_RESET ")\n");
    printf("  " COL_OPT "-cf, --config-file" COL_RESET " " COL_VAL "<配置文件>" COL_RESET "        " "指定配置文件路径\n");
    printf("  " COL_OPT "-bm, --blend_mode" COL_RESET "                     " "开启混合模式 (默认关闭)\n");
    printf("  " COL_OPT "-nbm, --no-blend_mode" COL_RESET "                 " "强制关闭混合模式\n");
    printf("  " COL_OPT "-im, --interpolation-mode" COL_RESET " " COL_VAL "<插值模式>" COL_RESET " " "设置插值模式 (可选: " COL_VAL "nearest, linear, cubic" COL_RESET ", 默认: " COL_VAL "linear" COL_RESET ")\n");
    printf("  " COL_OPT "-h, --help" COL_RESET "                           " "显示此帮助信息\n\n");

    printf(COL_HEADER "输出模式:" COL_RESET "\n");
    printf("  " COL_VAL "auto" COL_RESET "                                 " "输出一个tscn文件和多个tres文件，并自动关联（推荐）（默认）\n");
    printf("  " COL_VAL "tscn_by_anim" COL_RESET "                         " "输出一个tscn文件，包含所有动画资源\n");
    printf("  " COL_VAL "anim_tres" COL_RESET "                            " "输出多个tres文件，每个tres文件包含一个动画资源\n\n");

    printf(COL_HEADER "插值模式:" COL_RESET "\n");
    printf("  " COL_VAL "nearest" COL_RESET "                              " "最近邻插值\n");
    printf("  " COL_VAL "linear" COL_RESET "                               " "线性插值\n");
    printf("  " COL_VAL "cubic" COL_RESET "                                " "三次方插值" COL_RESET "\n");
}

static void PrintErrorMsg(char* error_msg)
{
    printf(COL_ERR "错误: %s\n" COL_RESET, error_msg);
}

static void PrintParamError(void)
{
    PrintErrorMsg("参数错误！请检查参数！使用 -h 或 --help 查看帮助信息。");
}

// 启用控制台虚拟终端, 使输出颜色更加丰富
static void enable_vt_mode(void)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

R2GAStartParam startParam;
int main(int argc, char* argv[])
{
    enable_vt_mode();

    StartParamInit(&startParam);
    const Result result = StartParamSetFromArgs(&startParam, argc, argv);
    printf("debug: %d\n", result);

    if (result == Result_Failed)
    {
        PrintParamError();
        return 1;
    }
    else if (startParam.help)
    {
        print_help(argv[0]);
        return ErrorCode_Success;
    }

    if (startParam.configFileWholePath[0] != '\0')
    {
        if (StartParamSetFromConfig(&startParam, startParam.configFileWholePath) == Result_Failed)
        {
            PrintParamError();
            return ErrorCode_CannotParseConfigFile;
        }
    }
    // 检查命令行参数数量是否为5
    /*if (argc != 5)
    {
        print_help(argv[0]);
        return 1;
    }*/
    // 判断输出模式
    /*sprintf_s(output_type, NAME_LENGTH, "%s", argv[4]);
    if (strcmp(output_type, MODE_TSCN_BY_ANIM_STR) && strcmp(output_type, MODE_ANIM_TRES_STR) && strcmp(output_type, MODE_AUTO_STR))
    {
        print_help(argv[0]);
        return 2;
    }*/

    // 打开输入文件
    //sprintf_s(input_file, NAME_LENGTH, "%s", argv[1]);
    //FileGetFilePath(input_file, input_filePath);
    //
    //errno_t err = fopen_s(&fp_input, argv[1], "r");
    //if (err != 0 || fp_input == NULL)
    //{
    //	printf(argv[1]);
    //	fprintf(stderr, "fp_input = NULL\n");
    //	return 3;
    //}

    FileOpen(&fp_input, startParam.inputFileWholePath, "r", ErrorCode_CannotOpenInputFile);

    // 初始化pvz_animations指针数组
    PvzAnimation* pvz_animations[MAX_ANIM_NUM];
    ResourceFile* resource_files[MAX_ANIM_NUM];
    for (int i = 0; i < MAX_ANIM_NUM; i++)
    {
        pvz_animations[i] = PvzAnimation_New();
        if (i == 0)
        {
            PvzAnimation_Init(pvz_animations[i], "all", "tscn", &startParam);
            resource_files[i] = (ResourceFile*)Tscn_New(pvz_animations[i], pvz_animations, MAX_ANIM_NUM);
        }
        else
        {
            PvzAnimation_Init(pvz_animations[i], "null", "tres", &startParam);
            resource_files[i] = (ResourceFile*)Tres_New(pvz_animations[i]);
        }
    }

    //FileGetFileNameWithoutExt(argv[1], pvz_animations[0]->res_file_name);
    strncpy(pvz_animations[0]->res_file_name, startParam.inputFileName, NAME_LENGTH);

    pvz_animations[0]->start_frame_time = 0;
    pvz_animations[0]->end_frame_time = MAX_TIMES_NUM - 1;

    // 分配50MB的内存用于读取文件内容
    // 应该没有超过50MB的reanim文件吧（笑）
    // TODO: 这里应该改成动态分配内存
    char* filetext = (char*)calloc((size_t)(50 * 1024) * 1024, sizeof(char));
    // 读取输入文件内容
    FileRead(fp_input, filetext);

    SeekAnim(filetext, pvz_animations, &startParam);
    printf_s("debug: 共有%d个动画\n\n", anim_nums);
    for (int i = 0; i <= anim_nums; i++)
    {
        printf_s("debug: 第%d个动画的资源名为%s\n", i, pvz_animations[i]->res_file_name);
        printf_s("debug: 第%d个动画的起始帧时间为%d\n", i, pvz_animations[i]->start_frame_time);
        printf_s("debug: 第%d个动画的结束帧时间为%d\n\n", i, pvz_animations[i]->end_frame_time);
    }

    for (int i = 0; i <= anim_nums; i++)
    {
        //pvz_animations[i]->vptr->OpenOutputFiles(pvz_animations[i], startParam.inputFilePath);
        ResourceFile* file = resource_files[i];
        ((ResourceFile_VTable*)file->vptr)->OpenOutputFile(file, startParam.inputFilePath);
    }
    IsBlendModeEnabled(filetext, &startParam);
    //anim_nums = 0;

    // 处理文件内容并写入输出文件
    text(filetext, pvz_animations, &startParam);

    for (int i = 0; i <= anim_nums; i++)
    {
        fprintf_s(pvz_animations[i]->ofp_third_track, "\n");

        //FileExtResource(pvz_animations, i, anim_nums, argv[2], argv[3], is_blend_mode_enabled, (strcmp(output_type, MODE_TSCN_BY_ANIM_STR) != 0));
        FileExtResource(pvz_animations, i, anim_nums, &startParam, startParam.outputMode != OutputMode_TscnByAnim);
        FileSetAnim(pvz_animations[i]->ofp_second_anim, pvz_animations[i]->output_file_extension, pvz_animations[i]->res_file_name, pvz_animations[i]->current_frame_time_num);
        FileAddNode(pvz_animations[i]->ofp_forth_node, pvz_animations[i]->current_tracks_num, anim_nums, pvz_animations[i]->track_name, pvz_animations, startParam.outputMode != OutputMode_TscnByAnim);

        // 重置输出文件指针到文件开头
        fseek(pvz_animations[i]->ofp_output, 0, SEEK_SET);
        if (strcmp(pvz_animations[i]->output_file_extension, "tres") == 0)
        {
            FILE* input_files[3] = { pvz_animations[i]->ofp_first_ext,
                                     pvz_animations[i]->ofp_second_anim,
                                     pvz_animations[i]->ofp_third_track
            };

            FileMergeFiles(pvz_animations[i]->ofp_output, pvz_animations[i]->output_file_extension, input_files, 3);
        }
        else if (strcmp(pvz_animations[i]->output_file_extension, "tscn") == 0)
        {
            FILE* input_files[4] = { pvz_animations[i]->ofp_first_ext,
                                     pvz_animations[i]->ofp_second_anim,
                                     pvz_animations[i]->ofp_third_track,
                                     pvz_animations[i]->ofp_forth_node
            };

            FileMergeFiles(pvz_animations[i]->ofp_output, pvz_animations[i]->output_file_extension, input_files, 4);
        }
    }

    bool is_first_remove_output_files = false;
    bool is_secondandmore_remove_output_files = false;
    if (startParam.outputMode == OutputMode_TscnByAnim)
    {
        is_secondandmore_remove_output_files = true;
    }
    if (startParam.outputMode == OutputMode_AnimTres)
    {
        is_first_remove_output_files = true;
    }

    //fprintf(ofp_output, "length = %.6Lf\n", (float)time_num * (1.0 / FPS));
    //fprintf(ofp_output, "step = %.6Lf\n", 1.0 / FPS);

    for (int i = 0; i < MAX_ANIM_NUM; i++)
    {
        PvzAnimation* anim = pvz_animations[i];
        anim->vptr->FreeFiles(anim, (i ? is_secondandmore_remove_output_files : is_first_remove_output_files));
        anim->vptr->Delete(anim);
    }

    // 释放内存
    free(filetext);
    return 0;
}