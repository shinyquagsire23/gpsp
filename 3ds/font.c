#include <3ds.h>
#include "font.h"
#include "font_bin.h" 
#include "gpspFont_bin.h"
font_s fontDefault =
{
	gpspFont_bin,
	font1Desc,
	14,
	(u8[]){0xFF,0xFF,0xFF}
};

font_s fontGreen =
{
	gpspFont_bin,
	font1Desc,
	14,
	(u8[]){104,160,144}
};

font_s fontPurple =
{
	gpspFont_bin,
	font1Desc,
	14,
	(u8[]){176,144,208}
};

font_s fontBlue =
{
	gpspFont_bin,
	font1Desc,
	14,
	(u8[]){133,163,194}
};

font_s fontBlack =
{
	font1Data,
	font1Desc,
	14,
	(u8[]){0x00,0x00,0x00}
};

font_s fontWhiteHeader =
{
	font1Data,
	font1Desc,
	24,
	(u8[]){0xFF,0xFF,0xFF}
};
