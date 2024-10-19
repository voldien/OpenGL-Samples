#include "Scene.h"
#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class MipMapScene : public Scene {
	  public:
		MipMapScene() : Scene() {}

		void renderNode(const NodeObject *node) override {

			for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

				glBindVertexArray(this->refGeometry[0].vao);

				/*	*/
				glDrawElementsBaseVertex(
					GL_TRIANGLES, this->refGeometry[node->geometryObjectIndex[i]].nrIndicesElements, GL_UNSIGNED_INT,
					(void *)(sizeof(unsigned int) * this->refGeometry[node->geometryObjectIndex[i]].indices_offset),
					this->refGeometry[node->geometryObjectIndex[i]].vertex_offset);

				glBindVertexArray(0);
			}
		}
	};
	static const std::vector<glm::vec3> mip_colors = {{1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 0, 0}, {1, 0, 1}};

	/**
	 * @brief
	 *
	 */
	class MipMapVisual : public GLSampleWindow {
	  public:
		MipMapVisual() : GLSampleWindow() {
			this->setTitle("MipMap Visualization");

			this->mipmapvisualSettingComponent =
				std::make_shared<MipMapVisualSettingComponent>(this->uniformStageBuffer, this->mipmapbias);
			this->addUIComponent(this->mipmapvisualSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 modelViewProjection;

		} uniformStageBuffer;

		unsigned int mipmap_texture;
		unsigned int mipmap_sampler;

		unsigned int mip_levels = 6;

		MipMapScene scene;

		/*	*/
		unsigned int mipmap_graphic_program;
		float mipmapbias;

		/*	Uniform buffer.	*/
		unsigned int uniform_graphic_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class MipMapVisualSettingComponent : public nekomimi::UIComponent {
		  public:
			MipMapVisualSettingComponent(struct uniform_buffer_block &uniform, float &mipmapbias)
				: mipmapbias(mipmapbias), uniform(uniform) {
				this->setName("MipMap Visualization");
			}

			void draw() override {
				ImGui::DragFloat("MipMap Bias", &this->mipmapbias, 1, -10.0f, 10.0f);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				for (size_t i = 0; i < mip_colors.size(); i++) {
					ImGui::Text("Color Mipmap %zu", i);
					ImGui::ColorEdit3("Color", (float *)&mip_colors[i],
									  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
				}
			}

			bool showWireFrame = false;
			float &mipmapbias;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<MipMapVisualSettingComponent> mipmapvisualSettingComponent;

		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/mipmap/mipmap_visual.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/mipmap/mipmap_visual.frag.spv";

		void Release() override {
			glDeleteProgram(this->mipmap_graphic_program);

			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteTextures(1, &this->mipmap_texture);
			glDeleteSamplers(1, &this->mipmap_sampler);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shaders	*/
				this->mipmap_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->mipmap_graphic_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->mipmap_graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->mipmap_graphic_program, "MipMapTexture"), 0);
			glUniformBlockBinding(this->mipmap_graphic_program, uniform_buffer_index,
								  this->uniform_graphic_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			// Create uniform buffer.
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create mipmap vector.	*/
				const size_t noiseW = 1 << (mip_levels + 1);
				const size_t noiseH = 1 << (mip_levels + 1);

				/*	Create mipmap texture.	*/
				glGenTextures(1, &this->mipmap_texture);
				glBindTexture(GL_TEXTURE_2D, this->mipmap_texture);
				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, this->mip_levels - 1));

				for (size_t i = 0; i < this->mip_levels; i++) {

					const size_t mip_level_width = noiseW >> i;
					const size_t mip_level_height = noiseH >> i;

					std::vector<glm::vec3> miplevelColorData(mip_level_width * mip_level_height);

					for (size_t x = 0; x < miplevelColorData.size(); x++) {
						miplevelColorData[x] = mip_colors[i];
					}

					glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA8, mip_level_width, mip_level_height, 0, GL_RGB, GL_FLOAT,
								 miplevelColorData.data());
				}

				glBindTexture(GL_TEXTURE_2D, 0);
			}

			/*	Create sampler.	*/
			glCreateSamplers(1, &this->mipmap_sampler);
			glSamplerParameteri(this->mipmap_sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glSamplerParameteri(this->mipmap_sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glSamplerParameteri(this->mipmap_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(this->mipmap_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glSamplerParameterf(this->mipmap_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
			glSamplerParameteri(this->mipmap_sampler, GL_TEXTURE_MAX_LOD, this->mip_levels);
			glSamplerParameteri(this->mipmap_sampler, GL_TEXTURE_MIN_LOD, 0);

			/*	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom<MipMapScene>(modelLoader);
		}

		void onResize(int width, int height) override {
			/*	Update camera	*/
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			glSamplerParameterf(this->mipmap_sampler, GL_TEXTURE_LOD_BIAS, this->mipmapbias);

			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_graphic_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glClearColor(0.1f, 0.1f, 0.1f, 1);

				glUseProgram(this->mipmap_graphic_program);

				glCullFace(GL_BACK);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->mipmapvisualSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->mipmap_texture);
				glBindSampler(0, this->mipmap_sampler);

				this->scene.render();
			}
		}

		void update() override {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			const glm::mat4 proj = this->camera.getProjectionMatrix();
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(1.05f));
			const glm::mat4 view = this->camera.getViewMatrix();
			this->uniformStageBuffer.modelViewProjection = proj * view * model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class MipMapVisualGLSample : public GLSample<MipMapVisual> {
	  public:
		MipMapVisualGLSample() : GLSample<MipMapVisual>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {

	try {
		glsample::MipMapVisualGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}