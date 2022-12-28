#include "GLSampleWindow.h"

#include "ImageImport.h"
#include "ModelImporter.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	// TODO add support for batching.
	class Fog : public GLSampleWindow {
	  public:
		Instance() : GLSampleWindow() { this->setTitle("Fog"); }

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	Light source.	*/
			alignas(4) glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f);
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			alignas(4) glm::vec3 viewPos;
			alignas(4) float shininess = 16.0f;
		} uniformData;

		/*  */
		size_t rows = 16;
		size_t cols = 16;

		size_t instanceBatch = 64;
		const size_t nrInstances = (rows * cols);
		std::vector<glm::mat4> instance_model_matrices;

		/*	*/
		GeometryObject instanceGeometry;

		/*	*/
		unsigned int diffuse_texture;
		/*	*/
		unsigned int instance_program;

		/*  Instance buffer model matrix.   */
		unsigned int instance_model_buffer;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_instance_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;
		unsigned int uniform_mvp_buffer;
		unsigned int uniform_instance_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);
		size_t uniformInstanceSize = 0;

		CameraController camera;

		class NormalMapSettingComponent : public nekomimi::UIComponent {

		  public:
			NormalMapSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("NormalMap Settings");
			}
			virtual void draw() override {
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<NormalMapSettingComponent> normalMapSettingComponent;

		const std::string diffuseTexturePath = "diffuse.png";
		const std::string modelPath = "asset/bunny.obj";

		const std::string vertexShaderPath = "Shaders/instance/instance.vert";
		const std::string fragmentShaderPath = "Shaders/instance/instance.frag";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->instance_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_mvp_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->instanceGeometry.vao);
			glDeleteBuffers(1, &this->instanceGeometry.vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

			/*	Load shader source.	*/
			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->instance_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->instance_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->instance_program, "DiffuseTexture"), 0);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformBufferBlock");
			glUniformBlockBinding(this->instance_program, uniform_buffer_index, this->uniform_buffer_binding);
			this->uniform_instance_buffer_index =
				glGetUniformBlockIndex(this->instance_program, "UniformInstanceBlock");

			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = fragcore::Math::align(this->uniformSize, (size_t)minMapBufferSize);

			glGenBuffers(1, &this->uniform_mvp_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			GLint uniformMaxSize;
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uniformMaxSize);
			this->instanceBatch = uniformMaxSize / sizeof(glm::mat4);

			/*	*/
			this->uniformInstanceSize =
				fragcore::Math::align(this->instanceBatch * sizeof(glm::mat4), (size_t)minMapBufferSize);
			this->instance_model_matrices.resize(nrInstances);
			glGenBuffers(1, &this->uniform_instance_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformInstanceSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			const ModelSystemObject &modelRef = modelLoader.getModels()[0];

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->instanceGeometry.vao);
			glBindVertexArray(this->instanceGeometry.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->instanceGeometry.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, instanceGeometry.vbo);
			glBufferData(GL_ARRAY_BUFFER, modelRef.nrVertices * modelRef.vertexStride, modelRef.vertexData,
						 GL_STATIC_DRAW);

			/*	*/
			glGenBuffers(1, &this->instanceGeometry.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instanceGeometry.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelRef.nrIndices * modelRef.indicesStride, modelRef.indicesData,
						 GL_STATIC_DRAW);
			this->instanceGeometry.nrIndicesElements = modelRef.nrIndices;

			/*	Vertices.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, nullptr);

			/*	UVs	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(12));

			/*	Normals.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(20));

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(32));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniformData.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_mvp_buffer,
							  (getFrameCount() % this->nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_instance_buffer_index, this->uniform_instance_buffer,
							  (getFrameCount() % this->nrUniformBuffer) * this->uniformInstanceSize,
							  this->uniformInstanceSize);

			// TODO add support for batching for limit amount of uniform buffers.

			glUseProgram(this->instance_program);

			glDisable(GL_CULL_FACE);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	Draw Instances.	*/
			glBindVertexArray(this->instanceGeometry.vao);
			glDrawElementsInstanced(GL_TRIANGLES, this->instanceGeometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
									this->nrInstances);
			glBindVertexArray(0);
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			for (size_t i = 0; i < rows; i++) {
				for (size_t j = 0; j < cols; j++) {
					const size_t index = i * cols + j;

					glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(i * 10.0f, 0, j * 10.0f));
					model = glm::rotate(model, glm::radians(elapsedTime * 45.0f + index * 11.5f),
										glm::vec3(0.0f, 1.0f, 0.0f));
					model = glm::scale(model, glm::vec3(1.95f));

					instance_model_matrices[index] = model;
				}
			}

			this->uniformData.model = glm::mat4(1.0f);
			this->uniformData.view = camera.getViewMatrix();
			this->uniformData.modelViewProjection = this->uniformData.model * camera.getViewMatrix();
			this->uniformData.viewPos = this->camera.getPosition();
			//	this->uniformData.viewPos = this->camera.getPosition();

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			void *uniformMVP =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
								 uniformSize, GL_MAP_WRITE_BIT);
			memcpy(uniformMVP, &this->uniformData, sizeof(uniformData));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			void *uniformInstance = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformInstanceSize,
				this->uniformInstanceSize, GL_MAP_WRITE_BIT);
			memcpy(uniformInstance, instance_model_matrices.data(),
				   sizeof(instance_model_matrices[0]) * instance_model_matrices.size());
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Fog> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}