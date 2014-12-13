#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "font.h"
#include "draw.h"
#include "ascii64.h"
#include "gpspFont_bin.h"

void clearScreenFast(u8* screen,gfxScreen_t screenPos)
{
	int width;
	int height=240;

	switch(screenPos){
	case GFX_BOTTOM:
		width=BOTTOM_WIDTH;
		break;
	default:
		width=TOP_WIDTH;
		break;
	}

	int i, j;
	for(i=0;i<width*height;i++)
	{
		*(u32*)(&screen[0]+i) = 0;
	}
}

void clearScreen(u16* screen,gfxScreen_t screenPos, u16 color)
{
	int width;
	int height=240;

	switch(screenPos){
	case GFX_BOTTOM:
		width=BOTTOM_WIDTH;
		break;
	default:
		width=TOP_WIDTH;
		break;
	}

	int i, j;
	for(i=0;i<width;i++)
	{
		for(j=0;j<height;j++)
		{
			drawPixel(i,j,color,screen);
		}
	}
	drawPixel(0,0,0x1F,screen);
	drawPixel(0,1,0x1F<<5,screen);
	drawPixel(0,2,0x1F<<10,screen);
	drawPixel(1,0,0x1F<<1,screen);
	drawPixel(1,1,0x1F<<6,screen);
	drawPixel(1,2,0x1F<<11,screen);
}

void drawPixel_(int x, int y, char r, char g, char b, u8* screen)
{
	drawPixel(x,y,r,screen);
}

void drawPixel(int x, int y, u16 color, u16* screen)
{
	int height=240;
	
	u32 v=((y)+x*height);
	screen[v]=0x1214;
}


void drawChar(char letter,int x,int y, char r, char g, char b, u8* screen)
{
	int i, k;
	unsigned char mask;
	unsigned char l;

	for (i = 0; i < 8; i++){
		mask = 0b10000000;
		l = asciiData[(int)letter][i];
		for (k = 0; k < 8; k++){
			if ((mask >> k) & l){
				drawPixel_(k+x,i+y,r,g,b,screen);
			}     
		}
	}
}

void drawString(char* word, int x,int y, char r, char g, char b, u8* screen,gfxScreen_t screenPos)
{
	int tmp_x =x;
	int i;
	int line = 0;

	int width;

	switch(screenPos){
	case GFX_BOTTOM:
		width=BOTTOM_WIDTH;
		break;
	default:
		width=TOP_WIDTH;
		break;
	}

	for (i = 0; i < (signed)strlen(word); i++){

		if (tmp_x+8 > width) {
			line++;
			tmp_x = x;
		}
		drawChar(word[i],tmp_x,y+(line*8),r,g,b, screen);

		tmp_x = tmp_x+8;
	}
}


/*
drawLine - H_line and V_line only
*/

void drawLine( int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen)
{

	int x, y;
	if (x1 == x2){
		if (y1<y2) for (y = y1; y < y2; y++) drawPixel_(x1,y,r,g,b,screen);
		else for (y = y2; y < y1; y++) drawPixel_(x1,y,r,g,b,screen);
	} else {
		if (x1<x2) for (x = x1; x < x2; x++) drawPixel_(x,y1,r,g,b,screen);
		else for (x = x2; x < x1; x++) drawPixel_(x,y1,r,g,b,screen);
	}
}

void drawRect( int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen)
{
	drawLine( x1, y1, x2, y1, r, g, b, screen);
	drawLine( x2, y1, x2, y2, r, g, b, screen);
	drawLine( x1, y2, x2, y2, r, g, b, screen);
	drawLine( x1, y1, x1, y2, r, g, b, screen);
}

void drawFillRect( int x1, int y1, int x2, int y2, char r, char g, char b, u8* screen)
{
	int X1,X2,Y1,Y2,i,j;

	if (x1<x2){ 
		X1=x1;
		X2=x2;
	} else { 
		X1=x2;
		X2=x1;
	} 

	if (y1<y2){ 
		Y1=y1;
		Y2=y2;
	} else { 
		Y1=y2;
		Y2=y1;
	} 
	for(i=X1;i<=X2;i++){
		for(j=Y1;j<=Y2;j++){
			drawPixel_(i,j, r, g, b, screen);
		}
	}
}

void drawCircle(int xCen, int yCen, int radius, char r, char g, char b, u8* screen)
{
	int x = 0;
	int y = radius;
	int p = (5 - radius*4)/4;
	drawCircleCircum(xCen, yCen, x, y, r, g, b, screen);
	while(x < y){
		x++;
		if(p < 0){
			p += 2*x+1;
		}
		else{
			y--;
			p += 2*(x-y)+1;
		}
		drawCircleCircum(xCen, yCen, x, y, r, g, b, screen);
	}
}

void drawFillCircle(int xCen, int yCen, int radius, char r, char g, char b, u8* screen)
{
	drawCircle(xCen, yCen, radius, r, g, b, screen);
	int x,y;
	for(y = -radius;y <= radius;y++){
		for(x = -radius;x <= radius;x++)
			if(x*x + y*y <= radius*radius + radius * .8f)
				drawPixel_(xCen + x, yCen + y, r, g, b, screen);
	}
}

void drawCircleCircum(int cx, int cy, int x, int y, char r, char g, char b, u8* screen)
{

	if(x == 0){
		drawPixel_(cx, cy + y, r, g, b, screen);
		drawPixel_(cx, cy - y, r, g, b, screen);
		drawPixel_(cx + y, cy, r, g, b, screen);
		drawPixel_(cx - y, cy, r, g, b, screen);
	}
	if(x == y){
		drawPixel_(cx + x, cy + y, r, g, b, screen);
		drawPixel_(cx - x, cy + y, r, g, b, screen);
		drawPixel_(cx + x, cy - y, r, g, b, screen);
		drawPixel_(cx - x, cy - y, r, g, b, screen);
	}
	if(x < y){
		drawPixel_(cx + x, cy + y, r, g, b, screen);
		drawPixel_(cx - x, cy + y, r, g, b, screen);
		drawPixel_(cx + x, cy - y, r, g, b, screen);
		drawPixel_(cx - x, cy - y, r, g, b, screen);
		drawPixel_(cx + y, cy + x, r, g, b, screen);
		drawPixel_(cx - y, cy + x, r, g, b, screen);
		drawPixel_(cx + y, cy - x, r, g, b, screen);
		drawPixel_(cx - y, cy - x, r, g, b, screen);
	}
}

int drawCharacter2(u16* fb, font_s* f, char c, s16 x, s16 y, u16 w, u16 h, u32 bg, u32 fg)
{
	charDesc_s* cd=&f->desc[(int)c];
	if(!cd->data)return 0;

	if(x<0 || x+cd->w>=w || y<-cd->h || y>=h+cd->h)return 0;

	u8* charData=&gpspFont_bin[(c*2*10)];

	int i, j;

	s16 cy=y, ch=16, cyo=0;

	if(y<0)
	{
		cy=0;cyo=-y;
		ch=cd->h-cyo;
	}
	else if(y+ch>h)
		ch=h-y;

	fb+=(x*h+cy);

     for(i = 7; i >=0; i--)
	{
		for(j = 9; j >= 0; j--)
		{
			u8 v=(charData[j*2] & BIT(i));
			if(v)
			{
				fb[0]=fg;
			}
			else //Copy color from 0,0
			{
				fb[0]=bg;//fb_orig[0];
			}
			fb+=1;
			
		}
		fb+=(h)-(10);
	}
	return 6;
}

int drawCharacter(u8* fb, font_s* f, char c, s16 x, s16 y, u16 w, u16 h)
{
	charDesc_s* cd=&f->desc[(int)c];
	if(!cd->data)return 0;
	x+=cd->xo; y+=f->height-cd->yo-cd->h;
	if(x<0 || x+cd->w>=w || y<-cd->h || y>=h+cd->h)return 0;
	u8* charData=cd->data;
	int i, j;
	s16 cy=y, ch=cd->h, cyo=0;
	if(y<0){cy=0;cyo=-y;ch=cd->h-cyo;}
	else if(y+ch>h)ch=h-y;
	fb+=(x*h+cy)*3;
	const u8 r=f->color[0], g=f->color[1], b=f->color[2];
	for(i=0;i<cd->w;i++)
	{
		charData+=cyo;
		for(j=0;j<ch;j++)
		{
			u8 v=*(charData++);
			if(v)
			{
				fb[0]=(fb[0]*(0xFF-v)+(b*v))>>8;
				fb[1]=(fb[1]*(0xFF-v)+(g*v))>>8;
				fb[2]=(fb[2]*(0xFF-v)+(r*v))>>8;
			}
			fb+=3;
		}
		charData+=(cd->h-(cyo+ch));
		fb+=(h-ch)*3;
	}
	return cd->xa;
}

void drawString2(u8* fb, font_s* f, char* str, s16 x, s16 y, u16 w, u16 h, u32 bg, u32 fg)
{
	if(!f || !fb || !str)return;
	int k; int dx=0, dy=0;
	int length=strlen(str);
	for(k=0;k<length;k++)
	{
		//if(str[k] == 'e')
			//dx+=drawCharacter2(fb,f,str[k],x+dx,y+dy,w,h);
		//else
			dx+=drawCharacter2(fb,f,str[k],x+dx,y+dy,w,h,bg,fg);
		if(str[k]=='\n'){dx=0;dy-=10;}
	}
}

void gfxDrawText(gfxScreen_t screen, gfx3dSide_t side, u16 bg, u16 fg, font_s* f, char* str, s16 x, s16 y)
{
	if(!str)return;
	if(!f)f=&fontDefault;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	drawString2(fbAdr, f, str, y, x, fbHeight, fbWidth, bg, fg);
}
void gfxDrawSprite(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	int j;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		memcpy(&fbAdr[((x+xOffset)+(y+j)*fbWidth)*3], &spriteData[((xOffset)+(j)*width)*3], widthDrawn*3);
	}
}

void gfxDrawDualSprite(u8* spriteData, u16 width, u16 height, s16 x, s16 y)
{
	if(!spriteData)return;

	gfxDrawSprite(GFX_TOP, GFX_LEFT, spriteData, width, height, x-240, y);
	gfxDrawSprite(GFX_BOTTOM, GFX_LEFT, spriteData, width, height, x, y-40);
}

void gfxDrawSpriteAlpha(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	//TODO : optimize
	fbAdr+=(y+yOffset)*fbWidth*3;
	spriteData+=yOffset*width*4;
	int j, i;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		u8* fbd=&fbAdr[(x+xOffset)*3];
		u8* data=&spriteData[(xOffset)*4];
		for(i=xOffset; i<xOffset+widthDrawn; i++)
		{
			if(data[3])
			{
				fbd[0]=data[0];
				fbd[1]=data[1];
				fbd[2]=data[2];
			}
			fbd+=3;
			data+=4;
		}
		fbAdr+=fbWidth*3;
		spriteData+=width*4;
	}
}
void gfxDrawSpriteAlphaBlend(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	//TODO : optimize
	fbAdr+=(y+yOffset)*fbWidth*3;
	spriteData+=yOffset*width*4;
	int j, i;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		u8* fbd=&fbAdr[(x+xOffset)*3];
		u8* data=&spriteData[(xOffset)*4];
		for(i=xOffset; i<xOffset+widthDrawn; i++)
		{
			if(data[3])
			{
				u8 alphaSource = data[3];
				fbd[0] = ((data[0] * alphaSource)+(fbd[0] * (255 - alphaSource))) / 256;
				fbd[1] = ((data[1] * alphaSource)+(fbd[1] * (255 - alphaSource))) / 256;
				fbd[2] = ((data[2] * alphaSource)+(fbd[2] * (255 - alphaSource))) / 256;
			}
			fbd+=3;
			data+=4;
		}
		fbAdr+=fbWidth*3;
		spriteData+=width*4;
	}
}
void gfxDrawSpriteAlphaBlendFade(gfxScreen_t screen, gfx3dSide_t side, u8* spriteData, u16 width, u16 height, s16 x, s16 y, u8 fadeValue)
{
	if(!spriteData)return;

	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	if(x+width<0 || x>=fbWidth)return;
	if(y+height<0 || y>=fbHeight)return;

	u16 xOffset=0, yOffset=0;
	u16 widthDrawn=width, heightDrawn=height;

	if(x<0)xOffset=-x;
	if(y<0)yOffset=-y;
	if(x+width>=fbWidth)widthDrawn=fbWidth-x;
	if(y+height>=fbHeight)heightDrawn=fbHeight-y;
	widthDrawn-=xOffset;
	heightDrawn-=yOffset;

	//TODO : optimize
	fbAdr+=(y+yOffset)*fbWidth*3;
	spriteData+=yOffset*width*4;
	int j, i;
	for(j=yOffset; j<yOffset+heightDrawn; j++)
	{
		u8* fbd=&fbAdr[(x+xOffset)*3];
		u8* data=&spriteData[(xOffset)*4];
		for(i=xOffset; i<xOffset+widthDrawn; i++)
		{
			if(data[3])
			{
				u8 alphaSource = (fadeValue * data[3]) / 256;
				fbd[0]=((data[0] * alphaSource) / 256)+((fbd[0] * (255 - alphaSource)) / 256);
				fbd[1]=((data[1] * alphaSource) / 256)+((fbd[1] * (255 - alphaSource)) / 256);
				fbd[2]=((data[2] * alphaSource) / 256)+((fbd[2] * (255 - alphaSource)) / 256);
			}
			fbd+=3;
			data+=4;
		}
		fbAdr+=fbWidth*3;
		spriteData+=width*4;
	}
}

void gfxFadeScreen(gfxScreen_t screen, gfx3dSide_t side, u32 f)
{
	u16 fbWidth, fbHeight;
	u8* fbAdr=gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

	int i; for(i=0; i<fbWidth*fbHeight/2; i++)
	{
		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
		*fbAdr=(*fbAdr*f)>>8;fbAdr++;
	}
}
