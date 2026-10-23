/* LXRXXL shim: portable C versions of the Cortex-M intrinsics the LXR DSP uses. Same results, bit for bit:
 * signed/unsigned saturation, the saturating (halfword) adds, PKHBT, and CLZ returning 32 for 0. */
#ifndef CMSIS_INTRINSICS_H_
#define CMSIS_INTRINSICS_H_
#include <stdint.h>
static inline int32_t lxr_ssat(int32_t v, int bits) {
    const int32_t mx = (int32_t)((1u << (bits - 1)) - 1u), mn = -mx - 1;
    return v > mx ? mx : (v < mn ? mn : v);
}
static inline uint32_t lxr_usat(int32_t v, int bits) {
    const int32_t mx = bits >= 31 ? INT32_MAX : (int32_t)((1u << bits) - 1u);
    return (uint32_t)(v > mx ? mx : (v < 0 ? 0 : v));
}
#define __SSAT(ARG1, ARG2) lxr_ssat((int32_t)(ARG1), (ARG2))
#define __USAT(ARG1, ARG2) lxr_usat((int32_t)(ARG1), (ARG2))
static inline uint32_t __QADD16(uint32_t a, uint32_t b) {
    int32_t lo = lxr_ssat((int32_t)(int16_t)a + (int16_t)b, 16), hi = lxr_ssat((int32_t)(int16_t)(a >> 16) + (int16_t)(b >> 16), 16);
    return ((uint32_t)(uint16_t)hi << 16) | (uint16_t)lo;
}
static inline uint32_t __QSUB16(uint32_t a, uint32_t b) {
    int32_t lo = lxr_ssat((int32_t)(int16_t)a - (int16_t)b, 16), hi = lxr_ssat((int32_t)(int16_t)(a >> 16) - (int16_t)(b >> 16), 16);
    return ((uint32_t)(uint16_t)hi << 16) | (uint16_t)lo;
}
static inline int32_t __QADD(int32_t a, int32_t b) { int64_t r = (int64_t)a + b; return r > INT32_MAX ? INT32_MAX : r < INT32_MIN ? INT32_MIN : (int32_t)r; }
static inline int32_t __QSUB(int32_t a, int32_t b) { int64_t r = (int64_t)a - b; return r > INT32_MAX ? INT32_MAX : r < INT32_MIN ? INT32_MIN : (int32_t)r; }
/* PKHBT: bottom halfword of a, (b << shift)'s top halfword */
static inline uint32_t lxr_pkhbt(uint32_t a, uint32_t b, int sh) { return (a & 0xFFFFu) | ((b << sh) & 0xFFFF0000u); }
#define __PKHBT(ARG1, ARG2, ARG3) lxr_pkhbt((uint32_t)(int32_t)(ARG1), (uint32_t)(int32_t)(ARG2), (ARG3))
static inline uint8_t lxr_clz(uint32_t v) { return v ? (uint8_t)__builtin_clz(v) : 32u; }
#define __CLZ(value) lxr_clz((uint32_t)(value))
#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE __attribute__((always_inline)) static inline
#endif
#ifndef __IO
#define __IO volatile
#endif
#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif
#endif
