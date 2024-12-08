#include "Core/Math3D.h"
#include "Core/math3D/Plane.h"
#include "UIComponent.h"
#include "imgui.h"
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>
#include <cstring>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <OsqpEigen/OsqpEigen.h>
#include <vector>

namespace glsample {
	/**
	 * @brief
	 *
	 */
	class SVM : public GLSampleWindow {
	  public:
		SVM() : GLSampleWindow() {
			this->setTitle("Supported Vector Machine");
			this->supportedVectorMachineSettingComponent =
				std::make_shared<InstanceSettingComponent>(this->uniformData);
			this->addUIComponent(this->supportedVectorMachineSettingComponent);

			this->camera.setPosition(glm::vec3(0));
			this->camera.lookAt(glm::vec3(1));
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	Light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f);
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 viewPos;

			float shininess = 16.0f;
		} uniformData;

		/*	*/
		MeshObject boundingBox;
		MeshObject sphere;
		MeshObject plan;

		std::vector<Vector4> points;

		/*  */ // TODO: pass as arguments.
		size_t rows = 16;
		size_t cols = 16;

		size_t instanceBatch = 0;
		const size_t nrInstances = (rows * cols);
		std::vector<glm::mat4> instance_model_matrices;

		/*	*/
		unsigned int diffuse_texture;
		/*	*/
		unsigned int instance_program;

		/*  SVM buffer model matrix.   */
		unsigned int instance_model_buffer;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;
		unsigned int uniform_mvp_buffer;
		unsigned int uniform_instance_buffer;
		const size_t nrUniformBuffers = 3;

		size_t uniformSharedBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;

		CameraController camera;

		class InstanceSettingComponent : public nekomimi::UIComponent {

		  public:
			InstanceSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("SVM Settings");
			}

			void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Specular", &this->uniform.specularColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				if (ImGui::Button("Refresh")) {
				}
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<InstanceSettingComponent> supportedVectorMachineSettingComponent;

		const std::string vertexInstanceShaderPath = "Shaders/instance/instance.vert.spv";
		const std::string fragmentInstanceShaderPath = "Shaders/instance/instance.frag.spv";

		void Release() {
			/*	*/
			glDeleteProgram(this->instance_program);

			/*	*/
			glDeleteTextures(1, (const uint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_mvp_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->sphere.vbo);
		}

		void Initialize() {

			{
				/*	Load shader source.	*/
				std::vector<uint32_t> instance_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexInstanceShaderPath, this->getFileSystem());
				std::vector<uint32_t> instance_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentInstanceShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->instance_program = ShaderLoader::loadGraphicProgram(compilerOptions, &instance_vertex_binary,
																		  &instance_fragment_binary);
			}
			/*	Setup SVM graphic pipeline.	*/
			glUseProgram(this->instance_program);
			glUniform1i(glGetUniformLocation(this->instance_program, "DiffuseTexture"), 0);
			/*	*/
			int uniform_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformBufferBlock");
			glUniformBlockBinding(this->instance_program, uniform_buffer_index, this->uniform_buffer_binding);
			/*	*/
			int uniform_instance_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformInstanceBlock");
			glUniformBlockBinding(this->instance_program, uniform_instance_buffer_index,
								  this->uniform_instance_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSharedBufferSize =
				fragcore::Math::align<size_t>(this->uniformSharedBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_mvp_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSharedBufferSize * this->nrUniformBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/**/
			/*	Load Light geometry.	*/
			{

				// TODO: add helper method to merge multiple.
				std::vector<ProceduralGeometry::Vertex> wireCubeVertices;
				std::vector<unsigned int> wireCubeIndices;
				ProceduralGeometry::generateCube(1, wireCubeVertices, wireCubeIndices);

				std::vector<ProceduralGeometry::Vertex> planVertices;
				std::vector<unsigned int> planIndices;
				ProceduralGeometry::generatePlan(1, planVertices, planIndices);

				std::vector<ProceduralGeometry::Vertex> CubeVertices;
				std::vector<unsigned int> CubeIndices;
				ProceduralGeometry::generateCube(1, CubeVertices, CubeIndices);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->sphere.vao);
				glBindVertexArray(this->sphere.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->sphere.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, sphere.vbo);
				glBufferData(GL_ARRAY_BUFFER,
							 (wireCubeVertices.size() + planVertices.size() + CubeVertices.size()) *
								 sizeof(ProceduralGeometry::Vertex),
							 nullptr, GL_STATIC_DRAW);
				/*	*/
				glBufferSubData(GL_ARRAY_BUFFER, 0, wireCubeVertices.size() * sizeof(ProceduralGeometry::Vertex),
								wireCubeVertices.data());
				/*	*/
				glBufferSubData(GL_ARRAY_BUFFER, wireCubeVertices.size() * sizeof(ProceduralGeometry::Vertex),
								planVertices.size() * sizeof(ProceduralGeometry::Vertex), planVertices.data());
				/*	*/
				glBufferSubData(GL_ARRAY_BUFFER,
								(wireCubeVertices.size() + planVertices.size()) * sizeof(ProceduralGeometry::Vertex),
								(CubeVertices.size()) * sizeof(ProceduralGeometry::Vertex), CubeVertices.data());

				/*	*/
				glGenBuffers(1, &this->sphere.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
							 (wireCubeIndices.size() + planIndices.size() + CubeIndices.size()) *
								 sizeof(wireCubeIndices[0]),
							 nullptr, GL_STATIC_DRAW);
				/*	*/
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, wireCubeIndices.size() * sizeof(wireCubeIndices[0]),
								wireCubeIndices.data());
				/*	*/
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, wireCubeIndices.size() * sizeof(wireCubeIndices[0]),
								planIndices.size() * sizeof(wireCubeIndices[0]), planIndices.data());
				/*	*/
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
								(planIndices.size() + wireCubeIndices.size()) * sizeof(wireCubeIndices[0]),
								CubeIndices.size() * sizeof(wireCubeIndices[0]), CubeIndices.data());

				this->sphere.nrIndicesElements = CubeIndices.size();
				this->boundingBox.nrIndicesElements = wireCubeIndices.size();
				this->plan.nrIndicesElements = planIndices.size();

				/*	Share VAO.	*/
				this->boundingBox.vao = sphere.vao;
				this->plan.vao = sphere.vao;

				this->boundingBox.indices_offset = wireCubeVertices.size();
				this->boundingBox.vertex_offset = wireCubeVertices.size();

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

			this->solve_svm();
		}

		void solve_svm() {

			/*	*/
			this->points.resize(1000);
			this->instance_model_matrices.resize(points.size());
			/*	Generate random data.	*/

			RandomUniform<float> uniformWorld(-10, 10);
			RandomUniform<float> uniformRandom(0, 1);
			for (size_t i = 0; i < points.size(); i++) {
				points[i] =
					Vector4(uniformWorld.rand(), uniformWorld.rand(), uniformWorld.rand(), uniformRandom.rand());
			}

			/*	Apply kernel trick.	*/
			for (size_t i = 0; i < points.size(); i++) {
				bool bin_class = points[i].w() > 0.5f;
			}

			/*	*/
			size_t n_constraints = this->points.size();
			size_t n_unknowns = 3;

			Eigen::VectorXf q;
			Eigen::SparseMatrix<float> H;
			Eigen::SparseMatrix<float> constraints_matrix(n_constraints, n_unknowns);

			OsqpEigen::Solver solver;
			Eigen::Matrix<c_float, Eigen::Dynamic, 1> lower_bound(1);
			Eigen::Matrix<c_float, Eigen::Dynamic, 1> upper_bound(1);

			solver.data()->setNumberOfVariables(n_unknowns);
			solver.data()->setNumberOfConstraints(n_constraints);
			solver.data()->setHessianMatrix(H);
			// solver.data()->setGradient(q);
			solver.data()->setLinearConstraintsMatrix(constraints_matrix);
			solver.data()->setLowerBound(lower_bound);
			solver.data()->setUpperBound(upper_bound);

			solver.settings()->setMaxIteration(2000);
			solver.settings()->setVerbosity(true);

			if (!solver.initSolver()) {
			}

			if (solver.solveProblem() == OsqpEigen::ErrorExitFlag::NoError) {
				Eigen::Matrix<c_float, Eigen::Dynamic, 1> solution = solver.getSolution();

				const auto &workspace = solver.workspace();
				const OSQPInfo *info = workspace->info;
				// workspace->solution->
				this->getLogger().debug("QP solver failed: {}", info->status);
				solution.cast<float>();

				/*	Update plane matrix.	*/
				auto seg = solution.segment(0, 3);
				Plane<float> plane(Vector3(seg.x(), seg.y(), seg.z()), solution.w());

				glm::vec3 position = glm::vec3(plane.getPoint().x(), plane.getPoint().y(), plane.getPoint().z());
				glm::vec3 normal = glm::vec3(plane.getNormal().x(), plane.getNormal().y(), plane.getNormal().z());

				glm::mat4 model = glm::translate(glm::mat4(1.0), position);
				glm::quat rotation = glm::quat() * normal;
				model = model * glm::toMat4(rotation);
				model = glm::scale(model, glm::vec3(1.95f));

			} else {
				const OSQPInfo *info = solver.workspace()->info;
				this->getLogger().debug("QP solver failed: {}", info->status);
			}

			/*	Update Instances.	*/
			for (size_t i = 0; i < points.size(); i++) {
				const Vector4 &pointRef = points[i];

				glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(pointRef.x(), pointRef.y(), pointRef.z()));
				model = glm::scale(model, glm::vec3(1.95f));

				instance_model_matrices[i] = model;
			}

			/*	Setup instance buffer.	*/
			{
				/*	*/
				GLint storageMaxSize;
				glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &storageMaxSize);
				GLint minStorageAlignSize;
				glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageAlignSize);
				this->instanceBatch = this->points.size();

				this->uniformInstanceSize =
					fragcore::Math::align<size_t>(this->instanceBatch * sizeof(glm::mat4), (size_t)minStorageAlignSize);

				glGenBuffers(1, &this->uniform_instance_buffer);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, this->uniformInstanceSize * this->nrUniformBuffers, nullptr,
							 GL_DYNAMIC_DRAW);
				void *instance_data = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
				memcpy(instance_data, &instance_model_matrices[0][0][0],
					   instance_model_matrices.size() * sizeof(instance_model_matrices[0]));
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}
		}

		void draw() {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Bind MVP Uniform Buffer.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_mvp_buffer,
							  (this->getFrameCount() % this->nrUniformBuffers) * this->uniformSharedBufferSize,
							  this->uniformSharedBufferSize);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK,
						  this->supportedVectorMachineSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			glUseProgram(this->instance_program);
			glDisable(GL_CULL_FACE);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	Draw Instances.	*/
			glBindVertexArray(this->sphere.vao);

			glBindBufferRange(
				GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer_binding, this->uniform_instance_buffer,
				(this->getFrameCount() % this->nrUniformBuffers) * this->instanceBatch * sizeof(glm::mat4),
				this->instanceBatch * sizeof(glm::mat4));

			glBindVertexArray(0);

			/*	Draw hyper plane.	*/
		}

		void update() {

			/*	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformData.proj = this->camera.getProjectionMatrix();
			this->uniformData.model = glm::mat4(1.0f);
			this->uniformData.view = this->camera.getViewMatrix();
			this->uniformData.modelViewProjection = this->uniformData.model * this->camera.getViewMatrix();
			this->uniformData.viewPos = glm::vec4(this->camera.getPosition(), 0);

			/*	Update uniform.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			void *uniformMVP =
				glMapBufferRange(GL_UNIFORM_BUFFER,
								 ((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformSharedBufferSize,
								 this->uniformSharedBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformMVP, &this->uniformData, sizeof(this->uniformData));
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			/*	Update SVM buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			void *uniformInstance = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformInstanceSize,
				this->uniformInstanceSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformInstance, this->instance_model_matrices.data(),
				   sizeof(this->instance_model_matrices[0]) * this->instance_model_matrices.size());

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class SVMGLSample : public GLSample<SVM> {
	  public:
		SVMGLSample() : GLSample<SVM>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("N,number-points", "Number of Points", cxxopts::value<int>()->default_value("10000"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SVMGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}