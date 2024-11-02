#include "Skybox.h"
#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class SimpleOcean : public GLSampleWindow {
	  public:
		SimpleOcean() : GLSampleWindow() {
			this->setTitle("Simple Ocean");

			this->simpleOceanSettingComponent =
				std::make_shared<SimpleOceanSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->simpleOceanSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(200.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		static const size_t nrMaxWaves = 64;

		typedef struct wave_t {
			glm::vec4 waveAmpSpeed; /*	*/
			glm::vec4 direction;	/*	*/
		} Wave;

		struct UniformOceanBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 lookDirection;
			glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 position;

			/*	*/
			Wave waves[nrMaxWaves];

			/*	*/
			int nrWaves = 16;
			float time = 0.0f;

			/*	Material	*/
			float shininess = 8;
			float fresnelPower = 4;
			glm::vec4 oceanColor = glm::vec4(0, 0.4, 1, 1);
		};

		/*	Combined uniform block.	*/
		struct uniform_buffer_block {
			UniformOceanBufferBlock ocean; /*	*/

		} uniform_stage_buffer;

		/*	*/
		MeshObject plan;
		Skybox skybox;

		/*	*/
		unsigned int normal_texture;
		unsigned int reflection_texture;

		/*	*/
		unsigned int simpleOcean_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t oceanUniformSize = 0;

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleOceanSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Simple Ocean Settings");
			}

			void draw() override {
				/*	*/
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.ocean.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ocean.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->uniform.ocean.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				if (ImGui::DragFloat3("Light Direction", &this->uniform.ocean.lightDirection[0])) {
					this->uniform.ocean.lightDirection = glm::normalize(this->uniform.ocean.lightDirection);
				}

				/*	*/
				ImGui::TextUnformatted("Ocean");
				ImGui::DragInt("Number Waves", &this->uniform.ocean.nrWaves, 1, 0, nrMaxWaves);

				if (ImGui::CollapsingHeader("Ocean Wave Setttings", &ocean_setting_visable,
											ImGuiTreeNodeFlags_CollapsingHeader)) {

					for (int i = 0; i < this->uniform.ocean.nrWaves; i++) {
						ImGui::PushID(i);
						ImGui::Text("Wave %d", i);
						ImGui::DragFloat("WaveLength", &this->uniform.ocean.waves[i].waveAmpSpeed[0]);
						ImGui::DragFloat("Amplitude", &this->uniform.ocean.waves[i].waveAmpSpeed[1]);
						ImGui::DragFloat("Speed", &this->uniform.ocean.waves[i].waveAmpSpeed[2]);
						ImGui::DragFloat("Steepness", &this->uniform.ocean.waves[i].waveAmpSpeed[3]);

						if (ImGui::DragFloat2("Direction", &this->uniform.ocean.waves[i].direction[0])) {
						}
						ImGui::PopID();
						/*	*/
					}
				}

				ImGui::TextUnformatted("Ocean Material");
				ImGui::DragFloat("Shinines", &this->uniform.ocean.shininess);
				ImGui::DragFloat("Fresnel Power", &this->uniform.ocean.fresnelPower);
				ImGui::ColorEdit4("Ocean Base Color", &this->uniform.ocean.oceanColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
			bool ocean_setting_visable = true;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleOceanSettingComponent;

		CameraController camera;

		/*	Simple Ocean Wave.	*/
		const std::string vertexSimpleOceanShaderPath = "Shaders/simpleocean/simpleocean.vert.spv";
		const std::string fragmentSimpleOceanShaderPath = "Shaders/simpleocean/simpleocean.frag.spv";

		/*	Skybox.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->simpleOcean_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->reflection_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_simple_ocean_binary =
					IOUtil::readFileData<uint32_t>(vertexSimpleOceanShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_simple_ocean_binary =
					IOUtil::readFileData<uint32_t>(fragmentSimpleOceanShaderPath, this->getFileSystem());

				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader programs.	*/
				this->simpleOcean_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_simple_ocean_binary, &fragment_simple_ocean_binary);
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->simpleOcean_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->simpleOcean_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->simpleOcean_program, "ReflectionTexture"), 0);
			glUniform1i(glGetUniformLocation(this->simpleOcean_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->simpleOcean_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->reflection_texture = textureImporter.loadImage2D(panoramicPath, ColorSpace::Raw);
			this->skybox.Init(this->reflection_texture, this->skybox_program);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->oceanUniformSize = Math::align<size_t>(sizeof(UniformOceanBufferBlock), (size_t)minMapBufferSize);
			this->uniformAlignBufferSize = Math::align<size_t>(this->oceanUniformSize, (size_t)minMapBufferSize);

			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			Common::loadPlan(this->plan, 1, 1024, 1024);

			/*	Initilize Waves.	*/
			for (size_t i = 0; i < nrMaxWaves; i++) {

				float waveLength = (i * 2.2 + 1);
				float waveAmplitude = 0.3f / (i + 1);
				float waveSpeed = (i + 1) * 0.1f;

				this->uniform_stage_buffer.ocean.waves[i].waveAmpSpeed =
					glm::vec4(waveLength, waveAmplitude, waveSpeed, 0);

				this->uniform_stage_buffer.ocean.waves[i].direction = glm::normalize(glm::vec4(
					2 * fragcore::Math::random<float>() - 1.0, 2 * fragcore::Math::random<float>() - 1.0, 0, 0));
			}
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			this->uniform_stage_buffer.ocean.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 10000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->simpleOceanSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	Ocean.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->oceanUniformSize);

				glUseProgram(this->simpleOcean_program);

				/*	*/
				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				/*	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->normal_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
				glUseProgram(0);
			}

			/*	Skybox	*/
			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update Ocean uniforms.	*/
			{
				this->uniform_stage_buffer.ocean.model = glm::mat4(1.0f);
				this->uniform_stage_buffer.ocean.model = glm::rotate(this->uniform_stage_buffer.ocean.model,
																	 glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				this->uniform_stage_buffer.ocean.model =
					glm::scale(this->uniform_stage_buffer.ocean.model, glm::vec3(4000.95f));

				this->uniform_stage_buffer.ocean.view = this->camera.getViewMatrix();
				this->uniform_stage_buffer.ocean.lookDirection = glm::vec4(this->camera.getLookDirection(), 0);

				this->uniform_stage_buffer.ocean.modelViewProjection = this->uniform_stage_buffer.ocean.proj *
																	   this->uniform_stage_buffer.ocean.view *
																	   this->uniform_stage_buffer.ocean.model;
				this->uniform_stage_buffer.ocean.position = glm::vec4(this->camera.getPosition(), 0);
				this->uniform_stage_buffer.ocean.time = elapsedTime;
			}

			/*  */
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			uint8_t *uniformPointer = static_cast<uint8_t *>(glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));

			/*	*/
			memcpy(uniformPointer, &this->uniform_stage_buffer.ocean, sizeof(this->uniform_stage_buffer.ocean));

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	}; // namespace glsample

	class SimpleOceanGLSample : public GLSample<SimpleOcean> {
	  public:
		SimpleOceanGLSample() : GLSample<SimpleOcean>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,skybox-texture", "Skybox Texture Path",
					cxxopts::value<std::string>()->default_value("asset/skybox-animestyle.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SimpleOceanGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}