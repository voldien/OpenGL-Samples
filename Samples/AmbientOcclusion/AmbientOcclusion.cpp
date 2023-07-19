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

	/**
	 * Screen Space Ambient Occlusion post processing effect. Both World and Depth Space.
	 */
	class ScreenSpaceAmbientOcclusion : public GLSampleWindow {
	  public:
		ScreenSpaceAmbientOcclusion() : GLSampleWindow() {
			this->setTitle("ScreenSpace AmbientOcclusion");

			/*	Setting Window.	*/
			this->ambientOcclusionSettingComponent =
				std::make_shared<AmbientOcclusionSettingComponent>(this->uniformBlockSSAO);
			this->addUIComponent(this->ambientOcclusionSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		// TODO combine uniform buffer stage.
		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

		} uniformBlock;

		struct UniformSSAOBufferBlock {
			glm::mat4 proj;
			int samples = 4;
			float radius = 1.5f;
			float intensity = 1.0f;
			float bias = 0.025;
			float cameraNear;
			float cameraFar;
			glm::vec2 screen;
			glm::vec4 kernel[64];

			glm::vec4 color;

		} uniformBlockSSAO;

		/*	*/
		GeometryObject plan;
		std::vector<GeometryObject> refObj;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;
		unsigned int multipass_program;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		std::vector<unsigned int> multipass_textures;
		unsigned int depthTexture;

		/*	White texture for each object.	*/
		unsigned int white_texture;
		/*	Random direction texture.	*/
		unsigned int random_texture;

		/*	*/
		unsigned int ssao_world_program;
		unsigned int ssao_depth_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_ssao_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_ssao_buffer;
		const size_t nrUniformBuffer = 3;

		size_t uniformBufferAlignSize = sizeof(UniformBufferBlock);
		size_t uniformSSAOBufferAlignSize = sizeof(UniformSSAOBufferBlock);

		CameraController camera;

		class AmbientOcclusionSettingComponent : public nekomimi::UIComponent {

		  public:
			AmbientOcclusionSettingComponent(struct UniformSSAOBufferBlock &uniform) : uniform(uniform) {
				this->setName("Ambient Occlusion Settings");
			}
			void draw() override {
				ImGui::DragFloat("Intensity", &this->uniform.intensity, 0.1f, 0.0f);
				ImGui::DragFloat("Radius", &this->uniform.radius, 0.35f, 0.0f);
				ImGui::DragInt("Sample", &this->uniform.samples, 1, 0);
				ImGui::DragFloat("Bias", &this->uniform.bias, 0.01f, 0, 1);
				ImGui::Checkbox("DownSample", &downScale);
				ImGui::Checkbox("Use Depth", &useDepth);
				ImGui::Checkbox("show Only AO", &showAOOnly);
			}

			bool downScale = false;
			bool useDepth = false;
			bool showAOOnly = true;

		  private:
			struct UniformSSAOBufferBlock &uniform;
		};
		std::shared_ptr<AmbientOcclusionSettingComponent> ambientOcclusionSettingComponent;

		/*	Shader to extract values.	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		/*	*/
		const std::string vertexSSAOShaderPath = "Shaders/ambientocclusion/ambientocclusion.vert.spv";
		const std::string fragmentShaderPath = "Shaders/ambientocclusion/ambientocclusion.frag.spv";

		void Release() override {
			/*	Delete graphic pipelines.	*/
			glDeleteProgram(this->ssao_world_program);
			glDeleteProgram(this->ssao_depth_program);
			glDeleteProgram(this->multipass_program);

			/*	*/
			glDeleteFramebuffers(1, &this->multipass_framebuffer);
			/*	Delete textures.	*/
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());
			glDeleteTextures(1, &this->random_texture);
			glDeleteTextures(1, &this->white_texture);

			/*	Delete uniform buffer.	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	Delete geometry data.	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_ssao_source =
					IOUtil::readFileData<uint32_t>(this->vertexSSAOShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_ssao_source =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				const std::vector<uint32_t> vertex_multi_pass_source =
					IOUtil::readFileData<uint32_t>(this->vertexMultiPassShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_multi_pass_source =
					IOUtil::readFileData<uint32_t>(this->fragmentMultiPassShaderPath, this->getFileSystem());

				/*	Load shader	*/
				this->ssao_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_ssao_source, &fragment_ssao_source);

				/*	Load shader	*/
				this->multipass_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_multi_pass_source,
																		   &fragment_multi_pass_source);
			}
			/*	Setup graphic ambient occlusion pipeline.	*/
			glUseProgram(this->ssao_world_program);
			int uniform_ssao_world_buffer_index =
				glGetUniformBlockIndex(this->ssao_world_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "WorldTexture"), 1);
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "NormalTexture"), 3);
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "DepthTexture"), 4);
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "NormalRandomize"), 5);
			glUniformBlockBinding(this->ssao_world_program, uniform_ssao_world_buffer_index,
								  this->uniform_ssao_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic multipass pipeline.	*/
			glUseProgram(this->multipass_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->multipass_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->multipass_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);
			this->uniformSSAOBufferSize = fragcore::Math::align(this->uniformSSAOBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferAlignSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenBuffers(1, &this->uniform_ssao_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSSAOBufferAlignSize * this->nrUniformBuffer, nullptr,
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
			{
				std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
				std::default_random_engine generator;
				for (size_t i = 0; i < 64; ++i) {
					glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
									 randomFloats(generator));
					sample = glm::normalize(sample);
					sample *= randomFloats(generator);
					float scale = (float)i / 64.0f;
					scale = fragcore::Math::lerp(0.1f, 1.0f, scale * scale);
					sample *= scale;
					uniformBlockSSAO.kernel[i] = glm::vec4(sample.x, sample.y, sample.z, 0); // .push_back(sample);
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

				/*	No Mipmap.	*/
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

				/*	Create random texture.	*/
				glGenTextures(1, &this->random_texture);
				glBindTexture(GL_TEXTURE_2D, this->random_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, noiseW, noiseH, 0, GL_RGB, GL_FLOAT, randomNoise.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

				FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			{
				/*	Create multipass framebuffer.	*/
				glGenFramebuffers(1, &this->multipass_framebuffer);

				/*	*/
				this->multipass_textures.resize(4);
				glGenTextures(this->multipass_textures.size(), this->multipass_textures.data());
				onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);
			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(multipass_textures.size());
			for (size_t i = 0; i < multipass_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->multipass_texture_width, this->multipass_texture_height,
							 0, GL_RGB, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

		void draw() override {

			this->update();
			int width, height;
			this->getSize(&width, &height);

			/*	*/
			this->uniformBlockSSAO.cameraNear = 0.15f;
			this->uniformBlockSSAO.cameraFar = 1000.0f;
			this->uniformBlockSSAO.screen = glm::vec2(width, height);
			this->uniformBlock.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);
			this->uniformBlockSSAO.proj = this->uniformBlock.proj;

			/*	G-Buffer extraction.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferAlignSize,
								  this->uniformBufferAlignSize);

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

			/*	Draw post processing effect - Screen Space Ambient Occlusion.	*/
			{

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer_binding, this->uniform_ssao_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSSAOBufferAlignSize,
								  this->uniformSSAOBufferAlignSize);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, width, height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(this->ssao_world_program);

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

				if (this->ambientOcclusionSettingComponent->downScale) {
					/*	Downscale the image.	*/
					for (size_t i = 0; i < 4; i++) {
						glReadBuffer(GL_NONE);
						const size_t w = ((float)width / (std::pow(2.0f, i) + 1));
						const size_t h = ((float)height / (std::pow(2.0f, i) + 1));
						// glBindTexture(GL_TEXTURE_2D, this->)
						// glCopyTexImage2D(GL_TEXTURE_2D, i + 1, GL_RGB8, 0, 0, w, h, 0);
					}
				}
			}
		}

		void update() override {

			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBlock.model = glm::mat4(1.0f);
			this->uniformBlock.model =
				glm::rotate(this->uniformBlock.model, glm::radians(elapsedTime * 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBlock.model = glm::scale(this->uniformBlock.model, glm::vec3(10.95f));
			this->uniformBlock.view = this->camera.getViewMatrix();
			this->uniformBlock.modelViewProjection =
				this->uniformBlock.proj * this->uniformBlock.view * this->uniformBlock.model;

			/*	Update uniform buffers.	*/
			{
				glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * uniformBufferAlignSize,
					this->uniformBufferAlignSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformBlock, sizeof(uniformBlock));
				glUnmapBufferARB(GL_UNIFORM_BUFFER);

				/*	*/
				glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
				void *uniformSSAOPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSSAOBufferAlignSize,
					this->uniformSSAOBufferAlignSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformSSAOPointer, &this->uniformBlockSSAO, sizeof(uniformBlockSSAO));
				glUnmapBufferARB(GL_UNIFORM_BUFFER);
			}
		}
	};

	/*	*/
	class AmbientOcclusionGLSample : public GLSample<AmbientOcclusion> {
	  public:
		AmbientOcclusionGLSample() : GLSample<AmbientOcclusion>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::AmbientOcclusionGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}