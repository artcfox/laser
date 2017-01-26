#include "PCM_mouse_down.inc"
#include "PCM_mouse_up.inc"
#include "PCM_tada.inc"

//bass instrument - gameover melody
const char bass_gameover[] PROGMEM = {
0,	PC_WAVE,		8,
0,	PC_TREMOLO_RATE,	1,
0,	PC_TREMOLO_LEVEL,	64,
0,	PC_ENV_VOL,	52,
25,	PC_ENV_SPEED,	-1,
10,	PATCH_END,		0,
};

//bass instrument - title melody
const char bass_title[] PROGMEM = {
0,	PC_WAVE,		4,
0,	PC_TREMOLO_RATE,	222,
0,	PC_TREMOLO_LEVEL,	128,
0,	PC_ENV_VOL,		33,
4,	PC_ENV_SPEED,		-1,
2,	PC_ENV_SPEED,		-2,
0,	PC_WAVE,		8,
14,	PATCH_END,		0,
};

//lead instrument - title melody
const char lead_title[] PROGMEM = {
0,	PC_WAVE,		4,
0,	PC_TREMOLO_RATE,	222,
0,	PC_TREMOLO_LEVEL,	128,
0,	PC_ENV_VOL,		28,
4,	PC_ENV_SPEED,		-2,
0,	PC_WAVE,		8,
14,	PATCH_END,		0,
};

const struct PatchStruct patches[] PROGMEM = {
{0,NULL,lead_title,0,0},
{0,NULL,bass_title,0,0},
{0,NULL,bass_gameover,0,0},
// For the below to work, the last 2 bytes of the PCM_Data arrays need to be 0
{2,PCM_mouse_down,NULL,sizeof(PCM_mouse_down)-2,sizeof(PCM_mouse_down)-1},
{2,PCM_mouse_up,NULL,sizeof(PCM_mouse_up)-2,sizeof(PCM_mouse_up)-1},
{2,PCM_tada,NULL,sizeof(PCM_tada)-2,sizeof(PCM_tada)-1},
};

