#include "GLSampleWindow.h"

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
		} OceanVertex;

		/*	*/
		GeometryObject skybox;
		GeometryObject ocean;

		/*	*/
		int skybox_panoramic_texture;

		/*	*/
		unsigned int skybox_program;
		unsigned int ocean_graphic_program;
		unsigned int spectrum_compute_program;
		unsigned int kff_compute_program;

		/*	*/
		unsigned int ocean_width = 128;
		unsigned int ocean_height = 128;

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

		std::string panoramicPath = "asset/panoramic.jpg";

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
			glDeleteTextures(1, (const GLuint *)&this->skybox_panoramic_texture);

			/*	*/
			glDeleteVertexArrays(1, &this->ocean.vao);
			glDeleteVertexArrays(1, &this->skybox.vao);

			/*	*/
			glDeleteBuffers(1, &this->ocean.vbo);
			glDeleteBuffers(1, &this->ocean.ibo);
			glDeleteBuffers(1, &this->skybox.vbo);
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		// float phillips(glm::vec2 k, float max_l) {
		// 	float k_len = vec_length(k);
		// 	if (k_len == 0.0f) {
		// 		return 0.0f;
		// 	}
		// 	float kL = k_len * L;
		// 	glm::vec2 k_dir = vec_normalize(k);
		// 	float kw = vec_dot(k_dir, wind_dir);
		// 	return pow(kw * kw, 1.0f) *						   // Directional
		// 		   exp(-1.0 * k_len * k_len * max_l * max_l) * // Suppress small waves at ~max_l.
		// 		   exp(-1.0f / (kL * kL)) * pow(k_len, -4.0f);
		// }

		// void generateHeightField(glm::vec2 *h0, unsigned int fftInputH, unsigned int fftInputW) {
		// 	float fMultiplier, fAmplitude, fTheta;
		// 	for (unsigned int y = 0; y < fftInputH; y++) {
		// 		for (unsigned int x = 0; x < fftInputW; x++) {
		// 			float kx = Math::PI * x / (float)_patchSize;
		// 			float ky = 2.0f * Math::PI * y / (float)_patchSize;
		// 			float Er = 2.0f * rand() / (float)RAND_MAX - 1.0f;
		// 			float Ei = 2.0f * rand() / (float)RAND_MAX - 1.0f;

		// 			if (!((kx == 0.f) && (ky == 0.f))) {
		// 				fMultiplier = sqrt(phillips(kx, ky, windSpeed, windDir));
		// 			} else {
		// 				fMultiplier = 0.f;
		// 			}
		// 			fAmplitude = Random::range(0.0f, 1.0f);
		// 			fTheta = rand() / (float)RAND_MAX * 2 * Math::PI;
		// 			float h0_re = fMultiplier * fAmplitude * Er;
		// 			float h0_im = fMultiplier * fAmplitude * Ei;
		// 			int i = y * fftInputW + x;
		// 			glm::vec2 tmp = {h0_re, h0_im};
		// 			h0[i] = tmp;
		// 		}
		// 	}
		// }

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load source code for the ocean shader.	*/
			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());
			std::vector<char> control_source = IOUtil::readFileString(tesscShaderPath, this->getFileSystem());
			std::vector<char> evolution_source = IOUtil::readFileString(teseShaderPath, this->getFileSystem());

			/*	Load source code for spectrum compute shader.	*/
			std::vector<char> compute_spectrum_source =
				IOUtil::readFileString(computeShaderPath, this->getFileSystem());
			/*	Load source code for fast furious transform.	*/
			std::vector<char> compute_kff_source = IOUtil::readFileString(computeKFFShaderPath, this->getFileSystem());

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
			glUniform1i(glGetUniformLocation(this->ocean_graphic_program, "reflection"), 0);
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
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Compute uniform size that is aligned with the requried for the hardware.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize = Math::align(uniformSize, (size_t)minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices, ocean_width, ocean_height);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->ocean.vao);
			glBindVertexArray(this->ocean.vao);

			glGenBuffers(1, &this->ocean.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, ocean.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			glGenBuffers(1, &this->ocean.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ocean.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
						 GL_STATIC_DRAW);
			this->ocean.nrIndicesElements = indices.size();

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  (void *)(sizeof(float) * 3));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OceanVertex), (void *)(sizeof(float) * 2));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(OceanVertex), (void *)(sizeof(float) * 2));

			glBindVertexArray(0);

			ProceduralGeometry::generateCube(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox.vbo);
			glBindVertexArray(this->skybox.vao);

			glGenBuffers(1, &this->skybox.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->skybox.nrIndicesElements = indices.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->skybox.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			glBindVertexArray(0);

			/*	Load skybox for reflective.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skybox_panoramic_texture = textureImporter.loadImage2D(this->panoramicPath);
		}

		virtual void draw() override {
			

			int width, height;
			getSize(&width, &height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Bind uniform buffer associated with all of the pipelines.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			/*	Compute fast fourier transformation.	*/
			{
				/*	*/
				glUseProgram(this->spectrum_compute_program);
				glBindVertexArray(this->ocean.vao);
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->ssbo_ocean_buffer_binding, this->ocean.vbo, 0, 0);
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
				glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic_texture);

				glUseProgram(this->ocean_graphic_program);

				/*	Draw triangle.	*/
				glBindVertexArray(this->ocean.vao);
				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElements(GL_PATCHES, ocean_width * ocean_height * 2, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			/*	Render Skybox.	*/
			{
				/*	*/
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_STENCIL);

				glUseProgram(this->skybox_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic_texture);

				/*	Draw triangle*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
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
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
									   uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->mvp, sizeof(mvp));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
} // namespace glsample

// TODO param, ocean with,height, skybox

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Ocean> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
