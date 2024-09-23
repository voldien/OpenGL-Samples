#include "Scene.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 */
	class NormalMapping : public GLSampleWindow {
	  public:
		NormalMapping() : GLSampleWindow() {
			this->setTitle("NormalMapping");

			this->normalMapSettingComponent = std::make_shared<NormalMapSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->normalMapSettingComponent);

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

			/*	*/
			glm::vec4 tintColor = glm::vec4(1, 1, 1, 1);

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 viewDir = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			/*	Normal attributes.	*/
			float normalStrength = 1.0f;
			float shininess = 16;
		} uniformStageBuffer;

		/*	*/
		MeshObject plan;
		Scene scene; /*	World Scene.	*/

		/*	Textures.	*/
		unsigned int diffuse_texture;
		unsigned int normal_texture;

		/*	*/
		unsigned int normalMapping_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class NormalMapSettingComponent : public nekomimi::UIComponent {
		  public:
			NormalMapSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("NormalMap Settings");
			}

			void draw() override {
				ImGui::ColorEdit4("Tint Color", &this->uniform.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Normal Setting");
				ImGui::DragFloat("Strength", &this->uniform.normalStrength);

				ImGui::DragFloat("shininess", &this->uniform.shininess);

				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Rotate", &this->useAnimate);
			}

			bool showWireFrame = false;
			bool useAnimate = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<NormalMapSettingComponent> normalMapSettingComponent;

		const std::string vertexShaderPath = "Shaders/normalmap/normalmap.vert.spv";
		const std::string fragmentShaderPath = "Shaders/normalmap/normalmap.frag.spv";

		void Release() override {
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

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string normalTexturePath = this->getResult()["normal-texture"].as<std::string>();

			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_normalmapping_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_normalmapping_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->normalMapping_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_normalmapping_binary, &fragment_normalmapping_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normalMapping_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->normalMapping_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->normalMapping_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->normalMapping_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->normalMapping_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);
			this->normal_texture = textureImporter.loadImage2D(normalTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = fragcore::Math::align(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load scene from model importer.	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateTorus(1, vertices, indices);

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
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glUseProgram(this->normalMapping_program);

				glDisable(GL_CULL_FACE);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->normal_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, normalMapSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				this->scene.render();
			}
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniform stage buffer values.	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			if (this->normalMapSettingComponent->useAnimate) {
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(elapsedTime * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(1.95f));
			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			this->uniformStageBuffer.ViewProj = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;
			this->uniformStageBuffer.viewDir = glm::vec4(this->camera.getLookDirection(), 0);

			/*	Update uniform buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class NormalMappingGLSample : public GLSample<NormalMapping> {
	  public:
		NormalMappingGLSample() : GLSample<NormalMapping>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"N,normal-texture", "NormalMap Path",
				cxxopts::value<std::string>()->default_value("asset/normalmap.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::NormalMappingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}