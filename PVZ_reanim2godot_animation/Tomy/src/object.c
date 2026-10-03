#include "class/object_class.h"
#include "ustring.h"
#include "print.h"

int _fast_print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int ret = vprintf(format, args);
    va_end(args);
    return ret;
}

String* _Object_ToString(Object* self)
{
    String* str = New(String, STRING_CAPACITY);
    Call(String, str, Append, "Object");
    #ifdef _ADDRESS_PRINT_
    char buf[TEMP_BUFFER_SIZE] = { 0 };
    const int len = snprintf(buf, sizeof(buf), " at %p", self);
    Call(String, str, AppendN, buf, len);
    #endif
    return str;
}