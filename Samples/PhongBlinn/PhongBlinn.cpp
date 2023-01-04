
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
			pointLightSettingComponent = std::make_shared<PointLightSettingComponent>(this->uniform);
			this->addUIComponent(pointLightSettingComponent);
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
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	*/
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 viewPos;

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

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class PointLightSettingComponent : public nekomimi::UIComponent {

		  public:
			PointLightSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Point Light Settings");
			}
			virtual void draw() override {

				for (size_t i = 0; i < sizeof(uniform.pointLights) / sizeof(uniform.pointLights[0]); i++) {
					ImGui::PushID(1000 + i);
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightVisable[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Light Color", &this->uniform.pointLights[i].color[0],
										  ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Light Position", &this->uniform.pointLights[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.pointLights[i].constant_attenuation);
						ImGui::DragFloat("Light Range", &this->uniform.pointLights[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.pointLights[i].intensity);
					}
					ImGui::PopID();
				}
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->uniform.specularColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Shinnes", &this->uniform.shininess);
				ImGui::Checkbox("Blinn", &this->uniform.useBlinn);
			}

		  private:
			struct UniformBufferBlock &uniform;
			bool lightVisable[4] = {true, true, true, true};
		};
		std::shared_ptr<PointLightSettingComponent> pointLightSettingComponent;

		std::string diffuseTexturePath = "asset/diffuse.png";

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

			/*	Load shader source.	*/
			/*	*/
			std::vector<uint32_t> vertex_source_binary =
				IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			std::vector<uint32_t> fragment_source_binary =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			std::vector<char> vertex_source_T =
				fragcore::ShaderCompiler::convertSPIRV(vertex_source_binary, compilerOptions);
			std::vector<char> fragment_source_T =
				fragcore::ShaderCompiler::convertSPIRV(fragment_source_binary, compilerOptions);

			/*	Load shader	*/
			this->phongblinn_program = ShaderLoader::loadGraphicProgram(&vertex_source_T, &fragment_source_T);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->phongblinn_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->phongblinn_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->phongblinn_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->phongblinn_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align(uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

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

			this->update();
			int width, height;
			getSize(&width, &height);

			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
							  (getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
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

		void update() {
			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			this->uniform.model =
				glm::rotate(this->uniform.model, glm::radians(45.0f * elapsedTime), glm::vec3(1.0f, 1.0f, 0.0f));
			this->uniform.model = glm::scale(this->uniform.model, glm::vec3(45.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
			this->uniform.viewPos = glm::vec4(this->camera.getPosition(), 1.0);

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(uniform));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};
	class PhongBlinnGLSample : public GLSample<PhongBlinn> {
	  public:
		PhongBlinnGLSample(int argc, const char **argv) : GLSample<PhongBlinn>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		glsample::PhongBlinnGLSample sample(argc, argv);
		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}