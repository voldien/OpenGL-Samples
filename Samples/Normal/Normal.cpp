#include "ImportHelper.h"
#include "ModelImporter.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Normal : public GLSampleWindow {
	  public:
		Normal() : GLSampleWindow() {
			this->setTitle("Normal");

			/*	*/
			this->normalSettingComponent = std::make_shared<NormalSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->normalSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(20.0f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct alignas(16) uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 ViewProj{};
			glm::mat4 modelViewProjection{};

			/*	*/
			float normalLength = 1.0f;

		} uniformStageBuffer;

		/*	*/
		std::vector<MeshObject> refObj;

		/*	Textures.	*/
		unsigned int diffuse_texture{};

		/*	*/
		unsigned int graphic_program{};
		unsigned int normal_vertex_program{};
		unsigned int normal_triangle_program{};

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class NormalSettingComponent : public nekomimi::UIComponent {

		  public:
			NormalSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Normal Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Normal Setting");
				ImGui::DragFloat("Face Normal Length", &this->uniform.normalLength);
				ImGui::Checkbox("Triangle Normal", &this->showTriangleNormal);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Rotation Animation", &this->useAnimation);
			}

			bool showWireFrame = false;
			bool showTriangleNormal = false;
			bool useAnimation = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<NormalSettingComponent> normalSettingComponent;

		/*	Graphic shader paths.	*/
		const std::string vertexGraphicShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/texture/texture.frag.spv";

		/*	Normal shader paths.	*/
		const std::string vertexNormalShaderPath = "Shaders/normal/normal.vert.spv";
		const std::string geomtryNormalVertexShaderPath = "Shaders/normal/normal.geom.spv";
		const std::string geomtryNormalTriangleShaderPath = "Shaders/normal/normal-triangle.geom.spv";
		const std::string fragmentNormalShaderPath = "Shaders/normal/normal.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->normal_triangle_program);
			glDeleteProgram(this->normal_vertex_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			glDeleteVertexArrays(1, &this->refObj[0].vao);
			glDeleteBuffers(1, &this->refObj[0].vbo);
			glDeleteBuffers(1, &this->refObj[0].ibo);
		}

		void Initialize() override {

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_normal_binary =
					IOUtil::readFileData<uint32_t>(this->vertexNormalShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geomtry_normal_binary =
					IOUtil::readFileData<uint32_t>(this->geomtryNormalVertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_normal_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentNormalShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geomtry_normal_triangle_binary =
					IOUtil::readFileData<uint32_t>(this->geomtryNormalTriangleShaderPath, this->getFileSystem());
				/*	*/
				const std::vector<uint32_t> vertex_graphic_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_graphic_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->normal_vertex_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_normal_binary, &fragment_normal_vertex_binary, &geomtry_normal_binary);
				this->normal_triangle_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_normal_binary,
													 &fragment_normal_vertex_binary, &geomtry_normal_triangle_binary);
				this->graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary, &fragment_graphic_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normal_vertex_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->normal_vertex_program, "UniformBufferBlock");
			glUniformBlockBinding(this->normal_vertex_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normal_triangle_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->normal_triangle_program, "UniformBufferBlock");
			glUniformBlockBinding(this->normal_triangle_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->graphic_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load scene from model importer.	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			ImportHelper::loadModelBuffer(modelLoader, refObj);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->normalSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	Draw geometry.  */
			{
				glUseProgram(this->graphic_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glDepthMask(GL_TRUE);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);
				glUseProgram(0);
			}

			/*  Draw surface normals.   */
			{
				glDepthMask(GL_FALSE);
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glBindVertexArray(this->refObj[0].vao);
				if (this->normalSettingComponent->showTriangleNormal) {
					glUseProgram(this->normal_triangle_program);

					for (size_t i = 0; i < this->refObj.size(); i++) {
						glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
												 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
												 this->refObj[i].vertex_offset);
					}

				} else {
					glUseProgram(this->normal_vertex_program);

					for (size_t i = 0; i < this->refObj.size(); i++) {
						glDrawElementsBaseVertex(GL_POINTS, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
												 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
												 this->refObj[i].vertex_offset);
					}
				}

				glBindVertexArray(0);
				glUseProgram(0);
				glDepthMask(GL_TRUE);
			}
		}

		void update() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			if (this->normalSettingComponent->useAnimation) {
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(elapsedTime * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(10.95f));
			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();

			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			this->uniformStageBuffer.ViewProj = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;

			/*	Update buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class NormalGLSample : public GLSample<Normal> {
	  public:
		NormalGLSample() : GLSample<Normal>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::NormalGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}