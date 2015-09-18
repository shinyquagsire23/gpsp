/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
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

extern void* __service_ptr; // used to detect if we're run from a homebrew launcher

// Special thanks to psp298 for the analog->dpad code!

void trigger_key(u32 key)
{
  u32 p1_cnt = io_registers[REG_P1CNT];

  if((p1_cnt >> 14) & 0x01)
  {
    u32 key_intersection = (p1_cnt & key) & 0x3FF;

    if(p1_cnt >> 15)
    {
      if(key_intersection == (p1_cnt & 0x3FF))
        raise_interrupt(IRQ_KEYPAD);
    }
    else
    {
      if(key_intersection)
        raise_interrupt(IRQ_KEYPAD);
    }
  }
}

u32 key = 0;

u32 global_enable_analog = 1;
u32 analog_sensitivity_level = 4;

typedef enum
{
  BUTTON_NOT_HELD,
  BUTTON_HELD_INITIAL,
  BUTTON_HELD_REPEAT
} button_repeat_state_type;


// These define autorepeat values (in microseconds), tweak as necessary.

#define BUTTON_REPEAT_START    200000
#define BUTTON_REPEAT_CONTINUE 50000

button_repeat_state_type button_repeat_state = BUTTON_NOT_HELD;
u32 button_repeat = 0;
gui_action_type cursor_repeat = CURSOR_NONE;


#ifdef PSP_BUILD

u32 gamepad_config_map[16] =
{
  BUTTON_ID_MENU,               // Triangle
  BUTTON_ID_A,                  // Circle
  BUTTON_ID_B,                  // Cross
  BUTTON_ID_START,              // Square
  BUTTON_ID_L,                  // Ltrigger
  BUTTON_ID_R,                  // Rtrigger
  BUTTON_ID_DOWN,               // Down
  BUTTON_ID_LEFT,               // Left
  BUTTON_ID_UP,                 // Up
  BUTTON_ID_RIGHT,              // Right
  BUTTON_ID_SELECT,             // Select
  BUTTON_ID_START,              // Start
  BUTTON_ID_UP,                 // Analog up
  BUTTON_ID_DOWN,               // Analog down
  BUTTON_ID_LEFT,               // Analog left
  BUTTON_ID_RIGHT               // Analog right
};

#define PSP_ALL_BUTTON_MASK 0xFFFF

gui_action_type get_gui_input()
{
  SceCtrlData ctrl_data;
  gui_action_type new_button = CURSOR_NONE;
  u32 new_buttons;

  static u32 last_buttons = 0;
  static u64 button_repeat_timestamp;

  delay_us(25000);

  sceCtrlPeekBufferPositive(&ctrl_data, 1);
  ctrl_data.Buttons &= PSP_ALL_BUTTON_MASK;
  new_buttons = (last_buttons ^ ctrl_data.Buttons) & ctrl_data.Buttons;
  last_buttons = ctrl_data.Buttons;

  if(new_buttons & PSP_CTRL_LEFT)
    new_button = CURSOR_LEFT;

  if(new_buttons & PSP_CTRL_RIGHT)
    new_button = CURSOR_RIGHT;

  if(new_buttons & PSP_CTRL_UP)
    new_button = CURSOR_UP;

  if(new_buttons & PSP_CTRL_DOWN)
    new_button = CURSOR_DOWN;

  if(new_buttons & PSP_CTRL_START)
    new_button = CURSOR_SELECT;

  if(new_buttons & PSP_CTRL_CIRCLE)
    new_button = CURSOR_SELECT;

  if(new_buttons & PSP_CTRL_CROSS)
    new_button = CURSOR_EXIT;

  if(new_buttons & PSP_CTRL_SQUARE)
    new_button = CURSOR_BACK;

  if(new_button != CURSOR_NONE)
  {
    get_ticks_us(&button_repeat_timestamp);
    button_repeat_state = BUTTON_HELD_INITIAL;
    button_repeat = new_buttons;
    cursor_repeat = new_button;
  }
  else
  {
    if(ctrl_data.Buttons & button_repeat)
    {
      u64 new_ticks;
      get_ticks_us(&new_ticks);

      if(button_repeat_state == BUTTON_HELD_INITIAL)
      {
        if((new_ticks - button_repeat_timestamp) >
         BUTTON_REPEAT_START)
        {
          new_button = cursor_repeat;
          button_repeat_timestamp = new_ticks;
          button_repeat_state = BUTTON_HELD_REPEAT;
        }
      }

      if(button_repeat_state == BUTTON_HELD_REPEAT)
      {
        if((new_ticks - button_repeat_timestamp) >
         BUTTON_REPEAT_CONTINUE)
        {
          new_button = cursor_repeat;
          button_repeat_timestamp = new_ticks;
        }
      }
    }
  }

  return new_button;
}

#define PSP_CTRL_ANALOG_UP    (1 << 28)
#define PSP_CTRL_ANALOG_DOWN  (1 << 29)
#define PSP_CTRL_ANALOG_LEFT  (1 << 30)
#define PSP_CTRL_ANALOG_RIGHT (1 << 31)

u32 button_psp_mask_to_config[] =
{
  PSP_CTRL_TRIANGLE,
  PSP_CTRL_CIRCLE,
  PSP_CTRL_CROSS,
  PSP_CTRL_SQUARE,
  PSP_CTRL_LTRIGGER,
  PSP_CTRL_RTRIGGER,
  PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,
  PSP_CTRL_UP,
  PSP_CTRL_RIGHT,
  PSP_CTRL_SELECT,
  PSP_CTRL_START,
  PSP_CTRL_ANALOG_UP,
  PSP_CTRL_ANALOG_DOWN,
  PSP_CTRL_ANALOG_LEFT,
  PSP_CTRL_ANALOG_RIGHT
};

u32 button_id_to_gba_mask[] =
{
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_A,
  BUTTON_B,
  BUTTON_L,
  BUTTON_R,
  BUTTON_START,
  BUTTON_SELECT,
  BUTTON_NONE,
  BUTTON_NONE,
  BUTTON_NONE,
  BUTTON_NONE
};

gui_action_type get_gui_input_fs_hold(u32 button_id)
{
  gui_action_type new_button = get_gui_input();
  if((last_buttons & button_psp_mask_to_config[button_id]) == 0)
    return CURSOR_BACK;

  return new_button;
}

u32 rapidfire_flag = 1;

u32 update_input()
{
  SceCtrlData ctrl_data;
  u32 buttons;
  u32 non_repeat_buttons;
  u32 button_id;
  u32 i;
  u32 new_key = 0;
  u32 analog_sensitivity = 92 - (analog_sensitivity_level * 4);
  u32 inv_analog_sensitivity = 256 - analog_sensitivity;

  sceCtrlPeekBufferPositive(&ctrl_data, 1);

  buttons = ctrl_data.Buttons;

  if(global_enable_analog)
  {
    if(ctrl_data.Lx < analog_sensitivity)
      buttons |= PSP_CTRL_ANALOG_LEFT;

    if(ctrl_data.Lx > inv_analog_sensitivity)
      buttons |= PSP_CTRL_ANALOG_RIGHT;

    if(ctrl_data.Ly < analog_sensitivity)
      buttons |= PSP_CTRL_ANALOG_UP;

    if(ctrl_data.Ly > inv_analog_sensitivity)
      buttons |= PSP_CTRL_ANALOG_DOWN;
  }

  non_repeat_buttons = (last_buttons ^ buttons) & buttons;
  last_buttons = buttons;

  for(i = 0; i < 16; i++)
  {
    if(non_repeat_buttons & button_psp_mask_to_config[i])
      button_id = gamepad_config_map[i];
    else
      button_id = BUTTON_ID_NONE;

    switch(button_id)
    {
      case BUTTON_ID_MENU:
      {
        u16 *screen_copy = copy_screen();
        u32 ret_val = menu(screen_copy);
        free(screen_copy);

        return ret_val;
      }

      case BUTTON_ID_LOADSTATE:
      {
        u8 current_savestate_filename[512];
        get_savestate_filename_noshot(savestate_slot,
         current_savestate_filename);
        load_state(current_savestate_filename);
        return 1;
      }

      case BUTTON_ID_SAVESTATE:
      {
        u8 current_savestate_filename[512];
        u16 *current_screen = copy_screen();
        get_savestate_filename_noshot(savestate_slot,
         current_savestate_filename);
        save_state(current_savestate_filename, current_screen);
        free(current_screen);
        return 0;
      }

      case BUTTON_ID_FASTFORWARD:
        print_string("FASTFORWARD", 0xFFFF, 0x0000, 0, 50);
        synchronize_flag ^= 1;
        return 0;
    }

    if(buttons & button_psp_mask_to_config[i])
    {
      button_id = gamepad_config_map[i];
      if(button_id < BUTTON_ID_MENU)
      {
        new_key |= button_id_to_gba_mask[button_id];
      }
      else

      if((button_id >= BUTTON_ID_RAPIDFIRE_A) &&
       (button_id <= BUTTON_ID_RAPIDFIRE_L))
      {
        rapidfire_flag ^= 1;
        if(rapidfire_flag)
        {
          new_key |= button_id_to_gba_mask[button_id -
           BUTTON_ID_RAPIDFIRE_A + BUTTON_ID_A];
        }
        else
        {
          new_key &= ~button_id_to_gba_mask[button_id -
           BUTTON_ID_RAPIDFIRE_A + BUTTON_ID_A];
        }
      }
    }
  }

  if((new_key | key) != key)
    trigger_key(new_key);

  key = new_key;

  io_registers[REG_P1] = (~key) & 0x3FF;

  return 0;
}

void init_input()
{
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

#endif


#if defined(GP2X_BUILD) || defined(PND_BUILD)

extern u32 fps_debug;

gui_action_type get_gui_input()
{
  gui_action_type new_button = CURSOR_NONE;
  u32 buttons = gpsp_plat_joystick_read();
  u32 new_buttons;

  static u32 last_buttons = 0;
  static u64 button_repeat_timestamp;

  delay_us(25000);

  new_buttons = (last_buttons ^ buttons) & buttons;
  last_buttons = buttons;

  new_button = gpsp_plat_buttons_to_cursor(new_buttons);
  if(new_button != CURSOR_NONE)
  {
    get_ticks_us(&button_repeat_timestamp);
    button_repeat_state = BUTTON_HELD_INITIAL;
    button_repeat = new_buttons;
    cursor_repeat = new_button;
  }
  else
  {
    if(buttons & button_repeat)
    {
      u64 new_ticks;
      get_ticks_us(&new_ticks);

      if(button_repeat_state == BUTTON_HELD_INITIAL)
      {
        if((new_ticks - button_repeat_timestamp) >
         BUTTON_REPEAT_START)
        {
          new_button = cursor_repeat;
          button_repeat_timestamp = new_ticks;
          button_repeat_state = BUTTON_HELD_REPEAT;
        }
      }

      if(button_repeat_state == BUTTON_HELD_REPEAT)
      {
        if((new_ticks - button_repeat_timestamp) >
         BUTTON_REPEAT_CONTINUE)
        {
          new_button = cursor_repeat;
          button_repeat_timestamp = new_ticks;
        }
      }
    }
  }

  return new_button;
}

u32 button_id_to_gba_mask[] =
{
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_A,
  BUTTON_B,
  BUTTON_L,
  BUTTON_R,
  BUTTON_START,
  BUTTON_SELECT,
  BUTTON_NONE,
  BUTTON_NONE,
  BUTTON_NONE,
  BUTTON_NONE
};

u32 update_input()
{
  static u32 rapidfire_flag = 1;
  static u32 last_buttons;
  u32 handled_buttons;
  u32 button_id;
  u32 new_key = 0;
  u32 buttons = gpsp_plat_joystick_read();
  u32 i;

#ifdef GP2X_BUILD
  if((buttons & GP2X_VOL_DOWN) && (buttons & GP2X_VOL_UP))
  {
    buttons &= ~(GP2X_VOL_DOWN | GP2X_VOL_UP);
    buttons |= GP2X_VOL_MIDDLE;
  }

  /* for Wiz */
  if((buttons & GP2X_VOL_DOWN) && (buttons & GP2X_SELECT))
  {
    buttons &= ~(GP2X_VOL_DOWN | GP2X_SELECT);
    buttons |= GP2X_VOL_MIDDLE;
  }

  last_buttons &= ~(GP2X_VOL_DOWN | GP2X_VOL_UP);
#endif

  handled_buttons = (last_buttons ^ buttons) & buttons;
  last_buttons = buttons;

  for(i = 0; i < PLAT_BUTTON_COUNT; i++)
  {
    if(handled_buttons & button_plat_mask_to_config[i])
      button_id = gamepad_config_map[i];
    else
      button_id = BUTTON_ID_NONE;

    switch(button_id)
    {
      case BUTTON_ID_MENU:
      {
        u16 *screen_copy = copy_screen();
        u32 ret_val = menu(screen_copy);
        free(screen_copy);

        return ret_val;
      }

      case BUTTON_ID_LOADSTATE:
      {
        char current_savestate_filename[512];
        get_savestate_filename_noshot(savestate_slot,
         current_savestate_filename);
        load_state(current_savestate_filename);
        return 1;
      }

      case BUTTON_ID_SAVESTATE:
      {
        char current_savestate_filename[512];
        u16 *current_screen = copy_screen();
        get_savestate_filename_noshot(savestate_slot,
         current_savestate_filename);
        save_state(current_savestate_filename, current_screen);
        free(current_screen);
        return 0;
      }

      case BUTTON_ID_FASTFORWARD:
        synchronize_flag ^= 1;
        return 0;

#ifdef GP2X_BUILD
      case BUTTON_ID_VOLUP:
        gp2x_sound_volume(1);
        break;

      case BUTTON_ID_VOLDOWN:
        gp2x_sound_volume(0);
        break;
#endif

      case BUTTON_ID_FPS:
        fps_debug ^= 1;
        break;
    }

    if(buttons & button_plat_mask_to_config[i])
    {
      button_id = gamepad_config_map[i];
      if(button_id < BUTTON_ID_MENU)
      {
        new_key |= button_id_to_gba_mask[button_id];
      }
      else

      if((button_id >= BUTTON_ID_RAPIDFIRE_A) &&
       (button_id <= BUTTON_ID_RAPIDFIRE_L))
      {
        rapidfire_flag ^= 1;
        if(rapidfire_flag)
        {
          new_key |= button_id_to_gba_mask[button_id -
           BUTTON_ID_RAPIDFIRE_A + BUTTON_ID_A];
        }
        else
        {
          new_key &= ~button_id_to_gba_mask[button_id -
           BUTTON_ID_RAPIDFIRE_A + BUTTON_ID_A];
        }
      }
    }
  }

  if((new_key | key) != key)
    trigger_key(new_key);

  key = new_key;

  io_registers[REG_P1] = (~key) & 0x3FF;

  return 0;
}

void init_input()
{

}

#endif


#ifdef PC_BUILD

u32 key_map(SDLKey key_sym)
{
  switch(key_sym)
  {
    case SDLK_LSHIFT:
      return BUTTON_L;

    case SDLK_x:
      return BUTTON_R;

    case SDLK_DOWN:
      return BUTTON_DOWN;

    case SDLK_UP:
      return BUTTON_UP;

    case SDLK_LEFT:
      return BUTTON_LEFT;

    case SDLK_RIGHT:
      return BUTTON_RIGHT;

    case SDLK_RETURN:
      return BUTTON_START;

    case SDLK_RSHIFT:
      return BUTTON_SELECT;

    case SDLK_LCTRL:
      return BUTTON_B;

    case SDLK_LALT:
      return BUTTON_A;

    default:
      return BUTTON_NONE;
  }
}

u32 joy_map(u32 button)
{
  switch(button)
  {
    case 4:
      return BUTTON_L;

    case 5:
      return BUTTON_R;

    case 9:
      return BUTTON_START;

    case 8:
      return BUTTON_SELECT;

    case 0:
      return BUTTON_B;

    case 1:
      return BUTTON_A;

    default:
      return BUTTON_NONE;
  }
}

gui_action_type get_gui_input()
{
  SDL_Event event;
  gui_action_type gui_action = CURSOR_NONE;

  delay_us(30000);

  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_QUIT:
        quit();

      case SDL_KEYDOWN:
      {
        switch(event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            gui_action = CURSOR_EXIT;
            break;

          case SDLK_DOWN:
            gui_action = CURSOR_DOWN;
            break;

          case SDLK_UP:
            gui_action = CURSOR_UP;
            break;

          case SDLK_LEFT:
            gui_action = CURSOR_LEFT;
            break;

          case SDLK_RIGHT:
            gui_action = CURSOR_RIGHT;
            break;

          case SDLK_RETURN:
            gui_action = CURSOR_SELECT;
            break;

          case SDLK_BACKSPACE:
            gui_action = CURSOR_BACK;
            break;

          default:
            break;
        }
        break;
      }
    }
  }

  return gui_action;
}

u32 update_input()
{
  SDL_Event event;

  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_QUIT:
        quit();

      case SDL_KEYDOWN:
      {
        if(event.key.keysym.sym == SDLK_ESCAPE)
        {
          quit();
        }

        if(event.key.keysym.sym == SDLK_BACKSPACE)
        {
          u16 *screen_copy = copy_screen();
          u32 ret_val = menu(screen_copy);
          free(screen_copy);

          return ret_val;
        }
        else

        if(event.key.keysym.sym == SDLK_F1)
        {
          debug_on();
        }
        else

        if(event.key.keysym.sym == SDLK_F2)
        {
          FILE *fp = fopen("palette_ram.bin", "wb");
          printf("writing palette RAM\n");
          fwrite(palette_ram, 1024, 1, fp);
          fclose(fp);
          printf("writing palette VRAM\n");
          fp = fopen("vram.bin", "wb");
          fwrite(vram, 1024 * 96, 1, fp);
          fclose(fp);
          printf("writing palette OAM RAM\n");
          fp = fopen("oam_ram.bin", "wb");
          fwrite(oam_ram, 1024, 1, fp);
          fclose(fp);
          printf("writing palette I/O registers\n");
          fp = fopen("io_registers.bin", "wb");
          fwrite(io_registers, 1024, 1, fp);
          fclose(fp);
        }
        else

        if(event.key.keysym.sym == SDLK_F3)
        {
          dump_translation_cache();
        }
        else

        if(event.key.keysym.sym == SDLK_F5)
        {
          char current_savestate_filename[512];
          u16 *current_screen = copy_screen();
          get_savestate_filename_noshot(savestate_slot,
           current_savestate_filename);
          save_state(current_savestate_filename, current_screen);
          free(current_screen);
        }
        else

        if(event.key.keysym.sym == SDLK_F7)
        {
          char current_savestate_filename[512];
          get_savestate_filename_noshot(savestate_slot,
           current_savestate_filename);
          load_state(current_savestate_filename);
          debug_on();
          return 1;
        }
        else

        if(event.key.keysym.sym == SDLK_BACKQUOTE)
        {
          synchronize_flag ^= 1;
        }
        else
        {
          key |= key_map(event.key.keysym.sym);
          trigger_key(key);
        }

        break;
      }

      case SDL_KEYUP:
      {
        key &= ~(key_map(event.key.keysym.sym));
        break;
      }

      case SDL_JOYBUTTONDOWN:
      {
        key |= joy_map(event.jbutton.button);
        trigger_key(key);
        break;
      }

      case SDL_JOYBUTTONUP:
      {
        key &= ~(joy_map(event.jbutton.button));
        break;
      }
    }
  }

  io_registers[REG_P1] = (~key) & 0x3FF;

  return 0;
}

void init_input()
{
  u32 joystick_count = SDL_NumJoysticks();

  if(joystick_count > 0)
  {
    SDL_JoystickOpen(0);
    SDL_JoystickEventState(SDL_ENABLE);
  }
}
#endif

#ifdef RPI_BUILD
u32 menu_hotkey = 1;

gui_action_type get_gui_input()
{
   SDL_Event event;
   gui_action_type gui_action = CURSOR_NONE;
   int action = BUTTON_NONE;

   delay_us(30000);

   while(SDL_PollEvent(&event)) {
      switch(event.type) {
         case SDL_QUIT:
           quit();

         case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
               action = BUTTON_B;
            } else {
               action = key_map(event.key.keysym.sym);
            }
         break;

         case SDL_JOYBUTTONDOWN:
            action = joy_map(event.jbutton.button+JOY_BUTTON_1);
         break;

         case SDL_JOYAXISMOTION: {
            if (event.jaxis.axis==0) { //Left-Right
               if (event.jaxis.value < -TRESHOLD) action = joy_map(JOY_ASIX_XM);
                  else if (event.jaxis.value > TRESHOLD) action = joy_map(JOY_ASIX_XP);
            }
            if (event.jaxis.axis==1) {  //Up-Down
               if (event.jaxis.value < -TRESHOLD) action = joy_map(JOY_ASIX_YM);
                  else if (event.jaxis.value > TRESHOLD) action = joy_map(JOY_ASIX_YP);
            }
        }
        default:
           break;
     }
  }

  switch (action) {
     case BUTTON_UP:
        gui_action = CURSOR_UP;
     break;

     case BUTTON_DOWN:
        gui_action = CURSOR_DOWN;
     break;

     case BUTTON_LEFT:
        gui_action = CURSOR_LEFT;
     break;

     case BUTTON_RIGHT:
        gui_action = CURSOR_RIGHT;
    break;

    case BUTTON_START:
    case BUTTON_A:
       gui_action = CURSOR_SELECT;
    break;

    case BUTTON_SELECT:
    case BUTTON_B:
       gui_action = CURSOR_EXIT;
    break;

    default:
    break;
  }

  return gui_action;
}

u32 update_input()
{
  SDL_Event event;
  int newkey;

  newkey = key;

  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_QUIT:
        quit();

      case SDL_KEYDOWN:
      {
        if(event.key.keysym.sym == SDLK_ESCAPE)
        {
          quit();
        }
        if(event.key.keysym.sym == SDLK_F10)
        {
          u16 *screen_copy = copy_screen();
          u32 ret_val = menu(screen_copy);
          free(screen_copy);

          return ret_val;
        }
        else
        if(event.key.keysym.sym == SDLK_F5)
        {
          char current_savestate_filename[512];
          u16 *current_screen = copy_screen();
          get_savestate_filename_noshot(savestate_slot,
           current_savestate_filename);
          save_state(current_savestate_filename, current_screen);
          free(current_screen);
        }
        else

        if(event.key.keysym.sym == SDLK_F7)
        {
          char current_savestate_filename[512];
          get_savestate_filename_noshot(savestate_slot,
           current_savestate_filename);
          load_state(current_savestate_filename);
          debug_on();
          return 1;
        }
        else

        if(event.key.keysym.sym == SDLK_BACKQUOTE)
        {
          synchronize_flag ^= 1;
        }
        else
        {
          newkey |= key_map(event.key.keysym.sym);
        }

        break;
      }

      case SDL_KEYUP:
      {
        newkey &= ~(key_map(event.key.keysym.sym));
        break;
      }

      case SDL_JOYBUTTONDOWN:
      {
        newkey |= joy_map(event.jbutton.button+JOY_BUTTON_1);
        break;
      }

      case SDL_JOYBUTTONUP:
      {
        newkey &= ~(joy_map(event.jbutton.button+JOY_BUTTON_1));
        break;
      }

      case SDL_JOYAXISMOTION:
      {
         if (event.jaxis.axis==0) { //Left-Right
            if (event.jaxis.value < -TRESHOLD) {
               newkey |= joy_map(JOY_ASIX_XM);
               newkey &= ~(joy_map(JOY_ASIX_XP));
            } else if (event.jaxis.value > TRESHOLD) {
               newkey &= ~(joy_map(JOY_ASIX_XM));
               newkey |= joy_map(JOY_ASIX_XP);
            }else {
               newkey &= ~(joy_map(JOY_ASIX_XM));
               newkey &= ~(joy_map(JOY_ASIX_XP));
            }
        }

        if (event.jaxis.axis==1) {  //Up-Down
            if (event.jaxis.value < -TRESHOLD)  {
               newkey |= joy_map(JOY_ASIX_YM);
               newkey &= ~(joy_map(JOY_ASIX_YP));
            } else if (event.jaxis.value > TRESHOLD) {
               newkey &= ~(joy_map(JOY_ASIX_YM));
               newkey |= joy_map(JOY_ASIX_YP);
            } else {
               newkey &= ~(joy_map(JOY_ASIX_YM));
               newkey &= ~(joy_map(JOY_ASIX_YP));
            }
         }
      break;
      }
    }
  }

  if ((menu_hotkey) && ( newkey == (BUTTON_SELECT | BUTTON_R))) {
      newkey &= ~(BUTTON_SELECT | BUTTON_R);

      u16 *screen_copy = copy_screen();
      u32 ret_val = menu(screen_copy);
      free(screen_copy);

      return ret_val;
  }

  if (key != newkey) {
     key = newkey;
     trigger_key(key);
     io_registers[REG_P1] = (~key) & 0x3FF;
  }
  return 0;
}

void  init_input()
{
  u32 joystick_count = SDL_NumJoysticks();

  if(joystick_count > 0)
  {
    SDL_JoystickOpen(0);
    SDL_JoystickEventState(SDL_ENABLE);
  }
}

#endif

#ifdef _3DS

u32 gamepad_config_map[32] =
{
  BUTTON_ID_A,                  // A
  BUTTON_ID_B,                  // B
  BUTTON_ID_MENU,               // Select
  BUTTON_ID_START,              // Start
  BUTTON_ID_RIGHT,              // Right
  BUTTON_ID_LEFT,               // Left
  BUTTON_ID_UP,                 // Up
  BUTTON_ID_DOWN,               // Down
  BUTTON_ID_R,                  // R
  BUTTON_ID_L,                  // L
  BUTTON_ID_START,              // X
  BUTTON_ID_SELECT,             // Y
  -1,
  -1,
  BUTTON_ID_L,                  // ZL
  BUTTON_ID_R,                  // ZR
  -1,
  -1,
  -1,
  -1,
  0,               // Touch
  -1,
  -1,
  -1,
  BUTTON_ID_RIGHT,              // C-stick Right
  BUTTON_ID_LEFT,               // C-stick Left
  BUTTON_ID_UP,                 // C-stick Up
  BUTTON_ID_DOWN,               // C-stick Down
  BUTTON_ID_RIGHT,              // C-pad Right
  BUTTON_ID_LEFT,               // C-pad Left
  BUTTON_ID_UP,                 // C-pad Up
  BUTTON_ID_DOWN                // C-pad Down
};

u32 gamepad_config[32] =
{
  BUTTON_A,                  // A
  BUTTON_B,                  // B
  BUTTON_SELECT,             // Select
  BUTTON_START,              // Start
  BUTTON_RIGHT,              // Right
  BUTTON_LEFT,               // Left
  BUTTON_UP,                 // Up
  BUTTON_DOWN,               // Down
  BUTTON_R,                  // R
  BUTTON_L,                  // L
  BUTTON_START,              // X
  BUTTON_SELECT,             // Y
  -1,
  -1,
  BUTTON_L,                  // ZL
  BUTTON_R,                  // ZR
  -1,
  -1,
  -1,
  -1,
  -2,                        // Touch
  -1,
  -1,
  -1,
  BUTTON_RIGHT,              // C-stick Right
  BUTTON_LEFT,               // C-stick Left
  BUTTON_UP,                 // C-stick Up
  BUTTON_DOWN,               // C-stick Down
  BUTTON_RIGHT,              // C-pad Right
  BUTTON_LEFT,               // C-pad Left
  BUTTON_UP,                 // C-pad Up
  BUTTON_DOWN                // C-pad Down
};

gui_action_type get_gui_input()
{
   //SDL_Event event;
   gui_action_type gui_action = CURSOR_NONE;
   int action = BUTTON_NONE;

   delay_us(30000);

   gspWaitForVBlank();
   hidScanInput();
   u32 kDown = hidKeysDown();
   u32 kHeld = hidKeysHeld();
   u32 button_name = -1;
   int i = 0;

   for(i = 0; i < 32; i++)
   {
	   if(kDown & BIT(i))
		   button_name = gamepad_config_map[i];
   }

  switch (button_name) {
     case BUTTON_ID_UP:
        gui_action = CURSOR_UP;
     break;

     case BUTTON_ID_DOWN:
        gui_action = CURSOR_DOWN;
     break;

     case BUTTON_ID_LEFT:
        gui_action = CURSOR_LEFT;
     break;

     case BUTTON_ID_RIGHT:
        gui_action = CURSOR_RIGHT;
    break;

    case BUTTON_ID_A:
       gui_action = CURSOR_SELECT;
    break;

    case BUTTON_ID_MENU:
    case BUTTON_ID_B:
       gui_action = CURSOR_BACK;
    break;
    
    case BUTTON_ID_START: //X
       gui_action = CURSOR_EXIT;
    break;

    default:
    break;
  }

  return gui_action;
}

u32 update_input()
{
   gui_action_type gui_action = CURSOR_NONE;
   int action = BUTTON_NONE;

   gspWaitForVBlank();
   hidScanInput();
   u32 kDown = hidKeysDown();
   u32 kHeld = hidKeysHeld();
   int newkey = 0;

   int i = 0;
   u16 *screen_offset = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
   for(i = 0; i < 32; i++)
   {
	   if(kHeld & BIT(i))
        {
		   newkey |= gamepad_config[i];
        } 
   }

   /*for(i = 0; i < 10; i++)
   {
	   if(key & BIT(i))
        {
		   screen_offset[i] = 0xFFFF; 
        }
        else
	   	screen_offset[i] = 0; 
   }*/

  if (key != newkey) {
     key = newkey;
     trigger_key(key);
     io_registers[REG_P1] = (~key) & 0x3FF;
  }

  if(kHeld & (KEY_X))
  {
     if(!__service_ptr) //We're running as a .cia
        aptReturnToMenu();
     else
        exit_time = 1;
  }
  
  if(!aptMainLoop())
  {
     exit_time = 1;
  }

  return 0;
}

void  init_input()
{
  
}
#endif


#define input_savestate_builder(type)                                         \
void input_##type##_savestate(file_tag_type savestate_file)                   \
{                                                                             \
  file_##type##_variable(savestate_file, key);                                \
}                                                                             \

input_savestate_builder(read);
input_savestate_builder(write_mem);

