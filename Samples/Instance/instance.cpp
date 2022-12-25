#include "GLSampleWindow.h"

#include "ImageImport.h"
#include "ModelImporter.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Instance : public GLSampleWindow {
	  public:
		Instance() : GLSampleWindow() { this->setTitle("Instance"); }

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			alignas(16) glm::mat4 normalMatrix;

			/*light source.	*/
			glm::vec3 direction = glm::vec3(0, 1, 0);
			glm::vec4 lightColor = glm::vec4(1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4);
		} mvp;

		const size_t nrInstances = (20 * 20);
		std::vector<glm::mat4> instance_model_matrices;

		/*	*/
		GeometryObject instanceGeometry;

		/*	*/
		unsigned int diffuse_texture;
		/*	*/
		unsigned int instance_program;

		/*  Instance buffer model matrix.   */
		unsigned int instance_model_buffer;

		/*  */
		int rows;
		int cols;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_instance_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

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

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->instanceGeometry.vao);
			glDeleteBuffers(1, &this->instanceGeometry.vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load shader source.	*/
			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->instance_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->instance_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->instance_program, "diffuse"), 0);
			glUniform1iARB(glGetUniformLocation(this->instance_program, "normal"), 1);
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
			uniformSize = fragcore::Math::align(uniformSize, (size_t)minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			instance_model_matrices.resize(nrInstances);

			// TODO create geometry

			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			// instanceGeometry = modelLoader.getNodeRoot()->geometryObject;
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
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_instance_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glUseProgram(this->instance_program);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	Draw triangle.	*/
			glBindVertexArray(this->instanceGeometry.vao);
			glDrawElements(GL_TRIANGLES, this->instanceGeometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			for (size_t i = 0; i < rows; i++) {
				for (size_t j = 0; j < cols; j++) {
					const size_t index = i * cols + j;

					glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(i * 4.0f, 0, j * 4.0f));
					model = glm::rotate(model, glm::radians(elapsedTime * 45.0f + index * 12.0f),
										glm::vec3(0.0f, 1.0f, 0.0f));
					model = glm::scale(model, glm::vec3(10.95f));

					instance_model_matrices[index] = this->mvp.proj * this->camera.getViewMatrix() * model;
				}
			}

			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model =
				glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));
			this->mvp.modelViewProjection = this->mvp.model * camera.getViewMatrix();

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(GL_UNIFORM_BUFFER, ((getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
									   uniformSize, GL_MAP_WRITE_BIT);
			memcpy(p, &this->mvp, sizeof(mvp));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Instance> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}