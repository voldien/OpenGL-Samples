#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class ZPrePass : public GLSampleWindow {
	  public:
		ZPrePass() : GLSampleWindow() {
			this->setTitle("ZPrePass");
			this->shadowSettingComponent = std::make_shared<PointLightShadowSettingComponent>(this->uniform);
			this->addUIComponent(this->shadowSettingComponent);
		}

		typedef struct point_light_t {
			glm::vec3 position;
			float range;
			glm::vec4 color;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;

			float bias = 0.01f;
			float shadowStrength = 1.0f;
		} PointLight;

		static const size_t nrPointLights = 4;
		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProjection[6];
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 lightPosition;

			PointLight pointLights[nrPointLights];
		} uniform;

		glm::mat4 PointView[6];

		/*	Point light shadow maps.	*/
		std::vector<unsigned int> pointShadowFrameBuffers;
		std::vector<unsigned int> pointShadowTextures;

		/*	*/
		unsigned int shadowWidth = 1024;
		unsigned int shadowHeight = 1024;

		std::string diffuseTexturePath = "asset/diffuse.png";

		unsigned int diffuse_texture;

		std::vector<GeometryObject> refObj;

		/*	*/
		unsigned int graphic_program;
		unsigned int shadow_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_shadow_index;
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class PointLightShadowSettingComponent : public nekomimi::UIComponent {
		  public:
			PointLightShadowSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Point Light Shadow Settings");
			}
			virtual void draw() override {

				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::TextUnformatted("Depth Texture");

				for (size_t i = 0; i < sizeof(uniform.pointLights) / sizeof(uniform.pointLights[0]); i++) {
					ImGui::PushID(1000 + i);
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightvisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Light Color", &this->uniform.pointLights[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Light Position", &this->uniform.pointLights[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.pointLights[i].constant_attenuation);
						ImGui::DragFloat("Light Range", &this->uniform.pointLights[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.pointLights[i].intensity);
						ImGui::DragFloat("Shadow Strength", &this->uniform.pointLights[i].shadowStrength, 1, 0.0f,
										 1.0f);
						ImGui::DragFloat("Shadow Bias", &this->uniform.pointLights[i].bias, 1, 0.0f, 1.0f);
					}
					ImGui::PopID();
				}
			}

			bool showWireFrame = false;
			bool animate = false;

			bool lightvisible[4] = {true, true, true, true};

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<PointLightShadowSettingComponent> shadowSettingComponent;

		std::string modelPath = "asset/sponza/sponza.obj";

		/*	Graphic shader paths.	*/
		const std::string vertexGraphicShaderPath = "Shaders/pointlightshadow/pointlightlight.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/pointlightshadow/pointlightlight.frag.spv";

		/*	Shadow shader paths.	*/
		const std::string vertexShadowShaderPath = "Shaders/pointlightshadow/pointlightshadow.vert.spv";
		const std::string geomtryShadowShaderPath = "Shaders/pointlightshadow/pointlightshadow.geom.spv";
		const std::string fragmentShadowShaderPath = "Shaders/pointlightshadow/pointlightshadow.frag.spv";

		virtual void Release() override {
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->shadow_program);

			glDeleteFramebuffers(this->pointShadowFrameBuffers.size(), this->pointShadowFrameBuffers.data());
			glDeleteTextures(this->pointShadowTextures.size(), this->pointShadowTextures.data());

			glDeleteBuffers(1, &this->uniform_buffer);

			glDeleteVertexArrays(1, &this->refObj[0].vao);
			glDeleteBuffers(1, &this->refObj[0].vbo);
			glDeleteBuffers(1, &this->refObj[0].ibo);
		}

		virtual void Initialize() override {

			/*	*/
			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

			/*	*/
			const std::vector<uint32_t> vertex_shadow_source =
				IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
			const std::vector<uint32_t> geometry_shadow_source =
				IOUtil::readFileData<uint32_t>(this->geomtryShadowShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_shadow_source =
				IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shaders	*/
			this->graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);
			this->shadow_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_shadow_source,
																	&fragment_shadow_source, &geometry_shadow_source);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	*/
			glUseProgram(this->shadow_program);
			this->uniform_buffer_shadow_index = glGetUniformBlockIndex(this->shadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->shadow_program, this->uniform_buffer_shadow_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->graphic_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_program, "ShadowTexture"), 1);
			glUniformBlockBinding(this->graphic_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	 Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create shadow map.	*/
				this->pointShadowFrameBuffers.resize(this->nrPointLights);
				glGenFramebuffers(this->pointShadowFrameBuffers.size(), this->pointShadowFrameBuffers.data());

				this->pointShadowTextures.resize(this->nrPointLights);
				glGenTextures(this->pointShadowTextures.size(), this->pointShadowTextures.data());

				for (size_t x = 0; x < pointShadowFrameBuffers.size(); x++) {
					glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowFrameBuffers[x]);

					/*	*/

					glBindTexture(GL_TEXTURE_CUBE_MAP, this->pointShadowTextures[x]);

					/*	*/
					for (size_t i = 0; i < 6; i++) {
						glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, this->shadowWidth,
									 this->shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
					}

					/*	*/
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					/*	Border clamped to max value, it makes the outside area.	*/
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
					/*	*/
					float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
					glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, borderColor);

					/*	*/
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 0);
					glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, 0.0f);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);

					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

					/*	*/

					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->pointShadowTextures[x], 0);

					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					/*	*/
					int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
					if (frstat != GL_FRAMEBUFFER_COMPLETE) {
						/*  Delete  */
						throw RuntimeException("Failed to create framebuffer, {}", glewGetErrorString(frstat));
					}
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			ImportHelper::loadModelBuffer(modelLoader, refObj);

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
										glm::vec4(1, 0, 1, 1)};
			for (size_t i = 0; i < this->nrPointLights; i++) {
				this->uniform.pointLights[i].range = 45.0f;
				this->uniform.pointLights[i].position =
					glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(2.0f);
				this->uniform.pointLights[i].color = colors[i];
				this->uniform.pointLights[i].constant_attenuation = 1.7f;
				this->uniform.pointLights[i].linear_attenuation = 1.5f;
				this->uniform.pointLights[i].qudratic_attenuation = 0.19f;
				this->uniform.pointLights[i].intensity = 1.0f;
			}
		}

		virtual void draw() override {

			int width, height;
			this->getSize(&width, &height);

			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_shadow_index, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowFrameBuffers[0]);

				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, this->shadowWidth, this->shadowHeight);
				glUseProgram(this->shadow_program);
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				/*	Setup the shadow.	*/
				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glUseProgram(this->graphic_program);

				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->shadowSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_CUBE_MAP, this->pointShadowTextures[0]);

				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);
			}
		}

		virtual void update() {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime());

			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 1.0f, 1000.0f);

			glm::mat4 model = glm::mat4(1.0f);
			glm::mat4 translation = glm::translate(model, this->uniform.pointLights[0].position);

			const glm::vec3 lightPosition = this->uniform.pointLights[0].position;
			for (size_t i = 0; i < this->nrPointLights; i++) {
			}
			this->PointView[0] =
				glm::lookAt(lightPosition, lightPosition + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
			this->PointView[1] =
				glm::lookAt(lightPosition, lightPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
			this->PointView[2] =
				glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
			this->PointView[3] =
				glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
			this->PointView[4] =
				glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
			this->PointView[5] =
				glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));
			/*	Compute light matrices.	*/
			glm::mat4 pointPer =
				glm::perspective(glm::radians(90.0f), (float)this->shadowWidth / (float)this->shadowHeight, 0.15f,
								 this->uniform.pointLights[0].range);
			for (size_t i = 0; i < 6; i++) {
				glm::mat4 model = glm::mat4(1.0f);

				model = pointPer * this->PointView[i];
				this->uniform.ViewProjection[i] = model;
			}

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			// this->mvp.model = glm::scale(this->mvp.model, glm::vec3(1.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
			this->uniform.lightPosition = glm::vec4(this->camera.getPosition(), 0.0f);

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(this->uniform));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class ZPrePassGLSample : public GLSample<ZPrePass> {
	  public:
		ZPrePassGLSample(int argc, const char **argv) : GLSample<ZPrePass>(argc, argv) {}
		virtual void commandline(cxxopts::OptionAdder &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::ZPrePassGLSample sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}