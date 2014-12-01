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

#ifndef COMMON_H
#define COMMON_H

#define ror(dest, value, shift)                                               \
  dest = ((value) >> shift) | ((value) << (32 - shift))                       \

#if defined(_WIN32) || defined(_WIN32_WCE)
  #define PATH_SEPARATOR "\\"
  #define PATH_SEPARATOR_CHAR '\\'
#else
  #define PATH_SEPARATOR "/"
  #define PATH_SEPARATOR_CHAR '/'
#endif

// These includes must be used before SDL is included.
#ifdef ARM_ARCH

#ifdef _WIN32_WCE
  #include <windows.h>
#else
  #define _BSD_SOURCE // sync
  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include <math.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <stdarg.h>
  #include <time.h>
  #include <sys/time.h>
#endif /* _WIN32_WCE */

#ifdef GIZ_BUILD
  #include "giz/giz.h"
#endif
#endif /* ARM_ARCH */

// Huge thanks to pollux for the heads up on using native file I/O
// functions on PSP for vastly improved memstick performance.

#define file_write_mem(filename_tag, buffer, size)                            \
{                                                                             \
  memcpy(write_mem_ptr, buffer, size);                                        \
  write_mem_ptr += size;                                                      \
}                                                                             \

#define file_write_mem_array(filename_tag, array)                             \
  file_write_mem(filename_tag, array, sizeof(array))                          \

#define file_write_mem_variable(filename_tag, variable)                       \
  file_write_mem(filename_tag, &variable, sizeof(variable))                   \

#if defined(PSP_BUILD)
  #define fastcall

  #include <pspkernel.h>
  #include <pspdebug.h>
  #include <pspctrl.h>
  #include <pspgu.h>
  #include <pspaudio.h>
  #include <pspaudiolib.h>
  #include <psprtc.h>

  #define function_cc

  #define convert_palette(value)                                              \
    value = ((value & 0x7FE0) << 1) | (value & 0x1F)                          \

  #define psp_file_open_read  PSP_O_RDONLY
  #define psp_file_open_write (PSP_O_CREAT | PSP_O_WRONLY | PSP_O_TRUNC)

  #define file_open(filename_tag, filename, mode)                             \
    s32 filename_tag = sceIoOpen(filename, psp_file_open_##mode, 0777)        \

  #define file_check_valid(filename_tag)                                      \
    (filename_tag >= 0)                                                       \

  #define file_close(filename_tag)                                            \
    sceIoClose(filename_tag)                                                  \

  #define file_read(filename_tag, buffer, size)                               \
    sceIoRead(filename_tag, buffer, size)                                     \

  #define file_write(filename_tag, buffer, size)                              \
    sceIoWrite(filename_tag, buffer, size)                                    \

  #define file_seek(filename_tag, offset, type)                               \
    sceIoLseek(filename_tag, offset, PSP_##type)                              \

  #define file_tag_type s32

  #include <time.h>
  #include <stdio.h>
#else

#if defined(_3DS)
  #include <3ds.h>
  #define file_tag_type s32
#else
  #include "SDL.h"
#endif

#ifdef ARM_ARCH
  #define function_cc
#else
  #define function_cc __attribute__((regparm(2)))
#endif

#ifndef _3DS
  typedef unsigned char u8;
  typedef signed char s8;
  typedef unsigned short int u16;
  typedef signed short int s16;
  typedef unsigned int u32;
  typedef signed int s32;
  typedef unsigned long long int u64;
  typedef signed long long int s64;
#endif

  #define convert_palette(value)                                              \
    value = ((value & 0x1F) << 11) | ((value & 0x03E0) << 1) | (value >> 10)  \

  #define stdio_file_open_read  "rb"
  #define stdio_file_open_write "wb"

  #define file_open(filename_tag, filename, mode)                             \
    FILE *filename_tag = fopen(filename, stdio_file_open_##mode)              \

  #define file_check_valid(filename_tag)                                      \
    (filename_tag)                                                            \

#ifdef GP2X_BUILD

  #define file_close(filename_tag)                                            \
  {                                                                           \
    fclose(filename_tag);                                                     \
    sync();                                                                   \
  }                                                                           \

#else

  #define file_close(filename_tag)                                            \
    fclose(filename_tag)                                                      \

#endif

  #define file_read(filename_tag, buffer, size)                               \
    fread(buffer, 1, size, filename_tag)                                      \

  #define file_write(filename_tag, buffer, size)                              \
    fwrite(buffer, 1, size, filename_tag)                                     \

  #define file_seek(filename_tag, offset, type)                               \
    fseek(filename_tag, offset, type)                                         \

  #define file_tag_type FILE *

#endif

// These must be variables, not constants.

#define file_read_variable(filename_tag, variable)                            \
  file_read(filename_tag, &variable, sizeof(variable))                        \

#define file_write_variable(filename_tag, variable)                           \
  file_write(filename_tag, &variable, sizeof(variable))                       \

// These must be statically declared arrays (ie, global or on the stack,
// not dynamically allocated on the heap)

#define file_read_array(filename_tag, array)                                  \
  file_read(filename_tag, array, sizeof(array))                               \

#define file_write_array(filename_tag, array)                                 \
  file_write(filename_tag, array, sizeof(array))                              \



typedef u32 fixed16_16;
typedef u32 fixed8_24;

#define float_to_fp16_16(value)                                               \
  (fixed16_16)((value) * 65536.0)                                             \

#define fp16_16_to_float(value)                                               \
  (float)((value) / 65536.0)                                                  \

#define u32_to_fp16_16(value)                                                 \
  ((value) << 16)                                                             \

#define fp16_16_to_u32(value)                                                 \
  ((value) >> 16)                                                             \

#define fp16_16_fractional_part(value)                                        \
  ((value) & 0xFFFF)                                                          \

#define float_to_fp8_24(value)                                                \
  (fixed8_24)((value) * 16777216.0)                                           \

#define fp8_24_fractional_part(value)                                         \
  ((value) & 0xFFFFFF)                                                        \

#define fixed_div(numerator, denominator, bits)                               \
  (((numerator * (1 << bits)) + (denominator / 2)) / denominator)             \

#define address8(base, offset)                                                \
  *((u8 *)((u8 *)base + (offset)))                                            \

#define address16(base, offset)                                               \
  *((u16 *)((u8 *)base + (offset)))                                           \

#define address32(base, offset)                                               \
  *((u32 *)((u8 *)base + (offset)))                                           \

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#ifndef _3DS
#include "SDL.h"
#endif

#include "cpu.h"
#include "memory.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "main.h"
#include "gui.h"

#ifndef _3DS
#include "zip.h"
#endif

#include "cheats.h"

#if defined(ARM_ARCH) && !defined(_3DS)
  #include "arm/warm.h"
#endif

#ifdef PSP_BUILD
  #define printf pspDebugScreenPrintf
#endif

#ifdef PC_BUILD
  #define STDIO_DEBUG
  //#define REGISTER_USAGE_ANALYZE
#endif

#ifdef GP2X_BUILD
  #include <strings.h>
  #include "gp2x/gp2x.h"

  #define printf(format, ...)                                                 \
    fprintf(stderr, format, ##__VA_ARGS__)                                    \

  #define vprintf(format, ap)                                                 \
    vfprintf(stderr, format, ap)                                              \

//  #define STDIO_DEBUG
#endif

#ifdef PND_BUILD
  #include "pandora/pnd.h"
#endif

#ifdef RPI_BUILD
  #include "raspberrypi/rpi.h"
#endif

#ifdef _3DS
  #include "3ds/_3ds.h"
#endif

#endif
