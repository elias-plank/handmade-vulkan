#include "handmade_types.h"

namespace handmade {

	VertexDescription VertexGetDescription() {

		VertexDescription description{};
		description.Binding.binding = 0;
		description.Binding.stride = sizeof(Vertex);
		description.Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		description.Attributes[0].binding = 0;
		description.Attributes[0].location = 0;
		description.Attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		description.Attributes[0].offset = offsetof(Vertex, Position);

		description.Attributes[1].binding = 0;
		description.Attributes[1].location = 1;
		description.Attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		description.Attributes[1].offset = offsetof(Vertex, Color);

		return description;
	}
}
