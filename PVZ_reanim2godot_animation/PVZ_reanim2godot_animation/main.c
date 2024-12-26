#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LENTH 300
#define PATH_LENTH 500
#define TIME_LENTH 1000
#define PI 3.1415926

//转换器

char* dictionary[] = 
{
	"fps", "track", "name",

	"t", "f", "i", "x", "y",

	"sx", "sy", "kx", "ky"
};

int FPS;
int time_num = 0;
int tracks_num = 0;
int tracks_vis_key_times = 0;
int tracks_pos_key_times = 0;
int tracks_rot_key_times = 0;
int tracks_scale_key_times = 0;
int tracks_skew_key_times = 0;

bool flag_x = false;
bool flag_sx = false;
bool flag_kx = false;

typedef struct Key
{
	float times[TIME_LENTH];
	float transitions[TIME_LENTH];
	int update;
	char values[TIME_LENTH][NAME_LENTH];
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

	//Init key
	for (int i = 0; i < TIME_LENTH; i++)
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
		
		fprintf(stderr, "text_content = NULL");
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
			pvz_tracks->tracks_vis.num   = tracks_num * 5 + 0;
			pvz_tracks->tracks_pos.num   = tracks_num * 5 + 1;
			pvz_tracks->tracks_rot.num   = tracks_num * 5 + 2;
			pvz_tracks->tracks_scale.num = tracks_num * 5 + 3;
			pvz_tracks->tracks_skew.num  = tracks_num * 5 + 4;
			time_num = 0;
			tracks_vis_key_times = 0;
			tracks_pos_key_times = 0;
			tracks_rot_key_times = 0;
			tracks_scale_key_times = 0;
			tracks_skew_key_times = 0;
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
		}
		if (!strcmp(tap_name, dictionary[3]/*t*/))
		{
			if (tracks_pos_key_times)
			{
				sprintf_s(pvz_tracks->tracks_pos.key.values[tracks_pos_key_times], NAME_LENTH, pvz_tracks->tracks_pos.key.values[tracks_pos_key_times - 1]);
				tracks_pos_key_times++;
				pvz_tracks->tracks_pos.key.times[tracks_pos_key_times - 1] = (float)(1.0 / FPS * time_num);
			}
			if (tracks_scale_key_times)
			{
				sprintf_s(pvz_tracks->tracks_scale.key.values[tracks_scale_key_times], NAME_LENTH, pvz_tracks->tracks_scale.key.values[tracks_scale_key_times - 1]);
				tracks_scale_key_times++;
				pvz_tracks->tracks_scale.key.times[tracks_scale_key_times - 1] = (float)(1.0 / FPS * time_num);
			}
			if (tracks_rot_key_times)
			{
				sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times], NAME_LENTH, pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 1]);
				tracks_rot_key_times++;
				pvz_tracks->tracks_rot.key.times[tracks_rot_key_times - 1] = (float)(1.0 / FPS * time_num);
			}
			if (tracks_skew_key_times)
			{
				sprintf_s(pvz_tracks->tracks_skew.key.values[tracks_skew_key_times], NAME_LENTH, pvz_tracks->tracks_skew.key.values[tracks_skew_key_times - 1]);
				tracks_skew_key_times++;
				pvz_tracks->tracks_skew.key.times[tracks_skew_key_times - 1] = (float)(1.0 / FPS * time_num);
			}
			text(new_content, output, pvz_tracks);
			flag_x = false;
			flag_sx = false;
			flag_kx = false;
			time_num++;
		}
		if (!strcmp(tap_name, dictionary[4]/*f*/))
		{
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
			sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times - 1], NAME_LENTH, "%10.6Lf", temp);
			pvz_tracks->tracks_rot.key.times[tracks_rot_key_times - 1] = (float)(1.0 / FPS * time_num);
			flag_kx = true;
		}

		
		if (!strcmp(tap_name, dictionary[11]/*ky*/))
		{
			if (!tracks_rot_key_times)
			{
				sprintf_s(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times], NAME_LENTH, "%10.6Lf", 0.0/180 * PI);
				tracks_rot_key_times++;
				pvz_tracks->tracks_rot.key.times[tracks_rot_key_times - 1] = (float)(1.0 / FPS * time_num);
			}
			if (!tracks_skew_key_times)
			{
				tracks_skew_key_times++;
			}
			double temp = atof(new_content) / 180 * PI 
				- atof(pvz_tracks->tracks_rot.key.values[tracks_rot_key_times-1]);
			sprintf_s(pvz_tracks->tracks_skew.key.values[tracks_skew_key_times-1], NAME_LENTH, "%10.7Lf", temp);
			pvz_tracks->tracks_skew.key.times[tracks_skew_key_times - 1] = (float)(1.0 / FPS * time_num);
			
		}
		
	}

}

int main(int argc, char* argv[])
{
	if (argc != 3)
		return 1;

	FILE* fp_input = fopen(argv[1], "r");
	if (fp_input == NULL)
	{
		printf(argv[1]);
		fprintf(stderr, "fp_input = NULL\n");
		return 2;
	}

	FILE* fp_output = fopen(argv[2], "w");
	if (fp_output == NULL)
	{
		fprintf(stderr, "fp_output = NULL\n");
		return 3;
	}

	PVZTracks* pvz_tracks = (PVZTracks*)malloc(sizeof(PVZTracks));
	InitPVZTracks(pvz_tracks);

	char* filetext = (char*)calloc(5000000, sizeof(char));
	readfile(fp_input, filetext);
	text(filetext, fp_output, pvz_tracks);

	fseek(fp_output, 0, SEEK_SET);
	//fprintf(fp_output, "length = %.6Lf\n", (float)time_num * (1.0 / FPS));
	//fprintf(fp_output, "step = %.6Lf\n", 1.0 / FPS);
	fclose(fp_input);
	fclose(fp_output);

	return 0;
}