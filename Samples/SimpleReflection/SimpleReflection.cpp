#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SimpleReflection : public GLSampleWindow {
	  public:
		SimpleReflection() : GLSampleWindow() {
			this->setTitle("Simple Reflection");
			this->simpleReflectionSettingComponent = std::make_shared<SimpleOceanSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->simpleReflectionSettingComponent);
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f);
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 viewPos;

			float shininess = 16.0f;

		} uniformBuffer;

		/*	*/
		GeometryObject plan;
		GeometryObject obj;
		GeometryObject skybox;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;
		unsigned int multipass_program;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		unsigned int multipass_textures;

		/*	*/
		unsigned int normal_texture;
		unsigned int reflection_texture;

		/*	*/
		unsigned int graphic_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleOceanSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Simple Reflection Settings");
			}
			virtual void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Debug");
				/*	*/
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleReflectionSettingComponent;

		CameraController camera;

		std::string panoramicPath = "asset/winter_lake_01_4k.exr";
		std::string normalTexturePath = "asset/normalmap.png";

		const std::string vertexShaderPath = "Shaders/phongblinn/phongblinn.vert.spv";
		const std::string fragmentShaderPath = "Shaders/phongblinn/phongblinn.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->graphic_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->reflection_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);

			glDeleteVertexArrays(1, &this->skybox.vao);
			glDeleteBuffers(1, &this->skybox.vbo);
			glDeleteBuffers(1, &this->skybox.ibo);
		}

		virtual void Initialize() override {

			/*	Load shader source data.	*/
			const std::vector<uint32_t> vertex_simple_ocean_source =
				IOUtil::readFileData<uint32_t>(vertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_simple_ocean_source =
				IOUtil::readFileData<uint32_t>(fragmentShaderPath, this->getFileSystem());
			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader programs.	*/
			this->graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_simple_ocean_source,
																	 &fragment_simple_ocean_source);
			this->skybox_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->graphic_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "ReflectionTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, this->uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->normal_texture = textureImporter.loadImage2D(this->normalTexturePath);
			this->reflection_texture = textureImporter.loadImage2D(this->panoramicPath);

			/*	Create Normal texture.	*/

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices, 256, 256);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*  Create index buffer.    */
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

			ProceduralGeometry::generateCube(1, vertices, indices);
			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox.vao);
			glBindVertexArray(this->skybox.vao);

			/*	*/
			glGenBuffers(1, &this->skybox.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->skybox.nrIndicesElements = indices.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->skybox.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(12));

			glBindVertexArray(0);

			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			glGenTextures(1, &this->multipass_textures);
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->multipass_framebuffer);

			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->multipass_textures);
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

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->multipass_textures, 0);

			unsigned int rbo;
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->multipass_texture_width,
								  this->multipass_texture_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

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
			this->getSize(&width, &height);

			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

			glViewport(0, 0, width, height);
			glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
			glStencilMask(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			/*	Create Stencils mask of the plan.	*/
			{
				glUseProgram(this->graphic_program);

				glEnable(GL_STENCIL_TEST);
				glEnable(GL_DEPTH_TEST);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				glDepthMask(GL_FALSE);
				glStencilMask(0xFF);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Render reflected objects.		*/
			{

				glUseProgram(this->graphic_program);
				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_STENCIL_TEST);
				glDepthMask(GL_TRUE);
				glStencilFunc(GL_EQUAL, 1, 0xFF);
				glStencilMask(0);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glDisable(GL_STENCIL_TEST);

				glUseProgram(0);
			}

			/*	Plan.	*/
			//{
			//	glUseProgram(this->graphic_program);
			//
			//	glDisable(GL_CULL_FACE);
			//	glEnable(GL_DEPTH_TEST);
			//
			//	glEnable(GL_BLEND);
			//	glBlendEquation(GL_FUNC_ADD);
			//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//
			//	/*	Optional - to display wireframe.	*/
			//	glPolygonMode(GL_FRONT_AND_BACK, this->simpleOceanSettingComponent->showWireFrame ? GL_LINE : GL_FILL);
			//
			//	/*	*/
			//	glActiveTexture(GL_TEXTURE0);
			//	glBindTexture(GL_TEXTURE_2D, this->reflection_texture);
			//
			//	/*	*/
			//	glActiveTexture(GL_TEXTURE0 + 1);
			//	glBindTexture(GL_TEXTURE_2D, this->normal_texture);
			//
			//	/*	Draw triangle.	*/
			//	glBindVertexArray(this->plan.vao);
			//	glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			//	glBindVertexArray(0);
			//
			//	glUseProgram(0);
			//}

			/*	Skybox	*/
			{
				glUseProgram(this->skybox_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, simpleReflectionSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multipass_framebuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			/*	*/
			glViewport(0, 0, width, height);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, this->multipass_texture_height, this->multipass_texture_height, 0, 0, width, height,
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual void update() override {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(10.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			// this->uniformBuffer. = glm::vec4(this->camera.getLookDirection(), 0);
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			this->uniformBuffer.viewPos = glm::vec4(this->camera.getPosition(), 0);

			/*  */
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	}; // namespace glsample

	class SimpleReflectionGLSample : public GLSample<SimpleReflection> {
	  public:
		SimpleReflectionGLSample() : GLSample<SimpleReflection>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		glsample::SimpleReflectionGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}