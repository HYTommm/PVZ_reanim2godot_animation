#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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

R2GAStartParam startParam;

/// <summary>
/// 通过文件内容判断是否应启用混合模式
/// 遍历file_text，查找"<bm>"关键字，如果找到，则设置is_enabled为true
/// </summary>
/// <param name="file_text">文件内容字符串</param>
/// <param name="start_param">启动参数</param>
void IsBlendModeEnabled(const char* file_text, R2GAStartParam* start_param)
{
    if (start_param->blendModeTrackEnabledSpecified) return;

    // 遍历file_text，查找"<bm>"关键字，如果找到，则设置is_enabled为true
    start_param->blendModeTrackEnabled = !!strstr(file_text, "<bm>");
}

void SetAnimName(PvzAnimation* anim, char* new_content)
{
    sprintf_s(anim->anim_name, NAME_LENGTH, "%s", new_content);
}
void SetAnimStartFrameTime(PvzAnimation* anim, const int frame_time_num)
{
    anim->start_frame_time = frame_time_num;
}
void SetAnimEndFrameTime(PvzAnimation* anim, const int frame_time_num)
{
    anim->end_frame_time = frame_time_num;
}
void SetAnimResName(PvzAnimation* anim, char* new_content)
{
    sprintf_s(anim->res_file_name, NAME_LENGTH, "%s", new_content);
}

void SetFPS(const char* new_content)
{
    FPS = atoi(new_content);
}
void SetAnimKeyTimes(const PvzTracks* tracks, const int num)
{
    tracks->vis->keys.times_num = num;
    tracks->pos->keys.times_num = num;
    tracks->rot->keys.times_num = num;
    tracks->scale->keys.times_num = num;
    tracks->skew->keys.times_num = num;
    tracks->texture->keys.times_num = num;
    tracks->alpha->keys.times_num = num;
    tracks->blend_mode->keys.times_num = num;
}
void SetTrack(PvzAnimation* anim, const R2GAStartParam* start_param)
{
    PvzTracks tracks;
    Create(PvzTracks, &tracks);
    PvzTracks_Init(&tracks, start_param);

    if (start_param->visibleTrackEnabled)
    {
        tracks.vis->num = anim->current_track_num;
        anim->current_track_num++;
    }
    tracks.pos->num = anim->current_track_num;
    anim->current_track_num++;
    tracks.rot->num = anim->current_track_num;
    anim->current_track_num++;
    tracks.scale->num = anim->current_track_num;
    anim->current_track_num++;
    tracks.skew->num = anim->current_track_num;
    anim->current_track_num++;
    if (start_param->textureTrackEnabled)
    {
        tracks.texture->num = anim->current_track_num;
        anim->current_track_num++;
    }
    if (start_param->alphaTrackEnabled)
    {
        tracks.alpha->num = anim->current_track_num;
        anim->current_track_num++;
    }
    if (start_param->blendModeTrackEnabled)
    {
        tracks.blend_mode->num = anim->current_track_num;
        anim->current_track_num++;
    }
    SetAnimKeyTimes(&tracks, 0);

    anim->current_frame_time_num = 0;
    VCall(Vec(PvzTracks), &anim->tracks, push_back, tracks);
}
void SetTrackName(PvzAnimation* anim[], const int anim_num, const char* new_content)
{
    const PvzAnimation* first_anim = anim[0];
    PvzAnimation* current_anim = anim[anim_num];
    PvzTracks* current_tracks = anim[anim_num]->tracks.back;
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
        snprintf(temp_name, NAME_LENGTH, "%s%d", current_tracks->name, track_str_end_num);
        snprintf(current_tracks->name, NAME_LENGTH, "%s", temp_name);
    }
    //snprintf(current_tracks->vis->path.data, PATH_LENGTH, "%s:visible", current_tracks->name);
    //snprintf(current_tracks->pos->path.data, PATH_LENGTH, "%s:position", current_tracks->name);
    //snprintf(current_tracks->rot->path.data, PATH_LENGTH, "%s:rotation", current_tracks->name);
    //snprintf(current_tracks->scale->path.data, PATH_LENGTH, "%s:scale", current_tracks->name);
    //snprintf(current_tracks->skew->path.data, PATH_LENGTH, "%s:skew", current_tracks->name);
    //snprintf(current_tracks->texture->path.data, PATH_LENGTH, "%s:texture", current_tracks->name);
    //snprintf(current_tracks->alpha->path.data, PATH_LENGTH, "%s:self_modulate", current_tracks->name);
    //snprintf(current_tracks->blend_mode->path.data, PATH_LENGTH, "%s:material", current_tracks->name);
    //snprintf(current_anim->track_name[current_anim->current_tracks_num], NAME_LENGTH, "%s", current_tracks->name);

    string_append_s(&current_tracks->vis->path, format("{}:visible", current_tracks->name)->data);
    string_append_s(&current_tracks->pos->path, format("{}:position", current_tracks->name)->data);
    string_append_s(&current_tracks->rot->path, format("{}:rotation", current_tracks->name)->data);
    string_append_s(&current_tracks->scale->path, format("{}:scale", current_tracks->name)->data);
    string_append_s(&current_tracks->skew->path, format("{}:skew", current_tracks->name)->data);
    string_append_s(&current_tracks->texture->path, format("{}:texture", current_tracks->name)->data);
    string_append_s(&current_tracks->alpha->path, format("{}:self_modulate", current_tracks->name)->data);
    string_append_s(&current_tracks->blend_mode->path, format("{}:material", current_tracks->name)->data);
    snprintf(current_anim->track_name[current_anim->current_tracks_num], NAME_LENGTH, "%s", current_tracks->name);
}
void PreSetTrackTVis([[maybe_unused]] const PvzAnimation* anim)
{
}
void PreSetTrackTPos(const PvzAnimation* anim)
{
    if (startParam.frameMode == FRAME_MODE_KEYFRAME) return;

    Vector2Keys* pos_keys = &anim->tracks.back->pos->keys;
    // 注释：如果当前帧时间不为0，则track的position设置为上一帧的值，否则设置为0，0
    //if (pos_keys->times_num)
    if (anim->current_frame_time_num)
    {
        VCall(Vec(Vector2), &pos_keys->values, push_back, *pos_keys->values.back);
        const f32 temp = 1.0 / FPS * anim->current_frame_time_num;
        VCall(Vec(f32), &pos_keys->times, push_back, temp); //pos_keys->times.push_back(temp);
        pos_keys->times_num++;
    }
}
void PreSetTrackTScale(const PvzAnimation* anim)
{
    if (startParam.frameMode == FRAME_MODE_KEYFRAME) return;

    Vector2Keys* scale_keys = &anim->tracks.back->scale->keys;
    // 注释：如果当前帧时间不为0，则track的scale设置为上一帧的值，否则设置为1，1
    if (anim->current_frame_time_num)
    {
        VCall(Vec(Vector2), &scale_keys->values, push_back, *scale_keys->values.back);
        VCall(Vec(f32), &scale_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);
        scale_keys->times_num++;
    }
}
void PreSetTrackTRot(const PvzAnimation* anim)
{
    if (startParam.frameMode == FRAME_MODE_KEYFRAME) return;

    FloatKeys* rot_keys = &anim->tracks.back->rot->keys;
    if (anim->current_frame_time_num)
    {
        VCall(Vec(f32), &rot_keys->values, push_back, *rot_keys->values.back);
        VCall(Vec(f32), &rot_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);
        rot_keys->times_num++;
    }
}
void PreSetTrackTSkew(const PvzAnimation* anim)
{
    if (startParam.frameMode == FRAME_MODE_KEYFRAME) return;

    FloatKeys* skew_keys = &anim->tracks.back->skew->keys;
    if (anim->current_frame_time_num)
    {
        VCall(Vec(f32), &skew_keys->values, push_back, *skew_keys->values.back);
        VCall(Vec(f32), &skew_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);
        skew_keys->times_num++;
    }
}
void PreSetTrackTTexture(const PvzAnimation* anim)
{
    ExtResourceKeys* texture_keys = &anim->tracks.back->texture->keys;
    if (!texture_keys->times_num)
    {
        //sprintf_s(anim->tracks->texture->key.values[anim->current_tracks_texture_key_times], NAME_LENGTH, "%s", "null");
    }
}

void PreSetTrackTAlpha(const PvzAnimation* anim)
{
    if (startParam.frameMode == FRAME_MODE_KEYFRAME) return;

    ColorKeys* alpha_keys = &anim->tracks.back->alpha->keys;
    if (anim->current_frame_time_num)
    {
        VCall(Vec(Color), &alpha_keys->values, push_back, *alpha_keys->values.back);
        VCall(Vec(f32), &alpha_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);
        alpha_keys->times_num++;
    }
}

// 确保当前帧存在可写的 keyframe，不存在则用默认值创建
static Vector2* EnsurePosKeyframe(const PvzAnimation* anim)
{
    Vector2Keys* keys = &anim->tracks.back->pos->keys;
    f32 time = 1.0f / FPS * anim->current_frame_time_num;
    if (keys->times_num && fabs(*keys->times.back - time) < 0.0001f)
        return keys->values.back;
    Vector2 v = { 0, 0 };
    VCall(Vec(Vector2), &keys->values, push_back, v);
    VCall(Vec(f32), &keys->times, push_back, time);
    keys->times_num++;
    return keys->values.back;
}
static Vector2* EnsureScaleKeyframe(const PvzAnimation* anim)
{
    Vector2Keys* keys = &anim->tracks.back->scale->keys;
    f32 time = 1.0f / FPS * anim->current_frame_time_num;
    if (keys->times_num && fabs(*keys->times.back - time) < 0.0001f)
        return keys->values.back;
    Vector2 v = { 1, 1 };
    VCall(Vec(Vector2), &keys->values, push_back, v);
    VCall(Vec(f32), &keys->times, push_back, time);
    keys->times_num++;
    return keys->values.back;
}
static f32* EnsureRotKeyframe(const PvzAnimation* anim)
{
    FloatKeys* keys = &anim->tracks.back->rot->keys;
    f32 time = 1.0f / FPS * anim->current_frame_time_num;
    if (keys->times_num && fabs(*keys->times.back - time) < 0.0001f)
        return keys->values.back;
    VCall(Vec(f32), &keys->values, push_back, 0.0f);
    VCall(Vec(f32), &keys->times, push_back, time);
    keys->times_num++;
    return keys->values.back;
}
static f32* EnsureSkewKeyframe(const PvzAnimation* anim)
{
    FloatKeys* keys = &anim->tracks.back->skew->keys;
    f32 time = 1.0f / FPS * anim->current_frame_time_num;
    if (keys->times_num && fabs(*keys->times.back - time) < 0.0001f)
        return keys->values.back;
    VCall(Vec(f32), &keys->values, push_back, 0.0f);
    VCall(Vec(f32), &keys->times, push_back, time);
    keys->times_num++;
    return keys->values.back;
}
static f32* EnsureAlphaKeyframe(const PvzAnimation* anim)
{
    ColorKeys* keys = &anim->tracks.back->alpha->keys;
    f32 time = 1.0f / FPS * anim->current_frame_time_num;
    if (keys->times_num && fabs(*keys->times.back - time) < 0.0001f)
        return &keys->values.back->a;
    Color c = { 1, 1, 1, 1 };
    VCall(Vec(Color), &keys->values, push_back, c);
    VCall(Vec(f32), &keys->times, push_back, time);
    keys->times_num++;
    return &keys->values.back->a;
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

    Text(new_content, &anim, start_param);

    anim->current_frame_time_num++;
}
void SetF(const PvzAnimation* anim, const char* new_content)
{
    BoolKeys* vis_keys = &anim->tracks.back->vis->keys;

    bool new_value = false;
    switch (atoi(new_content))
    {
        case -1:    new_value = false;    break;
        case  0:    new_value = true;     break;
        default:    break;
    }

    if (anim->current_frame_time_num) // 如果当前帧时间不为0
    {
        const f32 last_time = *vis_keys->times.back;
        if (fabs(last_time - 1.0 / FPS * (anim->current_frame_time_num - 1)) > 0.0001)
        {
            VCall(Vec(bool), &vis_keys->values, push_back, *vis_keys->values.back);
            VCall(Vec(f32), &vis_keys->times, push_back, 1.0 / FPS * (anim->current_frame_time_num - 1));
            vis_keys->times_num++;
        }
        VCall(Vec(bool), &vis_keys->values, push_back, new_value);
        VCall(Vec(f32), &vis_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);
        vis_keys->times_num++;
    }
    else
    {
        *vis_keys->values.front = new_value;
    }
}
void SetX(const PvzAnimation* anim, const char* new_content)
{
    EnsurePosKeyframe(anim)->x = (float)atof(new_content);
}
void SetY(const PvzAnimation* anim, const char* new_content)
{
    EnsurePosKeyframe(anim)->y = (float)atof(new_content);
}
void SetSx(const PvzAnimation* anim, const char* new_content)
{
    EnsureScaleKeyframe(anim)->x = (float)atof(new_content);
}
void SetSy(const PvzAnimation* anim, const char* new_content)
{
    EnsureScaleKeyframe(anim)->y = (float)atof(new_content);
}
void SetKx(const PvzAnimation* anim, const char* new_content)
{
    f32* rot = EnsureRotKeyframe(anim);
    f32* skew = EnsureSkewKeyframe(anim);

    f32 new_rot_value = (float)atof(new_content) / 180 * PI;

    f32 prev_rot = *rot;
    while (new_rot_value - prev_rot > PI)
        new_rot_value -= 2 * PI;
    while (new_rot_value - prev_rot < -PI)
        new_rot_value += 2 * PI;

    f32 prev_skew = *skew;
    *rot = new_rot_value;
    *skew = prev_skew + prev_rot - new_rot_value;
}
void SetKy(const PvzAnimation* anim, const char* new_content)
{
    f32* rot = EnsureRotKeyframe(anim);
    f32* skew = EnsureSkewKeyframe(anim);

    f32 new_skew_value = (float)fmod(atof(new_content), 360.0) / 180 * PI;
    f32 temp = new_skew_value - *rot;

    while (temp - *skew > PI)
        temp -= 2 * PI;
    while (temp - *skew < -PI)
        temp += 2 * PI;

    *skew = temp;
}
void SetI(PvzAnimation* anim, const char* new_content)
{
    ExtResourceKeys* texture_keys = &anim->tracks.back->texture->keys;
    char temp[NAME_LENGTH] = { 0 };
    int res_filename_index = 0;
    if (texture_keys->times_num && anim->current_frame_time_num == 0)
    {
        VCall(Vec(i32), &texture_keys->values, pop_back);
        VCall(Vec(f32), &texture_keys->times, pop_back);
        texture_keys->times_num--;
    }

    f32 last_time;

    if (texture_keys->times_num && (last_time = *texture_keys->times.back, true) &&
        fabs(last_time - 1.0 / FPS * (anim->current_frame_time_num - 1)) > 0.0001)
    {
        VCall(Vec(i32), &texture_keys->values, push_back, *texture_keys->values.back);
        VCall(Vec(f32), &texture_keys->times, push_back, 1.0 / FPS * (anim->current_frame_time_num - 1));
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
    for (res_filename_index = 0;
        res_filename_index < anim->texture_filename_times && strcmp(anim->texture_filename[res_filename_index], temp) != 0;
        res_filename_index++)
    {
    }
    if (res_filename_index == anim->texture_filename_times)
    {
        sprintf_s(anim->texture_filename[res_filename_index], NAME_LENGTH, "%s", temp);
        anim->texture_filename_times++;
    }

    VCall(Vec(i32), &texture_keys->values, push_back, res_filename_index);
    VCall(Vec(f32), &texture_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);

    texture_keys->times_num++;
}

void SetA(const PvzAnimation* anim, const char* new_content)
{
    *EnsureAlphaKeyframe(anim) = (float)atof(new_content);
}

void SetBm(const PvzAnimation* anim, const char* new_content)
{
    BlendModeKeys* blend_mode_keys = &anim->tracks.back->blend_mode->keys;
    if (strcmp(new_content, "normal") == 0)
        VCall(Vec(BlendMode), &blend_mode_keys->values, push_back, BLEND_MODE_NORMAL);
    else if (strcmp(new_content, "add") == 0)
        VCall(Vec(BlendMode), &blend_mode_keys->values, push_back, BLEND_MODE_ADD);
    VCall(Vec(f32), &blend_mode_keys->times, push_back, 1.0 / FPS * anim->current_frame_time_num);
    blend_mode_keys->times_num++;
}

void SetInitValue(PvzAnimation* anims[], const int anim_index)
{
    #define INIT_TRACK(track, type, ...) \
        do {if (anim_index) VCall(Vec(type), &current_anim_tracks->track->keys.values, push_back, *first_anim_tracks->track->keys.values.back); \
            else VCall(Vec(type), &current_anim_tracks->track->keys.values, push_back, ((type)__VA_ARGS__)); \
            VCall(Vec(f32), &current_anim_tracks->track->keys.times, push_back, 0); \
            current_anim_tracks->track->keys.times_num++; } while(0)

    const PvzTracks* first_anim_tracks = anims[0]->tracks.back;
    const PvzTracks* current_anim_tracks = anims[anim_index]->tracks.back;

    INIT_TRACK(vis, bool, true);
    INIT_TRACK(pos, Vector2, { 0, 0 });
    INIT_TRACK(scale, Vector2, { 1, 1 });
    INIT_TRACK(rot, f32, 0);
    INIT_TRACK(skew, f32, 0);
    INIT_TRACK(alpha, Color, { 1, 1, 1, 1 });

    // 纹理特殊处理（逻辑不同，独立保留）
    int zero_fuck_index = -1;
    if (first_anim_tracks->texture->keys.values.size)
    {
        zero_fuck_index = *first_anim_tracks->texture->keys.values.back;
    }
    if (zero_fuck_index != -1)
    {
        SetI(anims[anim_index], anims[0]->texture_filename[zero_fuck_index]);
    }
    else
    {
        VCall(Vec(i32), &current_anim_tracks->texture->keys.values, push_back, -1);
        VCall(Vec(f32), &current_anim_tracks->texture->keys.times, push_back, 0);
        current_anim_tracks->texture->keys.times_num++;
    }
    #undef INIT_TRACK
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
int Tap(const char* input, int* offset, char* tap_name, char* content)
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
    for (i = 0; (temp = input[*offset]) != '>'; i++, (*offset)++)    tap_name[i] = temp;
    tap_name[i] = '\0';
    (*offset)++;
    int k = 0;
    for (int j = 1; temp = input[*offset]; j++, (*offset)++)
    {
        if (temp == '<')
            k = j;
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
void SeekAnim(const char* old_content, PvzAnimation* pvz_animations[], R2GAStartParam* start_param)
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
    while (Tap(old_content, &offset, tap_name, new_content) != -1 && is_track_anim_finished == false)
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
                strncpy(res_file_name, start_param->animNameSpecified ? start_param->animName : start_param->inputFileName, NAME_LENGTH - 1);
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
void Text(char* old_content, PvzAnimation* pvz_animations[], R2GAStartParam* start_param)
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

    while (Tap(old_content, &offset, tap_name, new_content) != -1)
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
                SetTrack(pvz_animations[anim_index], start_param);
            }
            Text(new_content, pvz_animations, start_param);
            for (anim_index = 0; anim_index <= anim_nums; anim_index++)
            {
                PvzAnimation* anim = pvz_animations[anim_index];

                ExtResourceKeys* texture_keys = &anim->tracks.back->texture->keys;
                // 如果当前帧时间大于结束帧时间，则跳过
                if (texture_keys->times_num != 0)
                {
                    // 复制最后一个值
                    VCall(Vec(i32), &texture_keys->values, push_back, *texture_keys->values.back);
                    VCall(Vec(f32), &texture_keys->times, push_back, 1.0 / FPS * (anim->current_frame_time_num - 1));
                    texture_keys->times_num++;
                }
                BoolKeys* vis_keys = &anim->tracks.back->vis->keys;
                // 如果当前帧时间大于结束帧时间，则跳过
                if (vis_keys->times_num != 0)
                {
                    // 复制最后一个值
                    VCall(Vec(i32), &vis_keys->values, push_back, *vis_keys->values.back);
                    VCall(Vec(f32), &vis_keys->times, push_back, 1.0 / FPS * (anim->current_frame_time_num - 1));
                    vis_keys->times_num++;
                }
                //FileWriteTracks(anim, start_param);
                //anim->vptr->PrintTracksToFile(anim, start_param);
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

        if (!strcmp(tap_name, dictionary[A_INDEX]/*a*/))
        {
            SetA(pvz_animations[0], new_content);
            continue;
        }

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
    printf("  " COL_OPT "-fm, --frame-mode" COL_RESET " " COL_VAL "<帧模式>" COL_RESET "              " "设置帧模式 (可选: " COL_VAL "inherit, keyframe" COL_RESET ", 默认: " COL_VAL "inherit" COL_RESET ")\n");
    printf("  " COL_OPT "-h, --help" COL_RESET "                           " "显示此帮助信息\n\n");

    printf(COL_HEADER "输出模式:" COL_RESET "\n");
    printf("  " COL_VAL "auto" COL_RESET "                                 " "输出一个tscn文件和多个tres文件，并自动关联（推荐）（默认）\n");
    printf("  " COL_VAL "tscn_by_anim" COL_RESET "                         " "输出一个tscn文件，包含所有动画资源\n");
    printf("  " COL_VAL "anim_tres" COL_RESET "                            " "输出多个tres文件，每个tres文件包含一个动画资源\n\n");

    printf(COL_HEADER "插值模式:" COL_RESET "\n");
    printf("  " COL_VAL "nearest" COL_RESET "                              " "最近邻插值\n");
    printf("  " COL_VAL "linear" COL_RESET "                               " "线性插值\n");
    printf("  " COL_VAL "cubic" COL_RESET "                                " "三次方插值" COL_RESET "\n\n");

    printf(COL_HEADER "帧模式:" COL_RESET "\n");
    printf("  " COL_VAL "inherit" COL_RESET "                              " "空字段继承上一帧的值（默认）\n");
    printf("  " COL_VAL "keyframe" COL_RESET "                             " "空字段留空，由引擎插值" COL_RESET "\n");
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

    if (startParam.configFileSpecified && startParam.configFileWholePath[0] != '\0')
    {
        if (StartParamSetFromConfig(&startParam, startParam.configFileWholePath) == Result_Failed)
        {
            PrintParamError();
            return ErrorCode_CannotParseConfigFile;
        }
    }

    FileOpen(&fp_input, startParam.inputFileWholePath, "r", ErrorCode_CannotOpenInputFile);

    // 初始化pvz_animations指针数组
    PvzAnimation* pvz_animations[MAX_ANIM_NUM];

    for (int i = 0; i < MAX_ANIM_NUM; i++)
    {
        pvz_animations[i] = PvzAnimation_New();
        if (i == 0) PvzAnimation_Init(pvz_animations[i], "all", i);
        else        PvzAnimation_Init(pvz_animations[i], "null", i);
    }

    //FileGetFileNameWithoutExt(argv[1], pvz_animations[0]->res_file_name);
    strncpy(pvz_animations[0]->res_file_name, startParam.animNameSpecified ? startParam.animName : startParam.inputFileName, NAME_LENGTH);

    pvz_animations[0]->start_frame_time = 0;
    pvz_animations[0]->end_frame_time = MAX_TIMES_NUM - 1;

    // 获取文件大小
    const size_t file_size = FileGetSize(fp_input);

    Vec(byte) file_text_vec;
    Create(Vec(byte), &file_text_vec);
    VCall(Vec(byte), &file_text_vec, reserve, file_size + 1);
    char* file_text = (char*)file_text_vec.data;

    // 读取输入文件内容
    FileRead(fp_input, file_text);

    SeekAnim(file_text, pvz_animations, &startParam);
    printf_s("debug: 共有%d个动画\n\n", anim_nums);
    for (int i = 0; i <= anim_nums; i++)
    {
        printf_s("debug: 第%d个动画的资源名为%s\n", i, pvz_animations[i]->res_file_name);
        printf_s("debug: 第%d个动画的起始帧时间为%d\n", i, pvz_animations[i]->start_frame_time);
        printf_s("debug: 第%d个动画的结束帧时间为%d\n\n", i, pvz_animations[i]->end_frame_time);
    }

    ResourceFile* resource_files[MAX_ANIM_NUM];
    for (int i = 0; i <= anim_nums; i++)
    {
        if (i == 0)
        {
            resource_files[i] = (ResourceFile*)Tscn_New(pvz_animations[i], pvz_animations, anim_nums);
        }
        else
        {
            resource_files[i] = (ResourceFile*)Tres_New(pvz_animations[i]);
        }
    }

    for (int i = 0; i <= anim_nums; i++)
    {
        ResourceFile* file = resource_files[i];
    }
    IsBlendModeEnabled(file_text, &startParam);

    // 处理文件内容并写入输出文件
    Text(file_text, pvz_animations, &startParam);

    for (int i = 0; i <= anim_nums; i++)
    {
        ResourceFile* file = resource_files[i];

        // 统一条件：i==0时排除 AnimTres模式，i>0时排除 TscnByAnim模式
        if ((i == 0 && startParam.outputMode != OutputMode_AnimTres) ||
            (i > 0 && startParam.outputMode != OutputMode_TscnByAnim))
        {
            VCall(ResourceFile, file, OpenOutputFile,
                startParam.outputFileSpecified ? startParam.outputFilePath : startParam.inputFilePath);
            VCall(ResourceFile, file, PrintExtResource, &startParam);
            VCall(ResourceFile, file, PrintSetAnim, FPS);
            VCall(ResourceFile, file, PrintTracks, &startParam);

            if (i == 0) VCall(Tscn, file, PrintAddNode, &startParam);
        }
    }

    for (int i = 0; i < MAX_ANIM_NUM; i++)
    {
        PvzAnimation* anim = pvz_animations[i];
        anim->vptr->Delete(anim);
    }
    for (int i = 0; i <= anim_nums; i++)
    {
        ResourceFile* file = resource_files[i];
        file->vptr->Delete(file);
    }

    // 释放内存
    free(file_text);
    return 0;
}