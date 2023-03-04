#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class PhongBlinn : public GLSampleWindow {
	  public:
		PhongBlinn() : GLSampleWindow() {
			this->setTitle("PhongBlinn");
			this->phongblinnSettingComponent = std::make_shared<PhongBlinnSettingComponent>(this->uniform);
			this->addUIComponent(this->phongblinnSettingComponent);

			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		typedef struct point_light_t {
			glm::vec3 position;
			float range;
			glm::vec4 color;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;
		} PointLight;

		struct UniformBufferBlock {
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
		} uniform;

		/*	*/
		GeometryObject plan;
		const size_t nrPointLights = 4;

		/*	Textures.	*/
		unsigned int diffuse_texture;

		/*	*/
		unsigned int phongblinn_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class PhongBlinnSettingComponent : public nekomimi::UIComponent {

		  public:
			PhongBlinnSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Phong-Blinn Settings");
			}
			virtual void draw() override {

				for (size_t i = 0; i < sizeof(uniform.pointLights) / sizeof(uniform.pointLights[0]); i++) {
					ImGui::PushID(1000 + i);
					ImGui::TextUnformatted("Point Light Setting");
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightVisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Color", &this->uniform.pointLights[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Position", &this->uniform.pointLights[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.pointLights[i].constant_attenuation);
						ImGui::DragFloat(" Range", &this->uniform.pointLights[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.pointLights[i].intensity);
					}
					ImGui::PopID();
				}
				ImGui::TextUnformatted("Directional Light Setting");
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->uniform.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Material Setting");
				ImGui::DragFloat("Shinnes", &this->uniform.shininess);
				ImGui::Checkbox("Blinn", &this->uniform.useBlinn);
			}

		  private:
			struct UniformBufferBlock &uniform;
			bool lightVisible[4] = {true, true, true, true};
		};
		std::shared_ptr<PhongBlinnSettingComponent> phongblinnSettingComponent;

		const std::string vertexShaderPath = "Shaders/phongblinn/phongblinn.vert.spv";
		const std::string fragmentShaderPath = "Shaders/phongblinn/phongblinn.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->phongblinn_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {
			
			/*	*/
			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			/*	Load shader source.	*/
			const std::vector<uint32_t> vertex_source_binary =
				IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source_binary =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->phongblinn_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source_binary, &fragment_source_binary);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->phongblinn_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->phongblinn_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->phongblinn_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->phongblinn_program, uniform_buffer_index, this->uniform_buffer_binding);
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
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

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

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
										glm::vec4(1, 0, 1, 1)};
			for (size_t i = 0; i < this->nrPointLights; i++) {
				uniform.pointLights[i].range = 25.0f;
				uniform.pointLights[i].position =
					glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(1.0f, 1.0f, 1.0f);
				uniform.pointLights[i].color = colors[i];
				uniform.pointLights[i].constant_attenuation = 1.0f;
				uniform.pointLights[i].linear_attenuation = 0.1f;
				uniform.pointLights[i].qudratic_attenuation = 0.05f;
				uniform.pointLights[i].intensity = 2.0f;
			}
		}

		virtual void draw() override {

			int width, height;
			this->getSize(&width, &height);

			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);
				glUseProgram(this->phongblinn_program);

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
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			this->uniform.model =
				glm::rotate(this->uniform.model, glm::radians(45.0f * elapsedTime), glm::vec3(1.0f, 1.0f, 0.0f));
			this->uniform.model = glm::scale(this->uniform.model, glm::vec3(45.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
			this->uniform.viewPos = glm::vec4(this->camera.getPosition(), 0.0);

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(this->uniform));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
	class PhongBlinnGLSample : public GLSample<PhongBlinn> {
	  public:
		PhongBlinnGLSample() : GLSample<PhongBlinn>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PhongBlinnGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}