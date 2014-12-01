#include "_3ds.h"

void gpsp_plat_init(void)
{
  
}

void gpsp_plat_quit(void)
{
    
}

void *fb_flip_screen(void)
{
  video_draw(video_buff);
  return video_buff;
}

void fb_wait_vsync(void)
{
}

void fb_set_mode(int w, int h, int buffers, int scale,int filter, int filter2)
{
    
}

