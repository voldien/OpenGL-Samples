#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	class Ocean : public GLSampleWindow {
	  public:
		Ocean() : GLSampleWindow() {
			this->setTitle("Ocean");

			this->oceanSettingComponent = std::make_shared<OceanSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->oceanSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(200.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		typedef struct _ocean_vertex_t {
			/*	*/
			float h0[2];
			float ht_real_img[2];
		} OceanVertex;

		/*	*/
		MeshObject ocean;

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

		struct uniform_buffer_block {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4);

			float width, height;
			float speed;
			float delta;
			float patchSize;

			glm::vec4 gEyeWorldPos;
			float gDispFactor;
			float tessLevel;

		} uniformStageBuffer;

		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		unsigned int ssbo_ocean_buffer_binding = 1;

		CameraController camera;
		Skybox skybox;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;

		/*	*/
		const std::string vertexSkyboxShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxShaderPath = "Shaders/skybox/panoramic.frag.spv";
		/*	*/
		const std::string vertexShaderPath = "Shaders/ocean/ocean.vert.spv";
		const std::string fragmentShaderPath = "Shaders/ocean/ocean.frag.spv";
		const std::string tesscShaderPath = "Shaders/ocean/ocean.tesc.spv";
		const std::string teseShaderPath = "Shaders/ocean/ocean.tese.spv";

		/*	*/
		const std::string computeShaderPath = "Shaders/ocean/ocean.comp.spv";
		const std::string computeKFFShaderPath = "Shaders/ocean/kff.comp.spv";

		class OceanSettingComponent : public nekomimi::UIComponent {
		  public:
			OceanSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Ocean Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Rotate", &this->useAnimate);
			}

			bool showWireFrame = false;
			bool useAnimate = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<OceanSettingComponent> oceanSettingComponent;

		void Release() override {
			/*	*/
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->ocean_graphic_program);
			glDeleteProgram(this->spectrum_compute_program);
			glDeleteProgram(this->kff_compute_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->skybox_panoramic_texture);

			/*	*/
			glDeleteVertexArrays(1, &this->ocean.vao);

			/*	*/
			glDeleteBuffers(1, &this->ocean.vbo);
			glDeleteBuffers(1, &this->ocean.ibo);
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

		void Initialize() override {
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load source code for the ocean shader.	*/
				const std::vector<uint32_t> vertex_ocean_binary =
					IOUtil::readFileData<uint32_t>(vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(fragmentShaderPath, this->getFileSystem());
				const std::vector<uint32_t> control_binary =
					IOUtil::readFileData<uint32_t>(tesscShaderPath, this->getFileSystem());
				const std::vector<uint32_t> evolution_binary =
					IOUtil::readFileData<uint32_t>(teseShaderPath, this->getFileSystem());

				/*	Load source code for spectrum compute shader.	*/
				const std::vector<uint32_t> compute_spectrum_binary =
					IOUtil::readFileData<uint32_t>(computeShaderPath, this->getFileSystem());
				/*	Load source code for fast furious transform.	*/
				const std::vector<uint32_t> compute_kff_binary =
					IOUtil::readFileData<uint32_t>(computeKFFShaderPath, this->getFileSystem());

				/*	Load graphic program for skybox.	*/
				this->ocean_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_ocean_binary, &fragment_binary, nullptr,
													 &control_binary, &evolution_binary);

				/*	Load compute shader program.	*/
				this->spectrum_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_spectrum_binary);
				this->kff_compute_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_kff_binary);

				/*	Load shader binaries.	*/
				std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxShaderPath, this->getFileSystem());

				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Setup ocean Graphic Pipeline.	*/
			glUseProgram(this->ocean_graphic_program);
			unsigned int uniform_buffer_index =
				glGetUniformBlockIndex(this->ocean_graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->ocean_graphic_program, "reflection"), 0);
			glUniformBlockBinding(this->ocean_graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup spectrum compute shader.	*/
			glUseProgram(this->spectrum_compute_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->spectrum_compute_program, "UniformBufferBlock");
			glUniformBlockBinding(this->spectrum_compute_program, uniform_buffer_index, this->uniform_buffer_binding);
			int ocean_buffer_position_index =
				glGetProgramResourceIndex(this->spectrum_compute_program, GL_SHADER_STORAGE_BLOCK, "Positions");
			glShaderStorageBlockBinding(this->spectrum_compute_program, ocean_buffer_position_index,
										this->ssbo_ocean_buffer_binding);
			glUseProgram(0);

			/*	Setup fast furious transform.	*/
			glUseProgram(this->kff_compute_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->kff_compute_program, "UniformBufferBlock");
			glUniformBlockBinding(this->kff_compute_program, uniform_buffer_index, this->uniform_buffer_binding);
			int kff_buffer_position_index =
				glGetProgramResourceIndex(this->kff_compute_program, GL_SHADER_STORAGE_BLOCK, "Positions");
			glShaderStorageBlockBinding(this->kff_compute_program, kff_buffer_position_index,
										this->ssbo_ocean_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	Compute uniform size that is aligned with the requried for the hardware.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = Math::align(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load Ocean plan geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices, this->ocean_width, this->ocean_height);

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
			}

			/*	Load skybox for reflective.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skybox_panoramic_texture = textureImporter.loadImage2D(skyboxPath);
			this->skybox.Init(this->skybox_panoramic_texture, this->skybox_program);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Bind uniform buffer associated with all of the pipelines.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	Compute fast fourier transformation.	*/
			{
				/*	*/
				glUseProgram(this->spectrum_compute_program);
				glBindVertexArray(this->ocean.vao);
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->ssbo_ocean_buffer_binding, this->ocean.vbo, 0, 0);
				glDispatchCompute(ocean_width + 64, ocean_height, 1);

				/*	*/
				int FFT_SIZE = 0;
				const size_t workgroupX = FFT_SIZE * 64;
				glUseProgram(this->kff_compute_program);
				glDispatchCompute(FFT_SIZE * 64, 1, 1);
				glDispatchCompute(256 * 257 / 2 * 64, 1, 1);
				glDispatchCompute(FFT_SIZE * 64, 1, 1);
			}

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			/*	Render ocean.	*/
			{
				/*	Bind reflective material.	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic_texture);

				glUseProgram(this->ocean_graphic_program);

				/*	Draw triangle.	*/
				glBindVertexArray(this->ocean.vao);
				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElements(GL_PATCHES, this->ocean_width * this->ocean_height * 2, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	Render Skybox.	*/
			{
				/*	Draw triangle*/
				this->skybox.Render(this->camera);
			}

			/*	Post processing.	*/
			{}
		}

		void update() override {

			/*	*/
			const float elapsedTime = getTimer().getElapsed<float>();
			camera.update(getTimer().deltaTime<float>());

			/*	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(0.95f));
			this->uniformStageBuffer.view = camera.getViewMatrix();
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * camera.getViewMatrix() * this->uniformStageBuffer.model;

			/*	Updated uniform data.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformAlignBufferSize,
				uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class OceanGLSample : public GLSample<Ocean> {
	  public:
		OceanGLSample() : GLSample<Ocean>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("S,skybox", "Skybox Texture File Path",
					cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"))(
				"W,width", "Width", cxxopts::value<int>()->default_value("128"))(
				"H,height", "Height", cxxopts::value<int>()->default_value("128"));
		}
	};
} // namespace glsample

// TODO param, ocean with,height, skybox

int main(int argc, const char **argv) {
	try {
		glsample::OceanGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
