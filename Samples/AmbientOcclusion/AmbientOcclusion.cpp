
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class AmbientOcclusion : public GLSampleWindow {
	  public:
		AmbientOcclusion() : GLSampleWindow() {
			this->setTitle("AmbientOcclusion");

			this->ambientOcclusionSettingComponent =
				std::make_shared<AmbientOcclusionSettingComponent>(this->uniformBlock);
			this->addUIComponent(this->ambientOcclusionSettingComponent);
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
		} uniformBlock;

		/*	*/
		GeometryObject plan;

		/*	Textures.	*/
		unsigned int diffuse_texture;
		unsigned int normal_texture;

		/*	*/
		unsigned int normalMapping_program;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		std::string diffuseTexturePath = "asset/diffuse.png";
		std::string normalTexturePath = "asset/normalmap.png";

		class AmbientOcclusionSettingComponent : public nekomimi::UIComponent {

		  public:
			AmbientOcclusionSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Ambient Occlusion Settings");
			}
			virtual void draw() override {

				// for (size_t i = 0; i < sizeof(uniform.pointLights) / sizeof(uniform.pointLights[0]); i++) {
				//	ImGui::PushID(1000 + i);
				//	if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightVisable[i],
				//								ImGuiTreeNodeFlags_CollapsingHeader)) {
				//		// if (ImGui::BeginChild(i + 1000)) {
				//
				//		ImGui::ColorEdit4("Light Color", &this->uniform.pointLights[i].color[0],
				//						  ImGuiColorEditFlags_Float);
				//		ImGui::DragFloat3("Light Position", &this->uniform.pointLights[i].position[0]);
				//		ImGui::DragFloat3("Attenuation", &this->uniform.pointLights[i].constant_attenuation);
				//		ImGui::DragFloat("Light Range", &this->uniform.pointLights[i].range);
				//		ImGui::DragFloat("Intensity", &this->uniform.pointLights[i].intensity);
				//
				//		//}
				//		// ImGui::EndChild();
				//	}
				//	ImGui::PopID();
				//}
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
			}

		  private:
			struct UniformBufferBlock &uniform;
			bool lightVisable[4] = {true, true, true, true};
		};
		std::shared_ptr<AmbientOcclusionSettingComponent> ambientOcclusionSettingComponent;

		const std::string vertexShaderPath = "Shaders/normalmap/normalmap.vert";
		const std::string fragmentShaderPath = "Shaders/normalmap/normalmap.frag";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->normalMapping_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {

			/*	Load shader source.	*/
			std::vector<char> vertex_source = IOUtil::readFileString(this->vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(this->fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->normalMapping_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normalMapping_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->normalMapping_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->normalMapping_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->normalMapping_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->normalMapping_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);
			this->normal_texture = textureImporter.loadImage2D(this->normalTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align(uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateCube(1, vertices, indices);

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

			/*	Create noise vector.	*/
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniformBlock.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				glUseProgram(this->normalMapping_program);

				glDisable(GL_CULL_FACE);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->normal_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
		}

		void update() {
			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			/*	*/
			this->uniformBlock.model = glm::mat4(1.0f);
			this->uniformBlock.model =
				glm::rotate(this->uniformBlock.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBlock.model = glm::scale(this->uniformBlock.model, glm::vec3(10.95f));
			this->uniformBlock.view = this->camera.getViewMatrix();
			this->uniformBlock.modelViewProjection =
				this->uniformBlock.proj * this->uniformBlock.view * this->uniformBlock.model;

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(p, &this->uniformBlock, sizeof(uniformBlock));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	/*	*/
	class AmbientOcclusionGLSample : public GLSample<AmbientOcclusion> {
	  public:
		AmbientOcclusionGLSample(int argc, const char **argv) : GLSample<AmbientOcclusion>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		glsample::AmbientOcclusionGLSample sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}