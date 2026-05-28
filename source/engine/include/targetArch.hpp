// targetArch.hpp - ИСПРАВЛЕННАЯ ВЕРСИЯ
#ifndef TARGET_ARCH_H
#define TARGET_ARCH_H

// НЕ определяем _WIN32 и _WIN64 - они уже заданы компилятором!
// Visual Studio автоматически определяет:
// _WIN32 для 32-битных и 64-битных сборок
// _WIN64 только для 64-битных сборок

// Определяем только архитектурно-специфичные макросы
#ifdef _M_X64
#define _AMD64_
#pragma message("Target Architecture: x64")
#elif _M_IX86
#define _X86_
#pragma message("Target Architecture: x86")
#elif _M_ARM64
#define _ARM64_
#pragma message("Target Architecture: ARM64")
#elif _M_ARM
#define _ARM_
#pragma message("Target Architecture: ARM")
#endif

// Оптимизации для Windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif // TARGET_ARCH_H