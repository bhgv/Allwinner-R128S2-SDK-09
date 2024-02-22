#ifndef __LINUX_COMPILER_ATTRIBUTES_H
#define __LINUX_COMPILER_ATTRIBUTES_H

#define barrier() __asm__ __volatile__("" : : : "memory")

#define __inline	inline
#define __inline__	inline

#ifdef __always_inline
#define __always_inline	inline __attribute__((always_inline))
#endif

#ifndef __noinline
#define __noinline	__attribute__((__noinline__))
#endif

#ifndef __packed
#define __packed	__attribute__((__packed__))
#endif

#ifndef __asm
#define __asm		asm
#endif

#ifndef __weak
#define __weak		__attribute__((weak))
#endif

#ifndef __maybe_unused
#define __maybe_unused		__attribute__((unused))
#endif

#ifndef __always_unused
#define __always_unused                 __attribute__((__unused__))
#endif

#ifndef likely
#define likely(x)   __builtin_expect((long)!!(x), 1L)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((long)!!(x), 0L)
#endif

#endif /* __LINUX_COMPILER_ATTRIBUTES_H */
