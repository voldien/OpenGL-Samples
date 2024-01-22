#include "Core/Math.h"
#include "ModelImporter.h"
#include "Scene.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief Stencil buffer simple sample.
	 *
	 */
	class SimpleReflection : public GLSampleWindow {
	  public:
		SimpleReflection() : GLSampleWindow() {
			this->setTitle("Simple Reflection");
			this->simpleReflectionSettingComponent =
				std::make_shared<SimpleOceanSettingComponent>(this->uniform_stage_buffer, this->depthstencil_texture);
			this->addUIComponent(this->simpleReflectionSettingComponent);
		}

		struct UniformObjectBufferBlock {
			typedef struct point_light_t {
				glm::vec3 position;
				float range;
				glm::vec4 color;
				float intensity;
				float constant_attenuation;
				float linear_attenuation;
				float qudratic_attenuation;
			} PointLight;

			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	*/
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 viewPos = glm::vec4(0);

			/*	light source.	*/
			PointLight pointLights[4];

			float shininess = 8;
			bool useBlinn = true;

		} uniform_stage_buffer;
		const size_t nrPointLights = 4;

		/*	*/
		GeometryObject plan;
		GeometryObject obj;
		GeometryObject skybox;
		Skybox skybox_;
		Scene scene;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;
		unsigned int multipass_program;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		unsigned int multipass_texture;
		unsigned int depthstencil_texture;

		/*	*/
		unsigned int skybox_texture;

		/*	*/
		unsigned int graphic_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformObjectBufferBlock);
		size_t phong_offset = 0;

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleOceanSettingComponent(struct UniformObjectBufferBlock &uniform, unsigned int &depth)
				: uniform(uniform), depth(depth) {
				this->setName("Simple Reflection Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Light Setting");

				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Debug");
				/*	*/
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::TextUnformatted("Depth Texture");
				ImGui::Image(reinterpret_cast<ImTextureID>(this->depth), ImVec2(512, 512));
			}

			bool showWireFrame = false;
			unsigned int &depth;

		  private:
			struct UniformObjectBufferBlock &uniform;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleReflectionSettingComponent;

		CameraController camera;

		const std::string vertexShaderPath = "Shaders/phongblinn/phongblinn.vert.spv";
		const std::string fragmentShaderPath = "Shaders/phongblinn/phongblinn.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->graphic_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->skybox_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);

			glDeleteVertexArrays(1, &this->skybox.vao);
			glDeleteBuffers(1, &this->skybox.vbo);
			glDeleteBuffers(1, &this->skybox.ibo);
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
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
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);
			}

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->graphic_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skybox_texture = textureImporter.loadImage2D(panoramicPath);

			this->skybox_.Init(this->skybox_texture, this->skybox_program);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(sizeof(UniformObjectBufferBlock), (size_t)minMapBufferSize);
			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*	Plan.	*/
			{
				/*	Load geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices, 2, 2);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->plan.vao);
				glBindVertexArray(this->plan.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->plan.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex),
							 vertices.data(), GL_STATIC_DRAW);

				/*  Create index buffer.    */
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
			/*	Cube.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateCube(1, vertices, indices);
				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->skybox.vao);
				glBindVertexArray(this->skybox.vao);

				/*	*/
				glGenBuffers(1, &this->skybox.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->skybox.nrIndicesElements = indices.size();

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->skybox.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex),
							 vertices.data(), GL_STATIC_DRAW);

				/*	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

				/*	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				glBindVertexArray(0);
			}

			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			glGenTextures(1, &this->multipass_texture);
			glGenTextures(1, &this->depthstencil_texture);
			onResize(this->width(), this->height());

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
										glm::vec4(1, 0, 1, 1)};
			for (size_t i = 0; i < this->nrPointLights; i++) {
				uniform_stage_buffer.pointLights[i].range = 25.0f;
				uniform_stage_buffer.pointLights[i].position =
					glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(1.0f, 1.0f, 1.0f);
				uniform_stage_buffer.pointLights[i].color = colors[i];
				uniform_stage_buffer.pointLights[i].constant_attenuation = 1.0f;
				uniform_stage_buffer.pointLights[i].linear_attenuation = 0.1f;
				uniform_stage_buffer.pointLights[i].qudratic_attenuation = 0.05f;
				uniform_stage_buffer.pointLights[i].intensity = 2.0f;
			}
		}

		void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->multipass_framebuffer);

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

			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

			glViewport(0, 0, width, height);
			glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
			glStencilMask(0xff);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			/*	Create Stencils mask of the plan.	*/
			{
				glUseProgram(this->graphic_program);

				glEnable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glDepthMask(GL_FALSE);
				glStencilFunc(GL_NEVER, 255, 0xFF);
				glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
				glStencilMask(0xFF);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Normal Object		*/
			{
				glUseProgram(this->graphic_program);
				glDisable(GL_CULL_FACE);

				glEnable(GL_DEPTH_TEST);
				glEnable(GL_STENCIL_TEST);
				glDepthMask(GL_TRUE);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

				glStencilFunc(GL_EQUAL, 255, 0xff);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				glStencilMask(0x0);

				this->scene.render();

				glUseProgram(0);
			}
			glDisable(GL_STENCIL_TEST);

			/*	Render reflected objects.		*/
			{
				glUseProgram(this->graphic_program);
				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
				// glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
				// glDepthMask(GL_TRUE);
				// glStencilFunc(GL_EQUAL, 1, 0xFF);
				// glStencilMask(0);

				/*	*/

				this->scene.render();

				glUseProgram(0);
			}

			this->skybox_.Render(this->camera);

			// Transfer the result.
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multipass_framebuffer);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);

				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height, 0, 0, width,
								  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniform_stage_buffer.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.model =
				glm::rotate(this->uniform_stage_buffer.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniform_stage_buffer.model = glm::scale(this->uniform_stage_buffer.model, glm::vec3(10.95f));
			this->uniform_stage_buffer.view = this->camera.getViewMatrix();

			this->uniform_stage_buffer.modelViewProjection =
				this->uniform_stage_buffer.proj * this->uniform_stage_buffer.view * this->uniform_stage_buffer.model;
			this->uniform_stage_buffer.viewPos = glm::vec4(this->camera.getPosition(), 0);

			/*  */
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	}; // namespace glsample

	class SimpleReflectionGLSample : public GLSample<SimpleReflection> {
	  public:
		SimpleReflectionGLSample() : GLSample<SimpleReflection>() {}
		void customOptions(cxxopts::OptionAdder &options) override {

			options("S,skybox", "SkyboxPath",
					cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/rungholt/house.obj"));
		}
	};

} // namespace glsample

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