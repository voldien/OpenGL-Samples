#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <array>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ProjectedShadow : public GLSampleWindow {
	  public:
		ProjectedShadow() : GLSampleWindow() {
			this->setTitle("Projected Shadow");

			this->shadowProjectedSettingComponent = std::make_shared<ShadowProjectionSettingComponent>(
				this->uniformStageBuffer[0], this->projectShadowUniformBuffer);
			this->addUIComponent(this->shadowProjectedSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(15.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		typedef struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	Light source.	*/
			glm::vec4 direction = glm::vec4(0.7, 0.7, 1, 1);
			glm::vec4 lightColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 ambientColor = glm::vec4(0.3, 0.3, 0.3, 1);
			glm::vec4 viewDir;

			float shininess = 8;

		} UniformBufferBlock;

		std::array<UniformBufferBlock, 2> uniformStageBuffer;

		struct UniformProjectShadow {
			glm::mat4 model;
			glm::mat4 viewProjection;
			alignas(16) glm::mat4 shadowProjectMatrix;
			glm::vec4 color = glm::vec4(0.1, 0.1, 0.1, 0.85);

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
		size_t uniformPhongAlignSize;
		size_t uniformProjectShadowAlignSize;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		/*	*/
		const std::string vertexShaderPath = "Shaders/phongblinn/phongblinn_directional_light.vert.spv";
		const std::string fragmentShaderPath = "Shaders/phongblinn/phong_directional_light.frag.spv";

		/*	*/
		const std::string vertexShadowShaderPath = "Shaders/projectedshadow/projectedshadow.vert.spv";
		const std::string fragmentShadowShaderPath = "Shaders/projectedshadow/projectedshadow.frag.spv";

		class ShadowProjectionSettingComponent : public nekomimi::UIComponent {
		  public:
			ShadowProjectionSettingComponent(struct uniform_buffer_block &uniform, struct UniformProjectShadow &shadow)
				: uniform(uniform), shadow(shadow) {
				this->setName("Shadow Projection Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Projected Shadow Setting");

				ImGui::DragFloat3("Normal", &this->shadow.planeNormal[0]);
				if (ImGui::DragFloat3("Position", &this->shadow.lightPosition[0])) {
					uniform.direction = glm::vec4(glm::normalize(this->shadow.lightPosition), 0);
				}

				ImGui::TextUnformatted("Material Settings");
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Shinnes", &this->uniform.shininess);

				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
			struct UniformProjectShadow &shadow;
		};
		std::shared_ptr<ShadowProjectionSettingComponent> shadowProjectedSettingComponent;

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
			/*	*/
			this->uniformPhongAlignSize =
				fragcore::Math::align<size_t>(sizeof(this->uniformStageBuffer[0]), (size_t)minMapBufferSize);
			this->uniformProjectShadowAlignSize =
				fragcore::Math::align<size_t>(sizeof(this->projectShadowUniformBuffer), (size_t)minMapBufferSize);
			/*	*/
			this->uniformAlignBufferSize =
				(this->uniformPhongAlignSize * this->uniformStageBuffer.size()) + this->uniformProjectShadowAlignSize;

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
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

				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(20));

				glBindVertexArray(0);
			}
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->shadowProjectedSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			{

				/*	Draw plane.	*/
				{

					/*	*/
					const size_t plane_buffer_offset =
						(this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize +
						this->uniformPhongAlignSize * 1;
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  plane_buffer_offset, this->uniformPhongAlignSize);

					glUseProgram(this->phongblinn_program);

					/*	Bind texture.   */
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

					glDisable(GL_CULL_FACE);
					glDisable(GL_BLEND);

					/*	Draw triangle.	*/
					glBindVertexArray(this->plan.vao);
					glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
					glBindVertexArray(0);
				}

				/*	Draw Main Object.	*/
				{

					/*	*/
					const size_t main_object_buffer_offset =
						(this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize +
						this->uniformPhongAlignSize * 0;
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  main_object_buffer_offset, this->uniformPhongAlignSize);
					glUseProgram(this->phongblinn_program);

					/*	Bind texture.   */
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

					glDisable(GL_CULL_FACE);
					glDisable(GL_BLEND);

					/*	Draw triangle.	*/
					glBindVertexArray(this->model.vao);
					glDrawElements(GL_TRIANGLES, this->model.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
					glBindVertexArray(0);
				}

				/*	Draw Shadow, projected onto the plane.	*/
				{
					/*	*/
					const size_t shadow_buffer_offset =
						(this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize +
						this->uniformPhongAlignSize * this->uniformStageBuffer.size();
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  shadow_buffer_offset, this->uniformProjectShadowAlignSize);

					glUseProgram(this->shadow_program);

					glDisable(GL_CULL_FACE);

					/*	Offset */
					// glEnable(GL_POLYGON_OFFSET_FILL);
					// glPolygonOffset(1.0, 1.0);

					/*	Blending.	*/
					glDisable(GL_BLEND);
					glBlendEquation(GL_FUNC_ADD);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

			/*	Main Object uniform.	*/
			{
				this->uniformStageBuffer[0].model = glm::mat4(1.0f);
				this->uniformStageBuffer[0].model = glm::rotate(
					this->uniformStageBuffer[0].model, glm::radians(45.0f * elapsedTime), glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformStageBuffer[0].model = glm::scale(this->uniformStageBuffer[0].model, glm::vec3(2.95f));
				this->uniformStageBuffer[0].proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer[0].modelViewProjection =
					(this->uniformStageBuffer[0].proj * this->camera.getViewMatrix() *
					 this->uniformStageBuffer[0].model);
				this->uniformStageBuffer[0].viewDir = glm::vec4(this->camera.getLookDirection(), 0.0f);
				this->uniformStageBuffer[0].direction =
					glm::vec4(-glm::normalize(this->projectShadowUniformBuffer.lightPosition), 0);
			}

			/*	Plane uniform.	*/
			{
				this->uniformStageBuffer[1] = this->uniformStageBuffer[0];
				this->uniformStageBuffer[1].model = glm::mat4(1.0f);
				this->uniformStageBuffer[1].model =
					glm::translate(this->uniformStageBuffer[1].model, glm::vec3(0, -2, 0));
				this->uniformStageBuffer[1].model =
					glm::rotate(this->uniformStageBuffer[1].model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				this->uniformStageBuffer[1].model = glm::scale(this->uniformStageBuffer[1].model, glm::vec3(30.95f));

				this->uniformStageBuffer[1].modelViewProjection =
					(this->uniformStageBuffer[1].proj * this->camera.getViewMatrix() *
					 this->uniformStageBuffer[1].model);
			}

			/*	Compute project matrix.	*/
			const glm::mat4 shadowProjectedMatrix = this->computeProjectShadow();

			this->projectShadowUniformBuffer.shadowProjectMatrix = shadowProjectedMatrix;
			this->projectShadowUniformBuffer.model = uniformStageBuffer[0].model;
			this->projectShadowUniformBuffer.viewProjection =
				(this->uniformStageBuffer[0].proj * this->camera.getViewMatrix());

			/*	Update next uniform buffer region.	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
					this->uniformAlignBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

				/*	*/
				for (size_t i = 0; i < this->uniformStageBuffer.size(); i++) {
					memcpy(&uniformPointer[i * this->uniformPhongAlignSize], &this->uniformStageBuffer[i],
						   sizeof(this->uniformStageBuffer[0]));
				}

				/*	*/
				memcpy(&uniformPointer[this->uniformPhongAlignSize * this->uniformStageBuffer.size()],
					   &this->projectShadowUniformBuffer, sizeof(this->projectShadowUniformBuffer));

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
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"));
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
