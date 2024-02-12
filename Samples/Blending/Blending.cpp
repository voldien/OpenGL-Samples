#include "GLSampleSession.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Blending : public GLSampleWindow {
	  public:
		Blending() : GLSampleWindow() {
			this->setTitle("Blending");

			/*	Setting Window.	*/
			this->blendingSettingComponent = std::make_shared<BlendingSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->blendingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

		} uniformStageBuffer;

		struct InstanceSubBuffer {
			glm::vec4 color[4];
			glm::mat4 model[4];
		};

		const size_t rows = 2;
		const size_t cols = 2;

		/*	*/
		MeshObject geometry;
		MeshObject plan;
		InstanceSubBuffer instanceBuffer;

		/*	Textures.	*/
		unsigned int diffuse_texture;
		unsigned int ground_diffuse_texture;

		/*	*/
		unsigned int blending_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_instance_buffer_binding = 1;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_share_buffer;
		unsigned int uniform_instance_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;

		CameraController camera;

		class BlendingSettingComponent : public nekomimi::UIComponent {
		  public:
			BlendingSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("NormalMap Settings");
			}

			void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<BlendingSettingComponent> blendingSettingComponent;

		/*	*/
		const std::string vertexShaderPath = "Shaders/blending/blending.vert.spv";
		const std::string fragmentShaderPath = "Shaders/blending/blending.frag.spv";

		void Release() override {
			/*	Delete graphic pipeline.	*/
			glDeleteProgram(this->blending_program);
			/*	Delete texture.	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			/*	Delete uniform buffer.	*/
			glDeleteBuffers(1, &this->uniform_share_buffer);

			/*	Delete geometry data.	*/
			glDeleteVertexArrays(1, &this->geometry.vao);
			glDeleteBuffers(1, &this->geometry.vbo);
			glDeleteBuffers(1, &this->geometry.ibo);
		}

		void Initialize() override {

			const std::string diffuseTexturePath = getResult()["texture"].as<std::string>();
			const std::string diffuseGroundTexturePath = this->getResult()["ground-texture"].as<std::string>();

			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_blending_binary_data =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_blending_binary_data =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create graphic pipeline program.	*/
				this->blending_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_blending_binary_data,
																		  &fragment_blending_binary_data);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->blending_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->blending_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->blending_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->blending_program, uniform_buffer_index, this->uniform_buffer_binding);
			/*	*/
			int uniform_instance_buffer_index = glGetUniformBlockIndex(this->blending_program, "UniformInstanceBlock");
			glUniformBlockBinding(this->blending_program, uniform_instance_buffer_index,
								  this->uniform_instance_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);
			this->ground_diffuse_texture = textureImporter.loadImage2D(diffuseGroundTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_share_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_share_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			this->uniformInstanceSize = fragcore::Math::align(sizeof(instanceBuffer), (size_t)minMapBufferSize);
			glGenBuffers(1, &this->uniform_instance_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformInstanceSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateCube(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->geometry.vao);
			glBindVertexArray(this->geometry.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->geometry.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, geometry.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glGenBuffers(1, &this->geometry.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->geometry.nrIndicesElements = indices.size();

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

			/*	*/
			for (size_t i = 0; i < 4; i++) {
				instanceBuffer.color[i] = glm::vec4(i / 3.0f, (i + 1) / 4.0f, 1.0f, 0.5f);
			}
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			// TODO draw multiple instance with various position and color to show the blending.

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_share_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	Bind Model Instance Uniform Buffer.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_instance_buffer_binding, this->uniform_instance_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformInstanceSize,
							  this->uniformInstanceSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Draw plan.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_share_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->blending_program);

				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				glFrontFace(GL_CCW);

				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	Blending.	*/
				glDisable(GL_BLEND);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->ground_diffuse_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->blendingSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	Draw blending objects.	*/
			{
				glUseProgram(this->blending_program);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

				/*	Blending.	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->blendingSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Draw triangle.	*/
				glBindVertexArray(this->geometry.vao);
				glDrawElementsInstanced(GL_TRIANGLES, this->geometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr, 4);
				glBindVertexArray(0);
			}
		}

		void update() override {

			{
				/*	Update Camera.	*/
				const float elapsedTime = this->getTimer().getElapsed<float>();
				this->camera.update(this->getTimer().deltaTime<float>());

				/*	Update instance model matrix.	*/
				for (size_t i = 0; i < rows; i++) {
					for (size_t j = 0; j < cols; j++) {

						const size_t index = i * cols + j;

						glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(i * 10.0f, 0, j * 10.0f));
						model = glm::rotate(model, glm::radians(elapsedTime * 45.0f + index * 11.5f),
											glm::vec3(0.0f, 1.0f, 0.0f));
						model = glm::scale(model, glm::vec3(1.95f));

						this->instanceBuffer.model[index] = model;
					}
				}

				/*	*/
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(10.95f));
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
				this->uniformStageBuffer.ViewProj = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;
			}

			/*	Update uniform.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_share_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			/*	Update instance buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			void *uniformInstance = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformInstanceSize,
				this->uniformInstanceSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformInstance, &instanceBuffer, sizeof(this->instanceBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class BlendingGLSample : public GLSample<Blending> {
	  public:
		BlendingGLSample() : GLSample<Blending>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"))(
				"G,ground-texture", "Ground Texture",
				cxxopts::value<std::string>()->default_value("asset/stylized-ground.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::BlendingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}