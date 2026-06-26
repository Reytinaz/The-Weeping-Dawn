#ifndef TARGET_ARCH_H
#define TARGET_ARCH_H

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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif // TARGET_ARCH_H