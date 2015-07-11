/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"
#include "font.h"

#ifndef _WIN32_WCE

#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>

#endif

#include <stdlib.h> 
#include <stdio.h> 

#ifdef RPI_BUILD
//#include "input.h"
#endif

#define MAX_PATH 1024

// Blatantly stolen and trimmed from MZX (megazeux.sourceforge.net)

#ifdef GP2X_BUILD

#define FILE_LIST_ROWS 20
#define FILE_LIST_POSITION 5
#define DIR_LIST_POSITION 260
#elif defined(_3DS)
#define FILE_LIST_ROWS 18
#define FILE_LIST_POSITION 5
#define DIR_LIST_POSITION (320 * 3 / 4)
#else

#define FILE_LIST_ROWS 25
#define FILE_LIST_POSITION 5
#define DIR_LIST_POSITION (resolution_width * 3 / 4)

#endif

#ifdef PSP_BUILD

#define COLOR_BG            color16(2, 8, 10)

#define color16(red, green, blue)                                             \
  (blue << 10) | (green << 5) | red                                           \

#else

#define COLOR_BG            color16(2, 4, 10)

#define color16(red, green, blue)                                             \
  ((red & 0x1F) << 11) | ((green & 0x1F) << 6) | ((blue & 0x1F) << 1)                                           \

#endif

#define COLOR_ROM_INFO      color16(22, 36, 26)
#define COLOR_ACTIVE_ITEM   color16(31, 31, 31)
#define COLOR_INACTIVE_ITEM color16(13, 40, 18)
#define COLOR_FRAMESKIP_BAR color16(15, 31, 31)
#define COLOR_HELP_TEXT     color16(16, 40, 24)

#ifdef PSP_BUILD
  static const char *clock_speed_options[] =
  {
    "33MHz", "66MHz", "100MHz", "133MHz", "166MHz", "200MHz", "233MHz",
    "266MHz", "300MHz", "333MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = (clock_speed_number + 1) * 33
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed / 33) - 1
#elif defined(POLLUX_BUILD)
  static const char *clock_speed_options[] =
  {
    "300MHz", "333MHz", "366MHz", "400MHz", "433MHz",
    "466MHz", "500MHz", "533MHz", "566MHz", "600MHz",
    "633MHz", "666MHz", "700MHz", "733MHz", "766MHz",
    "800MHz", "833MHz", "866MHz", "900MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = 300 + (clock_speed_number * 3333) / 100
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed - 300) / 33
#elif defined(GP2X_BUILD)
  static const char *clock_speed_options[] =
  {
    "150MHz", "160MHz", "170MHz", "180MHz", "190MHz",
    "200MHz", "210MHz", "220MHz", "230MHz", "240MHz",
    "250MHz", "260MHz", "270MHz", "280MHz", "290MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = 150 + clock_speed_number * 10
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed - 150) / 10
#else
  static const char *clock_speed_options[] =
  {
    "0"
  };
  #define menu_get_clock_speed()
  #define get_clock_speed_number()
#endif


int sort_function(const void *dest_str_ptr, const void *src_str_ptr)
{
  char *dest_str = *((char **)dest_str_ptr);
  char *src_str = *((char **)src_str_ptr);

  if(src_str[0] == '.')
    return 1;

  if(dest_str[0] == '.')
    return -1;

  return strcasecmp(dest_str, src_str);
}

s32 load_file(const char **wildcards, char *result)
{
  DIR *current_dir;
  struct dirent *current_file;
  struct stat file_info;
  char current_dir_name[MAX_PATH];
  char current_dir_short[81];
  u32 current_dir_length;
  u32 total_filenames_allocated;
  u32 total_dirnames_allocated;
  char **file_list;
  char **dir_list;
  u32 num_files;
  u32 num_dirs;
  char *file_name;
  u32 file_name_length;
  u32 ext_pos = -1;
  u32 chosen_file, chosen_dir;
  s32 return_value = 1;
  s32 current_file_selection;
  s32 current_file_scroll_value;
  u32 current_dir_selection;
  u32 current_dir_scroll_value;
  s32 current_file_in_scroll;
  u32 current_dir_in_scroll;
  u32 current_file_number, current_dir_number;
  u32 current_column = 0;
  u32 repeat;
  u32 i;
  gui_action_type gui_action;
  
  chdir("sdmc:/"); //For some reason getcwd returns / instead of sdmc:/ otherwise

  while(return_value == 1)
  {
    current_file_selection = 0;
    current_file_scroll_value = 0;
    current_dir_selection = 0;
    current_dir_scroll_value = 0;
    current_file_in_scroll = 0;
    current_dir_in_scroll = 0;

    total_filenames_allocated = 32;
    total_dirnames_allocated = 32;
    file_list = (char **)malloc(sizeof(char *) * 32);
    dir_list = (char **)malloc(sizeof(char *) * 32);
    memset(file_list, 0, sizeof(char *) * 32);
    memset(dir_list, 0, sizeof(char *) * 32);

    num_files = 0;
    num_dirs = 0;
    chosen_file = 0;
    chosen_dir = 0;

    getcwd(current_dir_name, MAX_PATH);

    current_dir = opendir(current_dir_name);

    do
    {
      if(current_dir)
        current_file = readdir(current_dir);
      else
        current_file = NULL;

      if(current_file)
      {
        file_name = current_file->d_name;
        file_name_length = strlen(file_name);

        if((stat(file_name, &file_info) >= 0) &&
         ((file_name[0] != '.') || (file_name[1] == '.')))
        {
          if(S_ISDIR(file_info.st_mode))
          {
            dir_list[num_dirs] = malloc(file_name_length + 1);

            sprintf(dir_list[num_dirs], "%s", file_name);

            num_dirs++;
          }
          else
          {
            // Must match one of the wildcards, also ignore the .
            if(file_name_length >= 4)
            {
              if(file_name[file_name_length - 4] == '.')
                ext_pos = file_name_length - 4;
              else

              if(file_name[file_name_length - 3] == '.')
                ext_pos = file_name_length - 3;

              else
                ext_pos = 0;

              for(i = 0; wildcards[i] != NULL; i++)
              {
                if(!strcasecmp((file_name + ext_pos),
                 wildcards[i]))
                {
                  file_list[num_files] =
                   malloc(file_name_length + 1);

                  sprintf(file_list[num_files], "%s", file_name);

                  num_files++;
                  break;
                }
              }
            }
          }
        }

        if(num_files == total_filenames_allocated)
        {
          file_list = (char **)realloc(file_list, sizeof(char *) *
           total_filenames_allocated * 2);
          memset(file_list + total_filenames_allocated, 0,
           sizeof(char *) * total_filenames_allocated);
          total_filenames_allocated *= 2;
        }

        if(num_dirs == total_dirnames_allocated)
        {
          dir_list = (char **)realloc(dir_list, sizeof(char *) *
           total_dirnames_allocated * 2);
          memset(dir_list + total_dirnames_allocated, 0,
           sizeof(char *) * total_dirnames_allocated);
          total_dirnames_allocated *= 2;
        }
      }
    } while(current_file);

    qsort((void *)file_list, num_files, sizeof(char *), sort_function);
    qsort((void *)dir_list, num_dirs, sizeof(char *), sort_function);

    closedir(current_dir);

    current_dir_length = strlen(current_dir_name);

    if(current_dir_length > 80)
    {

#ifdef GP2X_BUILD
    snprintf(current_dir_short, 80,
     "...%s", current_dir_name + current_dir_length - 77);
#else
    memcpy(current_dir_short, "...", 3);
      memcpy(current_dir_short + 3,
       current_dir_name + current_dir_length - 77, 77);
      current_dir_short[80] = 0;
#endif
    }
    else
    {
#ifdef GP2X_BUILD
      snprintf(current_dir_short, 80, "%s", current_dir_name);
#else
      memcpy(current_dir_short, current_dir_name,
       current_dir_length + 1);
#endif
    }

    repeat = 1;

    if(num_files == 0)
      current_column = 1;

  {
    screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
    screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 
    clearScreen(screenBottom, GFX_BOTTOM,color16(2, 4, 10));
    clearScreen(screenTopLeft, GFX_TOP,color16(2, 4, 10)); 
    while(repeat)
    {
      gspWaitForVBlank();
      screenTopLeft = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); 
      screenBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); 

      print_string(current_dir_short, COLOR_ACTIVE_ITEM, COLOR_BG, 0, 0);
      print_string(has_kernel_hax ? "Dynrec" : "Interpret", COLOR_ACTIVE_ITEM, COLOR_BG, 0, 240-10);
#if defined GP2X_BUILD
      print_string("Press X to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 220);
#elif defined RPI_BUILD
      print_string("Press B or Select to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 260);
#else
      print_string("Press X to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 260);
#endif

      for(i = 0, current_file_number = i + current_file_scroll_value;
       i < FILE_LIST_ROWS; i++, current_file_number++)
      {
        if(current_file_number < num_files)
        {
          if((current_file_number == current_file_selection) &&
           (current_column == 0))
          {
            print_string(file_list[current_file_number], COLOR_ACTIVE_ITEM,
             COLOR_BG, FILE_LIST_POSITION, ((i + 1) * 10));
          }
          else
          {
            print_string(file_list[current_file_number], COLOR_INACTIVE_ITEM,
             COLOR_BG, FILE_LIST_POSITION, ((i + 1) * 10));
          }
        }
      }

      for(i = 0, current_dir_number = i + current_dir_scroll_value;
       i < FILE_LIST_ROWS; i++, current_dir_number++)
      {
        if(current_dir_number < num_dirs)
        {
          if((current_dir_number == current_dir_selection) &&
           (current_column == 1))
          {
            print_string(dir_list[current_dir_number], COLOR_ACTIVE_ITEM,
             COLOR_BG, DIR_LIST_POSITION, ((i + 1) * 10));
          }
          else
          {
            print_string(dir_list[current_dir_number], COLOR_INACTIVE_ITEM,
             COLOR_BG, DIR_LIST_POSITION, ((i + 1) * 10));
          }
        }
      }
      
      if(num_dirs == 0)
          current_column = 0;

      gui_action = get_gui_input();

      switch(gui_action)
      {
        case CURSOR_DOWN:
          if(current_column == 0)
          {
            if(current_file_selection < (num_files - 1))
            {
              current_file_selection++;
              if(current_file_in_scroll == (FILE_LIST_ROWS - 1))
              {
                clear_screen(COLOR_BG);
                current_file_scroll_value++;
              }
              else
              {
                current_file_in_scroll++;
              }
            }
            else
            {
              clear_screen(COLOR_BG);
              current_file_selection = 0;
              current_file_scroll_value = 0;
              current_file_in_scroll = 0;
            }
          }
          else
          {
            if(current_dir_selection < (num_dirs - 1))
            {
              current_dir_selection++;
              if(current_dir_in_scroll == (FILE_LIST_ROWS - 1))
              {
                clear_screen(COLOR_BG);
                current_dir_scroll_value++;
              }
              else
              {
                current_dir_in_scroll++;
              }
            }
          }

          break;

        case CURSOR_R:
          if (current_column != 0)
            break;
          clear_screen(COLOR_BG);
          current_file_selection += FILE_LIST_ROWS;
          if (current_file_selection > num_files - 1)
            current_file_selection = num_files - 1;
          current_file_scroll_value = current_file_selection - FILE_LIST_ROWS / 2;
          if (current_file_scroll_value < 0)
          {
            current_file_scroll_value = 0;
            current_file_in_scroll = current_file_selection;
          }
          else
          {
            current_file_in_scroll = FILE_LIST_ROWS / 2;
          }
          break;

        case CURSOR_UP:
          if(current_column == 0)
          {
            if(current_file_selection)
            {
              current_file_selection--;
              if(current_file_in_scroll == 0)
              {
                clear_screen(COLOR_BG);
                current_file_scroll_value--;
              }
              else
              {
                current_file_in_scroll--;
              }
            }
            else
            {
              clear_screen(COLOR_BG);
              current_file_selection = num_files - 1;
              current_file_in_scroll = FILE_LIST_ROWS - 1;
              if (current_file_in_scroll > num_files - 1)
                current_file_in_scroll = num_files - 1;
              current_file_scroll_value = num_files - FILE_LIST_ROWS;
              if (current_file_scroll_value < 0)
                current_file_scroll_value = 0;
            }
          }
          else
          {
            if(current_dir_selection)
            {
              current_dir_selection--;
              if(current_dir_in_scroll == 0)
              {
                clear_screen(COLOR_BG);
                current_dir_scroll_value--;
              }
              else
              {
                current_dir_in_scroll--;
              }
            }
          }
          break;

        case CURSOR_L:
          if (current_column != 0)
            break;
          clear_screen(COLOR_BG);
          current_file_selection -= FILE_LIST_ROWS;
          if (current_file_selection < 0)
            current_file_selection = 0;
          current_file_scroll_value = current_file_selection - FILE_LIST_ROWS / 2;
          if (current_file_scroll_value < 0)
          {
            current_file_scroll_value = 0;
            current_file_in_scroll = current_file_selection;
          }
          else
          {
            current_file_in_scroll = FILE_LIST_ROWS / 2;
          }
          break;

         case CURSOR_RIGHT:
          if(current_column == 0)
          {
            if(num_dirs != 0)
              current_column = 1;
          }
          break;

        case CURSOR_LEFT:
          if(current_column == 1)
          {
            if(num_files != 0)
              current_column = 0;
          }
          break;

        case CURSOR_SELECT:
          if(current_column == 1)
          {
            repeat = 0;
            strcat(current_dir_name, dir_list[current_dir_selection]);
            chdir(current_dir_name);
          }
          else
          {
            if(num_files != 0)
            {
              repeat = 0;
              return_value = 0;
              strcat(current_dir_name, file_list[current_file_selection]);
              strcpy(result, current_dir_name);
            }
          }
          break;

        case CURSOR_BACK:
        
          if(!strcmp(current_dir_name, "sdmc:/"))
            break;

          repeat = 0;
          strcat(current_dir_name, "../");
          chdir(current_dir_name);
          break;

        case CURSOR_EXIT:
          return_value = -1;
          repeat = 0;
          break;

        default:
          break;
      }
    }
  }

    for(i = 0; i < num_files; i++)
    {
      free(file_list[i]);
    }
    free(file_list);

    for(i = 0; i < num_dirs; i++)
    {
      free(dir_list[i]);
    }
    free(dir_list);
  }

  clear_screen(COLOR_BG);

  return return_value;
}

typedef enum
{
  NUMBER_SELECTION_OPTION = 0x01,
  STRING_SELECTION_OPTION = 0x02,
  SUBMENU_OPTION          = 0x04,
  ACTION_OPTION           = 0x08,
  JOY_SELECTION_OPTION    = 0x10,
  KEY_SELECTION_OPTION    = 0x20
} menu_option_type_enum;

struct _menu_type
{
  void (* init_function)();
  void (* passive_function)();
  struct _menu_option_type *options;
  u32 num_options;
};

struct _menu_option_type
{
  void (* action_function)();
  void (* passive_function)();
  struct _menu_type *sub_menu;
  const char *display_string;
  void *options;
  u32 *current_option;
  u32 num_options;
  const char *help_string;
  u32 line_number;
  menu_option_type_enum option_type;
};

typedef struct _menu_option_type menu_option_type;
typedef struct _menu_type menu_type;

#define make_menu(name, init_function, passive_function)                      \
  menu_type name##_menu =                                                     \
  {                                                                           \
    init_function,                                                            \
    passive_function,                                                         \
    name##_options,                                                           \
    sizeof(name##_options) / sizeof(menu_option_type)                         \
  }                                                                           \

#ifdef RPI_BUILD

#define gamepad_config_option(display_string, number)                         \
{                                                                             \
  menu_get_joy,                                                               \
  NULL,                                                                       \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  usb_gamepad_config_buttons,                                                 \
  gamepad_config_map + number,                                                \
  0,                                                                          \
  gamepad_help[number],                                                       \
  number,                                                                     \
  JOY_SELECTION_OPTION                                                        \
}                                                                             \

#define keyboard_config_option(display_string, number)                        \
{                                                                             \
  menu_get_kbd,                                                               \
  NULL,                                                                       \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  NULL,                                                                       \
  keyboard_config_map + number,                                               \
  0,                                                                          \
  gamepad_help[number],                                                       \
  number,                                                                     \
  KEY_SELECTION_OPTION                                                        \
}   

#else

#define gamepad_config_option(display_string, number)                         \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + gamepad_config_line_to_button[number],                 \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[                                            \
   gamepad_config_line_to_button[number]]],                                   \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#endif

#define analog_config_option(display_string, number)                          \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + number + 12,                                           \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[number + 12]],                              \
  number + 2,                                                                 \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define cheat_option(number)                                                  \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  cheat_format_str[number],                                                   \
  enable_disable_options,                                                     \
  &(cheats[number].cheat_active),                                             \
  2,                                                                          \
  "Activate/deactivate this cheat code.",                                     \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define action_option(action_function, passive_function, display_string,      \
 help_string, line_number)                                                    \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  0,                                                                          \
  help_string,                                                                \
  line_number,                                                                \
  ACTION_OPTION                                                               \
}                                                                             \

#define submenu_option(sub_menu, display_string, help_string, line_number)    \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sub_menu,                                                                   \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sizeof(sub_menu) / sizeof(menu_option_type),                                \
  help_string,                                                                \
  line_number,                                                                \
  SUBMENU_OPTION                                                              \
}                                                                             \

#define selection_option(passive_function, display_string, options,           \
 option_ptr, num_options, help_string, line_number, type)                     \
{                                                                             \
  NULL,                                                                       \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type                                                                        \
}                                                                             \

#define action_selection_option(action_function, passive_function,            \
 display_string, options, option_ptr, num_options, help_string, line_number,  \
 type)                                                                        \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type | ACTION_OPTION                                                        \
}                                                                             \


#define string_selection_option(passive_function, display_string, options,    \
 option_ptr, num_options, help_string, line_number)                           \
  selection_option(passive_function, display_string ": %s", options,          \
   option_ptr, num_options, help_string, line_number, STRING_SELECTION_OPTION)\

#define numeric_selection_option(passive_function, display_string,            \
 option_ptr, num_options, help_string, line_number)                           \
  selection_option(passive_function, display_string ": %d", NULL, option_ptr, \
   num_options, help_string, line_number, NUMBER_SELECTION_OPTION)            \

#define string_selection_action_option(action_function, passive_function,     \
 display_string, options, option_ptr, num_options, help_string, line_number)  \
  action_selection_option(action_function, passive_function,                  \
   display_string ": %s",  options, option_ptr, num_options, help_string,     \
   line_number, STRING_SELECTION_OPTION)                                      \

#define numeric_selection_action_option(action_function, passive_function,    \
 display_string, option_ptr, num_options, help_string, line_number)           \
  action_selection_option(action_function, passive_function,                  \
   display_string ": %d",  NULL, option_ptr, num_options, help_string,        \
   line_number, NUMBER_SELECTION_OPTION)                                      \

#define numeric_selection_action_hide_option(action_function,                 \
 passive_function, display_string, option_ptr, num_options, help_string,      \
 line_number)                                                                 \
  action_selection_option(action_function, passive_function,                  \
   display_string, NULL, option_ptr, num_options, help_string,                \
   line_number, NUMBER_SELECTION_OPTION)                                      \


#define GAMEPAD_MENU_WIDTH 15

#ifdef PSP_BUILD

u32 gamepad_config_line_to_button[] =
 { 8, 6, 7, 9, 1, 2, 3, 0, 4, 5, 11, 10 };

#endif

#ifdef GP2X_BUILD

u32 gamepad_config_line_to_button[] =
 { 0, 2, 1, 3, 8, 9, 10, 11, 6, 7, 4, 5, 14, 15 };

#endif

#ifdef PND_BUILD

u32 gamepad_config_line_to_button[] =
 { 0, 2, 1, 3, 8, 9, 10, 11, 6, 7, 4, 5, 12, 13, 14, 15 };

#endif

#ifdef _3DS
u32 gamepad_config_line_to_button[] =
 { 8, 6, 7, 9, 1, 2, 3, 0, 4, 5, 11, 10 };
#endif

static const char *scale_options[] =
{
#ifdef PSP_BUILD
  "unscaled 3:2", "scaled 3:2", "fullscreen 16:9"
#elif defined(WIZ_BUILD)
  "unscaled 3:2", "scaled 3:2 (slower)",
  "unscaled 3:2 (anti-tear)", "scaled 3:2 (anti-tear)"
#elif defined(POLLUX_BUILD)
  "unscaled 3:2", "scaled 3:2 (slower)"
#elif defined(PND_BUILD)
  "unscaled", "2x", "3x", "fullscreen"
#elif defined(GP2X_BUILD)
  "unscaled 3:2", "scaled 3:2", "fullscreen", "scaled 3:2 (software)"
#elif defined(RPI_BUILD)
  "fullscreen"
#else
  "unscaled 3:2"
#endif
};

const char *filter2_options[] =
{
  "none", "scale2x", "scale3x", "eagle2x"
};

#ifndef PSP_BUILD
static const char *audio_buffer_options[] =
{
  "16 bytes", "32 bytes", "64 bytes",
  "128 bytes", "256 bytes", "512 bytes", "1024 bytes", "2048 bytes",
  "4096 bytes", "8192 bytes", "16284 bytes"
};
#else
const char *audio_buffer_options[] =
{
  "3072 bytes", "4096 bytes", "5120 bytes", "6144 bytes", "7168 bytes",
  "8192 bytes", "9216 bytes", "10240 bytes", "11264 bytes", "12288 bytes"
};
#endif


s32 load_game_config_file()
{
  char game_config_filename[512];
  u32 file_loaded = 0;
  u32 i;
  make_rpath(game_config_filename, sizeof(game_config_filename), ".cfg");

  file_open(game_config_file, game_config_filename, read);

  if(file_check_valid(game_config_file))
  {
    u32 file_size = file_length(game_config_filename, game_config_file);

    // Sanity check: File size must be the right size
    if(file_size == 56)
    {
      u32 file_options[file_size / 4];

      file_read_array(game_config_file, file_options);
      current_frameskip_type = file_options[0] % 3;
      frameskip_value = file_options[1];
      random_skip = file_options[2] % 2;
      clock_speed = file_options[3];

#ifdef POLLUX_BUILD
      if(clock_speed > 900)
        clock_speed = 533;
#elif defined(GP2X_BUILD)
      if(clock_speed >= 300)
        clock_speed = 200;
#else
      if(clock_speed > 333)
        clock_speed = 333;
#endif

      if(clock_speed < 33)
        clock_speed = 33;

      if(frameskip_value < 0)
        frameskip_value = 0;

      if(frameskip_value > 99)
        frameskip_value = 99;

      for(i = 0; i < 10; i++)
      {
        cheats[i].cheat_active = file_options[4 + i] % 2;
        cheats[i].cheat_name[0] = 0;
      }

      file_close(game_config_file);
      file_loaded = 1;
    }
  }

  if(file_loaded)
    return 0;

#ifdef RPI_BUILD
  current_frameskip_type = auto_frameskip;
  frameskip_value = 1;
#else
  current_frameskip_type = auto_frameskip;
  frameskip_value = 4;
#ifdef POLLUX_BUILD
  frameskip_value = 1;
#endif
#endif
  random_skip = 0;
  clock_speed = default_clock_speed;

  for(i = 0; i < 10; i++)
  {
    cheats[i].cheat_active = 0;
    cheats[i].cheat_name[0] = 0;
  }

  return -1;
}

enum file_options {
  fo_screen_scale = 0,
  fo_screen_filter,
  fo_global_enable_audio,
  fo_audio_buffer_size,
  fo_update_backup_flag,
  fo_global_enable_analog,
  fo_analog_sensitivity_level,
  fo_screen_filter2,
#ifdef RPI_BUILD
  fo_menu_hotkey,
#endif
  fo_main_option_count,
};

#ifdef PC_BUILD
#define PLAT_BUTTON_COUNT 0
#endif
#ifdef RPI_BUILD
#define FILE_OPTION_COUNT (fo_main_option_count + PLAT_BUTTON_COUNT + PLAT_KEY_COUNT)
#else
#define FILE_OPTION_COUNT (fo_main_option_count + PLAT_BUTTON_COUNT)
#endif

s32 load_config_file()
{
  char config_path[512];

  sprintf(config_path, "%s" PATH_SEPARATOR "%s", main_path, GPSP_CONFIG_FILENAME);

  file_open(config_file, config_path, read);

  if(file_check_valid(config_file))
  {
    u32 file_size = file_length(config_path, config_file);

    // Sanity check: File size must be the right size
    if(file_size == FILE_OPTION_COUNT * 4)
    {
      u32 file_options[file_size / 4];
      file_read_array(config_file, file_options);

      screen_scale = file_options[fo_screen_scale] %
        (sizeof(scale_options) / sizeof(scale_options[0]));
      screen_filter = file_options[fo_screen_filter] % 2;
      global_enable_audio = file_options[fo_global_enable_audio] % 2;
      screen_filter2 = file_options[fo_screen_filter2] %
        (sizeof(filter2_options) / sizeof(filter2_options[0]));

      audio_buffer_size_number = file_options[fo_audio_buffer_size] %
        (sizeof(audio_buffer_options) / sizeof(audio_buffer_options[0]));

      update_backup_flag = file_options[fo_update_backup_flag] % 2;
      global_enable_analog = file_options[fo_global_enable_analog] % 2;
      analog_sensitivity_level = file_options[fo_analog_sensitivity_level] % 8;

#ifdef PSP_BUILD
    scePowerSetClockFrequency(clock_speed, clock_speed, clock_speed / 2);
#endif

#ifndef PC_BUILD
#ifdef RPI_BUILD
      menu_hotkey = file_options[fo_menu_hotkey] % 2;

      u32 i;

      for(i = 0; i < PLAT_BUTTON_COUNT; i++) {
         gamepad_config_map[i] = file_options[fo_main_option_count + i];
      }

      for(i = 0; i < PLAT_KEY_COUNT; i++) {
         keyboard_config_map[i] = file_options[fo_main_option_count + PLAT_BUTTON_COUNT + i];
      }
#else

      // Sanity check: Make sure there's a MENU or FRAMESKIP
      // key, if not assign to triangle

      u32 i;
      s32 menu_button = -1;
      for(i = 0; i < PLAT_BUTTON_COUNT; i++)
      {
        gamepad_config_map[i] = file_options[fo_main_option_count + i] %
         (BUTTON_ID_NONE + 1);

        if(gamepad_config_map[i] == BUTTON_ID_MENU)
        {
          menu_button = i;
        }
      }

      if(menu_button == -1 && PLAT_MENU_BUTTON >= 0)
      {
        gamepad_config_map[PLAT_MENU_BUTTON] = BUTTON_ID_MENU;
      }
#endif
#endif

      file_close(config_file);
    }

    return 0;
  }

  return -1;
}

s32 save_game_config_file()
{
  char game_config_filename[512];
  u32 i;

  make_rpath(game_config_filename, sizeof(game_config_filename), ".cfg");

  file_open(game_config_file, game_config_filename, write);

  if(file_check_valid(game_config_file))
  {
    u32 file_options[14];

    file_options[0] = current_frameskip_type;
    file_options[1] = frameskip_value;
    file_options[2] = random_skip;
    file_options[3] = clock_speed;

    for(i = 0; i < 10; i++)
    {
      file_options[4 + i] = cheats[i].cheat_active;
    }

    file_write_array(game_config_file, file_options);

    file_close(game_config_file);

    return 0;
  }

  return -1;
}

s32 save_config_file()
{
  char config_path[512];

  sprintf(config_path, "%s" PATH_SEPARATOR "%s", main_path, GPSP_CONFIG_FILENAME);

  file_open(config_file, config_path, write);

  save_game_config_file();

  if(file_check_valid(config_file))
  {
    u32 file_options[FILE_OPTION_COUNT];

    file_options[fo_screen_scale] = screen_scale;
    file_options[fo_screen_filter] = screen_filter;
    file_options[fo_global_enable_audio] = global_enable_audio;
    file_options[fo_audio_buffer_size] = audio_buffer_size_number;
    file_options[fo_update_backup_flag] = update_backup_flag;
    file_options[fo_global_enable_analog] = global_enable_analog;
    file_options[fo_analog_sensitivity_level] = analog_sensitivity_level;
    file_options[fo_screen_filter2] = screen_filter2;

#ifndef PC_BUILD
#ifdef RPI_BUILD
    file_options[fo_menu_hotkey] = menu_hotkey;
      u32 i;

      for(i = 0; i < PLAT_BUTTON_COUNT; i++) {
         file_options[fo_main_option_count + i] = gamepad_config_map[i] ;
      }

      for(i = 0; i < PLAT_KEY_COUNT; i++) {
         file_options[fo_main_option_count + PLAT_BUTTON_COUNT + i] = keyboard_config_map[i];
      }
#else
     u32 i;
     for(i = 0; i < PLAT_BUTTON_COUNT; i++) {
       file_options[fo_main_option_count + i] = gamepad_config_map[i];
     }
#endif
#endif

    file_write_array(config_file, file_options);

    file_close(config_file);

    return 0;
  }

  return -1;
}

typedef enum
{
  MAIN_MENU,
  GAMEPAD_MENU,
  SAVESTATE_MENU,
  FRAMESKIP_MENU,
  CHEAT_MENU
} menu_enum;

u32 savestate_slot = 0;

void get_savestate_snapshot(char *savestate_filename)
{
  u16 snapshot_buffer[240 * 160];
  char savestate_timestamp_string[80];

  file_open(savestate_file, savestate_filename, read);

  if(file_check_valid(savestate_file))
  {
    const char weekday_strings[7][11] =
    {
      "Sunday", "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday"
    };
    time_t savestate_time_flat;
    struct tm *current_time;
    file_read_array(savestate_file, snapshot_buffer);
    file_read_variable(savestate_file, savestate_time_flat);

    file_close(savestate_file);

    current_time = localtime(&savestate_time_flat);
    sprintf(savestate_timestamp_string,
     "%s  %02d/%02d/%04d  %02d:%02d:%02d                ",
     weekday_strings[current_time->tm_wday], current_time->tm_mon + 1,
     current_time->tm_mday, current_time->tm_year + 1900,
     current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    savestate_timestamp_string[40] = 0;
    print_string(savestate_timestamp_string, COLOR_HELP_TEXT, COLOR_BG,
     10, 40);
  }
  else
  {
    memset(snapshot_buffer, 0, 240 * 160 * 2);
    print_string_ext("No savestate in this slot.",
     0xFFFF, 0x0000, 15, 75, snapshot_buffer, 240, 0, 0, FONT_HEIGHT);
    print_string("---------- --/--/---- --:--:--          ", COLOR_HELP_TEXT,
     COLOR_BG, 10, 40);
  }

#ifndef GP2X_BUILD
  blit_to_screen(snapshot_buffer, 240, 160, 230, 40);
#endif
}

void get_savestate_filename_noshot(u32 slot, char *name_buffer)
{
  char savestate_ext[16];

  sprintf(savestate_ext, "%d.svs", slot);
  make_rpath(name_buffer, 512, savestate_ext);
}

void get_savestate_filename(u32 slot, char *name_buffer)
{
  get_savestate_filename_noshot(slot, name_buffer);
  get_savestate_snapshot(name_buffer);
}

#ifdef PSP_BUILD
  void _flush_cache()
  {
    invalidate_all_cache();
  }
#endif

u32 menu(u16 *original_screen)
{
  char print_buffer[81];
  u32 clock_speed_number;
  gui_action_type gui_action;
  u32 i;
  u32 repeat = 1;
  u32 return_value = 0;
  u32 first_load = 0;
  char current_savestate_filename[512];
  char line_buffer[80];
  char cheat_format_str[10][41];

  menu_type *current_menu;
  menu_option_type *current_option;
  menu_option_type *display_option;
  u32 current_option_num;

  auto void choose_menu();
  auto void clear_help();

#ifndef PC_BUILD
  static const char * const gamepad_help[] =
  {
    "Up button on GBA d-pad.",
    "Down button on GBA d-pad.",
    "Left button on GBA d-pad.",
    "Right button on GBA d-pad.",
    "A button on GBA.",
    "B button on GBA.",
    "Left shoulder button on GBA.",
    "Right shoulder button on GBA.",
    "Start button on GBA.",
    "Select button on GBA.",
    "Brings up the options menu.",
    "Toggles fastforward on/off.",
    "Loads the game state from the current slot.",
    "Saves the game state to the current slot.",
    "Rapidly press/release the A button on GBA.",
    "Rapidly press/release the B button on GBA.",
    "Rapidly press/release the L shoulder on GBA.",
    "Rapidly press/release the R shoulder on GBA.",
    "Increases the volume.",
    "Decreases the volume.",
    "Displays virtual/drawn frames per second.",
    "Does nothing."
  };

  static const char *gamepad_config_buttons[] =
  {
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "A",
    "B",
    "L",
    "R",
    "START",
    "SELECT",
    "MENU",
    "FASTFORWARD",
    "LOAD STATE",
    "SAVE STATE",
    "RAPIDFIRE A",
    "RAPIDFIRE B",
    "RAPIDFIRE L",
    "RAPIDFIRE R",
    "VOLUME UP",
    "VOLUME DOWN",
    "DISPLAY FPS",
    "NOTHING"
  };

#ifdef RPI_BUILD
  static const char *usb_gamepad_config_buttons[] =
  {
    "Asix Y-",
    "Asix Y+",
    "Asix X-",
    "Asix X+",
    "Button 1",
    "Button 2",
    "Button 3",
    "Button 4",
    "Button 5",
    "Button 6",
    "Butoon 7",
    "Button 8",
    "Button 9",
    "Button 10",
    "Button 11",
    "Button 12",
    "Button 13",
    "Button 14",
    "Button 15",
    "Button 16"
  };
#endif
#endif

  void menu_update_clock()
  {
    get_clock_speed_number();
    if (clock_speed_number < 0 || clock_speed_number >=
     sizeof(clock_speed_options) / sizeof(clock_speed_options[0]))
    {
      clock_speed = default_clock_speed;
      get_clock_speed_number();
    }
  }

  void menu_exit()
  {
    if(!first_load)
     repeat = 0;
  }

  void menu_quit()
  {
    menu_get_clock_speed();
    save_config_file();
    quit();
  }

  void menu_load()
  {
    const char *file_ext[] = { ".gba", ".bin", ".zip", NULL };
    char load_filename[512];
    save_game_config_file();
    if(load_file(file_ext, load_filename) != -1)
    {
       if(load_gamepak(load_filename) == -1)
       {
         quit();
       }
       reset_gba();
       return_value = 1;
       repeat = 0;
       reg[CHANGED_PC_STATUS] = 1;
       menu_update_clock();
    }
  }

  void menu_restart()
  {
    if(!first_load)
    {
      reset_gba();
      reg[CHANGED_PC_STATUS] = 1;
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_change_state()
  {
    get_savestate_filename(savestate_slot, current_savestate_filename);
  }

  void menu_save_state()
  {
    if(!first_load)
    {
      get_savestate_filename_noshot(savestate_slot,
       current_savestate_filename);
      save_state(current_savestate_filename, original_screen);
    }
    menu_change_state();
  }

  void menu_load_state()
  {
    if(!first_load)
    {
      load_state(current_savestate_filename);
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_load_state_file()
  {
    const char *file_ext[] = { ".svs", NULL };
    char load_filename[512];
    if(load_file(file_ext, load_filename) != -1)
    {
      load_state(load_filename);
      return_value = 1;
      repeat = 0;
    }
    else
    {
      choose_menu(current_menu);
    }
  }

  void menu_get_joy()
  {
    const char help_string[] = { "Choose asix or button on the gamepad or press ESC to exit." };

    int Tmp;
    print_string(help_string, COLOR_HELP_TEXT,COLOR_BG, 8, 210);

    sprintf(line_buffer,current_option->display_string,"   ???");
    print_string_pad(line_buffer, COLOR_ACTIVE_ITEM, COLOR_BG, 6,
        (current_option->line_number * 10) + 40, 36);

    flip_screen();
    Tmp=get_joystick();
    if (Tmp>=0) *(current_option->current_option)=Tmp;

    clear_help();
    print_string(current_option->help_string, COLOR_HELP_TEXT,COLOR_BG, 8, 210);
    flip_screen();

  }

  void menu_get_kbd()
  {
    const char help_string[] = { "Choose key on the keyboard or press ESC to exit." };

    int Tmp;
    print_string(help_string, COLOR_HELP_TEXT,COLOR_BG, 8, 210);

    sprintf(line_buffer,current_option->display_string,"   ???");
    print_string_pad(line_buffer, COLOR_ACTIVE_ITEM, COLOR_BG, 6,
        (current_option->line_number * 10) + 40, 36);

    flip_screen();
    Tmp=get_keyboard();
    if (Tmp>=0) *(current_option->current_option)=Tmp;

    clear_help();
    print_string(current_option->help_string, COLOR_HELP_TEXT,COLOR_BG, 8, 210);
    flip_screen();

  }

#ifndef RPI_BUILD
  void menu_fix_gamepad_help()
  {
#ifndef PC_BUILD
    clear_help();
    current_option->help_string =
      gamepad_help[gamepad_config_map[
      gamepad_config_line_to_button[current_option_num]]];
#endif
  }
#endif

  void submenu_graphics_sound()
  {

  }

  void submenu_cheats_misc()
  {

  }

  void submenu_gamepad()
  {

  }

  void submenu_keyboard()
  {

  }

  void submenu_analog()
  {

  }

  void submenu_savestate()
  {
    print_string("Savestate options:", COLOR_ACTIVE_ITEM, COLOR_BG, 10, 70);
    menu_change_state();
  }

  void submenu_main()
  {
    strncpy(print_buffer, gamepak_filename, 80);
    print_string(print_buffer, COLOR_ROM_INFO, COLOR_BG, 10, 10);
    sprintf(print_buffer, "%s  %s  %s", gamepak_title,
     gamepak_code, gamepak_maker);
    print_string(print_buffer, COLOR_ROM_INFO, COLOR_BG, 10, 20);

    get_savestate_filename_noshot(savestate_slot,
     current_savestate_filename);
  }

  const char *yes_no_options[] = { "no", "yes" };
  const char *enable_disable_options[] = { "disabled", "enabled" };

  const char *frameskip_options[] = { "automatic", "manual", "off" };
  const char *frameskip_variation_options[] = { "uniform", "random" };

  static const char *update_backup_options[] = { "Exit only", "Automatic" };

  // Marker for help information, don't go past this mark (except \n)------*
  menu_option_type graphics_sound_options[] =
 {
#ifndef RPI_BUILD
    string_selection_option(NULL, "Display scaling", scale_options,
     (u32 *)(&screen_scale),
     sizeof(scale_options) / sizeof(scale_options[0]),
#ifndef GP2X_BUILD
     "Determines how the GBA screen is resized in relation to the\n"
     "entire screen."
#ifdef PSP_BUILD
     " Select unscaled 3:2 for GBA resolution, scaled 3:2 for GBA\n"
     "aspect ratio scaled to fill the height of the PSP screen, and\n"
     "fullscreen to fill the entire PSP screen."
#endif
#endif
     "", 2),
#endif

#ifndef GP2X_BUILD
    string_selection_option(NULL, "Screen filtering", yes_no_options,
     (u32 *)(&screen_filter), 2,
     "Determines whether or not filtering should be used when\n"
     "scaling the screen. Selecting this will produce a more even and\n"
     "smooth image, at the cost of being blurry and having less vibrant\n"
     "colors.", 3),
#endif
#if defined (PND_BUILD)
    string_selection_option(NULL, "Scaling filter", filter2_options,
     (u32 *)(&screen_filter2),
     sizeof(filter2_options) / sizeof(filter2_options[0]),
     "Optional pixel art scaling filter", 4),
#endif
    string_selection_option(NULL, "Frameskip type", frameskip_options,
     (u32 *)(&current_frameskip_type), 3,
#ifndef GP2X_BUILD
     "Determines what kind of frameskipping to use.\n"
     "Frameskipping may improve emulation speed of many games.\n"
#endif
     "Off: Do not skip any frames.\n"
     "Auto: Skip up to N frames (see next opt) as needed.\n"
     "Manual: Always render only 1 out of N + 1 frames."
     , 5),
    numeric_selection_option(NULL, "Frameskip value", &frameskip_value, 100,
#ifndef GP2X_BUILD
     "For auto frameskip, determines the maximum number of frames that\n"
     "are allowed to be skipped consecutively.\n"
     "For manual frameskip, determines the number of frames that will\n"
     "always be skipped."
#endif
     "", 6),
    string_selection_option(NULL, "Framskip variation",
     frameskip_variation_options, &random_skip, 2,
#ifndef GP2X_BUILD
     "If objects in the game flicker at a regular rate certain manual\n"
     "frameskip values may cause them to normally disappear. Change this\n"
     "value to 'random' to avoid this. Do not use otherwise, as it tends\n"
     "to make the image quality worse, especially in high motion games."
#endif
     "", 7),
    string_selection_option(NULL, "Audio output", yes_no_options,
     &global_enable_audio, 2,
     "Select 'no' to turn off all audio output. This will\n"
     "not result in a significant change in performance.", 9),
#ifndef PSP_BUILD
    string_selection_option(NULL, "Audio buffer", audio_buffer_options,
             &audio_buffer_size_number, 11,
#else
    string_selection_option(NULL, "Audio buffer", audio_buffer_options,
             &audio_buffer_size_number, 10,
#endif

#ifdef PSP_BUILD
     "Set the size (in bytes) of the audio buffer. Larger values may result\n"
     "in slightly better performance at the cost of latency; the lowest\n"
     "value will give the most responsive audio.\n"
     "This option requires gpSP to be restarted before it will take effect.",
#else
     "Set the size (in bytes) of the audio buffer.\n"
     "This option requires gpSP restart to take effect.\n"
     "Settable values may be limited by SDL implementation.",
#endif
     10),
    submenu_option(NULL, "Back", "Return to the main menu.", 12)
  };

  make_menu(graphics_sound, submenu_graphics_sound, NULL);

  menu_option_type cheats_misc_options[] =
  {
    cheat_option(0),
    cheat_option(1),
    cheat_option(2),
    cheat_option(3),
    cheat_option(4),
    cheat_option(5),
    cheat_option(6),
    cheat_option(7),
    cheat_option(8),
    cheat_option(9),
#if defined(PSP_BUILD) || defined(GP2X_BUILD)
    string_selection_option(NULL, "Clock speed",
     clock_speed_options, &clock_speed_number,
     sizeof(clock_speed_options) / sizeof(clock_speed_options[0]),
     "Change the clock speed of the device. Higher clock\n"
     "speed will yield better performance, but will drain\n"
     "battery life further.", 11),
#endif
    string_selection_option(NULL, "Update backup",
     update_backup_options, &update_backup_flag, 2,
#ifdef GP2X_BUILD
     "Determines when in-game save files should be\n"
     "written back to SD card."
#else
     "Determines when in-game save files should be written back to\n"
     "card. If set to 'automatic' writebacks will occur shortly after\n"
     "the game's backup is altered. On 'exit only' it will only be\n"
     "written back when you exit from this menu.\n"
#ifdef PSP
     "(NOT from using the home button), use the latter with extreme care."
#endif
#endif
     "", 12),
    submenu_option(NULL, "Back", "Return to the main menu.", 14)
  };

  make_menu(cheats_misc, submenu_cheats_misc, NULL);

  menu_option_type savestate_options[] =
  {
    numeric_selection_action_hide_option(menu_load_state, menu_change_state,
     "Load savestate from current slot", &savestate_slot, 10,
     "Select to load the game state from the current slot\n"
     "for this game.\n"
     "Press left + right to change the current slot.", 6),
    numeric_selection_action_hide_option(menu_save_state, menu_change_state,
     "Save savestate to current slot", &savestate_slot, 10,
     "Select to save the game state to the current slot\n"
     "for this game.\n"
     "Press left + right to change the current slot.", 7),
    numeric_selection_action_hide_option(menu_load_state_file,
      menu_change_state,
     "Load savestate from file", &savestate_slot, 10,
     "Restore gameplay from a savestate file.\n"
     "Note: The same file used to save the state must be\n"
     "present.\n", 9),
    numeric_selection_option(menu_change_state,
     "Current savestate slot", &savestate_slot, 10,
     "Change the current savestate slot.\n", 11),
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  make_menu(savestate, submenu_savestate, NULL);

#ifdef PSP_BUILD

  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("D-pad up     ", 0),
    gamepad_config_option("D-pad down   ", 1),
    gamepad_config_option("D-pad left   ", 2),
    gamepad_config_option("D-pad right  ", 3),
    gamepad_config_option("Circle       ", 4),
    gamepad_config_option("Cross        ", 5),
    gamepad_config_option("Square       ", 6),
    gamepad_config_option("Triangle     ", 7),
    gamepad_config_option("Left Trigger ", 8),
    gamepad_config_option("Right Trigger", 9),
    gamepad_config_option("Start        ", 10),
    gamepad_config_option("Select       ", 11),
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };


  menu_option_type analog_config_options[] =
  {
    analog_config_option("Analog up   ", 0),
    analog_config_option("Analog down ", 1),
    analog_config_option("Analog left ", 2),
    analog_config_option("Analog right", 3),
    string_selection_option(NULL, "Enable analog", yes_no_options,
     &global_enable_analog, 2,
     "Select 'no' to block analog input entirely.", 7),
    numeric_selection_option(NULL, "Analog sensitivity",
     &analog_sensitivity_level, 10,
     "Determine sensitivity/responsiveness of the analog input.\n"
     "Lower numbers are less sensitive.", 8),
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

#if defined(GP2X_BUILD) || defined(PND_BUILD)

  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("D-pad up     ", 0),
    gamepad_config_option("D-pad down   ", 1),
    gamepad_config_option("D-pad left   ", 2),
    gamepad_config_option("D-pad right  ", 3),
    gamepad_config_option("A            ", 4),
    gamepad_config_option("B            ", 5),
    gamepad_config_option("X            ", 6),
    gamepad_config_option("Y            ", 7),
    gamepad_config_option("Left Trigger ", 8),
    gamepad_config_option("Right Trigger", 9),
#ifdef WIZ_BUILD
    gamepad_config_option("Menu         ", 10),
    gamepad_config_option("Select       ", 11),
#elif defined(POLLUX_BUILD)
    gamepad_config_option("I            ", 10),
    gamepad_config_option("II           ", 11),
    gamepad_config_option("Push         ", 12),
    gamepad_config_option("Home         ", 13),
#elif defined(PND_BUILD)
    gamepad_config_option("Start        ", 10),
    gamepad_config_option("Select       ", 11),
    gamepad_config_option("1            ", 12),
    gamepad_config_option("2            ", 13),
    gamepad_config_option("3            ", 14),
    gamepad_config_option("4            ", 15),
#else // GP2X
    gamepad_config_option("Start        ", 10),
    gamepad_config_option("Select       ", 11),
    gamepad_config_option("Stick Push   ", 12),
#endif
#ifdef PND_BUILD
    submenu_option(NULL, "Back", "Return to the main menu.", 16)
#else
    submenu_option(NULL, "Back", "Return to the main menu.", 14)
#endif
  };


  menu_option_type analog_config_options[] =
  {
#if defined(POLLUX_BUILD)
    numeric_selection_option(NULL, "Analog sensitivity",
     &analog_sensitivity_level, 10,
     "Determine sensitivity/responsiveness of the analog input.\n"
     "Lower numbers are less sensitive.", 8),
#endif
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

#if defined(PC_BUILD)

  menu_option_type gamepad_config_options[] =
  {
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  menu_option_type analog_config_options[] =
  {
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

#if defined(RPI_BUILD)

  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("D-pad up     ", 0),
    gamepad_config_option("D-pad down   ", 1),
    gamepad_config_option("D-pad left   ", 2),
    gamepad_config_option("D-pad right  ", 3),
    gamepad_config_option("A            ", 4),
    gamepad_config_option("B            ", 5),
    gamepad_config_option("Left Trigger ", 6),
    gamepad_config_option("Right Trigger", 7),
    gamepad_config_option("Start        ", 8),
    gamepad_config_option("Select       ", 9),

    string_selection_option(NULL, "Menu hotkey  ", yes_no_options,
     &menu_hotkey, 2,
     "Select 'yes' to open Menu by pressing buttons\nSelect + Right Trigger.", 11),

    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  menu_option_type keyboard_config_options[] =
  {
    keyboard_config_option("D-pad up     ", 0),
    keyboard_config_option("D-pad down   ", 1),
    keyboard_config_option("D-pad left   ", 2),
    keyboard_config_option("D-pad right  ", 3),
    keyboard_config_option("A            ", 4),
    keyboard_config_option("B            ", 5),
    keyboard_config_option("Left Trigger ", 6),
    keyboard_config_option("Right Trigger", 7),
    keyboard_config_option("Start        ", 8),
    keyboard_config_option("Select       ", 9),

    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

#if defined(_3DS)

  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("A            ", 0),
    gamepad_config_option("B            ", 1),
    gamepad_config_option("Select       ", 2),
    gamepad_config_option("Start        ", 3),
    gamepad_config_option("D-pad right  ", 4),
    gamepad_config_option("D-pad left   ", 5),
    gamepad_config_option("D-pad up     ", 6),
    gamepad_config_option("D-pad down   ", 7),
    gamepad_config_option("Right Trigger", 8),
    gamepad_config_option("Left Trigger ", 9),
    gamepad_config_option("X            ", 10),
    gamepad_config_option("Y            ", 11),
    gamepad_config_option("ZL           ", 12),
    gamepad_config_option("ZR           ", 13),
    gamepad_config_option("C-stick right", 14),
    gamepad_config_option("C-stick left ", 15),
    gamepad_config_option("C-stick up   ", 16),
    gamepad_config_option("C-stick down ", 17),
    gamepad_config_option("C-pad right  ", 18),
    gamepad_config_option("C-pad left   ", 19),
    gamepad_config_option("C-pad up     ", 20),
    gamepad_config_option("C-pad down   ", 21),

    submenu_option(NULL, "Back", "Return to the main menu.", 23)
  };

#endif

  make_menu(gamepad_config, submenu_gamepad, NULL);
#ifdef RPI_BUILD
  make_menu(keyboard_config, submenu_keyboard, NULL);
#elif !defined(_3DS)
  make_menu(analog_config, submenu_analog, NULL);
#endif

  menu_option_type main_options[] =
  {
    submenu_option(&graphics_sound_menu, "Graphics and Sound options",
     "Select to set display parameters and frameskip\n"
     "behavior, audio on/off, buffer size, and filtering.", 0),
    numeric_selection_action_option(menu_load_state, NULL,
     "Load state from slot", &savestate_slot, 10,
     "Select to load the game state from the current slot\n"
     "for this game, if it exists.\n"
     "Press left + right to change the current slot.", 2),
    numeric_selection_action_option(menu_save_state, NULL,
     "Save state to slot", &savestate_slot, 10,
     "Select to save the game state to the current slot\n"
     "for this game. See the extended menu for more info.\n"
     "Press left + right to change the current slot.", 3),
    submenu_option(&savestate_menu, "Savestate options",
     "Select to enter a menu for loading, saving, and\n"
     "viewing the currently active savestate for this game\n"
     "(or to load a savestate file from another game)", 4),
    submenu_option(&gamepad_config_menu, "Configure gamepad input",
     "Select to change the in-game behavior of buttons\n"
     "and d-pad.", 6),
#ifndef WIZ_BUILD
#ifdef RPI_BUILD
    submenu_option(&keyboard_config_menu, "Configure keyboard input",
     "Select to change the in-game behavior of the keyboard.", 7),
#elif !defined(_3DS)
    submenu_option(&analog_config_menu, "Configure analog input",
     "Select to change the in-game behavior of the analog nub.", 7),
#endif
#endif
    submenu_option(&cheats_misc_menu, "Cheats and Miscellaneous options",
     "Select to manage cheats, set backup behavior,\n"
     "and set device clock speed.", 9),
    action_option(menu_load, NULL, "Load new game",
     "Select to load a new game\n"
     "(will exit a game if currently playing).", 11),
    action_option(menu_restart, NULL, "Restart game",
     "Select to reset the GBA with the current game\n"
     "loaded.", 12),
    action_option(menu_exit, NULL, "Return to game",
     "Select to exit this menu and resume gameplay.", 13),
    action_option(menu_quit, NULL, "Exit gpSP",
     "Select to exit gpSP and return to the menu.", 15)
  };

  make_menu(main, submenu_main, NULL);

  void choose_menu(menu_type *new_menu)
  {
    if(new_menu == NULL)
      new_menu = &main_menu;

    clear_screen(COLOR_BG);

#ifndef GP2X_BUILD
    blit_to_screen(original_screen, 240, 160, 230, 40);
#endif

    current_menu = new_menu;
    current_option = new_menu->options;
    current_option_num = 0;
    if(current_menu->init_function)
     current_menu->init_function();
  }

  void clear_help()
  {
    for(i = 0; i < 6; i++)
    {
      print_string_pad(" ", COLOR_BG, COLOR_BG, 8, 210 + (i * 10), 70);
    }
  }

  menu_update_clock();
  video_resolution_large();

#ifndef GP2X_BUILD
  //SDL_LockMutex(sound_mutex);
#endif
  //SDL_PauseAudio(1);

#ifndef GP2X_BUILD
  //SDL_UnlockMutex(sound_mutex);
#endif

  if(gamepak_filename[0] == 0)
  {
    first_load = 1;
    memset(original_screen, 0x00, 240 * 160 * 2);
    print_string_ext("No game loaded yet.", 0xFFFF, 0x0000,
     60, 75,original_screen, 240, 0, 0, FONT_HEIGHT);
  }

  choose_menu(&main_menu);

  for(i = 0; i < 10; i++)
  {
    if(i >= num_cheats)
    {
      sprintf(cheat_format_str[i], "cheat %d (none loaded)", i);
    }
    else
    {
      sprintf(cheat_format_str[i], "cheat %d (%s): %%s", i,
       cheats[i].cheat_name);
    }
  }

  current_menu->init_function();

  while(repeat)
  {
    display_option = current_menu->options;

    for(i = 0; i < current_menu->num_options; i++, display_option++)
    {
      if(display_option->option_type & NUMBER_SELECTION_OPTION) {
        sprintf(line_buffer, display_option->display_string,
         *(display_option->current_option));
      }  else if(display_option->option_type & STRING_SELECTION_OPTION) {
        sprintf(line_buffer, display_option->display_string,
         ((u32 *)display_option->options)[*(display_option->current_option)]);
      }
#ifdef RPI_BUILD
        else if(display_option->option_type & JOY_SELECTION_OPTION) {
        sprintf(line_buffer, display_option->display_string,
         ((u32 *)display_option->options)[*(display_option->current_option)]);
      } else if(display_option->option_type & KEY_SELECTION_OPTION) {
        sprintf(line_buffer, display_option->display_string,
         //SDL_GetKeyName(*(display_option->current_option)));
        line_buffer[15] = toupper(line_buffer[15]);
      }
#endif
        else {
        strcpy(line_buffer, display_option->display_string);
      }

      if(display_option == current_option) {
        print_string_pad(line_buffer, COLOR_ACTIVE_ITEM, COLOR_BG, 6,
         (display_option->line_number * 10) + 40, 36);
      } else {
        print_string_pad(line_buffer, COLOR_INACTIVE_ITEM, COLOR_BG, 6,
         (display_option->line_number * 10) + 40, 36);
      }
    }

    print_string(current_option->help_string, COLOR_HELP_TEXT,
     COLOR_BG, 8, 210);

    flip_screen();

    gui_action = get_gui_input();

    switch(gui_action)
    {
      case CURSOR_DOWN:
        current_option_num = (current_option_num + 1) %
          current_menu->num_options;

        current_option = current_menu->options + current_option_num;
        clear_help();
        break;

      case CURSOR_UP:
        if(current_option_num)
          current_option_num--;
        else
          current_option_num = current_menu->num_options - 1;

        current_option = current_menu->options + current_option_num;
        clear_help();
        break;

      case CURSOR_RIGHT:
        if(current_option->option_type & (NUMBER_SELECTION_OPTION |
         STRING_SELECTION_OPTION))
        {
          *(current_option->current_option) =
           (*current_option->current_option + 1) %
           current_option->num_options;

          if(current_option->passive_function)
            current_option->passive_function();
        }
        break;

      case CURSOR_LEFT:
        if(current_option->option_type & (NUMBER_SELECTION_OPTION |
         STRING_SELECTION_OPTION))
        {
          u32 current_option_val = *(current_option->current_option);

          if(current_option_val)
            current_option_val--;
          else
            current_option_val = current_option->num_options - 1;

          *(current_option->current_option) = current_option_val;

          if(current_option->passive_function)
            current_option->passive_function();
        }
        break;

      case CURSOR_EXIT:
        if(current_menu == &main_menu)
          menu_exit();

        choose_menu(&main_menu);
        break;

      case CURSOR_SELECT:
        if(current_option->option_type & ACTION_OPTION) {
          current_option->action_function();
#ifdef RPI_BUILD
        } else if(current_option->option_type & JOY_SELECTION_OPTION) {
          current_option->action_function();
        } else if(current_option->option_type & KEY_SELECTION_OPTION) {
          current_option->action_function();
#endif
        }else  if(current_option->option_type & SUBMENU_OPTION) {
          choose_menu(current_option->sub_menu);
        }

        if(current_menu == &main_menu)
           choose_menu(&main_menu);

        break;

      default:
        break;
    }
  }

  set_gba_resolution(screen_scale);
  video_resolution_small();
  menu_get_clock_speed();
  set_clock_speed();

  //SDL_PauseAudio(0);
  num_skipped_frames = 100;

  return return_value;
}
