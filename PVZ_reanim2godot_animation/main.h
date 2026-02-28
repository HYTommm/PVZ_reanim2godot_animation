#pragma once

int tap(char* input, int* offset, char* tap_name, char* content);

void text(char* old_content, PvzAnimation* pvz_animation[], R2GAStartParam* start_param);
void arrayprintf_s(char* str, int num, char* array);

void SetAnimName(PvzAnimation* pvz_animation, char* new_content);
void SetAnimStartFrameTime(PvzAnimation* pvz_animation, int frame_time_num);
void SetAnimEndFrameTime(PvzAnimation* pvz_animation, int frame_time_num);

//void SetAllTracksNum(PvzAnimation* pvz_animation, int num);// 尚未实现

void SetAnimResName(PvzAnimation* pvz_animation, char* new_content);

void SetFPS(const char* new_content);

void PreSetTrackTVis(const PvzAnimation* anim);

void SetTrackName(PvzAnimation* anim[], int anim_num, const char* new_content);

void SetTrack(PvzAnimation* pvz_animation, char* new_content, const R2GAStartParam* start_param);

void SetAnimKeyTimes(const PvzAnimation* anim, int num);

void PreSetTrackTPos(const PvzAnimation* anim);

void PreSetTrackTScale(const PvzAnimation* anim);

void PreSetTrackTRot(const PvzAnimation* anim);

void PreSetTrackTSkew(const PvzAnimation* anim);

void PreSetTrackTAlpha(const PvzAnimation* anim);

void PreSetTrackTTexture(const PvzAnimation* anim);

void SetTrackT(PvzAnimation* anim, char* new_content, R2GAStartParam* start_param);

void SetF(const PvzAnimation* anim, const char* new_content);

void SetX(const PvzAnimation* anim, const char* new_content);

void SetY(const PvzAnimation* anim, const char* new_content);

void SetSx(const PvzAnimation* anim, const char* new_content);

void SetSy(const PvzAnimation* anim, const char* new_content);

void SetKx(const PvzAnimation* anim, const char* new_content);

void SetKy(const PvzAnimation* anim, const char* new_content);

void SetI(PvzAnimation* anim, const char* new_content);

void SetA(const PvzAnimation* anim, const char* new_content);

void SetBm(const PvzAnimation* anim, const char* new_content);
