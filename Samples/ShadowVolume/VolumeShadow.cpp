#include "GLSampleSession.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <Scene.h>
#include <ShaderLoader.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class VolumeShadow : public GLSampleWindow {
	  public:
		VolumeShadow() : GLSampleWindow() {
			this->setTitle("Stencil/Volume Shadow");

			/*	Setting Window.	*/
			this->shadowSettingComponent = std::make_shared<StencilVolumeShadowSettingComponent>(this->uniform);
			this->addUIComponent(this->shadowSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(10.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};

			/*	Light source.	*/
			glm::vec4 direction = glm::vec4(0.7, 0.7, 1, 1);
			glm::vec4 lightColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 ambientColor = glm::vec4(0.2, 0.2, 0.2, 1);
			glm::vec4 viewDir{};

			float shininess = 8;

		} uniform;

		/*	*/
		MeshObject plan;

		Skybox skybox;
		Scene scene;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		/*	G-Buffer	*/
		unsigned int graphic_framebuffer{};
		unsigned int multipass_program{};
		unsigned int multipass_texture_width{};
		unsigned int multipass_texture_height{};
		unsigned int multipass_texture{};
		unsigned int depthstencil_texture{};

		int skybox_program{};
		int volumeshadow_program{};
		int graphic_program{};

		CameraController camera;

		class StencilVolumeShadowSettingComponent : public nekomimi::UIComponent {
		  public:
			StencilVolumeShadowSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Point Light Shadow Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::Checkbox("Use Shadow", &this->useShadow);
				ImGui::Checkbox("Show Graphic", &this->showGraphic);

				/*	*/
				ImGui::TextUnformatted("Debug Seting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Show Volume", &this->showVolume);
			}

			bool showWireFrame = false;
			bool showVolume = false;
			bool useShadow = true;
			bool showGraphic = true;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<StencilVolumeShadowSettingComponent> shadowSettingComponent;

		/*	Graphic shader.	*/
		const std::string vertexGraphicShaderPath = "Shaders/phongblinn/phongblinn_directional_light.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/phongblinn/phong_directional_light.frag.spv";

		/*	Shadow volume extraction.	*/
		const std::string vertexShadowShaderPath = "Shaders/volumeshadow/volumeshadow.vert.spv";
		const std::string geomtryShadowShaderPath = "Shaders/volumeshadow/volumeshadow.geom.spv";
		const std::string fragmentShadowShaderPath = "Shaders/volumeshadow/volumeshadow.frag.spv";

		/*	Skybox Shader Path.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		void Release() override {
			glDeleteProgram(this->volumeshadow_program);
			glDeleteProgram(this->graphic_program);

			/*	*/
			glDeleteFramebuffers(1, &this->graphic_framebuffer);
			glDeleteTextures(1, &this->depthstencil_texture);
			glDeleteTextures(1, &this->multipass_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	Load shader source data.	*/
				const std::vector<uint32_t> graphic_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> graphic_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> volume_shadow_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> volume_shadow_geometry_binary =
					IOUtil::readFileData<uint32_t>(this->geomtryShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> volume_shadow_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->volumeshadow_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &volume_shadow_vertex_binary,
													 &volume_shadow_fragment_binary, &volume_shadow_geometry_binary);
				/*	Load shader programs.	*/
				this->graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &graphic_vertex_binary, &graphic_fragment_binary);

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	*/
			glUseProgram(this->volumeshadow_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->volumeshadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->volumeshadow_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			glUseProgram(this->graphic_program);
			int uniform_buffer_shadow_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_shadow_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "PanoramaTexture"), 0);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Load geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices);

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
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->plan.nrIndicesElements = indices.size();

				/*	Vertex.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

				/*	UV.	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				/*	Normal.	*/
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(skytexture, this->skybox_program);

			/*	*/
			ModelImporter *modelLoader = new ModelImporter(this->getFileSystem());
			modelLoader->loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(*modelLoader);

			// TODO, fix geometry topology
			for (size_t i = 0; i < scene.getMeshes().size(); i++) {
				scene.getMeshes()[i].primitiveType = GL_TRIANGLES_ADJACENCY;
			}

			/*	Create multipass framebuffer.	*/
			{
				glGenFramebuffers(1, &this->graphic_framebuffer);
				glGenTextures(1, &this->multipass_texture);
				/*	*/
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->graphic_framebuffer);

			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->multipass_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->multipass_texture_width, this->multipass_texture_height, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			glBindTexture(GL_TEXTURE_2D, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->multipass_texture, 0);

			/*	Depth and Stencil texture attachment.	*/
			glBindTexture(GL_TEXTURE_2D, this->depthstencil_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, this->multipass_texture_width,
						 this->multipass_texture_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
								   this->depthstencil_texture, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			/*  Validate if created properly.*/
			const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());

			/*	*/
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->shadowSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->graphic_framebuffer);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			/*	*/
			glViewport(0, 0, width, height);

			// Create stencil outline geometry from the light direction, Stencil shadow.
			if (this->shadowSettingComponent->useShadow) {
				{
					glEnable(GL_STENCIL_TEST);
					glDisable(GL_DEPTH_TEST);
					glDisable(GL_BLEND_ADVANCED_COHERENT_NV);

					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
					glDepthMask(GL_FALSE);
					glStencilFunc(GL_NEVER, 255, 0xFF);
					glStencilMask(0xFF);

					// Set the stencil test per the depth fail algorithm
					glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
					glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

					// Draw camera
					glUseProgram(this->volumeshadow_program);
					this->scene.render();

					/*	*/
					glEnable(GL_DEPTH_CLAMP);
					glEnable(GL_CULL_FACE);

					glDisable(GL_STENCIL_TEST);
					glEnable(GL_DEPTH_TEST);

					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
					glDepthMask(GL_TRUE);

					glUseProgram(0);
				}
			}

			/*	Draw scene.	*/
			if (this->shadowSettingComponent->showGraphic) {

				glDisable(GL_STENCIL_TEST);

				glUseProgram(this->graphic_program);
				this->scene.render();
				glUseProgram(0);
			}

			/*	Draw shadow.	*/
			if (this->shadowSettingComponent->useShadow) {
				glEnable(GL_STENCIL_TEST);
				glUseProgram(this->graphic_program);
				// Draw plan
				glUseProgram(0);
			}

			/*	Draw */
			if (this->shadowSettingComponent->showVolume) {

				glUseProgram(this->volumeshadow_program);

				glDisable(GL_STENCIL_TEST);
				glEnable(GL_DEPTH_TEST);

				/*	Blending.	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				this->scene.render();
				glUseProgram(0);
			}

			this->skybox.Render(this->camera);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());

			/* Transfer the result. (blit)	*/
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->graphic_framebuffer);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
				/*	*/
				glViewport(0, 0, width, height);

				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height, 0, 0, width,
								  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
		}

		void update() override {

			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			this->uniform.model =
				glm::rotate(this->uniform.model, glm::radians(0 * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniform.model = glm::scale(this->uniform.model, glm::vec3(1.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.proj = this->camera.getProjectionMatrix();

			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
			this->uniform.viewDir = glm::vec4(this->camera.getLookDirection(), 0.0f);

			/*	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
					this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniform, sizeof(this->uniform));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class VolumeShadowGLSample : public GLSample<VolumeShadow> {
	  public:
		VolumeShadowGLSample() : GLSample<VolumeShadow>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("S,skybox", "SkyboxPath",
					cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::VolumeShadowGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
