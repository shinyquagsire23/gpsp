#include "_3ds.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include "font.h"
#include "../common.h"

char main_path[512];
char *file_ext[] = { ".gba", ".bin", NULL };

struct FileHandle {
    Handle handle;
    size_t head;
};

struct DirStruct {
    Handle handle;
    struct dirent activeEntry;
};

FS_archive sdmcArchive;
u8* fileBuffer;

u8* screenTopLeft;
u8* screenTopRight;
u8* screenBottom;

char fs_cwd[MAX_FILENAME_LEN];
struct DirStruct dir;

u32 random_skip = 0;
u32 fps_debug = 0;

u32 frameskip_value = 2;

u64 last_frame_interval_timestamp;

u32 skip_next_frame = 0;

u32 frameskip_counter = 0;

u32 cpu_ticks = 0;
u32 frame_ticks = 0;

u32 execute_cycles = 960;
s32 video_count = 960;
u32 ticks;

u32 arm_frame = 0;
u32 thumb_frame = 0;
u32 last_frame = 0;

u32 cycle_memory_access = 0;
u32 cycle_pc_relative_access = 0;
u32 cycle_sp_relative_access = 0;
u32 cycle_block_memory_access = 0;
u32 cycle_block_memory_sp_access = 0;
u32 cycle_block_memory_words = 0;
u32 cycle_dma16_words = 0;
u32 cycle_dma32_words = 0;
u32 flush_ram_count = 0;
u32 gbc_update_count = 0;
u32 oam_update_count = 0;

u32 synchronize_flag = 1;

u32 update_backup_flag = 1;
u8 exit_time = 0;
u8 has_ninjhax = 0;

u16 *screen_buffer;

void gpsp_plat_init(void)
{
	// Initialize services
	srvInit();			// mandatory
	aptInit();			// mandatory
	hidInit(NULL);	// input (buttons, screen)
	gfxInit();			// graphics
	has_ninjhax = !hbInit();			//ninjhax magics	

	//if(has_ninjhax)
		ninjhax_handlememory();

	gfxSetDoubleBuffering(GFX_TOP, 0);
	gfxSetDoubleBuffering(GFX_BOTTOM, 0);
	gfxSetScreenFormat(GFX_TOP, GSP_RGB5_A1_OES);
	gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB5_A1_OES);
	gfxWriteFramebufferInfo(GFX_TOP);
	gfxWriteFramebufferInfo(GFX_BOTTOM);

	fsInit();
	sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);

    strcpy(fs_cwd, "");
    dir.handle = 0;
    fs_chdir("/");

	screen_buffer = linearAlloc(240*160*4);

	gspWaitForVBlank(); //wait to let the app register itself
}

/*
	u32* test = memalign(0x1000, 0x10000);

	HB_ReprotectMemory(test, 10, 7, &result);
	*((u32*)test) = 0xE12FFF11; //bx r1
	HB_FlushInvalidateCache();
	void (*test7_func)(char* str, void* printaddr) = test;
	test7_func("test7 success\n", (void*)&printf);
*/

void ninjhax_handlememory()
{
	int result = 0;

	HB_ReprotectMemory(rom_translation_cache, ROM_TRANSLATION_CACHE_SIZE / 4096, 7, &result);
	HB_ReprotectMemory(ram_translation_cache, RAM_TRANSLATION_CACHE_SIZE / 4096, 7, &result);
	HB_ReprotectMemory(bios_translation_cache, BIOS_TRANSLATION_CACHE_SIZE / 4096, 7, &result);

	//Test to see if we can run stuff
	*((u32*)rom_translation_cache) = 0xE12FFF11; //bx r1
	*((u32*)ram_translation_cache) = 0xE12FFF11; //bx r1
	*((u32*)bios_translation_cache) = 0xE12FFF11; //bx r1
	HB_FlushInvalidateCache();
	void (*test_func)(char* str, void* printaddr) = rom_translation_cache;
	test_func("success", (void*)&printf);
	void (*test_func2)(char* str, void* printaddr) = ram_translation_cache;
	test_func2("success", (void*)&printf);
	void (*test_func3)(char* str, void* printaddr) = bios_translation_cache;
	test_func3("success", (void*)&printf);
}

void gpsp_plat_quit(void)
{
	  	// Exit services
	fsExit();
 	hbExit();
	gfxExit();
	hidExit();
	aptExit();
	srvExit();
}

void *fb_flip_screen(void)
{

}

void fb_wait_vsync(void)
{
}

void fb_set_mode(int w, int h, int buffers, int scale,int filter, int filter2)
{
	  
}

#define color16(red, green, blue)                                             \
  ((red & 0x1F) << 11) | ((green & 0x1F) << 6) | ((blue & 0x1F) << 1)                                           \


int main(int argv, char** argc)
{
	char bios_filename[512];
	int ret;

	init_gamepak_buffer();
	getcwd(main_path, 512);

	gpsp_plat_init();
	//load_config_file();

	gamepak_filename[0] = 0;

	init_video();

	sprintf(bios_filename, "/gba/%s", "gba_bios.bin");
	ret = load_bios(bios_filename);
	if (ret != 0)
	  ret = load_bios("/gba_bios.bin");
	if (ret != 0)
	{
	  	gui_action_type gui_action = CURSOR_NONE;

		//Init screen buffers
		screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
		screenTopRight = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
		screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 

		/* Clear Screen */
		clearScreen(screenBottom, GFX_BOTTOM,color16(2, 4, 10));
		clearScreen(screenTopLeft, GFX_TOP,color16(2, 4, 10)); 
	  	char* errorMsg[256];
		sprintf(errorMsg, "                                                      \nSorry, but gpSP requires a Gameboy Advance BIOS       \nimage to run correctly. Make sure to get an           \nauthentic one, it'll be exactly 16384 bytes large     \nand should have the following md5sum value:           \n                                                      \na860e8c0b6d573d191e4ec7db1b1e4f6                      \n                                                      \nWhen you do get it name it gba_bios.bin and put it    \nin sdmc:/gba/ .                                       \n                                                      \nPress any button to exit.                             ");

		print_string(errorMsg, 0xFFFF, color16(2, 4, 10), 0, 0);
		gfxFlushBuffers();
		gfxSwapBuffers();

		while(gui_action == CURSOR_NONE)
		{
  			gui_action = get_gui_input();
		}

	  	debug_screen_end();
	  	gpsp_plat_quit();
	  	return 0;
	}

	if(bios_rom[0] != 0x18)
  	{
		gui_action_type gui_action = CURSOR_NONE;

		//Init screen buffers
		screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
		screenTopRight = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
		screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 

		/* Clear Screen */
		clearScreen(screenBottom, GFX_BOTTOM,color16(2, 4, 10));
		clearScreen(screenTopLeft, GFX_TOP,color16(2, 4, 10)); 
          char* errorMsg[256];
		sprintf(errorMsg,"You have an incorrect BIOS image.                     \nWhile many games will work fine, some will not. It    \nis strongly recommended that you obtain the           \ncorrect BIOS file. Do NOT report any bugs if you      \nare seeing this message.                              \n                                                      \nPress any button to resume, at your own risk.         \n");

		//debug_screen_update();
		print_string(errorMsg, 0xFFFF, color16(2, 4, 10), 0, 0);
		gfxFlushBuffers();
		gfxSwapBuffers();

		while(gui_action == CURSOR_NONE)
		{
  			gui_action = get_gui_input();
		}
  	}

	init_main();
	init_sound(1);
	sound_on = 0; //For now

	init_input();

	char load_filename[512];
	//switch_to_romdir();
	if(load_file(file_ext, load_filename) == -1)
	{
	  menu(copy_screen());
	}
	else
	{
	  if(load_gamepak(load_filename) == -1)
	  {
	  	char error[256];
       	sprintf(error, "Failed to load gamepak\n%s,\npress any key to exit.\n", load_filename);
		print_string(error, 0xFFFF, 0x0, 0, 0);
		gfxFlushBuffers();
		gfxSwapBuffers();
		while(1)
		{
			gspWaitForVBlank();
			hidScanInput();
			u32 kDown = hidKeysDown();
			u32 kHeld = hidKeysHeld();
		
			if (kDown){
				break;
			}
		}
		gpsp_plat_quit();
		return 0;
	  }

	  set_clock_speed();
	  set_gba_resolution(screen_scale);
	  video_resolution_small();

	  init_cpu();
	  init_memory();
	}

	last_frame = 0;

 	/*if(argc > 2)
  	{
    		current_debug_state = COUNTDOWN_BREAKPOINT;
    		breakpoint_value = strtol(argv[2], NULL, 16);
  	}*/

  	trigger_ext_event();
	
	//if(has_ninjhax)
  		execute_arm_translate(execute_cycles); //ninjhax dynrec
  	//execute_arm(execute_cycles);



	// Main loop
	while (aptMainLoop())
	{
		// Wait next screen refresh
		gspWaitForVBlank();

		// Read which buttons are currently pressed 
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		
		// If START is pressed, break loop and quit
		if (kDown & KEY_START){
			break;
		}
		
		draw();
	}
	
	gpsp_plat_quit();
	
	// Return to hbmenu
	return 0;
}

void draw(void)
{
	//Init screen buffers
	screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
	screenTopRight = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
	screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 

	/* Clear Screen */
	clearScreen(screenBottom, GFX_BOTTOM,color16(2, 4, 10));
	clearScreen(screenTopLeft, GFX_TOP,color16(2, 4, 10)); 
	//clearScreen(screenTopRight, GFX_TOP);

	/*char str[256];
	"lol: %02x", bios_rom[0]);
	debug_screen_update();
	debug_screen_clear();*/
	//gfxDrawText(GFX_BOTTOM, GFX_LEFT, &fontPurple, str, 30, 30);

	// Flush and swap framebuffers
	gfxFlushBuffers();
	gfxSwapBuffers();
}

u8 *loadROM()
{
	Handle fileHandle;
	char* path = "/gba_bios.bin";
	FS_path filePath;
	filePath.type = PATH_CHAR;
	filePath.size = strlen(path) + 1;
	filePath.data = (u8*)path;
	u8 *buffer;

	Result ret = FSUSER_OpenFile(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(!ret)
	{
		u64 size;
		u32 bytesRead;
		ret = FSFILE_GetSize(fileHandle, &size);
		if(ret)
			return;	

		buffer = linearAlloc(size);
		if(!fileBuffer)
			return;

		ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, buffer, size);
		if(ret || size != bytesRead)
			return;

		ret = FSFILE_Close(fileHandle);

		// Copy all the data we need to the DCPU memory 
		// (only 0x10000 words worth, the rest of the file is ignored)
		//memcpy(&DCPU_Mem[0], &fileBuffer[0], 0x10000*2);
	}
	return buffer;
}

struct dirent* fs_readdir() {
    u32 numEntries = 0;
    FS_dirent entry;
    FSDIR_Read(dir.handle, &numEntries, 1, &entry);

    if (numEntries == 0)
        return 0;

    int i = 0;
    int add = 0;
    for (i = 0; i < 256-add; i++) 
    {
        dir.activeEntry.d_name[i+add] = (char)entry.name[i];
        /*if(entry.name[i] == ' ')
        {
            dir.activeEntry.d_name[i+add] = '\\';
            add++;
            dir.activeEntry.d_name[i+add] = ' ';
        }*/
        if (dir.activeEntry.d_name[i] == '\0')
            break;
    }
    dir.activeEntry.d_type = 0;
    if (entry.isDirectory)
        dir.activeEntry.d_type |= DT_DIR;

    return &dir.activeEntry;
}

void fs_chdir(const char* s) {
    char buffer[256];
    fs_relativePath(buffer, s);

    if (dir.handle != 0)
        FSDIR_Close(dir.handle);

    Result res = FSUSER_OpenDirectory(NULL, &dir.handle, sdmcArchive,
            FS_makePath(PATH_CHAR, buffer));

    if (res) {
        FSUSER_OpenDirectory(NULL, &dir.handle, sdmcArchive,
                FS_makePath(PATH_CHAR, fs_cwd));
        return;
    }

    strcpy(fs_cwd, buffer);
}

void fs_relativePath(char* dest, const char* src) {
    bool back = false;
    if (strcmp(src, "..") == 0 || strcmp(src, "../") == 0) {
        if (dest != fs_cwd)
            strcpy(dest, fs_cwd);
        back = true;
    }
    else if (src[0] == '/')
        strcpy(dest, src);
    else {
        if (dest != fs_cwd)
            strcpy(dest, fs_cwd);
        if (dest[strlen(dest)-1] != '/')
            strcat(dest, "/");
        strcat(dest, src);
    }

    while (strlen(dest) > 1 &&
            strrchr(dest, '/') != 0 && strrchr(dest, '/') == dest+strlen(dest)-1)
        *strrchr(dest, '/') = '\0';

    if (back) {
        if (strrchr(dest, '/') != 0) {
            *(strrchr(dest, '/')) = '\0';
            if (strcmp(dest, "") == 0)
                strcpy(dest, "/");
        }
    }
}

char* fs_getfilelist(int *nfiles)
{
	FS_path dirPath = (FS_path){PATH_CHAR, 6, (u8*)"/"};
	FS_dirent entry;
	int i;
	
	FSUSER_OpenDirectory(NULL, &dir.handle, sdmcArchive, dirPath);
	*nfiles = 0;
	for (;;)
	{
		u32 nread = 0;
		FSDIR_Read(dir.handle, &nread, 1, &entry);
		if (!nread) break;
		//if (!IsGoodFile(&entry)) continue;
		*nfiles++;
	}
	FSDIR_Close(dir.handle);

	char *filelist = (char*)linearAlloc(0x106 * (*nfiles));
	
	// TODO: find out how to rewind it rather than reopening it?
	FSUSER_OpenDirectory(NULL, &dir.handle, sdmcArchive, dirPath);
	i = 0;
	for (;;)
	{
		u32 nread = 0;
		FSDIR_Read(dir.handle, &nread, 1, &entry);
		if (!nread) break;
		//if (!IsGoodFile(&entry)) continue;
		
		// dirty way to copy an Unicode string
		strncpy(&filelist[0x106 * i], entry.name, 0x105);
		i++;
	}
	FSDIR_Close(dir.handle);
	//filelist[0] = "l";
	return filelist;
}

