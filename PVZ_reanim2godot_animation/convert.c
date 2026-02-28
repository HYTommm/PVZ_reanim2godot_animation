#include <malloc.h>
#include <stdio.h>
#include <ctype.h>

#include <string.h>
#include <stdlib.h>
#include "FileIO.h"
#include "convert.h"

#include "PvzAnimation.h"
#include "PvzReanim.h"

//void InitTrack(Track* track, R2GAStartParam* start_param)
//{
//    //Init num
//    track->num = 0;
//
//    //Init type
//    for (int i = 0; i < NAME_LENGTH; i++)
//    {
//        track->type[i] = '\0';
//    }
//    sprintf_s(track->type, NAME_LENGTH, "value");
//
//    //Init imported
//    track->imported = false;
//
//    //Init enabled
//    track->enabled = true;
//
//    //Init path
//    for (int i = 0; i < PATH_LENGTH; i++)
//    {
//        track->path[i] = '\0';
//    }
//
//    //Init interp
//    track->interp = start_param->interpolationMode;
//
//    //Init loop_wrap
//    track->loop_wrap = true;
//
//    //Init key
//    for (int i = 0; i < MAX_TIMES_NUM; i++)
//    {
//        track->base.keys.times[i] = 0;
//        track->key.transitions[i] = 1.0;
//        for (int j = 0; j < NAME_LENGTH; j++)
//        {
//            track->key.values[i][j] = '\0';
//        }
//    }
//    track->key.update = start_param->updateMode;
//}