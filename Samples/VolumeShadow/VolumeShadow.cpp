#include "GLSampleSession.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class VolumeShadow : public GLSampleWindow {
	  public:
		VolumeShadow() : GLSampleWindow() {
			this->setTitle("Stencil/Volume Shadow");

			this->shadowSettingComponent = std::make_shared<StencilVolumeShadowSettingComponent>(this->uniform);
			this->addUIComponent(this->shadowSettingComponent);
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProjection;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

		} uniform;

		/*	*/
		GeometryObject plan;
		// TODO add scene
		GeometryObject scene;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		/*	G-Buffer	*/
		unsigned int graphic_framebuffer;
		unsigned int multipass_program;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		unsigned int multipass_texture;
		unsigned int depthstencil_texture;

		int volumeshadow_program;
		int graphic_program;

		CameraController camera;
		class StencilVolumeShadowSettingComponent : public nekomimi::UIComponent {
		  public:
			StencilVolumeShadowSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Point Light Shadow Settings");
			}
			virtual void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				/**/
				ImGui::TextUnformatted("Debug Seting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Show Volume", &this->showVolume);
			}

			bool showWireFrame = false;
			bool showVolume = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<StencilVolumeShadowSettingComponent> shadowSettingComponent;

		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/phong/phong.vert";
		const std::string fragmentGraphicShaderPath = "Shaders/phong/phong.frag";

		/*	Shadow shader paths.	*/
		const std::string vertexShadowShaderPath = "Shaders/volumeshadow/volumeshadow.vert.spv";
		const std::string geomtryShadowShaderPath = "Shaders/volumeshadow/volumeshadow.geom.spv";
		const std::string fragmentShadowShaderPath = "Shaders/volumeshadow/volumeshadow.frag.spv";

		void Release() override {
			glDeleteProgram(this->volumeshadow_program);
			glDeleteProgram(this->graphic_program);

			/*	*/
			glDeleteFramebuffers(1, &this->graphic_framebuffer);
			glDeleteTextures(1, &this->depthstencil_texture);
			glDeleteTextures(1, &this->multipass_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			// glDeleteTextures(1, (const GLuint *)&this->gl_texture);

			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	Load shader source data.	*/
				const std::vector<uint32_t> graphic_vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> graphic_fragment_ocean_source =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> volume_shadow_vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> volume_shadow_geometry_source =
					IOUtil::readFileData<uint32_t>(this->geomtryShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> volume_shadow_fragment_source =
					IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->volumeshadow_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &volume_shadow_vertex_source,
													 &volume_shadow_fragment_source, &volume_shadow_geometry_source);
				/*	Load shader programs.	*/
				this->graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &graphic_vertex_source,
																		 &graphic_fragment_ocean_source);
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

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load Texture	*/
			// TextureImporter textureImporter(this->getFileSystem());
			// this->gl_texture = textureImporter.loadImage2D(this->texturePath);

			{
				/*	Load geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateTorus(1, vertices, indices);

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

			{
				/*	Create multipass framebuffer.	*/
				glGenFramebuffers(1, &this->graphic_framebuffer);

				/*	*/
				glGenTextures(1, &this->multipass_texture);
				this->onResize(this->width(), this->height());
			}
		} // namespace glsample

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
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Stencil shadow.
			{
				glEnable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glDepthMask(GL_FALSE);
				glStencilFunc(GL_NEVER, 255, 0xFF);
				glStencilMask(0xFF);

				// Set the stencil test per the depth fail algorithm
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

				// Draw camera
				glUseProgram(this->volumeshadow_program);

				/*	*/
				glDisable(GL_DEPTH_CLAMP);
				glEnable(GL_CULL_FACE);
			}

			/*	Draw shadow.	*/
			{}

			/*	Draw scene.	*/
			{

				glDisable(GL_STENCIL_TEST);

				glUseProgram(this->graphic_program);

				glBindVertexArray(this->plan.vao);
				glDrawElementsBaseVertex(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT,
										 (void *)(sizeof(unsigned int) * this->plan.indices_offset),
										 this->plan.vertex_offset);
				glBindVertexArray(0);
				glUseProgram(0);
			}

			/*	Draw volume geometry.	 */
			if (this->shadowSettingComponent->showVolume) {

				glDisable(GL_CULL_FACE);
				glUseProgram(this->volumeshadow_program);
				glBindVertexArray(this->plan.vao);
				glDrawElementsBaseVertex(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT,
										 (void *)(sizeof(unsigned int) * this->plan.indices_offset),
										 this->plan.vertex_offset);
				glBindVertexArray(0);
				glUseProgram(0);
			}
		}

		void update() override {

			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			this->uniform.model =
				glm::rotate(this->uniform.model, glm::radians(0 * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniform.model = glm::scale(this->uniform.model, glm::vec3(40.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;

			/*	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniform, sizeof(this->uniform));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class VolumeShadowGLSample : public GLSample<VolumeShadow> {
	  public:
		VolumeShadowGLSample() : GLSample<VolumeShadow>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"))(
				"S,skybox", "SkyboxPath", cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"));
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
