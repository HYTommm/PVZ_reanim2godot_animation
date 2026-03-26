#pragma once

#define CLASS typedef struct
#define VTABLE typedef struct
#define FROM(base) base

#define _VCALL_1(T, self, func, ...) ((_##T##_VTable*)((T*)(self))->vptr)->func(self __VA_OPT__(,) __VA_ARGS__)
#define _VCALL_2(T, self, func, ...) _VCALL_1(T, self, func, __VA_ARGS__)
#define _VCALL_3(T, self, func, ...) _VCALL_2(T, self, func, __VA_ARGS__)
#define VCall(T, self, func, ...) _VCALL_3(T, self, func, __VA_ARGS__)

#define _Call_1(T, self, func, ...) _##T##_##func(self __VA_OPT__(,) __VA_ARGS__)
#define _Call_2(T, self, func, ...) _Call_1(T, self, func, __VA_ARGS__)
#define _Call_3(T, self, func, ...) _Call_2(T, self, func, __VA_ARGS__)
#define Call(T, self, func, ...) _Call_3(T, self, func, __VA_ARGS__)

#define _FT_1(T, self) ((_##T##_VTable*)((T*)(self))->vptr)
#define _FT_2(T, self) _FT_1(T, self)
#define _FT_3(T, self) _FT_2(T, self)
#define FT(T, self) _FT_3(T, self)

#define _CREATE_1(T, self, ...) _##T##_Create(self __VA_OPT__(,) __VA_ARGS__)
#define _CREATE_2(T, self, ...) _CREATE_1(T, self,  __VA_ARGS__)
#define _CREATE_3(T, self,  ...) _CREATE_2(T, self, __VA_ARGS__)
#define Create(T, self, ...) _CREATE_3(T, self, __VA_ARGS__)

#define _NEW_1(T, self, ...) _##T##_New(self __VA_OPT__(,) __VA_ARGS__)
#define _NEW_2(T, self, ...) _NEW_1(T, self, __VA_ARGS__)
#define _NEW_3(T, self, ...) _NEW_2(T, self, __VA_ARGS__)
#define New(T, self, ...) _NEW_3(T, self, __VA_ARGS__)
