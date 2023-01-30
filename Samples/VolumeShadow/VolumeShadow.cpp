#include <GL/glew.h>
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
			this->setTitle("Volume Shadow");

			this->shadowSettingComponent = std::make_shared<PointLightShadowSettingComponent>(this->uniform);
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

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		unsigned int multipass_texture;
		unsigned int depthTexture;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_shadow_index;
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		int volumeshadow_program;
		int graphic_program;

		CameraController camera;

		std::string texturePath = "texture.png";

		class PointLightShadowSettingComponent : public nekomimi::UIComponent {
		  public:
			PointLightShadowSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
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
		std::shared_ptr<PointLightShadowSettingComponent> shadowSettingComponent;

		/*	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag";

		/*	Shadow shader paths.	*/
		const std::string vertexShadowShaderPath = "Shaders/volumeshadow/volumeshadow.vert.spv";
		const std::string geomtryShadowShaderPath = "Shaders/volumeshadow/volumeshadow.geom.spv";
		const std::string fragmentShadowShaderPath = "Shaders/volumeshadow/volumeshadow.frag.spv";

		virtual void Release() override {
			glDeleteProgram(this->volumeshadow_program);
			glDeleteProgram(this->graphic_program);

			// glDeleteTextures(1, (const GLuint *)&this->gl_texture);

			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->volumeshadow_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->volumeshadow_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->volumeshadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->volumeshadow_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			glUseProgram(this->graphic_program);
			this->uniform_buffer_shadow_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_program, "ShadowTexture"), 1);
			glUniformBlockBinding(this->graphic_program, this->uniform_buffer_shadow_index,
								  this->uniform_buffer_binding);
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
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
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

			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			glGenTextures(1, &this->multipass_texture);
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

			/*	Resize the image.	*/

			glBindTexture(GL_TEXTURE_2D, this->multipass_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->multipass_texture_width, this->multipass_texture_height, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->multipass_texture, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);

			/*	*/
			glGenTextures(1, &this->depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, this->multipass_texture_width,
						 this->multipass_texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture, 0);

			/*  Validate if created properly.*/
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Stencil shadow.
			{
				glEnable(GL_STENCIL_TEST);

				glDepthMask(GL_FALSE);
				glEnable(GL_DEPTH_CLAMP);
				glDisable(GL_CULL_FACE);

				glStencilFunc(GL_ALWAYS, 0, 0xff);

				// Set the stencil test per the depth fail algorithm
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

				// Draw camera
				glUseProgram(this->volumeshadow_program);

				glDisable(GL_DEPTH_CLAMP);
				glEnable(GL_CULL_FACE);
			}

			/*	Draw scene.	*/
			{

				glDisable(GL_STENCIL_TEST);

				glUseProgram(this->volumeshadow_program);

				// glActiveTexture(GL_TEXTURE0);
				// glBindTexture(GL_TEXTURE_2D, this->gl_texture);

				/*	Draw triangle	*/
				glBindVertexArray(this->plan.vao);
				// glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
				glBindVertexArray(0);
			}
			if (this->shadowSettingComponent->showVolume) {

				glDisable(GL_STENCIL_TEST);
				glUseProgram(this->volumeshadow_program);
				glBindVertexArray(this->plan.vao);
				glDrawElementsBaseVertex(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT,
										 (void *)(sizeof(unsigned int) * this->plan.indices_offset),
										 this->plan.vertex_offset);
				glBindVertexArray(0);
				glUseProgram(0);
			}

			/*	Draw shadow.	*/
		}

		virtual void update() {
			this->camera.update(getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			this->uniform.model =
				glm::rotate(this->uniform.model, glm::radians(0 * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniform.model = glm::scale(this->uniform.model, glm::vec3(40.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(this->uniform));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class VolumeShadowGLSample : public GLSample<VolumeShadow> {
	  public:
		VolumeShadowGLSample() : GLSample<VolumeShadow>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
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
