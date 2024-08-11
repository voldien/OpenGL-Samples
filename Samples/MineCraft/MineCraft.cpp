#include "Chunk.h"
#include "Skybox.h"
#include "World.h"
#include "WorldBuilder.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

namespace glsample {

	class MineCraft : public GLSampleWindow {
	  public:
		MineCraft() : GLSampleWindow() {
			this->setTitle("MineCraft");

			this->mineCraftSettingComponent = std::make_shared<MineCraftSettingComponent>();
			this->addUIComponent(this->mineCraftSettingComponent);
		}

		struct uniform_buffer_block {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

		} uniformBlock;

		Skybox skybox;
		World *world;

		unsigned int white_texture;

		/*	*/
		unsigned int minecraft_program;
		unsigned int minecraft_texture;
		unsigned int random_texture;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_ssao_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		unsigned int uniform_ssao_buffer;
		const size_t nrUniformBuffer = 3;

		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class MineCraftSettingComponent : public nekomimi::UIComponent {

		  public:
			MineCraftSettingComponent() { this->setName("MineCraft Settings"); }
			void draw() override {

				ImGui::Checkbox("DownSample", &downScale);
				ImGui::Checkbox("Use Depth", &useDepth);
			}

			bool downScale = false;
			bool useDepth = false;

		  private:
			// struct UniformSSAOBufferBlock &uniform;
		};
		std::shared_ptr<MineCraftSettingComponent> mineCraftSettingComponent;

		const std::string vertexMineCraftShaderPath = "Shaders/minecraft/minecraft.vert.spv";
		const std::string fragmentMineCraftShaderPath = "Shaders/minecraft/minecraft.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->minecraft_program);
			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			{

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_minecraft_binary =
					IOUtil::readFileData<uint32_t>(this->vertexMineCraftShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_minecraft_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentMineCraftShaderPath, this->getFileSystem());
				/*	Load shader	*/
				this->minecraft_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_minecraft_binary,
																		   &fragment_minecraft_binary);
			}

			/*	Setup graphic ambient occlusion pipeline.	*/
			glUseProgram(this->minecraft_program);
			this->uniform_ssao_buffer_index = glGetUniformBlockIndex(this->minecraft_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->minecraft_program, "WorldTexture"), 1);
			glUniform1iARB(glGetUniformLocation(this->minecraft_program, "NormalTexture"), 3);
			glUniform1iARB(glGetUniformLocation(this->minecraft_program, "DepthTexture"), 4);
			glUniform1iARB(glGetUniformLocation(this->minecraft_program, "NormalRandomize"), 5);
			glUniformBlockBinding(this->minecraft_program, this->uniform_ssao_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);
			// this->uniformSSAOBufferSize = fragcore::Math::align(this->uniformSSAOBufferSize,
			// (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			WorldBuilder worldBuilder(64, 64, 64);
			this->world = worldBuilder.buildWorld();
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed<float>();
			camera.update(getTimer().deltaTime<float>());

			/*	*/
			this->uniformBlock.model = glm::mat4(1.0f);
			this->uniformBlock.model =
				glm::rotate(this->uniformBlock.model, glm::radians(elapsedTime * 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBlock.model = glm::scale(this->uniformBlock.model, glm::vec3(10.95f));
			this->uniformBlock.view = this->camera.getViewMatrix();
			this->uniformBlock.modelViewProjection =
				this->uniformBlock.proj * this->uniformBlock.view * this->uniformBlock.model;

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBlock, sizeof(uniformBlock));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	/*	*/
	class MineCraftGLSample : public GLSample<MineCraft> {
	  public:
		MineCraftGLSample() : GLSample<MineCraft>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			// ADD skybox
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::MineCraftGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}