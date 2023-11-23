#include "Skybox.h"
#include "imgui.h"
#include <GL/glew.h>
#include <ProceduralGeometry.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace fragcore;

namespace glsample {

	Skybox::Skybox() {}
	Skybox::~Skybox() {}

	void Skybox::Init(unsigned int texture, unsigned int program) {

		/*	*/
		GLint minMapBufferSize;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
		this->uniformSize = Math::align(sizeof(UniformBufferBlock), (size_t)minMapBufferSize);

		/*	Create uniform buffer.	*/
		glGenBuffers(1, &this->uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		/*	Load geometry.	*/
		std::vector<ProceduralGeometry::Vertex> vertices;
		std::vector<unsigned int> indices;
		ProceduralGeometry::generateCube(1.0f, vertices, indices);

		/*	Create array buffer, for rendering static geometry.	*/
		glGenVertexArrays(1, &this->SkyboxCube.vao);
		glBindVertexArray(this->SkyboxCube.vao);

		/*	*/
		glGenBuffers(1, &this->SkyboxCube.ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SkyboxCube.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
		this->SkyboxCube.nrIndicesElements = indices.size();

		/*	Create array buffer, for rendering static geometry.	*/
		glGenBuffers(1, &this->SkyboxCube.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, SkyboxCube.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
					 GL_STATIC_DRAW);

		/*	*/
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

		glBindVertexArray(0);

		this->skybox_texture_panoramic = texture;
		this->skybox_program = program;
	}

	void Skybox::Render(const CameraController &camera) {

		if (!this->isEnabled) {
			return;
		}

		/*	*/
		this->uniform_stage_buffer.modelViewProjection = (camera.getProjectionMatrix() * camera.getRotationMatrix());

		/*	*/
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		void *uniformPointer =
			glMapBufferRange(GL_UNIFORM_BUFFER, ((frameIndex + 1) % this->nrUniformBuffer) * this->uniformSize,
							 this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
		memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		{

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (frameIndex % this->nrUniformBuffer) * this->uniformSize, this->uniformSize);

			GLint cullstate, blend, depth_test;
			glGetIntegerv(GL_CULL_FACE, &cullstate);
			glGetIntegerv(GL_BLEND, &blend);
			glGetIntegerv(GL_DEPTH_TEST, &depth_test);

			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);

			glDepthFunc(GL_LEQUAL);

			/*	*/
			glUseProgram(this->skybox_program);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->skybox_texture_panoramic);

			/*	Draw triangle.	*/
			glBindVertexArray(this->SkyboxCube.vao);
			glDrawElements(GL_TRIANGLES, this->SkyboxCube.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			if (cullstate) {
				glEnable(GL_CULL_FACE);
			}

			if (blend) {
				glEnable(GL_BLEND);
			}

			if (depth_test) {
				glEnable(GL_DEPTH_TEST);
			}
		}

		this->frameIndex = (this->frameIndex + 1) % this->nrUniformBuffer;
	}

	void Skybox::RenderImGUI() {
		if (ImGui::CollapsingHeader("Skybox  Settings", &this->skybox_settings_visable,
									ImGuiTreeNodeFlags_CollapsingHeader)) {
			ImGui::Checkbox("Enabled", &this->isEnabled);
			ImGui::DragFloat3("Rotation", &this->rotation[0]);
			ImGui::ColorEdit4("Tint", &this->uniform_stage_buffer.tintColor[0],
							  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

			ImGui::DragFloat("Exposure", &this->uniform_stage_buffer.exposure);
			ImGui::DragFloat("Gamma", &this->uniform_stage_buffer.gamma);
		}
	}

} // namespace glsample