#include "Math3D/Transform.h"
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>

namespace glsample {
	using TransformGLM = fragcore::Transform<glm::vec3, glm::mat3x3, glm::mat4x4, glm::quat>;
} // namespace glsample
