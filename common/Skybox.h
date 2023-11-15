#pragma once

#include "GLSampleSession.h"
#include "Util/CameraController.h"
#include <Core/IO/FileSystem.h>
#include <Core/IO/IOUtil.h>
#include <Exception.hpp>
#include <fmt/format.h>
#include <fstream>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vector>

using namespace fragcore;

namespace glsample {

	class Skybox {
	  public:
		struct UniformBufferBlock {
			glm::mat4 proj;
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			float exposure = 1.0f;
			float gamma = 2.2;
		} uniform_stage_buffer;

		Skybox();
		virtual ~Skybox();

		virtual void Init(unsigned int texture, unsigned int program);

		virtual void Render(const CameraController &camera);
		virtual void RenderImGUI();

	  private:
		GeometryObject SkyboxCube;
		unsigned int skybox_program;

		int frameIndex = 0;

		int skybox_texture_panoramic;
		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);
		glm::vec3 rotation;
		bool isEnabled = true;

		bool skybox_settings_visable = true;
	};

}; // namespace glsample