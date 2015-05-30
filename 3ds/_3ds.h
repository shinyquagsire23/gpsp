#include <3ds.h>

#define PLAT_BUTTON_COUNT 22
#define PLAT_KEY_COUNT    22
#define PLAT_MENU_BUTTON -1 // have one hardcoded

extern FS_archive sdmcArchive;
extern u8* fileBuffer;

extern u8* screenTopLeft;
extern u8* screenTopRight;
extern u8* screenBottom;

extern u32 random_skip;
extern u32 fps_debug;

extern u32 frameskip_value;

extern u64 last_frame_interval_timestamp;

extern u32 skip_next_frame;

extern u32 frameskip_counter;

extern u32 cpu_ticks;
extern u32 frame_ticks;

extern u32 execute_cycles;
s32 video_count;
extern u32 ticks;

extern u32 arm_frame;
extern u32 thumb_frame;
extern u32 last_frame;

extern u32 cycle_memory_access;
extern u32 cycle_pc_relative_access;
extern u32 cycle_sp_relative_access;
extern u32 cycle_block_memory_access;
extern u32 cycle_block_memory_sp_access;
extern u32 cycle_block_memory_words;
extern u32 cycle_dma16_words;
extern u32 cycle_dma32_words;
extern u32 flush_ram_count;
extern u32 gbc_update_count;
extern u32 oam_update_count;

extern u32 synchronize_flag;

extern u32 update_backup_flag;

extern u8 exit_time;
extern u8 has_ninjhax;
extern u8 has_sound;
extern u32 old_stack;
extern u32 return_place;

extern u16 *screen_buffer;

extern u32* flush_all;

void InvalidateEntireInstructionCache(void);
void InvalidateEntireDataCache(void);
int PatchKernel(void);

int svcFlushIcache(u32 offset, u32 length);
int svcFlushDcache(u32 offset, u32 length);

#define MAX_FILENAME_LEN 256

//extern char *file_ext[];

void gpsp_plat_init(void);
void gpsp_plat_quit(void);

void *fb_flip_screen(void);
void fb_set_mode(int w, int h, int buffers, int scale, int filter, int filter2);
void fb_wait_vsync(void);
struct dirent* fs_readdir();
void fs_chdir(const char* s);
void fs_relativePath(char* dest, const char* src);
char* fs_getfilelist();
u8 *loadROM();
