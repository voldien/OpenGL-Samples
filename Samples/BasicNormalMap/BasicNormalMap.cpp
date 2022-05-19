#include "GLWindow.h"
#include "ImageImport.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class BasicNormalMap : public GLSampleWindow {
	  public:
		BasicNormalMap() : GLSampleWindow() { this->setTitle("Basic NormalMap"); }

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			alignas(16) glm::mat4 normalMatrix;

			/*light source.	*/
			glm::vec3 direction = glm::vec3(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4);
		} mvp;

		/*	*/
		unsigned int vbo;
		unsigned int vao;
		unsigned int ibo;
		unsigned int nrElements;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int normal_texture;

		/*	*/
		unsigned int normalMapping_program;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		CameraController camera;

		std::string diffuseTexturePath = "diffuse.png";
		std::string normalTexturePath = "normalmap.png";

		const std::string vertexShaderPath = "Shaders/normalmap/normalmap.vert";
		const std::string fragmentShaderPath = "Shaders/normalmap/normalmap.frag";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->normalMapping_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load shader source.	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			/*	Load shader	*/
			this->normalMapping_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->normalMapping_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->normalMapping_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->normalMapping_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->normalMapping_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->normalMapping_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			this->diffuse_texture = TextureImporter::loadImage2D(this->diffuseTexturePath);
			this->normal_texture = TextureImporter::loadImage2D(this->normalTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize += minMapBufferSize - (uniformSize % minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			glGenBuffers(1, &this->ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->nrElements = indices.size();

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
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glUseProgram(this->normalMapping_program);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	*/
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, this->normal_texture);

			/*	Draw triangle.	*/
			glBindVertexArray(this->vao);
			glDrawElements(GL_TRIANGLES, this->nrElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model =
				glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));
			this->mvp.modelViewProjection = this->mvp.proj * camera.getViewMatrix() * this->mvp.model;

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
									   uniformSize, GL_MAP_WRITE_BIT);
			memcpy(p, &this->mvp, sizeof(mvp));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		GLSample<glsample::BasicNormalMap> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}