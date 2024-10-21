#include "Skybox.h"
#include "Common.h"
#include "IO/FileSystem.h"
#include "IOUtil.h"
#include "imgui.h"
#include <GL/glew.h>
#include <ProceduralGeometry.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>

using namespace fragcore;

namespace glsample {

	Skybox::Skybox() {}
	Skybox::~Skybox() {
		glDeleteBuffers(1, &this->SkyboxCube.vbo);
		glDeleteBuffers(1, &this->SkyboxCube.ibo);
	}

	void Skybox::Init(unsigned int texture, unsigned int program) {

		/*	*/
		GLint minMapBufferSize;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
		this->uniformSize = Math::align(sizeof(uniform_buffer_block), (size_t)minMapBufferSize);

		/*	Create uniform buffer.	*/
		glGenBuffers(1, &this->uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		/*	Load geometry.	*/
		Common::loadCube(this->SkyboxCube, 1, 1, 1);

		this->skybox_texture_panoramic = texture;
		this->skybox_program = program;
	}

	void Skybox::Render(const CameraController &camera) {
		this->Render((camera.getProjectionMatrix() * glm::inverse(camera.getRotationMatrix())));
	}

	void Skybox::Render(const glm::mat4 &viewProj) {
		if (!this->isEnabled) {
			return;
		}

		/*	*/
		this->uniform_stage_buffer.modelViewProjection = viewProj;

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

			/*	Extract current state. to restore afterward rendered the skybox.	*/
			GLint cullstate, blend, depth_test, depth_func;
			glGetIntegerv(GL_CULL_FACE, &cullstate);
			glGetIntegerv(GL_BLEND, &blend);
			glGetIntegerv(GL_DEPTH_TEST, &depth_test);
			glGetIntegerv(GL_DEPTH_FUNC, &depth_func);

			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);

			glDepthFunc(GL_LEQUAL);

			/*	*/
			glUseProgram(this->skybox_program);

			/*	*/
			if (glBindTextureUnit) {
				glBindTextureUnit(0, this->skybox_texture_panoramic);
			} else {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_texture_panoramic);
			}

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

			glDepthFunc(depth_func);
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

	int Skybox::loadDefaultProgram(fragcore::IFileSystem *filesystem) {

		/*	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";
		/*	Load shader binaries.	*/
		const std::vector<uint32_t> vertex_skybox_binary =
			IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, filesystem);
		const std::vector<uint32_t> fragment_skybox_binary =
			IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, filesystem);

		/*	*/
		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*	Create skybox graphic pipeline program.	*/
		int skybox_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

		/*	Setup graphic pipeline.	*/
		glUseProgram(skybox_program);
		int uniform_buffer_index = glGetUniformBlockIndex(skybox_program, "UniformBufferBlock");
		glUniformBlockBinding(skybox_program, uniform_buffer_index, 0);
		glUniform1i(glGetUniformLocation(skybox_program, "panorama"), 0);
		glUseProgram(0);

		return skybox_program;
	}

} // namespace glsample