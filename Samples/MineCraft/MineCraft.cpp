
#include "Chunk.h"
#include "World.h"
#include "WorldBuilder.h"
#include <GL/glew.h>
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

			this->ambientOcclusionSettingComponent =
				std::make_shared<AmbientOcclusionSettingComponent>(this->uniformBlockSSAO);
			this->addUIComponent(this->ambientOcclusionSettingComponent);
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

		} uniformBlock;

		struct UniformSSAOBufferBlock {
			glm::mat4 proj;
			int samples = 4;
			float radius = 1.5f;
			float intensity = 1.0f;
			float bias;
			float cameraNear;
			float cameraFar;
			glm::vec2 screen;

			glm::vec3 kernel[64];

		} uniformBlockSSAO;

		/*	*/
		GeometryObject plan;
		std::vector<GeometryObject> refObj;

		World *world;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;
		unsigned int multipass_program;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		std::vector<unsigned int> multipass_textures;
		unsigned int depthTexture;

		unsigned int white_texture;

		/*	*/
		unsigned int ssao_program;
		unsigned int random_texture;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_ssao_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		unsigned int uniform_ssao_buffer;
		const size_t nrUniformBuffer = 3;

		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t uniformSSAOBufferSize = sizeof(UniformSSAOBufferBlock);

		CameraController camera;

		const std::string modelPath = "asset/sponza/sponza.obj";

		class AmbientOcclusionSettingComponent : public nekomimi::UIComponent {

		  public:
			AmbientOcclusionSettingComponent(struct UniformSSAOBufferBlock &uniform) : uniform(uniform) {
				this->setName("Ambient Occlusion Settings");
			}
			virtual void draw() override {
				ImGui::DragFloat("Intensity", &this->uniform.intensity, 0.1f, 0.0f);
				ImGui::DragFloat("Radius", &this->uniform.radius, 0.35f, 0.0f);
				ImGui::DragInt("Sample", &this->uniform.samples, 1, 0);
				ImGui::Checkbox("DownSample", &downScale);
				ImGui::Checkbox("Use Depth", &useDepth);
			}

			bool downScale = false;
			bool useDepth = false;

		  private:
			struct UniformSSAOBufferBlock &uniform;
		};
		std::shared_ptr<AmbientOcclusionSettingComponent> ambientOcclusionSettingComponent;

		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		const std::string vertexSSAOShaderPath = "Shaders/ambientocclusion/ambientocclusion.vert.spv";
		const std::string fragmentShaderPath = "Shaders/ambientocclusion/ambientocclusion.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->ssao_program);
			glDeleteProgram(this->multipass_program);

			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader source.	*/
			std::vector<uint32_t> vertex_ssao_source =
				IOUtil::readFileData<uint32_t>(this->vertexSSAOShaderPath, this->getFileSystem());
			std::vector<uint32_t> fragment_ssao_source =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

			std::vector<uint32_t> vertex_multi_pass_source =
				IOUtil::readFileData<uint32_t>(this->vertexMultiPassShaderPath, this->getFileSystem());
			std::vector<uint32_t> fragment_multi_pass_source =
				IOUtil::readFileData<uint32_t>(this->fragmentMultiPassShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->ssao_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_ssao_source, &fragment_ssao_source);

			/*	Load shader	*/
			this->multipass_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_multi_pass_source,
																	   &fragment_multi_pass_source);

			/*	Setup graphic ambient occlusion pipeline.	*/
			glUseProgram(this->ssao_program);
			this->uniform_ssao_buffer_index = glGetUniformBlockIndex(this->ssao_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->ssao_program, "WorldTexture"), 1);
			glUniform1iARB(glGetUniformLocation(this->ssao_program, "NormalTexture"), 3);
			glUniform1iARB(glGetUniformLocation(this->ssao_program, "DepthTexture"), 4);
			glUniform1iARB(glGetUniformLocation(this->ssao_program, "NormalRandomize"), 5);
			glUniformBlockBinding(this->ssao_program, this->uniform_ssao_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic multipass pipeline.	*/
			glUseProgram(this->multipass_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->multipass_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->multipass_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);
			this->uniformSSAOBufferSize = fragcore::Math::align(this->uniformSSAOBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenBuffers(1, &this->uniform_ssao_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSSAOBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			ImportHelper::loadModelBuffer(modelLoader, refObj);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices, 1, 1);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glGenBuffers(1, &this->plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->plan.nrIndicesElements = indices.size();

			/*	Vertex.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	UV.	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			/*	Normal.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(20));

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(32));

			glBindVertexArray(0);

			/*	FIXME:	*/
			std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
			std::default_random_engine generator;
			for (size_t i = 0; i < 64; ++i) {
				glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
								 randomFloats(generator));
				sample = glm::normalize(sample);
				sample *= randomFloats(generator);
				float scale = (float)i / 64.0;
				scale = fragcore::Math::lerp(0.1f, 1.0f, scale * scale);
				sample *= scale;
				uniformBlockSSAO.kernel[i] = sample; // .push_back(sample);
			}

			/*	Create white texture.	*/
			glGenTextures(1, &this->white_texture);
			glBindTexture(GL_TEXTURE_2D, this->white_texture);
			unsigned char white[] = {255, 255, 255, 255};
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			glBindTexture(GL_TEXTURE_2D, 0);

			/*	Create noise vector.	*/
			const size_t noiseW = 4;
			const size_t noiseH = 4;
			std::vector<glm::vec3> randomNoise(noiseW * noiseH);
			for (size_t i = 0; i < randomNoise.size(); i++) {
				randomNoise[i].r = Random::normalizeRand<float>() * 2.0 - 1.0;
				randomNoise[i].g = Random::normalizeRand<float>() * 2.0 - 1.0;
				randomNoise[i].b = 0.0f;
			}

			/*	Create white texture.	*/
			glGenTextures(1, &this->random_texture);
			glBindTexture(GL_TEXTURE_2D, this->random_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, noiseW, noiseH, 0, GL_RGB, GL_FLOAT, randomNoise.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			glBindTexture(GL_TEXTURE_2D, 0);

			/*	*/
			WorldBuilder worldBuilder(64, 64, 64);
			this->world = worldBuilder.buildWorld();

			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			this->multipass_textures.resize(4);
			glGenTextures(this->multipass_textures.size(), this->multipass_textures.data());
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);
			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(multipass_textures.size());
			for (size_t i = 0; i < multipass_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->multipass_texture_width, this->multipass_texture_height,
							 0, GL_RGB, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

				FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->multipass_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			glGenTextures(1, &this->depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->multipass_texture_width,
						 this->multipass_texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));

			/*	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

			glDrawBuffers(drawAttach.size(), drawAttach.data());

			/*  Validate if created properly.*/
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniformBlockSSAO.cameraNear = 0.15f;
			this->uniformBlockSSAO.cameraFar = 1000.0f;
			this->uniformBlockSSAO.screen = glm::vec2(width, height);
			this->uniformBlock.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);
			this->uniformBlockSSAO.proj = this->uniformBlock.proj;

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	G-Buffer extraction.	*/
			{
				glBindFramebuffer(GL_FRAMEBUFFER, this->multipass_framebuffer);
				glViewport(0, 0, this->multipass_texture_width, this->multipass_texture_height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				/*	*/
				glUseProgram(this->multipass_program);

				glDisable(GL_CULL_FACE);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->white_texture);

				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);

				glUseProgram(0);
			}

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer_index, this->uniform_ssao_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSSAOBufferSize,
							  this->uniformSSAOBufferSize);

			/*	Draw post processing effect - Screen Space Ambient Occlusion.	*/
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, width, height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(this->ssao_program);

				glDisable(GL_CULL_FACE);

				for (size_t i = 0; i < this->multipass_textures.size(); i++) {
					/*	*/
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
				}
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, this->depthTexture);
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, this->random_texture);

				/*	Draw.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);

				/*	Downscale.	*/
				for (size_t i = 0; i < 4; i++) {
					glReadBuffer(GL_NONE);
					const size_t w = ((float)width / (std::pow(2.0f, i) + 1));
					const size_t h = ((float)height / (std::pow(2.0f, i) + 1));
					// glBindTexture(GL_TEXTURE_2D, this->)
					// glCopyTexImage2D(GL_TEXTURE_2D, i + 1, GL_RGB8, 0, 0, w, h, 0);
				}
			}
		}

		void update() {

			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

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

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
			void *uniformSSAOPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSSAOBufferSize,
				this->uniformSSAOBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformSSAOPointer, &this->uniformBlockSSAO, sizeof(uniformBlockSSAO));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	/*	*/
	class MineCraftGLSample : public GLSample<MineCraft> {
	  public:
		MineCraftGLSample() : GLSample<MineCraft>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

// TODO add custom options.
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