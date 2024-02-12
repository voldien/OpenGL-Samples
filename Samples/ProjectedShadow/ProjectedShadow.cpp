#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ProjectedShadow : public GLSampleWindow {
	  public:
		ProjectedShadow() : GLSampleWindow() {
			this->setTitle("Projected Shadow");

			this->shadowProjectedSettingComponent =
				std::make_shared<AlphaClippingSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->shadowProjectedSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	Light source.	*/
			glm::vec4 lightDirection = glm::vec4(0, 1, 0, 0);
			glm::vec4 lightColor = glm::vec4(1, 1, 1, 1);

			/*	*/
			glm::vec4 ambientColor = glm::vec4(0.4f, 0.4f, 0.4f, 1);
			glm::vec4 specularColor = glm::vec4(1);
			glm::vec4 viewPos;

			/*	*/
			float shininess = 8;

		} uniformStageBuffer;

		struct UniformProjectShadow {
			glm::mat4 model;
			glm::mat4 viewProjection;
			alignas(16) glm::mat4 shadowProjectMatrix;
			glm::vec4 color = glm::vec4(1, 1, 1, 1);

			glm::vec3 planeNormal = glm::vec3(0, 1, 0);
			glm::vec3 lightPosition = glm::vec3(20, 20, 20);

		} projectShadowUniformBuffer;

		MeshObject plan;
		MeshObject model;
		glm::quat lightRotation;
		/*	*/
		unsigned int diffuse_texture;

		/*	*/
		unsigned int phongblinn_program;
		unsigned int shadow_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignSize;
		size_t uniformProjectShadowAlignSize;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		/*	*/
		const std::string vertexShaderPath = "Shaders/phong/phong.vert.spv";
		const std::string fragmentShaderPath = "Shaders/phong/phong.frag.spv";

		/*	*/
		const std::string vertexShadowShaderPath = "Shaders/projectedshadow/projectedshadow.vert.spv";
		const std::string fragmentShadowShaderPath = "Shaders/projectedshadow/projectedshadow.frag.spv";

		class AlphaClippingSettingComponent : public nekomimi::UIComponent {
		  public:
			AlphaClippingSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("NormalMap Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Projected Shadow Setting");
				// TODO add support for plane
				// ImGui::DragFloat3("Normal", &this->uniform.ambientColor[0]);
				// ImGui::DragFloat3("Position", &this->uniform.ambientColor[0]);

				ImGui::TextUnformatted("Material Settings");
				ImGui::ColorEdit4("Light Color", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->uniform.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				ImGui::TextUnformatted("Material Setting");
				ImGui::DragFloat("Shinnes", &this->uniform.shininess);

				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<AlphaClippingSettingComponent> shadowProjectedSettingComponent;

		void Release() override {
			/*	*/
			glDeleteProgram(this->phongblinn_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			/*	*/
			std::string texturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			/*	*/
			{
				/*	*/
				const std::vector<uint32_t> texture_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> texture_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->phongblinn_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &texture_vertex_binary, &texture_fragment_binary);
			}

			/*	*/
			{
				/*	*/
				const std::vector<uint32_t> shadow_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> shadow_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->shadow_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &shadow_vertex_binary, &shadow_fragment_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->phongblinn_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->phongblinn_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->phongblinn_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->phongblinn_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup shadow graphic pipeline.	*/
			glUseProgram(this->shadow_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->shadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->shadow_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align the uniform buffer size to hardware specific.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignSize = fragcore::Math::align(sizeof(this->uniformStageBuffer), (size_t)minMapBufferSize);
			this->uniformProjectShadowAlignSize =
				fragcore::Math::align(sizeof(this->projectShadowUniformBuffer), (size_t)minMapBufferSize);

			this->uniformBufferSize = this->uniformAlignSize + this->uniformProjectShadowAlignSize;

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(texturePath);

			{
				/*	*/
				ModelImporter modelLoader(FileSystem::getFileSystem());
				modelLoader.loadContent(modelPath, 0);

				const ModelSystemObject &modelRef = modelLoader.getModels()[0];

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->model.vao);
				glBindVertexArray(this->model.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->model.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
				glBufferData(GL_ARRAY_BUFFER, modelRef.nrVertices * modelRef.vertexStride, modelRef.vertexData,
							 GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->model.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelRef.nrIndices * modelRef.indicesStride, modelRef.indicesData,
							 GL_STATIC_DRAW);
				this->model.nrIndicesElements = modelRef.nrIndices;

				/*	Vertices.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, nullptr);

				/*	UVs	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(12));

				/*	Normals.	*/
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}

			{
				/*	Load plane geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->plan.vao);
				glBindVertexArray(this->plan.vao);

				/*	Create indices buffer.	*/
				glGenBuffers(1, &this->plan.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->plan.nrIndicesElements = indices.size();

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->plan.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
							 GL_STATIC_DRAW);

				/*	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(0));

				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				glBindVertexArray(0);
			}
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			this->uniformStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->shadowProjectedSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformAlignSize);

				/*	Draw plane.	*/
				{

					glUseProgram(this->phongblinn_program);

					/*	Bind texture.   */
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

					glDisable(GL_CULL_FACE);

					/*	Draw triangle.	*/
					glBindVertexArray(this->plan.vao);
					glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
					glBindVertexArray(0);
				}

				/*	Draw Object.	*/
				{
					glUseProgram(this->phongblinn_program);

					/*	Bind texture.   */
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

					glDisable(GL_CULL_FACE);

					/*	Draw triangle.	*/
					glBindVertexArray(this->model.vao);
					glDrawElements(GL_TRIANGLES, this->model.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
					glBindVertexArray(0);
				}

				/*	Draw Shadow, projected onto the plane.	*/
				{
					/*	*/
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize +
										  this->uniformAlignSize,
									  this->uniformProjectShadowAlignSize);

					glUseProgram(this->shadow_program);

					glDisable(GL_CULL_FACE);

					/*	Offset */
					// glEnable(GL_POLYGON_OFFSET_FILL);
					// glPolygonOffset(1.0, 1.0);

					/*	Draw triangle.	*/
					glBindVertexArray(this->model.vao);
					glDrawElements(GL_TRIANGLES, this->model.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
					glBindVertexArray(0);
				}
			}
		}

		void update() override {

			/*	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model = glm::rotate(
				this->uniformStageBuffer.model, glm::radians(45.0f * elapsedTime), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(2.95f));
			this->uniformStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			this->uniformStageBuffer.modelViewProjection =
				(this->uniformStageBuffer.proj * this->camera.getViewMatrix() * this->uniformStageBuffer.model);
			this->uniformStageBuffer.viewPos = glm::vec4(this->camera.getPosition(), 0.0f);
			this->uniformStageBuffer.lightDirection =
				glm::vec4(-glm::normalize(this->projectShadowUniformBuffer.lightPosition), 0);

			/*	Compute project matrix.	*/
			const glm::mat4 projectedMatrix = this->computeProjectShadow();

			this->projectShadowUniformBuffer.shadowProjectMatrix = projectedMatrix;
			this->projectShadowUniformBuffer.model = uniformStageBuffer.model;
			this->projectShadowUniformBuffer.viewProjection =
				(this->uniformStageBuffer.proj * this->camera.getViewMatrix());

			/*	Update next uniform buffer region.	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				/*	*/
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				/*	*/
				memcpy(&uniformPointer[uniformAlignSize], &this->projectShadowUniformBuffer,
					   sizeof(this->projectShadowUniformBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}

		/*	Create projection matrix from light point and the plane surface orientation.	*/
		glm::mat4 computeProjectShadow() const noexcept {
			glm::mat4 projectedMatrix;
			const glm::vec3 planeNormal = glm::normalize(this->projectShadowUniformBuffer.planeNormal);
			const glm::vec3 &lightPosition = glm::vec4(this->projectShadowUniformBuffer.lightPosition, 0);
			const float d = 1;

			projectedMatrix[0][0] = glm::dot(planeNormal, lightPosition) - planeNormal.x * lightPosition.x;
			projectedMatrix[0][1] = -planeNormal.x * lightPosition.y;
			projectedMatrix[0][2] = -planeNormal.x * lightPosition.z;
			projectedMatrix[0][3] = -planeNormal.x;

			projectedMatrix[1][0] = -planeNormal.y * lightPosition.x;
			projectedMatrix[1][1] = glm::dot(planeNormal, lightPosition) - planeNormal.y * lightPosition.y;
			projectedMatrix[1][2] = -planeNormal.y * lightPosition.z;
			projectedMatrix[1][3] = -planeNormal.y;

			projectedMatrix[2][0] = -planeNormal.z * lightPosition.x;
			projectedMatrix[2][1] = -planeNormal.z * lightPosition.y;
			projectedMatrix[2][2] = glm::dot(planeNormal, lightPosition) - planeNormal.z * lightPosition.z;
			projectedMatrix[2][3] = -planeNormal.z;

			projectedMatrix[3][0] = -d * lightPosition.x;
			projectedMatrix[3][1] = -d * lightPosition.y;
			projectedMatrix[3][2] = -d * lightPosition.z;
			projectedMatrix[3][3] = glm::dot(planeNormal, lightPosition);

			return projectedMatrix;
		}
	};

	class ProjectedShadowGLSample : public GLSample<ProjectedShadow> {
	  public:
		ProjectedShadowGLSample() : GLSample<ProjectedShadow>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::ProjectedShadowGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
