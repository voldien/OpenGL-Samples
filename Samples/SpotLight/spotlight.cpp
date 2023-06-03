#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SpotLight : public GLSampleWindow {
	  public:
		SpotLight() : GLSampleWindow() {
			this->setTitle("SpotLight");
			this->spotLightSettingComponent = std::make_shared<SpotLightSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->spotLightSettingComponent);

			this->camera.setPosition(glm::vec3(18.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		typedef struct spot_light_source_t {
			glm::vec4 position;
			glm::vec4 direction;
			glm::vec4 color;
			float range;
			float angle;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;
			float padd0;
			float padd1;
		} SpotLightSource;

		static const size_t nrSpotLights = 4;
		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

			SpotLightSource spotLights[nrSpotLights];
		} uniformStageBuffer;

		/*	*/
		GeometryObject plan;

		/*	Textures.	*/
		unsigned int diffuse_texture;

		/*	Program.	*/
		unsigned int pointLight_program;

		/*	Uniforms.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class SpotLightSettingComponent : public nekomimi::UIComponent {

		  public:
			SpotLightSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("SpotLight Settings");
			}
			virtual void draw() override {

				for (size_t i = 0; i < sizeof(uniform.spotLights) / sizeof(uniform.spotLights[0]); i++) {
					ImGui::PushID(1000 + i);
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightvisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Color", &this->uniform.spotLights[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Position", &this->uniform.spotLights[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.spotLights[i].constant_attenuation);
						ImGui::DragFloat("Range", &this->uniform.spotLights[i].range);
						ImGui::DragFloat("Angle", &this->uniform.spotLights[i].angle, 0.01f, 0.0f, 1.0f);
						ImGui::DragFloat3("Direction", &this->uniform.spotLights[i].direction[0]);
						ImGui::DragFloat("Intensity", &this->uniform.spotLights[i].intensity);
					}
					ImGui::PopID();
				}
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
			}

		  private:
			struct UniformBufferBlock &uniform;
			bool lightvisible[4] = {true, true, true, true};
		};
		std::shared_ptr<SpotLightSettingComponent> spotLightSettingComponent;

		const std::string vertexSpotlightShaderPath = "Shaders/spotlight/spotlight.vert.spv";
		const std::string fragmentSpotlightShaderPath = "Shaders/spotlight/spotlight.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->pointLight_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			/*	Load shader source.	*/
			std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexSpotlightShaderPath, this->getFileSystem());
			std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentSpotlightShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load graphic pipeline.	*/
			this->pointLight_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->pointLight_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->pointLight_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->pointLight_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->pointLight_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

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

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
										glm::vec4(1, 0, 1, 1)};
			for (size_t i = 0; i < this->nrSpotLights; i++) {
				this->uniformStageBuffer.spotLights[i].range = 45.0f;
				this->uniformStageBuffer.spotLights[i].position =
					glm::vec4(i * -1.0f, i * 1.0f, i * -1.5f, 0) * 12.0f + glm::vec4(2.0f);
				this->uniformStageBuffer.spotLights[i].direction = glm::normalize(
					glm::vec4(fragcore::Math::degToRad(-20.0f * (i + 1)), fragcore::Math::degToRad(-20.0f * (i + 1)),
							  fragcore::Math::degToRad(20.0f * (i + 1)), 0));
				this->uniformStageBuffer.spotLights[i].angle = std::sin(fragcore::Math::degToRad(20.0f * i + 30.0f));
				this->uniformStageBuffer.spotLights[i].color = colors[i];
				this->uniformStageBuffer.spotLights[i].constant_attenuation = 1.0f;
				this->uniformStageBuffer.spotLights[i].linear_attenuation = 0.1f;
				this->uniformStageBuffer.spotLights[i].qudratic_attenuation = 0.05f;
				this->uniformStageBuffer.spotLights[i].intensity = 1.0f;
			}
		}

		virtual void draw() override {

			int width, height;
			this->getSize(&width, &height);

			this->uniformStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Render.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->pointLight_program);

				glDisable(GL_CULL_FACE);

				/*	Bind texture.   */
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
		}

		virtual void update() override {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model =
				glm::rotate(this->uniformStageBuffer.model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(45.95f));
			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class SpotLightGLSample : public GLSample<SpotLight> {
	  public:
		SpotLightGLSample() : GLSample<SpotLight>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SpotLightGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}