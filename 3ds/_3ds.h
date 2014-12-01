#define PLAT_BUTTON_COUNT 22
#define PLAT_KEY_COUNT    22
#define PLAT_MENU_BUTTON -1 // have one hardcoded

void gpsp_plat_init(void);
void gpsp_plat_quit(void);

void *fb_flip_screen(void);
void fb_set_mode(int w, int h, int buffers, int scale, int filter, int filter2);
void fb_wait_vsync(void);
