#include "Skybox.h"
#include "Common.h"
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

	Skybox::Skybox() = default;
	Skybox::~Skybox() {
		glDeleteBuffers(1, &this->SkyboxCube.vbo);
		glDeleteBuffers(1, &this->SkyboxCube.ibo);
		glDeleteSamplers(1, &this->skybox_sampler);
	}

	void Skybox::Init(unsigned int texture, unsigned int program) {

		/*	*/
		GLint minMapBufferSize = 0;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
		this->uniformAlignSize = Math::align<size_t>(sizeof(uniform_buffer_block), (size_t)minMapBufferSize);

		/*	Create uniform buffer.	*/
		glGenBuffers(1, &this->uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glCreateSamplers(1, &this->skybox_sampler);
		glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glSamplerParameterf(this->skybox_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
		//	glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_MAX_LOD, 0);
		glSamplerParameteri(this->skybox_sampler, GL_TEXTURE_MIN_LOD, 0);

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

		/*	Update uniform values.	*/
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		void *uniformPointer =
			glMapBufferRange(GL_UNIFORM_BUFFER, ((frameIndex + 1) % this->nrUniformBuffer) * this->uniformAlignSize,
							 this->uniformAlignSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		{

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (frameIndex % this->nrUniformBuffer) * this->uniformAlignSize, this->uniformAlignSize);

			/*	Extract current state. to restore afterward rendered the skybox.	*/
			GLint cullstate = 0, blend = 0, depth_test = 0, depth_func = 0, cull_face_mode = 0;
			GLboolean depth_write = 0, use_multisample = 0;
			glGetIntegerv(GL_CULL_FACE, &cullstate);
			glGetIntegerv(GL_BLEND, &blend);
			glGetIntegerv(GL_DEPTH_TEST, &depth_test);
			glGetIntegerv(GL_DEPTH_FUNC, &depth_func);
			glGetIntegerv(GL_CULL_FACE_MODE, &cull_face_mode);
			glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write);
			glGetBooleanv(GL_MULTISAMPLE, &use_multisample);

			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, sizeof("Skybox"), "Skybox");

			/*	*/
			glDisable(GL_MULTISAMPLE); /*	*/
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);

			/*	*/
			glUseProgram(this->skybox_program);

			/*	*/
			if (glBindTextureUnit) {
				glBindTextureUnit(0, this->skybox_texture_panoramic);
			} else {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_texture_panoramic);
			}
			glBindSampler(0, this->skybox_sampler);

			/*	Draw triangle.	*/
			glBindVertexArray(this->SkyboxCube.vao);
			glDrawElements(GL_TRIANGLES, this->SkyboxCube.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			glUseProgram(0);

			glBindSampler(0, 0);

			glPopDebugGroup();

			if (cullstate) {
				glEnable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
			}

			if (blend) {
				glEnable(GL_BLEND);
			}

			/*	Optioncally restore depth.	*/
			if (!depth_test) {
				glDisable(GL_DEPTH_TEST);
			}

			/*	*/
			if (use_multisample) {
				glEnable(GL_MULTISAMPLE);
			}

			glDepthFunc(depth_func);
			glDepthMask(depth_write);
			glCullFace(cull_face_mode);
		}

		this->frameIndex = (this->frameIndex + 1) % this->nrUniformBuffer;
	}

	void Skybox::RenderImGUI() {

		ImGui::PushID(10);
		if (ImGui::CollapsingHeader("Skybox Settings", &this->skybox_settings_visable,
									ImGuiTreeNodeFlags_CollapsingHeader)) {

			ImGui::BeginGroup();
			ImGui::Checkbox("Enabled", &this->isEnabled);
			ImGui::DragFloat3("Rotation", &this->rotation[0]);
			ImGui::ColorEdit4("Tint", &this->uniform_stage_buffer.tintColor[0],
							  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

			ImGui::DragFloat("Exposure", &this->uniform_stage_buffer.correct_settings.exposure);
			ImGui::SameLine();
			ImGui::DragFloat("Gamma", &this->uniform_stage_buffer.correct_settings.gamma);
			ImGui::EndGroup();
		}
		ImGui::PopID();
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
		const int skybox_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

		/*	Setup graphic pipeline.	*/
		glUseProgram(skybox_program);
		int uniform_buffer_index = glGetUniformBlockIndex(skybox_program, "UniformBufferBlock");
		glUniformBlockBinding(skybox_program, uniform_buffer_index, 0);
		glUniform1i(glGetUniformLocation(skybox_program, "PanoramaTexture"), 0);
		glUseProgram(0);

		return skybox_program;
	}

} // namespace glsample