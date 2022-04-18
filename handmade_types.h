/* date = April 12th 2022 11:39 pm */

#ifndef HANDMADE_TYPES_H
#define HANDMADE_TYPES_H

#include <cstdint>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

namespace handmade {

	typedef uint8_t u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;

	typedef int8_t i8;
	typedef int16_t i16;
	typedef int32_t i32;
	typedef int64_t i64;

	typedef float f32;
	typedef double f64;
}

#endif //HANDMADE_TYPES_H
