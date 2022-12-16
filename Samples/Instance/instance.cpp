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
		typedef struct _vertex_t {
			float pos[3];
			float uv[2];
			float normal[3];
			float tangent[3];
		} Vertex;

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
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		CameraController camera;

		std::string diffuseTexturePath = "diffuse.png";

		const std::string vertexShaderPath = "Shaders/instance/instance.vert";
		const std::string fragmentShaderPath = "Shaders/instance/instance.frag";

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

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
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			/*	Load shader	*/
			this->instance_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->instance_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->instance_program, "diffuse"), 0);
			glUniform1iARB(glGetUniformLocation(this->instance_program, "normal"), 1);
			glUniformBlockBinding(this->instance_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(FileSystem::getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize += minMapBufferSize - (uniformSize % minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			// TODO create geometry
			std::string modelPath = "awesome.glfp";
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->instanceGeometry.vao);
			glBindVertexArray(this->instanceGeometry.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->instanceGeometry.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, instanceGeometry.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	Vertices.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	UVs	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	Normals.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

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

			glUseProgram(this->instance_program);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	Draw triangle.	*/
			glBindVertexArray(this->instanceGeometry.vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			// Update all instances.
			for (int i = 0; i < instance_model_matrices.size(); i++) {
			}

			for (unsigned int i = 0; i < rows; i++) {
				for (unsigned int j = 0; j < cols; j++) {
					int index = i * rows + j;
					// hpmvec4f pos = {i, 0, j, 0};
					// pos *= 3;
					//
					///*	Create model matrix.	*/
					// hpm_quat_axisf(&scene->quat[index], 0, rot + i, 0);
					// hpm_mat4x4_translationfv(scene->model[index], &pos);
					// hpm_mat4x4_multi_rotationQfv(scene->model[index], &scene->quat[index]);
					//
					///*	Create mode view matrix.	*/
					// hpm_mat4x4_multiply_mat4x4fv(scene->view, scene->model[index], scene->modelview[index]);
					// hpm_mat4x4_multiply_mat4x4fv(scene->proj, scene->modelview[index], scene->mvp[index]);
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