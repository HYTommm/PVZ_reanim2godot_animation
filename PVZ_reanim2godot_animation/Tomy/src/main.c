#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "tomy.h"

//#define PRINT_ANY_TEST
//#define PRINT_FMT_TEST
//#define PRINT_COLOR_TEST
//#define PRINT_256_COLOR_TABLE
#define TEST_1
//#define TEST_2
//#define TEST_3

void print_256_color_table(void)
{
    for (int i = 0; i < 256; i++)
    {
        char buf[4] = { 0 };
        snprintf(buf, sizeof(buf), "%3d", i);
        print_emin(set_bg_idx(i), buf, reset_style(), set(sep = "", end = ""));
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
        else
        {
            printf(" ");
        }
    }
}

void print_any_test(void)
{
    print_emin();
    print_emin("Hello", "Emin", 42, 3.141592);
}

void print_fmt_test(void)
{
    print_emin("{} is {}", "Emin", 42);
}

void print_color_test(void)
{
    print_emin(set_colors_idx(COLOR_RED, COLOR_WHITE), "REDRUM", reset_style(), set(sep = ""));
    print_emin(set_fg_rgb(102, 255, 178), "Here is Johnny~", reset_style(), set(sep = ""));
}

void test_1(void)
{
    print_emin(1, 2, set(sep = "", end = ""));
    print_emin(3, 4, set(sep = "", end = ""));
    print_emin(set_cursor_pos(2, 1), set(end = ""));
    print_emin(set_fg_idx(COLOR_BRIGHT_GREEN), format("{}/{}", 70, 100), reset_style(), set(sep = ""));
}

void test_2(void)
{
    String* content = format("{{+} print in {}", "C23");
    print_emin((const String*)content);
    FILE* file = fopen("test.txt", "wb");
    print_emin(content, set(file = file));
    fclose(file);
}

void test_3(void)
{
    Color24 color = rgba(255, 0, 0, 1);
    print_emin(color);
    color = rgba(0, 255, 0, 1);
    print_emin(&color);
    color = rgba(144, 12, 12, 1);
    print_emin(set_fg_color(color), "MURDER", reset_style(), set(sep = ""));
}

void test(void)
{
    #ifdef PRINT_ANY_TEST
    print_any_test();
    #endif
    #ifdef PRINT_FMT_TEST
    print_fmt_test();
    #endif
    #ifdef PRINT_COLOR_TEST
    print_color_test();
    #endif
    #ifdef PRINT_256_COLOR_TABLE
    print_256_color_table();
    #endif
    #ifdef TEST_1
    test_1();
    #endif
    #ifdef TEST_2
    test_2();
    #endif
    #ifdef TEST_3
    test_3();
    #endif
}

int main(int argc, char** argv)
{
    Vec(i32) vec_i32;
    Create(Vec(i32), &vec_i32);
    Call(Vec(i32), &vec_i32, push_back, 1);
    Call(Vec(i32), &vec_i32, push_back, 2);
    FT(Vec(i32), &vec_i32)->push_back(&vec_i32, 3);
    FT(Vec(i32), &vec_i32)->push_back(&vec_i32, 4);

    FT(Vec(i32), &vec_i32)->reserve(&vec_i32, 10000);

    for (i32 i = 5; i < 9000; i++)
    {
        FT(Vec(i32), &vec_i32)->push_back(&vec_i32, i);
    }

    i32 num = FT(Vec(i32), &vec_i32)->at(&vec_i32, 35);

    FT(Vec(i32), &vec_i32)->clear(&vec_i32);

    println_emin(num);
    println_emin(vec_i32.vptr->to_string(&vec_i32));

    vec_i32.vptr->destroy(&vec_i32);

    Vec(String) vec_str;
    Create(Vec(String), &vec_str);

    String* str = format("hello");
    FT(Vec(String), &vec_str)->push_back(&vec_str, *str);
    string_delete(str);

    const String a = FT(Vec(String), &vec_str)->at(&vec_str, 0);
    println_emin(&a);

    vec_str.vptr->destroy(&vec_str);

    return 0;
}