#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LENTH 300
#define PATH_LENTH 500
#define MAX_TIMES_NUM 10000
#define MAX_TEXTURE_NUM 10000
#define MAX_TRACKS_NUM 10000
#define PI 3.1415926

//转换器

char* dictionary[] = 
{
	"fps", "track", "name",

	"t", "f", "i", "x", "y",

	"sx", "sy", "kx", "ky", "i"
};

int FPS;
int time_num = 0;
int tracks_num = 0;
int tracks_vis_key_times = 0;
int tracks_pos_key_times = 0;
int tracks_rot_key_times = 0;
int tracks_scale_key_times = 0;
int tracks_skew_key_times = 0;
int tracks_texture_key_times = 0;
int ARG_NUM;

char filename_fuck[MAX_TEXTURE_NUM][NAME_LENTH];
int filename_fuck_times = 0;
char name[MAX_TRACKS_NUM][NAME_LENTH];


bool flag_x = false;
bool flag_sx = false;
bool flag_kx = false;
bool flag_kx2 = false;
bool flag_ky = false;
bool flag_ky2 = false;
bool tap_is_t = false;

typedef struct Key
{
	float times[MAX_TIMES_NUM];
	float transitions[MAX_TIMES_NUM];
	int update;
	char values[MAX_TIMES_NUM][NAME_LENTH];
}Key;

typedef struct Tracks
{
	int num;
	char type[NAME_LENTH];
	bool imported ;
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
	Tracks tracks_vis;
	Tracks tracks_pos;
	Tracks tracks_rot;
	Tracks tracks_scale;
	Tracks tracks_skew;
	Tracks tracks_texture;
}PVZTracks;

void InitTracks(Tracks* track)
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
	track->interp = 1;

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
	track->key.update = 0;
}

void InitPVZTracks(PVZTracks* pvz_tracks)
{
	//Init name
	for (int i = 0; i < NAME_LENTH; i++)
	{
		pvz_tracks->name[i] = '\0';
	}
	//Init length
	pvz_tracks->length = 0;
	InitTracks(&(pvz_tracks->tracks_vis));
	pvz_tracks->tracks_vis.key.update = 1;
	InitTracks(&(pvz_tracks->tracks_pos));
	InitTracks(&(pvz_tracks->tracks_rot));
	InitTracks(&(pvz_tracks->tracks_scale));
	InitTracks(&(pvz_tracks->tracks_skew));
	InitTracks(&(pvz_tracks->tracks_texture));
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

void readfile(FILE* input, char* filetext)
{
	for (int i = 0; (filetext[i] = fgetc(input)) != EOF; i++);
}

void write_track_to_file(FILE* output, Tracks* track, int times)
{
	fprintf(output, "tracks/%d/type = \"%s\"\n",           track->num, track->type);
	fprintf(output, "tracks/%d/imported = %s\n",           track->num, track->imported  ? "true" : "false");
	fprintf(output, "tracks/%d/enabled = %s\n",            track->num, track->enabled   ? "true" : "false");
	fprintf(output, "tracks/%d/path = NodePath(\"%s\")\n", track->num, track->path);
	fprintf(output, "tracks/%d/interp = %d\n",             track->num, track->interp);
	fprintf(output, "tracks/%d/loop_wrap = %s\n",          track->num, track->loop_wrap ? "true" : "false");
	fprintf(output, "tracks/%d/keys = {\n", track->num);
		fprintf(output, "\"times\": PackedFloat32Array(");
		for (int i = 0; i < times; i++)
		{
			fprintf(output, "%f", track->key.times[i]);
			if (i != times - 1)
			{
				fprintf(output, ", ");
			}
		}
		fprintf(output, "),\n");
		fprintf(output, "\"transitions\": PackedFloat32Array(");
		for (int i = 0; i < times; i++)
		{
			fprintf(output, "%f", track->key.transitions[i]);
			if (i != times - 1)
			{
				fprintf(output, ", ");
			}
		}
		fprintf(output, "),\n");
		fprintf(output, "\"update\": %d,\n", track->key.update);
		fprintf(output, "\"values\": [");
		for (int i = 0; i < times; i++)
		{
			fprintf(output, "%s", track->key.values[i]);
			if (i != times - 1)
			{
				fprintf(output, ", ");
			}
		}
		fprintf(output, "]\n");
	fprintf(output, "}\n");
}

void write_to_file(FILE* output, PVZTracks* pvz_tracks)
{
	write_track_to_file(output, &(pvz_tracks->tracks_vis), tracks_vis_key_times);
	write_track_to_file(output, &(pvz_tracks->tracks_pos), tracks_pos_key_times);
	write_track_to_file(output, &(pvz_tracks->tracks_rot), tracks_rot_key_times);
	write_track_to_file(output, &(pvz_tracks->tracks_scale), tracks_scale_key_times);
	write_track_to_file(output, &(pvz_tracks->tracks_skew), tracks_skew_key_times);
	write_track_to_file(output, &(pvz_tracks->tracks_texture), tracks_texture_key_times);
	fflush(output);
}
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
			if (temp == '>' && '\0' == tap_name[j - k -2])
			{
				(*offset)++;
				content[k-1] = '\0';
				return 0;
			}
			else
			{
				k = 0;
			}
		}
		content[j-1] = temp;
	}
	return 1;
}

void text(char* old_content, FILE* output, PVZTracks* pvz_tracks)
{
	int offset = 0;
	char tap_name[20] = {'\0'};
	char* new_content = (char*)calloc(50000, sizeof(char));

	if (new_content == NULL)
	{
		
		fprintf(stderr, "text_content = NULL\n");
		return;
	}
	
	while(tap(old_content, &offset, tap_name, new_content) != -1)
	{
		
		if (!strcmp(tap_name, dictionary[0]/*fps*/))
		{
			FPS = atoi(new_content);
		}
		if (!strcmp(tap_name, dictionary[1]/*track*/))
		{
			time_num = 0;
            pvz_tracks->tracks_vis.num     = tracks_num * 6 + 0;
            pvz_tracks->tracks_pos.num     = tracks_num * 6 + 1;
            pvz_tracks->tracks_rot.num     = tracks_num * 6 + 2;
            pvz_tracks->tracks_scale.num   = tracks_num * 6 + 3;
            pvz_tracks->tracks_skew.num    = tracks_num * 6 + 4;
            pvz_tracks->tracks_texture.num = tracks_num * 6 + 5;
			time_num = 0;
            tracks_vis_key_times     = 0;
            tracks_pos_key_times     = 0;
            tracks_rot_key_times     = 0;
            tracks_scale_key_times   = 0;
            tracks_skew_key_times    = 0;
			tracks_texture_key_times = 0;
			text(new_content, output, pvz_tracks);
			write_to_file(output, pvz_tracks);
			tracks_num++;
			continue;
		}
		if (!strcmp(tap_name, dictionary[2]/*name*/))
		{
			sprintf_s(pvz_tracks->name, NAME_LENTH, new_content);
			pvz_tracks->name[0] &= 0b1011111;
			for (int i = 0; i < NAME_LENTH; i++)
			{
				if (pvz_tracks->name[i] == '.')
					pvz_tracks->name[i] = '_';
			}
			sprintf_s(pvz_tracks->tracks_vis.path, PATH_LENTH, "%s:visible", pvz_tracks->name);
			sprintf_s(pvz_tracks->tracks_pos.path, PATH_LENTH, "%s:position", pvz_tracks->name);
			sprintf_s(pvz_tracks->tracks_rot.path, PATH_LENTH, "%s:rotation", pvz_tracks->name);
			sprintf_s(pvz_tracks->tracks_scale.path, PATH_LENTH, "%s:scale", pvz_tracks->name);
			sprintf_s(pvz_tracks->tracks_skew.path, PATH_LENTH, "%s:skew", pvz_tracks->name);
			sprintf_s(pvz_tracks->tracks_texture.path, PATH_LENTH, "%s:texture", pvz_tracks->name);
			sprintf_s(name[tracks_num], NAME_LENTH, "%s", pvz_tracks->name);
		}
		if (!strcmp(tap_name, dictionary[3]/*t*/))
		{
			if (time_num == 0)
			{
				sprintf_s(pvz_tracks->tracks_vis.key.values[tracks_vis_key_times], NAME_LENTH, "true");
				pvz_tracks->tracks_vis.key.times[tracks_vis_key_times] = 0;
				tracks_vis_key_times++;

			}

			
			if (tracks_pos_key_times)
			{
				sprintf_s(pvz_tracks->tracks_pos.key.values[tracks_pos_key_times], NAME_LENTH, pvz_tracks->tracks_pos.key.values[tracks_pos_key_times - 1]);
			}
			else
			{
				sprintf_s(pvz_tracks->tracks_pos.key.values[tracks_pos_key_times], NAME_LENTH, "Vector2(%6.2f, %6.2f)", 0.0, 0.0);
			}
			tracks_pos_key_times++;
			pvz_tracks->tracks_pos.key.times[tracks_pos_key_times - 1] = (float)(1.0 / FPS * time_num);

			
			if (tracks_scale_key_times)
			{
				sprintf_s(pvz_tracks->tracks_scale.key.values[tracks_scale_key_times], NAME_LENTH, pvz_tracks->tracks_scale.key.values[tracks_scale_key_times - 1]);
			}
			else
			{
				sprintf_s(pvz_tracks->tracks_scale.key.values[tracks_scale_key_times], NAME_LENTH, "Vector2(1.000, 1.000)");
				flag_sx = true;
			}
			tracks_scale_key_times++;
			pvz_tracks->tracks_scale.key.times[tracks_scale_key_times - 1] = (float)(1.0 / FPS * time_num);

			if (tracks_rot_key_times)
			{
				sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times], NAME_LENTH, pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 1]);
				
			}
			else
			{
				sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times], NAME_LENTH, "%10.6Lf", 0.0 / 180 * PI);
				flag_kx = true;
			}
			tracks_rot_key_times++;
			pvz_tracks->tracks_rot.key.times[tracks_rot_key_times - 1] = (float)(1.0 / FPS * time_num);

			if (tracks_skew_key_times)
			{
				sprintf_s(pvz_tracks->tracks_skew.key.values[tracks_skew_key_times], NAME_LENTH, pvz_tracks->tracks_skew.key.values[tracks_skew_key_times - 1]);
			}
			else
			{
				sprintf_s(pvz_tracks->tracks_skew.key.values[tracks_skew_key_times], NAME_LENTH, "%10.7Lf", 0.0 / 180 * PI);
			}
			tracks_skew_key_times++;
			pvz_tracks->tracks_skew.key.times[tracks_skew_key_times - 1] = (float)(1.0 / FPS * time_num);

			text(new_content, output, pvz_tracks);
			flag_x = false;
			flag_sx = false;
			flag_kx = false;
			flag_ky = false;
			time_num++;
		}
		
		if (!strcmp(tap_name, dictionary[4]/*f*/))
		{
			if (time_num == 0)
			{
				tracks_vis_key_times--;
			}
			switch (atoi(new_content))
			{
				case -1:
					sprintf_s(pvz_tracks->tracks_vis.key.values[tracks_vis_key_times], NAME_LENTH, "false");
					break;
				case 0:
					sprintf_s(pvz_tracks->tracks_vis.key.values[tracks_vis_key_times], NAME_LENTH, "true");
					break;
			}
			pvz_tracks->tracks_vis.key.times[tracks_vis_key_times] = (float)(1.0 / FPS * time_num);
			tracks_vis_key_times++;
		}

		
		if (!strcmp(tap_name, dictionary[6]/*x*/))
		{
			char temp[NAME_LENTH];
			if (!tracks_pos_key_times)
			{
				tracks_pos_key_times++;
				sprintf_s(temp, NAME_LENTH, "Vector2(%6.2f, %6.2f)", 0.0, 0.0);
				arrayprintf_s(pvz_tracks->tracks_pos.key.values[tracks_pos_key_times - 1], NAME_LENTH, temp);
				flag_x = true;
			}
			sprintf_s(temp, NAME_LENTH, "Vector2(%6.2Lf, ", atof(new_content));
			arrayprintf_s(pvz_tracks->tracks_pos.key.values[tracks_pos_key_times - 1], NAME_LENTH, temp);
			pvz_tracks->tracks_pos.key.times[tracks_pos_key_times - 1] = (float)(1.0 / FPS * time_num);
			
			
		}
		if (!strcmp(tap_name, dictionary[7]/*y*/))
		{

			if (!tracks_pos_key_times)
			{
				tracks_pos_key_times++;
			}
			
			sprintf_s(pvz_tracks->tracks_pos.key.values[tracks_pos_key_times - 1] + 14, NAME_LENTH, ", %6.2Lf)", atof(new_content));
			pvz_tracks->tracks_pos.key.times[tracks_pos_key_times - 1] = (float)(1.0 / FPS * time_num);
			
		}
		

		
		if (!strcmp(tap_name, dictionary[8]/*sx*/))
		{
			char temp[NAME_LENTH];
			if (tracks_scale_key_times)
			{
				sprintf_s(temp, NAME_LENTH, "Vector2(%5.3Lf, ", atof(new_content));
			}
			else
			{
				sprintf_s(temp, NAME_LENTH, "Vector2(%5.3Lf, 1.000)", atof(new_content));
				tracks_scale_key_times++;
				flag_sx = true;
			}
			arrayprintf_s(pvz_tracks->tracks_scale.key.values[tracks_scale_key_times - 1], NAME_LENTH, temp);
			pvz_tracks->tracks_scale.key.times[tracks_scale_key_times - 1] = (float)(1.0 / FPS * time_num);
			
		}
		if (!strcmp(tap_name, dictionary[9]/*sy*/))
		{
			if (tracks_scale_key_times)
			{
				sprintf_s(pvz_tracks->tracks_scale.key.values[tracks_scale_key_times - 1] + 13, NAME_LENTH, ", %5.3Lf)", atof(new_content));
			}
			else
			{
				sprintf_s(pvz_tracks->tracks_scale.key.values[tracks_scale_key_times], NAME_LENTH, "Vector2(1.000, %5.3Lf)", atof(new_content));
				tracks_scale_key_times++;
			}
			pvz_tracks->tracks_scale.key.times[tracks_scale_key_times - 1] = (float)(1.0 / FPS * time_num);

		}
		
		
		if (!strcmp(tap_name, dictionary[10]/*kx*/))
		{
			if (!tracks_rot_key_times)
			{
				tracks_rot_key_times++;
			}
			double temp = atof(new_content) / 180 * PI;
			//if (flag_ky)
			sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 1], NAME_LENTH, "%10.6Lf", temp);
			pvz_tracks->tracks_rot.key.times[tracks_rot_key_times - 1] = (float)(1.0 / FPS * time_num);
			flag_kx = true;
			flag_kx2 = true;

		}

		
		if (!strcmp(tap_name, dictionary[11]/*ky*/) || flag_kx)
		{
			
			double temp = atof(new_content) / 180 * PI 
				- atof(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times-1]);
			if (flag_kx)
			{
				flag_kx = false;
				float last_skew, last_rot;
				if (tracks_skew_key_times == 1)
				{
					last_skew = 0;
				}
				else
				{
					last_skew = atof(pvz_tracks->tracks_skew.key.values[tracks_skew_key_times - 2]);
					
				}
				if (tracks_rot_key_times == 1)
				{
					last_rot = 0;
				}
				else
				{
					last_rot = atof(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 2]);
				}
				temp = last_skew 
					+ last_rot
					- atof(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 1]);
			}
			else
			{
				if (!flag_kx2)
				{
					//sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 1], NAME_LENTH, "%10.6Lf", atof(new_content) / 180 * PI);
					//temp = 0;
				}
			}
			sprintf_s(pvz_tracks->tracks_skew.key.values[tracks_skew_key_times - 1], NAME_LENTH, "%10.7Lf", temp);
			pvz_tracks->tracks_skew.key.times[tracks_skew_key_times - 1] = (float)(1.0 / FPS * time_num);
			flag_ky2 = true;
			flag_ky = true;
		}
		

        if (!strcmp(tap_name, dictionary[12]/*i*/) && ARG_NUM == 4)
        {
            // 提取 IMAGE_REANIM_XXX 中的 XXX 并将除首字母外的其他字符转为小写字母
            if (strncmp(new_content, "IMAGE_REANIM_", 13) == 0) {
                char temp[NAME_LENTH] = {0};
                int i = 0;
                temp[0] = new_content[13]; // 保留首字母
                for (i = 14; new_content[i] != '\0'; i++) {
                    temp[i - 13] = tolower(new_content[i]);
                }
                temp[i - 13] = '\0';
                strncat_s(temp, NAME_LENTH, ".png", _TRUNCATE);
                // 这里可以使用 temp 进行后续操作
				for (i = 0; i < filename_fuck_times; i++)
				{
					if (strcmp(filename_fuck[i], temp) == 0)
					{
						sprintf_s(pvz_tracks->tracks_texture.key.values[tracks_texture_key_times], NAME_LENTH, "ExtResource(\"%d_fuck\")", i);
						break;
					}
				}
				if (i == filename_fuck_times)
				{
					sprintf_s(filename_fuck[i], NAME_LENTH, "%s", temp);
					sprintf_s(pvz_tracks->tracks_texture.key.values[tracks_texture_key_times], NAME_LENTH, "ExtResource(\"%d_fuck\")", i);
					filename_fuck_times++;
				}
				pvz_tracks->tracks_texture.key.times[tracks_texture_key_times] = (float)(1.0 / FPS * time_num);
				tracks_texture_key_times++;
            }
        }

	}

	free(new_content);
}

void ext_resource(FILE* output, const char* path)
{
	for (int i = 0; i < filename_fuck_times; i++)
	{
		fprintf_s(output, "[ext_resource type=\"Texture2D\" path=\"%s/%s\" id=\"%d_fuck\"]\n", path, filename_fuck[i], i);
	}
	fprintf_s(output, "\n");
	fflush(output);
}

void set_anim(FILE* output)
{
	fprintf_s(output, "[sub_resource type=\"Animation\" id=\"Animation_fuck\"]\n");
	fprintf_s(output, "length = %.6Lf\n", (float)(time_num - 1) * (1.0 / FPS));
	fprintf_s(output, "step = %.6Lf\n", 1.0 / FPS);
	fflush(output);
}

void add_node(FILE* output, int tracks_num)
{
	fprintf_s(output, "[sub_resource type=\"AnimationLibrary\" id=\"AnimationLibrary_fuck\"]\n_data = {\n\"FUCK_YOU\": SubResource(\"Animation_fuck\")\n}\n\n");
	fprintf_s(output, "[node name=\"Node2D\" type=\"Node2D\"]\n\n");
	for (int i = 0; i < tracks_num; i++)
	{
		fprintf_s(output, "[node name=\"%s\" type=\"Sprite2D\" parent=\".\"]\n", name[i]);
		fprintf_s(output, "centered = false\n\n");
	}
	fprintf_s(output, "[node name=\"FUCKYOU\" type=\"AnimationPlayer\" parent=\".\"]\nlibraries = {\n\"\": SubResource(\"AnimationLibrary_fuck\")\n}\n\n");
	fflush(output);
}

void merge_files(FILE* output_file, FILE* input_files[], int num_files) {
	for (int i = 0; i < num_files; i++) {
		FILE* input_file = input_files[i];
		if (input_file == NULL) {
			fprintf(stderr, "Input file %d is NULL\n", i);
			continue;
		}
		// 重置输入文件指针到文件开头
		fseek(input_file, 0, SEEK_SET);
		int ch;
		while ((ch = fgetc(input_file)) != EOF)
		{
			fputc(ch, output_file);
		}
	}
}

FILE* fp_input;
FILE* fp_output;
FILE* fp_first_output;
FILE* fp_second_output;
FILE* fp_third_output;
FILE* fp_forth_output;


int main(int argc, char* argv[])
{
	// 检查命令行参数数量是否为3
	if (argc != 3 && argc != 4)
	{
		fprintf(stderr, "Usage: %s <input_file> <output_file> [path]\n", argv[0]);
		return 1;
	}
	ARG_NUM = argc;

	// 打开输入文件
	errno_t err = fopen_s(&fp_input, argv[1], "r");
	if (err != 0 || fp_input == NULL)
	{
		printf(argv[1]);
		fprintf(stderr, "fp_input = NULL\n");
		return 2;
	}

	// 打开输出文件
	err = fopen_s(&fp_output, argv[2], "w");
	if (err != 0 || fp_output == NULL)
	{
		fprintf(stderr, "fp_output = NULL\n");
		return 3;
	}

	// 创建两个输出文件名
	char str_first_output[NAME_LENTH];
	char str_second_output[NAME_LENTH];
	char str_third_output[NAME_LENTH];
	char str_forth_output[NAME_LENTH];

	// 将argv[2]和"first"写入str_first_output
	sprintf_s(str_first_output, NAME_LENTH, "%s_first.jiema", argv[2]);
	// 将argv[2]和"second"写入str_second_output
	sprintf_s(str_second_output, NAME_LENTH, "%s_second.jiema", argv[2]);
	// 将argv[2]和"third"写入str_third_output
	sprintf_s(str_third_output, NAME_LENTH, "%s_third.jiema", argv[2]);
	// 将argv[2]和"forth"写入str_forth_output
	sprintf_s(str_forth_output, NAME_LENTH, "%s_forth.jiema", argv[2]);

	// 打开第一个输出文件
	err = fopen_s(&fp_first_output, str_first_output, "w+");
	if (err != 0 || fp_first_output == NULL)
	{
		fprintf(stderr, "fp_first_output = NULL\n");
		return 4;
	}
	// 打开第二个输出文件
	err = fopen_s(&fp_second_output, str_second_output, "w+");
	if (err != 0 || fp_second_output == NULL)
	{
		fprintf(stderr, "fp_second_output = NULL\n");
		return 5;
	}

	// 打开第三个输出文件
	err = fopen_s(&fp_third_output, str_third_output, "w+");
	if (err != 0 || fp_third_output == NULL)
	{
		fprintf(stderr, "fp_third_output = NULL\n");
		return 6;
	}

	// 打开第四个输出文件
	err = fopen_s(&fp_forth_output, str_forth_output, "w+");
	if (err != 0 || fp_forth_output == NULL)
	{
		fprintf(stderr, "fp_forth_output = NULL\n");
		return 7;
	}

	// 分配并初始化PVZTracks结构体
	PVZTracks* pvz_tracks = (PVZTracks*)malloc(sizeof(PVZTracks));
	InitPVZTracks(pvz_tracks);

	// 分配50MB的内存用于读取文件内容
	char* filetext = (char*)calloc(50 * 1024 * 1024, sizeof(char));
	// 读取输入文件内容
	readfile(fp_input, filetext);
	// 处理文件内容并写入输出文件
	text(filetext, fp_third_output, pvz_tracks);
	fprintf_s(fp_third_output, "\n");

	ext_resource(fp_first_output, argv[3]);
	set_anim(fp_second_output);
	add_node(fp_forth_output, tracks_num);
	if (argc == 3)
	{
	
		FILE* input_files[1] = { fp_third_output };
		// 重置输出文件指针到文件开头
		fseek(fp_output, 0, SEEK_SET);
		merge_files(fp_output, input_files, 1);
	}
	else
	{
		FILE* input_files[4] = { fp_first_output, fp_second_output, fp_third_output, fp_forth_output };
		// 重置输出文件指针到文件开头
		fseek(fp_output, 0, SEEK_SET);
		merge_files(fp_output, input_files, 4);
	}
	
    
    //fprintf(fp_output, "length = %.6Lf\n", (float)time_num * (1.0 / FPS));
    //fprintf(fp_output, "step = %.6Lf\n", 1.0 / FPS);

    // 关闭文件
    fclose(fp_input);
    fclose(fp_output);
    fclose(fp_first_output);
    fclose(fp_third_output);
    fclose(fp_forth_output);

    return 0;
}