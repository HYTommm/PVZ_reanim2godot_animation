#pragma once

int tap(char* input, int* offset, char* tap_name, char* content);

void text(char* old_content, PVZAnimation* pvz_animation[], R2GAStartParam* start_param);
void arrayprintf_s(char* str, int num, char* array);

void SetAnimName(PVZAnimation* pvz_animation, char* new_content);
void SetAnimStartFrameTime(PVZAnimation* pvz_animation, int frame_time_num);
void SetAnimEndFrameTime(PVZAnimation* pvz_animation, int frame_time_num);

//void SetAllTracksNum(PVZAnimation* pvz_animation, int num);// 尚未实现

void SetAnimResName(PVZAnimation* pvz_animation, char* new_content);

void SetFPS(char* new_content);

void PreSetTrackTVis(PVZAnimation* pvz_animation);

void SetTrackName(PVZAnimation* pvz_animation[], int anim_num, char* new_content);

void SetTrack(PVZAnimation* pvz_animation, char* new_content, R2GAStartParam* start_param);

void SetAnimKeyTimes(PVZAnimation* pvz_animation, int num);

void PreSetTrackTPos(PVZAnimation* pvz_animation);

void PreSetTrackTScale(PVZAnimation* pvz_animation);

void PreSetTrackTRot(PVZAnimation* pvz_animation);

void PreSetTrackTSkew(PVZAnimation* pvz_animation);

void PreSetTrackTAlpha(PVZAnimation* pvz_animation);

void PreSetTrackTTexture(PVZAnimation* pvz_animation);

void SetTrackT(PVZAnimation* pvz_animation, char* new_content, R2GAStartParam* start_param);

void SetF(PVZAnimation* pvz_animation, char* new_content);

void SetX(PVZAnimation* pvz_animation, char* new_content);

void SetX(PVZAnimation* pvz_animation, char* new_content);

void SetY(PVZAnimation* pvz_animation, char* new_content);

void SetSx(PVZAnimation* pvz_animation, char* new_content);

void SetSy(PVZAnimation* pvz_animation, char* new_content);

void SetKx(PVZAnimation* pvz_animation, char* new_content);

void SetKy(PVZAnimation* pvz_animation, char* new_content);

void SetI(PVZAnimation* pvz_animation, char* new_content);

void SetA(PVZAnimation* pvz_animation, char* new_content);

void SetBm(PVZAnimation* pvz_animation, char* new_content);
