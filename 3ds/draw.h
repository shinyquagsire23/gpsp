#pragma once

#define TOP_ASPECT_X 0x5
#define TOP_ASPECT_Y 0x3
#define TOP_HEIGHT 240
#define TOP_WIDTH 400
#define BOTTOM_HEIGHT 240
#define BOTTOM_WIDTH 320

#include <3ds/types.h>
#include <3ds/gfx.h>
#include <stdio.h>
#include "font.h"

int drawCharacter(u8* fb, font_s* f, char c, s16 x, s16 y, u16 w, u16 h);
void drawString2(u8* fb, font_s* f, char* str, s16 x, s16 y, u16 w, u16 h, u32 bg, u32 fg);
void gfxDrawText(gfxScreen_t screen, gfx3dSide_t side, u16 fg, u16 bg, font_s* f, char* str, s16 x, s16 y);
void clearScreen(u16* screen,gfxScreen_t screenPos, u16 color);
void drawPixel_(int x, int y, char r, char g, char b, u8* screen);
void drawPixel(int x, int y, u16 color, u16* screen);
void drawChar(char letter,int x,int y, char r, char g, char b, u8* screen);
void drawString(char* word, int x,int y, char r, char g, char b, u8* screen,gfxScreen_t screenPos);
void drawLine( int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen);
void drawRect( int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen);
void drawFillRect( int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen);
void drawCircle(int x, int y, int radius, char r, char g, char b, u8* screen);
void drawFillCircle(int x, int y, int radius, char r , char g, char b, u8* screen);
void drawCircleCircum(int xc, int yc, int x, int y, char r, char g, char b, u8* screen);

void gfxFadeScreen(gfxScreen_t screen, gfx3dSide_t side, u32 f);
void gfxDrawSprite(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y);
void gfxDrawDualSprite(u8* spriteData, u16 width, u16 height, s16 x, s16 y);
void gfxDrawSpriteAlpha(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y);
void gfxDrawSpriteAlphaBlend(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y);
void gfxDrawSpriteAlphaBlendFade(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y, u8 fadeValue);
