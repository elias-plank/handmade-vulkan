/* date = April 12th 2022 11:39 pm */

#ifndef HANDMADE_TYPES_H
#define HANDMADE_TYPES_H

#include <cstdint>
#include <vulkan/vulkan.h>

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

    typedef u32 b32;

	struct Vector2 {

		f32 X;
		f32 Y;
	};

	struct Vector3 {

		f32 X;
		f32 Y;
		f32 Z;
	};

	struct Vertex {

		Vector3 Position;
		Vector3 Color;
	};

	struct VertexDescription {

		VkVertexInputBindingDescription Binding;
		VkVertexInputAttributeDescription Attributes[2];
	};

	VertexDescription VertexGetDescription();
}

#endif //HANDMADE_TYPES_H
