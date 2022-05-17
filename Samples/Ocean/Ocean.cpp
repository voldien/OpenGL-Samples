#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <Importer/ImageImport.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Ocean : public GLSampleWindow {
	  public:
		Ocean() : GLSampleWindow() { this->setTitle("Ocean"); }
		// TODO rename.
		typedef struct _vertex_t {
			float h0[2];
			float ht_real_img[2];
		} Vertex;

		typedef struct geometry_t {
			unsigned int vao;
			unsigned int vbo;
			unsigned int ibo;
			unsigned int count;
		} Geometry;

		/*	*/
		unsigned int skybox_vao;
		unsigned int skybox_vbo;
		/*	*/
		unsigned int ocean_vao;
		unsigned int ocean_vbo;
		unsigned int ocean_ibo;

		/*	*/
		int skybox_panoramic;
		unsigned int skybox_texture;

		/*	*/
		unsigned int skybox_program;
		unsigned int ocean_graphic_program;
		unsigned int spectrum_compute_program;
		unsigned int kff_compute_program;

		unsigned int ocean_width, ocean_height;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			float delta;
			/*	*/
			float speed;

			/*light source.	*/
			glm::vec3 direction = glm::vec3(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4);

		} mvp;

		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		unsigned int ssbo_ocean_buffer_binding = 1;

		CameraController camera;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;

		std::string panoramicPath = "panoramic.jpg";

		const std::string vertexSkyboxShaderPath = "Shaders/skybox-panoramic/skybox.vert";
		const std::string fragmentSkyboxShaderPath = "Shaders/skybox-panoramic/skybox-panoramic.frag";

		const std::string vertexShaderPath = "Shaders/ocean/ocean.vert";
		const std::string fragmentShaderPath = "Shaders/ocean/ocean.frag";
		const std::string tesscShaderPath = "Shaders/ocean/ocean.tesc";
		const std::string teseShaderPath = "Shaders/ocean/ocean.tese";

		const std::string computeShaderPath = "Shaders/ocean/ocean.comp";
		const std::string computeKFFShaderPath = "Shaders/ocean/kff.comp";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->ocean_graphic_program);
			glDeleteProgram(this->spectrum_compute_program);
			glDeleteProgram(this->kff_compute_program);
			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->skybox_texture);
			/*	*/
			glDeleteVertexArrays(1, &this->ocean_vao);
			glDeleteVertexArrays(1, &this->skybox_vao);
			/*	*/
			glDeleteBuffers(1, &this->ocean_vbo);
			glDeleteBuffers(1, &this->ocean_ibo);
			glDeleteBuffers(1, &this->skybox_vbo);
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load source code for the ocean shader.	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);
			std::vector<char> control_source = IOUtil::readFile(tesscShaderPath);
			std::vector<char> evolution_source = IOUtil::readFile(teseShaderPath);

			/*	Load source code for spectrum compute shader.	*/
			std::vector<char> compute_spectrum_source = IOUtil::readFile(computeShaderPath);
			/*	Load source code for fast furious transform.	*/
			std::vector<char> compute_kff_source = IOUtil::readFile(computeKFFShaderPath);

			/*	Load graphic program for skybox.	*/
			this->ocean_graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, nullptr,
																		   &control_source, &evolution_source);

			/*	Load compute shader program.	*/
			this->spectrum_compute_program = ShaderLoader::loadComputeProgram({&compute_spectrum_source});
			this->kff_compute_program = ShaderLoader::loadComputeProgram({&compute_kff_source});

			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Setup ocean shader.	*/
			glUseProgram(this->ocean_graphic_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->ocean_graphic_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->ocean_graphic_program, "reflection"), 0);
			glUniformBlockBinding(this->ocean_graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup spectrum compute shader.	*/
			glUseProgram(this->spectrum_compute_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->spectrum_compute_program, "UniformBufferBlock");
			glUniformBlockBinding(this->spectrum_compute_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup fast furious transform.	*/
			glUseProgram(this->kff_compute_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->kff_compute_program, "UniformBufferBlock");
			glUniformBlockBinding(this->kff_compute_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup skybox shader.	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Compute uniform size that is aligned with the requried for the hardware.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &minMapBufferSize);
			uniformSize += uniformSize % minMapBufferSize;

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			// TODO add plane with subdivision.

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->ocean_vao);
			glBindVertexArray(this->ocean_vao);

			glGenBuffers(1, &this->ocean_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, ocean_vbo);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

			glGenBuffers(1, &this->ocean_ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ocean_ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glBindVertexArray(0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox_vao);
			glBindVertexArray(this->skybox_vao);

			/*	*/
			glGenBuffers(1, &this->skybox_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
			// glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr); // TODO fix.

			glBindVertexArray(0);

			/*	Load skybox for reflective.	*/
			this->skybox_panoramic = TextureImporter::loadImage2D(this->panoramicPath);
		}

		virtual void draw() override {
			this->update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Bind uniform buffer associated with all of the pipelines.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			/*	*/

			/*	Compute fast fourier transformation.	*/
			{
				/*	*/
				glUseProgram(this->spectrum_compute_program);
				glBindVertexArray(this->ocean_vao);
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->ssbo_ocean_buffer_binding, this->ocean_vbo, 0, 0);
				glDispatchCompute(ocean_width + 64, ocean_height, 1);

				/*	*/
				int FFT_SIZE = 0;
				glUseProgram(this->kff_compute_program);
				glDispatchCompute(FFT_SIZE * 64, 1, 1);
				glDispatchCompute(256 * 257 / 2 * 64, 1, 1);
				glDispatchCompute(FFT_SIZE * 64, 1, 1);
			}

			/*	Render ocean.	*/
			{
				/*	Bind reflective material.	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_texture);

				glUseProgram(this->ocean_graphic_program);

				/*	Draw triangle.	*/
				glBindVertexArray(this->ocean_vao);
				glDrawElements(GL_PATCHES, ocean_width * ocean_height * 2, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	Render Skybox.	*/
			{
				/*	*/
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_STENCIL);

				glUseProgram(this->skybox_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic);

				/*	Draw triangle*/
				glBindVertexArray(this->skybox_vao);
				glDrawArrays(GL_TRIANGLES, 0, 7); // TODO fix
				glBindVertexArray(0);
			}
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			/*	*/
			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));
			this->mvp.view = camera.getViewMatrix();
			this->mvp.modelViewProjection = this->mvp.proj * camera.getViewMatrix() * this->mvp.model;

			/*	Updated uniform data.	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
									   uniformSize, GL_MAP_WRITE_BIT);
			memcpy(p, &this->mvp, sizeof(mvp));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};
} // namespace glsample

// TODO param, ocean with,height, skybox

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Ocean> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
