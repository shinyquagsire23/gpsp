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
//#include <SDL.h>
u32 global_enable_audio = 1;

direct_sound_struct direct_sound_channel[2];
gbc_sound_struct gbc_sound_channel[4];

#ifdef _3DS
u32 sound_frequency = 22050;
#else
u32 sound_frequency = 44100;
#endif

//SDL_mutex *sound_mutex;
//static SDL_cond *sound_cv;

#ifdef PSP_BUILD
u32 audio_buffer_size_number = 1;
#elif defined(POLLUX_BUILD)
u32 audio_buffer_size_number = 7;
#else
u32 audio_buffer_size_number = 8;
#endif

u32 sound_on;
static u32 audio_buffer_size;
static s16 *sound_buffer_l;
static s16 *sound_buffer_r;
static s16 *sound_buffer_play_l;
static s16 *sound_buffer_play_r;
static u32 sound_buffer_base;

static u32 sound_last_cpu_ticks;
static fixed16_16 gbc_sound_tick_step;

static u32 sound_exit_flag;

// Queue 1, 2, or 4 samples to the top of the DS FIFO, wrap around circularly

#define sound_timer_queue(size, value)                                        \
  *((s##size *)(ds->fifo + ds->fifo_top)) = value;                            \
  ds->fifo_top = (ds->fifo_top + 1) % 32;                                     \

void sound_timer_queue8(u32 channel, u8 value)
{
  direct_sound_struct *ds = direct_sound_channel + channel;
  sound_timer_queue(8, value);
}

void sound_timer_queue16(u32 channel, u16 value)
{
  direct_sound_struct *ds = direct_sound_channel + channel;
  sound_timer_queue(8, value & 0xFF);
  sound_timer_queue(8, value >> 8);
}

void sound_timer_queue32(u32 channel, u32 value)
{
  direct_sound_struct *ds = direct_sound_channel + channel;

  sound_timer_queue(8, value & 0xFF);
  sound_timer_queue(8, (value >> 8) & 0xFF);
  sound_timer_queue(8, (value >> 16) & 0xFF);
  sound_timer_queue(8, value >> 24);
}

// Unqueue 1 sample from the base of the DS FIFO and place it on the audio
// buffer for as many samples as necessary. If the DS FIFO is 16 bytes or
// smaller and if DMA is enabled for the sound channel initiate a DMA transfer
// to the DS FIFO.

#define render_sample_null()                                                  \

#define render_sample_left()                                                  \
  sound_buffer_play_l[buffer_index] += current_sample +                              \
   fp16_16_to_u32((next_sample - current_sample) * (fifo_fractional >> 8))    \

#define render_sample_right()                                                 \
  sound_buffer_play_r[buffer_index] += current_sample +                          \
   fp16_16_to_u32((next_sample - current_sample) * (fifo_fractional >> 8))    \

#define render_sample_both()                                                  \
  dest_sample = current_sample +                                              \
   fp16_16_to_u32((next_sample - current_sample) * (fifo_fractional >> 8));   \
  sound_buffer_play_l[buffer_index] += dest_sample;                                  \
  sound_buffer_play_r[buffer_index] += dest_sample                               \

#define render_samples(type)                                                  \
  while(fifo_fractional <= 0xFFFFFF)                                          \
  {                                                                           \
    render_sample_##type();                                                   \
    fifo_fractional += frequency_step;                                        \
    buffer_index = (buffer_index + 1);                          \
  }                                                                           \

void sound_timer(fixed8_24 frequency_step, u32 channel)
{
  direct_sound_struct *ds = direct_sound_channel + channel;

  fixed8_24 fifo_fractional = ds->fifo_fractional;
  u32 buffer_index = ds->buffer_index;
  s16 current_sample, next_sample, dest_sample;

  current_sample = ds->fifo[ds->fifo_base] << 4;
  ds->fifo_base = (ds->fifo_base + 1) % 32;
  next_sample = ds->fifo[ds->fifo_base] << 4;

  if(sound_on == 1)
  {
    if(ds->volume == DIRECT_SOUND_VOLUME_50)
    {
      current_sample >>= 1;
      next_sample >>= 1;
    }

    switch(ds->status)
    {
      case DIRECT_SOUND_INACTIVE:
        render_samples(null);
        break;

      case DIRECT_SOUND_RIGHT:
        render_samples(right);
        break;

      case DIRECT_SOUND_LEFT:
        render_samples(left);
        break;

      case DIRECT_SOUND_LEFTRIGHT:
        render_samples(both);
        break;
    }
  }
  else
  {
    render_samples(null);
  }

  if(buffer_index > 0x2000) //0x1000 works but kinda lags, 0x8000 and above have too much latency
  {
    if(channel == 1)
      sound_callback(sound_buffer_play_l, sound_buffer_play_r, buffer_index);
    buffer_index = 0;
  }
  ds->buffer_index = buffer_index;
  ds->fifo_fractional = fp8_24_fractional_part(fifo_fractional);

  if(((ds->fifo_top - ds->fifo_base) % 32) <= 16)
  {
    if(dma[1].direct_sound_channel == channel)
      dma_transfer(dma + 1);

    if(dma[2].direct_sound_channel == channel)
      dma_transfer(dma + 2);
  }
}

void sound_reset_fifo(u32 channel)
{
  direct_sound_struct *ds = direct_sound_channel;

  memset(ds->fifo, 0, 32);
}

// Initial pattern data = 4bits (signed)
// Channel volume = 12bits
// Envelope volume = 14bits
// Master volume = 2bits

// Recalculate left and right volume as volume changes.
// To calculate the current sample, use (sample * volume) >> 16

// Square waves range from -8 (low) to 7 (high)

s8 square_pattern_duty[4][8] =
{
  { 0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0xF8, 0xF8, 0xF8 },
  { 0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0x07, 0xF8, 0xF8 },
  { 0xF8, 0xF8, 0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8 },
  { 0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8, 0x07, 0x07 },
};

s8 wave_samples[64];

u32 noise_table15[1024];
u32 noise_table7[4];

u32 gbc_sound_master_volume_table[4] = { 1, 2, 4, 0 };

u32 gbc_sound_channel_volume_table[8] =
{
  fixed_div(0, 7, 12),
  fixed_div(1, 7, 12),
  fixed_div(2, 7, 12),
  fixed_div(3, 7, 12),
  fixed_div(4, 7, 12),
  fixed_div(5, 7, 12),
  fixed_div(6, 7, 12),
  fixed_div(7, 7, 12)
};

u32 gbc_sound_envelope_volume_table[16] =
{
  fixed_div(0, 15, 14),
  fixed_div(1, 15, 14),
  fixed_div(2, 15, 14),
  fixed_div(3, 15, 14),
  fixed_div(4, 15, 14),
  fixed_div(5, 15, 14),
  fixed_div(6, 15, 14),
  fixed_div(7, 15, 14),
  fixed_div(8, 15, 14),
  fixed_div(9, 15, 14),
  fixed_div(10, 15, 14),
  fixed_div(11, 15, 14),
  fixed_div(12, 15, 14),
  fixed_div(13, 15, 14),
  fixed_div(14, 15, 14),
  fixed_div(15, 15, 14)
};

u32 gbc_sound_buffer_index = 0;
u32 gbc_sound_last_cpu_ticks = 0;
u32 gbc_sound_partial_ticks = 0;

u32 gbc_sound_master_volume_left;
u32 gbc_sound_master_volume_right;
u32 gbc_sound_master_volume;

u32 last_buffer_index;

#define update_volume_channel_envelope(channel)                               \
  volume_##channel = gbc_sound_envelope_volume_table[envelope_volume] *       \
   gbc_sound_channel_volume_table[gbc_sound_master_volume_##channel] *        \
   gbc_sound_master_volume_table[gbc_sound_master_volume]                     \

#define update_volume_channel_noenvelope(channel)                             \
  volume_##channel = gs->wave_volume *                                        \
   gbc_sound_channel_volume_table[gbc_sound_master_volume_##channel] *        \
   gbc_sound_master_volume_table[gbc_sound_master_volume]                     \

#define update_volume(type)                                                   \
  update_volume_channel_##type(left);                                         \
  update_volume_channel_##type(right)                                         \

#define update_tone_sweep()                                                   \
  if(gs->sweep_status)                                                        \
  {                                                                           \
    u32 sweep_ticks = gs->sweep_ticks - 1;                                    \
                                                                              \
    if(sweep_ticks == 0)                                                      \
    {                                                                         \
      u32 rate = gs->rate;                                                    \
                                                                              \
      if(gs->sweep_direction)                                                 \
        rate = rate - (rate >> gs->sweep_shift);                              \
      else                                                                    \
        rate = rate + (rate >> gs->sweep_shift);                              \
                                                                              \
      if(rate > 2048)                                                         \
        rate = 2048;                                                          \
                                                                              \
      frequency_step = float_to_fp16_16(((131072.0f / (2048 - rate)) * 8.0f)  \
       / sound_frequency);                                                    \
                                                                              \
      gs->frequency_step = frequency_step;                                    \
      gs->rate = rate;                                                        \
                                                                              \
      sweep_ticks = gs->sweep_initial_ticks;                                  \
    }                                                                         \
    gs->sweep_ticks = sweep_ticks;                                            \
  }                                                                           \

#define update_tone_nosweep()                                                 \

#define update_tone_envelope()                                                \
  if(gs->envelope_status)                                                     \
  {                                                                           \
    u32 envelope_ticks = gs->envelope_ticks - 1;                              \
    envelope_volume = gs->envelope_volume;                                    \
                                                                              \
    if(envelope_ticks == 0)                                                   \
    {                                                                         \
      if(gs->envelope_direction)                                              \
      {                                                                       \
        if(envelope_volume != 15)                                             \
          envelope_volume = gs->envelope_volume + 1;                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        if(envelope_volume != 0)                                              \
          envelope_volume = gs->envelope_volume - 1;                          \
      }                                                                       \
                                                                              \
      update_volume(envelope);                                                \
                                                                              \
      gs->envelope_volume = envelope_volume;                                  \
      gs->envelope_ticks = gs->envelope_initial_ticks;                        \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      gs->envelope_ticks = envelope_ticks;                                    \
    }                                                                         \
  }                                                                           \

#define update_tone_noenvelope()                                              \

#define update_tone_counters(envelope_op, sweep_op)                           \
  tick_counter += gbc_sound_tick_step;                                        \
  if(tick_counter > 0xFFFF)                                                   \
  {                                                                           \
    if(gs->length_status)                                                     \
    {                                                                         \
      u32 length_ticks = gs->length_ticks - 1;                                \
      gs->length_ticks = length_ticks;                                        \
                                                                              \
      if(length_ticks == 0)                                                   \
      {                                                                       \
        gs->active_flag = 0;                                                  \
        break;                                                                \
      }                                                                       \
    }                                                                         \
                                                                              \
    update_tone_##envelope_op();                                              \
    update_tone_##sweep_op();                                                 \
                                                                              \
    tick_counter &= 0xFFFF;                                                   \
  }                                                                           \

#define gbc_sound_render_sample_right()                                       \
  sound_buffer_play_r[buffer_index/2] += (current_sample * volume_right) >> 22     \

#define gbc_sound_render_sample_left()                                        \
  sound_buffer_play_l[buffer_index/2] += (current_sample * volume_left) >> 22          \

#define gbc_sound_render_sample_both()                                        \
  gbc_sound_render_sample_right();                                            \
  gbc_sound_render_sample_left()                                              \

#define gbc_sound_render_samples(type, sample_length, envelope_op, sweep_op)  \
  for(i = 0; i < buffer_ticks; i++)                                           \
  {                                                                           \
    current_sample =                                                          \
     sample_data[fp16_16_to_u32(sample_index) % sample_length];               \
    gbc_sound_render_sample_##type();                                         \
                                                                              \
    sample_index += frequency_step;                                           \
    buffer_index = (buffer_index + 2) % BUFFER_SIZE;                          \
                                                                              \
    update_tone_counters(envelope_op, sweep_op);                              \
  }                                                                           \

#define gbc_noise_wrap_full 32767

#define gbc_noise_wrap_half 126

#define get_noise_sample_full()                                               \
  current_sample =                                                            \
   ((s32)(noise_table15[fp16_16_to_u32(sample_index) >> 5] <<                 \
   (fp16_16_to_u32(sample_index) & 0x1F)) >> 31) & 0x0F                       \

#define get_noise_sample_half()                                               \
  current_sample =                                                            \
   ((s32)(noise_table7[fp16_16_to_u32(sample_index) >> 5] <<                  \
   (fp16_16_to_u32(sample_index) & 0x1F)) >> 31) & 0x0F                       \

#define gbc_sound_render_noise(type, noise_type, envelope_op, sweep_op)       \
  for(i = 0; i < buffer_ticks; i++)                                           \
  {                                                                           \
    get_noise_sample_##noise_type();                                          \
    gbc_sound_render_sample_##type();                                         \
                                                                              \
    sample_index += frequency_step;                                           \
                                                                              \
    if(sample_index >= u32_to_fp16_16(gbc_noise_wrap_##noise_type))           \
      sample_index -= u32_to_fp16_16(gbc_noise_wrap_##noise_type);            \
                                                                              \
    buffer_index = (buffer_index + 2) % BUFFER_SIZE;                          \
    update_tone_counters(envelope_op, sweep_op);                              \
  }                                                                           \

#define gbc_sound_render_channel(type, sample_length, envelope_op, sweep_op)  \
  buffer_index = gbc_sound_buffer_index;                                      \
  sample_index = gs->sample_index;                                            \
  frequency_step = gs->frequency_step;                                        \
  tick_counter = gs->tick_counter;                                            \
                                                                              \
  update_volume(envelope_op);                                                 \
                                                                              \
  switch(gs->status)                                                          \
  {                                                                           \
    case GBC_SOUND_INACTIVE:                                                  \
      break;                                                                  \
                                                                              \
    case GBC_SOUND_LEFT:                                                      \
      gbc_sound_render_##type(left, sample_length, envelope_op, sweep_op);    \
      break;                                                                  \
                                                                              \
    case GBC_SOUND_RIGHT:                                                     \
      gbc_sound_render_##type(right, sample_length, envelope_op, sweep_op);   \
      break;                                                                  \
                                                                              \
    case GBC_SOUND_LEFTRIGHT:                                                 \
      gbc_sound_render_##type(both, sample_length, envelope_op, sweep_op);    \
      break;                                                                  \
  }                                                                           \
                                                                              \
  gs->sample_index = sample_index;                                            \
  gs->tick_counter = tick_counter;                                            \

#define gbc_sound_load_wave_ram(bank)                                         \
  wave_bank = wave_samples + (bank * 32);                                     \
  for(i = 0, i2 = 0; i < 16; i++, i2 += 2)                                    \
  {                                                                           \
    current_sample = wave_ram[i];                                             \
    wave_bank[i2] = (((current_sample >> 4) & 0x0F) - 8);                     \
    wave_bank[i2 + 1] = ((current_sample & 0x0F) - 8);                        \
  }                                                                           \

void update_gbc_sound(u32 cpu_ticks)
{
  fixed16_16 buffer_ticks = float_to_fp16_16((float)(cpu_ticks -
   gbc_sound_last_cpu_ticks) * sound_frequency / GBC_BASE_RATE);
  u32 i, i2;
  gbc_sound_struct *gs = gbc_sound_channel;
  fixed16_16 sample_index, frequency_step;
  fixed16_16 tick_counter;
  u32 buffer_index;
  s32 volume_left, volume_right;
  u32 envelope_volume;
  s32 current_sample;
  u32 sound_status = address16(io_registers, 0x84) & 0xFFF0;
  s8 *sample_data;
  s8 *wave_bank;
  u8 *wave_ram = ((u8 *)io_registers) + 0x90;

  gbc_sound_partial_ticks += fp16_16_fractional_part(buffer_ticks);
  buffer_ticks = fp16_16_to_u32(buffer_ticks);

  if(gbc_sound_partial_ticks > 0xFFFF)
  {
    buffer_ticks += 1;
    gbc_sound_partial_ticks &= 0xFFFF;
  }

  //SDL_LockMutex(sound_mutex);
  if(synchronize_flag)
  {
    if(((gbc_sound_buffer_index - sound_buffer_base) % BUFFER_SIZE) >=
     (audio_buffer_size * 2))
    {
      while(((gbc_sound_buffer_index - sound_buffer_base) % BUFFER_SIZE) >
       (audio_buffer_size * 3 / 2))
      {
	   break; //TODO: Fix this
        //SDL_CondWait(sound_cv, sound_mutex);
      }

#ifdef PSP_BUILD
      if(current_frameskip_type == auto_frameskip)
      {
        sceDisplayWaitVblankStart();
        real_frame_count = 0;
        virtual_frame_count = 0;
      }
#else
      if(current_frameskip_type == auto_frameskip)
      {
/*
        u64 current_ticks;
        u64 next_ticks;
        get_ticks_us(&current_ticks);

        next_ticks = ((current_ticks + 16666) / 16667) * 16667;
        delay_us(next_ticks - current_ticks);

        get_ticks_us(&frame_count_initial_timestamp);
*/
        /* prevent frameskip, or it will cause more audio,
	 * then more waiting here, then frame skip again, ... */
        num_skipped_frames = 100;
      }
#endif

    }
  }
  if(sound_on == 1)
  {
    gs = gbc_sound_channel + 0;
    if(gs->active_flag)
    {
      sound_status |= 0x01;
      sample_data = gs->sample_data;
      envelope_volume = gs->envelope_volume;
      gbc_sound_render_channel(samples, 8, envelope, sweep);
    }

    gs = gbc_sound_channel + 1;
    if(gs->active_flag)
    {
      sound_status |= 0x02;
      sample_data = gs->sample_data;
      envelope_volume = gs->envelope_volume;
      gbc_sound_render_channel(samples, 8, envelope, nosweep);
    }

    gs = gbc_sound_channel + 2;
    if(gbc_sound_wave_update)
    {
      if(gs->wave_bank == 1)
      {
        gbc_sound_load_wave_ram(1);
      }
      else
      {
        gbc_sound_load_wave_ram(0);
      }

      gbc_sound_wave_update = 0;
    }

    if((gs->active_flag) && (gs->master_enable))
    {
      sound_status |= 0x04;
      sample_data = wave_samples;
      if(gs->wave_type == 0)
      {
        if(gs->wave_bank == 1)
          sample_data += 32;

        gbc_sound_render_channel(samples, 32, noenvelope, nosweep);
      }
      else
      {
        gbc_sound_render_channel(samples, 64, noenvelope, nosweep);
      }
    }

    gs = gbc_sound_channel + 3;
    if(gs->active_flag)
    {
      sound_status |= 0x08;
      envelope_volume = gs->envelope_volume;

      if(gs->noise_type == 1)
      {
        gbc_sound_render_channel(noise, half, envelope, nosweep);
      }
      else
      {
        gbc_sound_render_channel(noise, full, envelope, nosweep);
      }
    }
  }

  address16(io_registers, 0x84) = sound_status;

  gbc_sound_last_cpu_ticks = cpu_ticks;
  gbc_sound_buffer_index =
   (gbc_sound_buffer_index + (buffer_ticks * 2)) % BUFFER_SIZE;

  last_buffer_index = buffer_index;

  //SDL_UnlockMutex(sound_mutex);

  //SDL_CondSignal(sound_cv);
}


void sound_callback(u8 *streamL, u8 *streamR, int length)
{
  u32 sample_length = length / 2;
  u32 _length;
  u32 i;
  u32 j;
  s16 *stream_base_l = (s16 *)streamL;
  s16 *stream_base_r = (s16 *)streamR;
  s16 *source;
  s32 current_sample;
  u32 copied_bytes = 0;

  gbc_sound_buffer_index = 0;

  if(global_enable_audio)
  {
    for(i = 0; i < length; i++)
    {
      current_sample = sound_buffer_play_l[i];
      if(current_sample > 2047)
        current_sample = 2047;
      if(current_sample < -2048)
        current_sample = -2048;

      sound_buffer_l[i] = current_sample << 4;
      sound_buffer_play_l[i] = 0;
    }

    for(i = 0; i < length; i++)
    {
      current_sample = sound_buffer_play_r[i];
      if(current_sample > 2047)
        current_sample = 2047;
      if(current_sample < -2048)
        current_sample = -2048;

      sound_buffer_r[i] = current_sample << 4;
      sound_buffer_play_r[i] = 0;
    }
  }
  sound_buffer_base = 0;

  CSND_playsound(0x8, CSND_LOOP_DISABLE, CSND_ENCODING_PCM16, sound_frequency, (u32*)sound_buffer_l, (u32*)sound_buffer_r, length*2, 2, 1);

  //SDL_CondSignal(sound_cv);

  //SDL_UnlockMutex(sound_mutex);
}

// Special thanks to blarrg for the LSFR frequency used in Meridian, as posted
// on the forum at http://meridian.overclocked.org:
// http://meridian.overclocked.org/cgi-bin/wwwthreads/showpost.pl?Board=merid
// angeneraldiscussion&Number=2069&page=0&view=expanded&mode=threaded&sb=4
// Hope you don't mind me borrowing it ^_-

void init_noise_table(u32 *table, u32 period, u32 bit_length)
{
  u32 shift_register = 0xFF;
  u32 mask = ~(1 << bit_length);
  s32 table_pos, bit_pos;
  u32 current_entry;
  u32 table_period = (period + 31) / 32;

  // Bits are stored in reverse order so they can be more easily moved to
  // bit 31, for sign extended shift down.

  for(table_pos = 0; table_pos < table_period; table_pos++)
  {
    current_entry = 0;
    for(bit_pos = 31; bit_pos >= 0; bit_pos--)
    {
      current_entry |= (shift_register & 0x01) << bit_pos;

      shift_register =
       ((1 & (shift_register ^ (shift_register >> 1))) << bit_length) |
       ((shift_register >> 1) & mask);
    }

    table[table_pos] = current_entry;
  }
}

void reset_sound()
{
  direct_sound_struct *ds = direct_sound_channel;
  gbc_sound_struct *gs = gbc_sound_channel;
  u32 i;

  //SDL_LockMutex(sound_mutex);

  sound_on = 0;
  sound_buffer_base = 0;
  sound_last_cpu_ticks = 0;
  memset(sound_buffer_l, 0, sizeof(sound_buffer_l));
  memset(sound_buffer_r, 0, sizeof(sound_buffer_r));
  memset(sound_buffer_play_l, 0, sizeof(sound_buffer_play_l));
  memset(sound_buffer_play_r, 0, sizeof(sound_buffer_play_r));

  for(i = 0; i < 2; i++, ds++)
  {
    ds->buffer_index = 0;
    ds->status = DIRECT_SOUND_INACTIVE;
    ds->fifo_top = 0;
    ds->fifo_base = 0;
    ds->fifo_fractional = 0;
    ds->last_cpu_ticks = 0;
    memset(ds->fifo, 0, 32);
  }

  gbc_sound_buffer_index = 0;
  gbc_sound_last_cpu_ticks = 0;
  gbc_sound_partial_ticks = 0;

  gbc_sound_master_volume_left = 0;
  gbc_sound_master_volume_right = 0;
  gbc_sound_master_volume = 0;
  memset(wave_samples, 0, 64);

  for(i = 0; i < 4; i++, gs++)
  {
    gs->status = GBC_SOUND_INACTIVE;
    gs->sample_data = square_pattern_duty[2];
    gs->active_flag = 0;
  }

  //SDL_UnlockMutex(sound_mutex);
}

void sound_exit()
{
  linearFree(sound_buffer_l);
  linearFree(sound_buffer_r);
  linearFree(sound_buffer_play_l);
  linearFree(sound_buffer_play_r);
  gbc_sound_buffer_index =
   (sound_buffer_base + audio_buffer_size) % BUFFER_SIZE;
  //SDL_PauseAudio(1);
  sound_exit_flag = 1;
  //SDL_CondSignal(sound_cv);
  //SDL_CloseAudio();
  //SDL_Delay(200);
  //SDL_DestroyMutex(sound_mutex);
  //sound_mutex = NULL;
  //SDL_DestroyCond(sound_cv);
  //sound_cv = NULL;
}

void init_sound(int need_reset)
{
  //SDL_AudioSpec sound_settings;
  sound_buffer_l = linearAlloc(BUFFER_SIZE*2);
  sound_buffer_r = linearAlloc(BUFFER_SIZE*2);
  sound_buffer_play_l = linearAlloc(BUFFER_SIZE*2);
  sound_buffer_play_r = linearAlloc(BUFFER_SIZE*2);
  sound_exit_flag = 0;
#ifdef PSP_BUILD
  audio_buffer_size = (audio_buffer_size_number * 1024) + 3072;
#else
  audio_buffer_size = 16 << audio_buffer_size_number;
//  audio_buffer_size = 16384;
#endif

  /*SDL_AudioSpec desired_spec =
  {
    sound_frequency,
    AUDIO_S16,
    2,
    0,
    audio_buffer_size / 4,
    0,
    0,
    sound_callback,
    NULL
  };*/

  //sound_mutex = SDL_CreateMutex();
  //sound_cv = SDL_CreateCond();

  //SDL_OpenAudio(&desired_spec, &sound_settings);
  //sound_frequency = sound_settings.freq;
  //audio_buffer_size = sound_settings.size;
  u32 i = audio_buffer_size / 16;
  for (audio_buffer_size_number = 0; i && (i & 1) == 0; i >>= 1)
    audio_buffer_size_number++;
#ifndef PSP_BUILD
  printf("audio: freq %d, size %d\n", sound_frequency, audio_buffer_size);
#endif

  gbc_sound_tick_step =
   float_to_fp16_16(256.0f / sound_frequency);

  init_noise_table(noise_table15, 32767, 14);
  init_noise_table(noise_table7, 127, 6);

  if (need_reset)
    reset_sound();

  //SDL_PauseAudio(0);
}

#define sound_savestate_builder(type)                                       \
void sound_##type##_savestate(file_tag_type savestate_file)                 \
{                                                                           \
  file_##type##_variable(savestate_file, sound_on);                         \
  file_##type##_variable(savestate_file, sound_buffer_base);                \
  file_##type##_variable(savestate_file, sound_last_cpu_ticks);             \
  file_##type##_variable(savestate_file, gbc_sound_buffer_index);           \
  file_##type##_variable(savestate_file, gbc_sound_last_cpu_ticks);         \
  file_##type##_variable(savestate_file, gbc_sound_partial_ticks);          \
  file_##type##_variable(savestate_file, gbc_sound_master_volume_left);     \
  file_##type##_variable(savestate_file, gbc_sound_master_volume_right);    \
  file_##type##_variable(savestate_file, gbc_sound_master_volume);          \
  file_##type##_array(savestate_file, wave_samples);                        \
  file_##type##_array(savestate_file, direct_sound_channel);                \
  file_##type##_array(savestate_file, gbc_sound_channel);                   \
}                                                                           \

sound_savestate_builder(read);
sound_savestate_builder(write_mem);

