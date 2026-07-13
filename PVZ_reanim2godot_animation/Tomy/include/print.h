#pragma once

#include <stdarg.h>
//#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macro.h"
#include "ustring.h"

//#include "data_type.h"

#define END "\n"
#define SEPARATOR " "
#define FILE_STREAM stdout
#define FLUSH_STREAM false

#define LEFT_BRACE '{'
#define RIGHT_BRACE '}'

#define TEMP_BUFFER_SIZE 32

#define FPRINTSN(fp, s, n) fprintf(fp, "%s", s)
#define FFLUSH(fp) fflush(fp)

enum
{
    COLOR_DARK_BLACK = 0,
    COLOR_DARK_RED = 1,
    COLOR_DARK_GREEN = 2,
    COLOR_DARK_YELLOW = 3,
    COLOR_DARK_BLUE = 4,
    COLOR_DARK_MAGENTA = 5,
    COLOR_DARK_CYAN = 6,
    COLOR_DARK_WHITE = 7,
    COLOR_BRIGHT_BLACK = 8,
    COLOR_BRIGHT_RED = 9,
    COLOR_BRIGHT_GREEN = 10,
    COLOR_BRIGHT_YELLOW = 11,
    COLOR_BRIGHT_BLUE = 12,
    COLOR_BRIGHT_MAGENTA = 13,
    COLOR_BRIGHT_CYAN = 14,
    COLOR_BRIGHT_WHITE = 15
};

//#define COLOR_BLACK           COLOR_DARK_BLACK
//#define COLOR_RED             COLOR_DARK_RED
//#define COLOR_GREEN           COLOR_DARK_GREEN
//#define COLOR_YELLOW          COLOR_DARK_YELLOW
//#define COLOR_BLUE            COLOR_DARK_BLUE
//#define COLOR_MAGENTA         COLOR_DARK_MAGENTA
//#define COLOR_CYAN            COLOR_DARK_CYAN
//#define COLOR_WHITE           COLOR_DARK_WHITE

enum
{
    COLOR_BLACK = COLOR_DARK_BLACK,
    COLOR_RED = COLOR_DARK_RED,
    COLOR_GREEN = COLOR_DARK_GREEN,
    COLOR_YELLOW = COLOR_DARK_YELLOW,
    COLOR_BLUE = COLOR_DARK_BLUE,
    COLOR_MAGENTA = COLOR_DARK_MAGENTA,
    COLOR_CYAN = COLOR_DARK_CYAN,
    COLOR_WHITE = COLOR_DARK_WHITE
};

#define RESET_STYLE "\033[0m"

#define ERR_OK              0
#define ERR_FAIL            1

//#define TYPE_NONE           0
//#define TYPE_BOOL           1
//#define TYPE_CHAR           2
//#define TYPE_SCHAR          3
//#define TYPE_UCHAR          4
//#define TYPE_SHORT          5
//#define TYPE_INT            6
//#define TYPE_LONG           7
//#define TYPE_LONGLONG       8
//#define TYPE_USHORT         9
//#define TYPE_UINT           10
//#define TYPE_ULONG          11
//#define TYPE_ULONGLONG      12
//#define TYPE_FLOAT          13
//#define TYPE_DOUBLE         14
//#define TYPE_STRING         15
//#define TYPE_CONST_STRING   16
//#define TYPE_ANY            17
//#define TYPE_CONST_ANY      18
//#define TYPE_STRING_PTR               19
//#define TYPE_CONST_STRING_PTR         20
//#define TYPE_COLOR24                  21
//#define TYPE_COLOR24_PTR              22
//#define TYPE_CONST_COLOR24_PTR        23

#define format_of(x) _Generic((x), \
  _Bool:              "%d", \
  signed char:        "%hhd", \
  unsigned char:      "%hhu", \
  short:              "%hd", \
  int:                "%d", \
  long:               "%ld", \
  long long:          "%lld", \
  unsigned short:     "%hu", \
  unsigned int:       "%u", \
  unsigned long:      "%lu", \
  unsigned long long: "%llu", \
  float:              "%f", \
  double:             "%f", \
  char*:              "%s", \
  const char*:        "%s", \
  void*:              "%p", \
  const void*:        "%p", \
  char:               "%c", \
  default:            "")

#define typeid_value_pair(x) typeid_of(x), (x)

#define dot(x) .x

#define const_of_ptr(x) ((const typeof(*(x))*)(x))

#define as_print_config_ptr(x) _Generic((x), \
  PrintConfig*: (x), \
  default: NULL)

#define read_from_value(str, value, err) do { \
  char buf[TEMP_BUFFER_SIZE] = { 0 }; \
  const char* fmt = format_of(value); \
  int len = snprintf(buf, sizeof(buf), fmt, value); \
  if (len < 0) { (err) = ERR_FAIL; break; } \
  bool ok = string_append_sn(str, buf, len); \
  if (!ok) { (err) = ERR_FAIL; break; } \
  (err) = ERR_OK; \
} while (0)

#define rgb(r, g, b) ((Color24){ r, g, b })

#define rgba(r, g, b, a) rgb(r, g, b)

#define set(...) (&(PrintConfig){ EXPAND(dot, __VA_ARGS__) })

#define format(fmt, ...) \
  format_func(fmt, ARGS_COUNT(__VA_ARGS__) __VA_OPT__(,) EXPAND(typeid_value_pair, __VA_ARGS__))

#define SELECT_2(_1, _2, ...) _2

#define print_emin(...) SELECT_2(__VA_OPT__(,) \
  print_func( \
    as_print_config_ptr(LAST(__VA_ARGS__)), \
    COUNT_ARGS(__VA_ARGS__), \
    EXPAND(typeid_value_pair, __VA_ARGS__) \
  ), \
  print_func(NULL, ARGS_COUNT(__VA_ARGS__)))

#define println_emin(...) SELECT_2(__VA_OPT__(,) \
  println_func( \
    as_print_config_ptr(LAST(__VA_ARGS__)), \
    COUNT_ARGS(__VA_ARGS__), \
    EXPAND(typeid_value_pair, __VA_ARGS__) \
  ), \
  println_func(NULL, ARGS_COUNT(__VA_ARGS__)))

//typedef int8_t i8;
//typedef int16_t i16;
//typedef int32_t i32;
//typedef int64_t i64;
//
//typedef uint8_t u8;
//typedef uint16_t u16;
//typedef uint32_t u32;
//typedef uint64_t u64;

typedef struct color24_t
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Color24;

typedef struct print_config_t
{
    const char* sep;
    const char* end;
    FILE* file;
    bool flush;
} PrintConfig;

String* format_func(const char* fmt, int count, ...);
void print_func(const PrintConfig* config, int count, ...);
void println_func(const PrintConfig* config, int count, ...);
String* set_cursor_pos(int x, int y);
String* set_fg_idx(int idx);
String* set_fg_rgb(int r, int g, int b);
String* set_fg_color(Color24 color);
String* set_bg_idx(int idx);
String* set_bg_rgb(int r, int g, int b);
String* set_bg_color(Color24 color);
String* set_colors_idx(int fg_idx, int bg_idx);
String* set_colors_rgb(int fg_r, int fg_g, int fg_b, int bg_r, int bg_g, int bg_b);
String* set_colors_color(Color24 fg_color, Color24 bg_color);
const char* reset_style(void);

int _fast_print(const char* format, ...);

inline size_t read_from_color24(String* str, Color24 color)
{
    String* temp = format("{}[R:{} G:{} B:{}]{}", set_fg_color(color), color.r, color.g, color.b, reset_style());
    const size_t len = temp->size;
    string_append_sn(str, temp->data, len);
    string_delete(temp);
    return len;
}

inline size_t read_from_va_list(String* str, const int type, va_list* args_ptr)
{
    switch (type)
    {
        case TYPE_NONE:
        {
            return 0;
        }
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_SCHAR:
        case TYPE_UCHAR:
        case TYPE_SHORT:
        case TYPE_USHORT:
        case TYPE_INT:
        {
            int arg = va_arg(*args_ptr, int);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_UINT:
        {
            unsigned int arg = va_arg(*args_ptr, unsigned int);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_LONG:
        {
            long arg = va_arg(*args_ptr, long);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_ULONG:
        {
            unsigned long arg = va_arg(*args_ptr, unsigned long);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_LONGLONG:
        {
            long long arg = va_arg(*args_ptr, long long);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_ULONGLONG:
        {
            unsigned long long arg = va_arg(*args_ptr, unsigned long long);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        {
            double arg = va_arg(*args_ptr, double);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_STRING:
        case TYPE_CONST_STRING:
        {
            char* arg = va_arg(*args_ptr, char*);
            size_t len = strlen(arg);
            if (len == 0)
            {
                return 1;
            }
            [[maybe_unused]] bool ok = string_append_sn(str, arg, len);
            return len;
        }
        case TYPE_ANY:
        case TYPE_CONST_ANY:
        {
            void* arg = va_arg(*args_ptr, void*);
            int err = 0;
            read_from_value(str, arg, err);
            return sizeof(arg);
        }
        case TYPE_STRING_PTR:
        case TYPE_CONST_STRING_PTR:
        {
            String* arg = va_arg(*args_ptr, String*);
            size_t len = arg->size;
            if (len == 0)
            {
                return 1;
            }
            [[maybe_unused]] bool ok = string_append_sn(str, arg->data, len);
            if (type == TYPE_STRING_PTR)
            {
                string_delete(arg);
            }
            return len;
        }
        case TYPE_COLOR24:
        {
            Color24 arg = va_arg(*args_ptr, Color24);
            return read_from_color24(str, arg);
        }
        case TYPE_COLOR24_PTR:
        case TYPE_CONST_COLOR24_PTR:
        {
            const Color24* arg = va_arg(*args_ptr, Color24*);
            return read_from_color24(str, *arg);
        }
        default: break;
    }
    return 0;
}

inline int format_from_va_list(String* str, const char* fmt, const int count, va_list* args_ptr)
{
    int arg_index = 0;
    const size_t fmt_len = strlen(fmt);
    for (size_t i = 0; i < fmt_len; i++)
    {
        bool match = false;
        if (fmt[i] == LEFT_BRACE)
        {
            size_t next_i = i + 1;
            if (next_i < fmt_len && fmt[next_i] == LEFT_BRACE)
            {
                i = next_i;
            }
            else
            {
                for (size_t j = i + 1; j < fmt_len; j++)
                {
                    if (fmt[j] == RIGHT_BRACE)
                    {
                        i = j;
                        match = true;
                        break;
                    }
                }
            }
        }
        if (match)
        {
            if (arg_index < count)
            {
                const int type = va_arg(*args_ptr, int);
                size_t ret = read_from_va_list(str, type, args_ptr);
                arg_index++;
            }
        }
        else
        {
            string_append_sn(str, fmt + i, 1);
        }
    }
    return arg_index;
}

inline void parse_va_list(String* str, const char* sep, const int count, va_list args)
{
    const size_t sep_len = strlen(sep);
    for (int i = 0; i < count; i++)
    {
        const int type = va_arg(args, int);
        if (i == 0 && (type == TYPE_STRING || type == TYPE_CONST_STRING))
        {
            const char* fmt = va_arg(args, char*);
            const int ret = format_from_va_list(str, fmt, count - 1, &args);
            i += ret;
        }
        else
        {
            const size_t ret = read_from_va_list(str, type, &args);
            if (ret == 0)
            {
                break;
            }
        }
        if (i < count - 1)
        {
            [[maybe_unused]] bool ok = string_append_sn(str, sep, sep_len);
        }
    }
}

inline String* format_func(const char* fmt, const int count, ...)
{
    String* str = string_new(STRING_CAPACITY);
    va_list args;
    va_start(args, count);
    format_from_va_list(str, fmt, count, &args);
    va_end(args);
    return str;
}

inline void print_func(const PrintConfig* config, int count, ...)
{
    const char* sep = SEPARATOR;
    const char* end = "";
    FILE* file = FILE_STREAM;
    bool flush = FLUSH_STREAM;
    if (config)
    {
        count -= 1;
        if (config->sep)
        {
            sep = config->sep;
        }
        if (config->end)
        {
            end = config->end;
        }
        if (config->file)
        {
            file = config->file;
        }
        if (config->flush)
        {
            flush = config->flush;
        }
    }
    va_list args;
    va_start(args, count);
    String a = { 0 };
    string_init_with_size(&a, STRING_CAPACITY);
    parse_va_list(&a, sep, count, args);
    string_append_s(&a, end);
    if (a.data)
    {
        FPRINTSN(file, a.data, a.size);
        if (flush)
        {
            FFLUSH(file);
        }
    }
    string_deinit(&a);
    va_end(args);
}

inline void println_func(const PrintConfig* config, int count, ...)
{
    const char* sep = SEPARATOR;
    const char* end = END;
    FILE* file = FILE_STREAM;
    bool flush = FLUSH_STREAM;
    if (config)
    {
        count -= 1;
        if (config->sep)
        {
            sep = config->sep;
        }
        if (config->end)
        {
            end = config->end;
        }
        if (config->file)
        {
            file = config->file;
        }
        if (config->flush)
        {
            flush = config->flush;
        }
    }
    va_list args;
    va_start(args, count);
    String a = { 0 };
    string_init_with_size(&a, STRING_CAPACITY);
    parse_va_list(&a, sep, count, args);
    string_append_s(&a, end);
    if (a.data)
    {
        FPRINTSN(file, a.data, a.size);
        if (flush)
        {
            FFLUSH(file);
        }
    }
    string_deinit(&a);
    va_end(args);
}

inline String* set_cursor_pos(const int x, const int y)
{
    return format("\033[{};{}H", y + 1, x + 1);
}

inline String* set_fg_idx(int idx)
{
    return format("\033[38;5;{}m", idx);
}

inline String* set_fg_rgb(int r, int g, int b)
{
    return format("\033[38;2;{};{};{}m", r, g, b);
}

inline String* set_fg_color(const Color24 color)
{
    return set_fg_rgb(color.r, color.g, color.b);
}

inline String* set_bg_idx(int idx)
{
    return format("\033[48;5;{}m", idx);
}

inline String* set_bg_rgb(int r, int g, int b)
{
    return format("\033[48;2;{};{};{}m", r, g, b);
}

inline String* set_bg_color(const Color24 color)
{
    return set_bg_rgb(color.r, color.g, color.b);
}

inline String* set_colors_idx(int fg_idx, int bg_idx)
{
    return format("\033[38;5;{};48;5;{}m", fg_idx, bg_idx);
}

inline String* set_colors_rgb(int fg_r, int fg_g, int fg_b, int bg_r, int bg_g, int bg_b)
{
    return format("\033[38;2;{};{};{};48;2;{};{};{}m", fg_r, fg_g, fg_b, bg_r, bg_g, bg_b);
}

inline String* set_colors_color(const Color24 fg_color, const Color24 bg_color)
{
    return set_colors_rgb(fg_color.r, fg_color.g, fg_color.b, bg_color.r, bg_color.g, bg_color.b);
}

inline const char* reset_style(void)
{
    return RESET_STYLE;
}

inline int _fast_print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int ret = vprintf(format, args);
    va_end(args);
    return ret;
}
