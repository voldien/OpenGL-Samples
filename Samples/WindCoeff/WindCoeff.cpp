#include "Common.h"
#include "GraphicFormat.h"
#include "SampleHelper.h"
#include "Util/Camera.h"
#include "imgui.h"
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <cmath>
#include <cstddef>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/trigonometric.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class WindCoeff : public GLSampleWindow {
	  public:
		WindCoeff() : GLSampleWindow() {
			this->setTitle("Wind Coefficient");

			this->windCoeffSettingComponent = std::make_shared<WindCoeffSettingComponent>(*this);
			this->addUIComponent(this->windCoeffSettingComponent);

			this->camera.setPosition(glm::vec3(10));
			this->camera.lookAt(glm::vec3(0));
		}

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 ViewProj{};
			glm::mat4 modelViewProjection{};

			CameraInstanceData camera;

			BoundingShapeData bounding{};
		} uniformData{};

		using ResultBuffer = struct uniform_buffer_result_t {
			float area;
			float averageNormal;
			glm::vec3 averagePosition;
		};

		/*	*/
		unsigned int result_buffer = 0;
		ResultBuffer resultStage{};
		size_t result_buffer_size = sizeof(ResultBuffer);

		FrameBuffer framebuffer;

		unsigned int DispatchX = 0;
		unsigned int DispatchY = 0;

		/*	*/
		MeshObject instanceGeometry;
		MeshObject boundingMesh;
		AABB boundingBox;

		unsigned int sample_query{};

		unsigned long int bounding_sample_count = 1;

		/*	*/
		unsigned int shaded_graphic_program{};

		unsigned int compute_program{};
		int localWorkGroupSize[3]{};

		/*  Instance buffer model matrix.   */
		unsigned int result_model_buffer{};

		/*	*/
		const unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_mvp_buffer{};
		const size_t nrUniformBuffers = 3;

		const unsigned int uniform_result_binding = 2;

		size_t uniformSharedBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;

		CameraController camera;

		class WindCoeffSettingComponent : public GLUIComponent<WindCoeff> {

		  public:
			WindCoeffSettingComponent(WindCoeff &sample) : GLUIComponent(sample, "Wind Coefficient Settings") {}

			void draw() override {

				const float inv_area_num_pixels =
					Math::max<float>(1.0f / this->getRefSample().bounding_sample_count, Math::Epsilon);

				const float areaRatio = this->getRefSample().resultStage.area * inv_area_num_pixels;
				const float normalRatio =
					this->getRefSample().resultStage.averageNormal / this->getRefSample().resultStage.area;

				ImGui::TextUnformatted("Orientation and Scale Settings");
				ImGui::DragFloat3("Volume Size (X,Y,Z)", &bounding_box_size[0]);

				ImGui::DragFloat3("Rotation", &offsetRotation[0]);
				ImGui::DragFloat3("Offset Position", &offsetPosition[0]);
				ImGui::DragFloat3("Scale Factor", &scaleFactor[0]);

				/*	*/
				const glm::vec3 volume = bounding_box_size;

				const float top_area = volume.x * volume.z;
				const float front_area = volume.x * volume.y;
				const float side_area = volume.z * volume.y; // TODO: improve

				const glm::vec3 area_vector(front_area, top_area, side_area);
				const glm::vec3 view_area = area_vector * glm::abs(this->getRefSample().camera.getLookDirection());
				const float total_area = Math::sum(&view_area[0], 3) * areaRatio;

				/*	*/
				const float dragCoeff = Math::lerpClamped(1.0f, normalRatio, drag_influence);
				const float dynamic_pressure = 0.5f * airPressure * (wind_speed_meter_second * wind_speed_meter_second);

				glm::vec3 force =
					dragCoeff * total_area * dynamic_pressure * this->getRefSample().camera.getLookDirection();

				ImGui::TextUnformatted("Wind Force Settings");
				ImGui::DragFloat("Wind Speed", &wind_speed_meter_second);
				ImGui::DragFloat("Pressure", &airPressure);
				ImGui::DragFloat("Drag Influence", &drag_influence);

				ImGui::Text("Surface Area : %f", total_area);
				ImGui::DragFloat3("Wind Force (Netwon)", &force[0]);
				ImGui::Text("Force Magnitude %f", glm::length(force));

				Vector3 size = this->getRefSample().boundingBox.getHalfSize();
				Vector3 center = this->getRefSample().boundingBox.getCenter();
				ImGui::DragFloat3("Model Max Bounding Box", size.data());
				ImGui::DragFloat3("Bounding Box Center", center.data());

				glm::vec3 average_position_offset =
					this->getRefSample().resultStage.averagePosition / this->getRefSample().resultStage.area;

				glm::vec3 torque = glm::cross(average_position_offset, force);

				ImGui::DragFloat3("Average Position", &average_position_offset[0]);
				ImGui::DragFloat3("Torque", &torque[0]);

				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Focus Camera", &this->focusCamera);
				if (ImGui::Button("Reset Camera Position")) {
					this->getRefSample().camera.setPosition(glm::vec3(0, 0, 1) * 10.0f);
				}

				ImGui::TextUnformatted("Debug Information");
				ImGui::Text("Area (Pixels): %f", this->getRefSample().resultStage.area);
				ImGui::Text("Normal (Pixels): %f", this->getRefSample().resultStage.averageNormal);

				ImGui::Text("Area Ratio: %f", areaRatio);
				ImGui::Text("Normal Ratio: %f", normalRatio);

				ImGui::Text("Samples: %lu", this->getRefSample().bounding_sample_count);

				ImGui::DragFloat3("Total Position", &this->getRefSample().resultStage.averagePosition[0]);
			}

			bool showWireFrame = false;
			bool focusCamera = false;

			float airPressure = 1.225f;

			float wind_speed_meter_second = 10.0f;
			float drag_influence = 1;
			glm::vec3 bounding_box_size{10, 10, 10};

			glm::vec3 scaleFactor = {1, 1, 1};
			glm::vec3 offsetPosition = {0, 0, 0};
			glm::vec3 offsetRotation = {0, 0, 0};
		};
		std::shared_ptr<WindCoeffSettingComponent> windCoeffSettingComponent;

		const std::string vertexNormalShaderPath = "Shaders/debug/normal.vert.spv";
		const std::string fragmentNormalShaderPath = "Shaders/debug/normal.frag.spv";
		const std::string computeShaderPath = "Shaders/area/windcoeff/windcoeff_screen_space.comp.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->shaded_graphic_program);

			glDeleteBuffers(1, &this->uniform_mvp_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->instanceGeometry.vao);
			glDeleteBuffers(1, &this->instanceGeometry.vbo);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	Load shader source.	*/
				std::vector<uint32_t> normal_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexNormalShaderPath, this->getFileSystem());
				std::vector<uint32_t> normal_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentNormalShaderPath, this->getFileSystem());

				const std::vector<uint32_t> area_normal_compute_binary =
					IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->shaded_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &normal_vertex_binary, &normal_fragment_binary);

				/*	Create compute pipeline.	*/
				this->compute_program = ShaderLoader::loadComputeProgram(compilerOptions, &area_normal_compute_binary);
			}
			/*	Setup instance graphic pipeline.	*/
			glUseProgram(this->shaded_graphic_program);

			/*	*/
			int uniform_buffer_index = glGetUniformBlockIndex(this->shaded_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->shaded_graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup compute pipeline.	*/
			glUseProgram(this->compute_program);
			glUniform1i(glGetUniformLocation(this->compute_program, "ColorTexture"), 0);
			glUniform1i(glGetUniformLocation(this->compute_program, "DepthTexture"), 1);

			int result_index =
				glGetProgramResourceIndex(this->compute_program, GL_SHADER_STORAGE_BLOCK, "ResultBuffer");
			glShaderStorageBlockBinding(this->compute_program, result_index, this->uniform_result_binding);

			glGetProgramiv(this->compute_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSharedBufferSize =
				fragcore::Math::align<size_t>(this->uniformSharedBufferSize, (size_t)minMapBufferSize);
			GLint minStorageAlignSize = 0;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageAlignSize);
			this->result_buffer_size = Math::align<size_t>(this->result_buffer_size * (static_cast<size_t>(4096 * 32)),
														   (size_t)minStorageAlignSize);

			/*	*/
			glGenBuffers(1, &this->uniform_mvp_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSharedBufferSize * this->nrUniformBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->result_buffer);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->result_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->result_buffer_size * this->nrUniformBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			/*	*/
			{
				ModelImporter modelLoader(FileSystem::getFileSystem());
				modelLoader.loadContent(modelPath, 0);

				const ModelSystemObject &modelRef = modelLoader.getModels()[0];

				/*	*/
				this->boundingBox = GeometryUtility::computeBoundingBox(
					fragcore::AABB::createMinMax(
						Vector3(modelRef.bound.aabb.min[0], modelRef.bound.aabb.min[1], modelRef.bound.aabb.min[2]),
						Vector3(modelRef.bound.aabb.max[0], modelRef.bound.aabb.max[1], modelRef.bound.aabb.max[2])),
					GLM2E<float, 4, 4>(glm::mat4(1)));
				/*	*/
				Vector3 halfSize = this->boundingBox.getHalfSize();
				halfSize.x() *= sqrt(2.0f);
				halfSize.y() *= sqrt(2.0f);
				halfSize.z() *= sqrt(2.0f);
				this->boundingBox.setHalfSize(halfSize);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->instanceGeometry.vao);
				glBindVertexArray(this->instanceGeometry.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->instanceGeometry.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, instanceGeometry.vbo);
				glBufferData(GL_ARRAY_BUFFER, modelRef.nrVertices * modelRef.vertexStride, modelRef.vertexData,
							 GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->instanceGeometry.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instanceGeometry.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelRef.nrIndices * modelRef.indicesStride, modelRef.indicesData,
							 GL_STATIC_DRAW);
				this->instanceGeometry.nrIndicesElements = modelRef.nrIndices;

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

			glGenQueries(1, &sample_query);
			CommonUtil::loadCube(boundingMesh, 1);

			/*	Create multipass framebuffer.	*/
			CommonUtil::createFrameBuffer(&this->framebuffer, 1);
			onResize(width(), height());
		}

		void onResize(int width, int height) override {

			const size_t frame_width = 512;
			const size_t frame_height = 512;

			CommonUtil::updateFrameBuffer(&this->framebuffer,
										  {{
											  .width = frame_width,
											  .height = frame_height,
											  .graphicFormat = GraphicFormat::R16G16B16A16_SNorm,
											  .nrSamples = 0,
										  }},
										  {
											  .width = frame_width,
											  .height = frame_height,
											  .graphicFormat = GraphicFormat::Depth_32Bit,
											  .nrSamples = 0,
										  });

			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			/*	Extract Result.	*/
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->result_buffer);
			ResultBuffer *resultBuffer = (ResultBuffer *)glMapBufferRange(
				GL_SHADER_STORAGE_BUFFER,
				((this->getFrameCount() + 0) % this->nrUniformBuffers) * this->result_buffer_size,
				this->result_buffer_size, GL_MAP_READ_BIT);

			this->resultStage.area = 0;
			this->resultStage.averageNormal = 0;
			this->resultStage.averagePosition = glm::vec3(0);

			for (size_t i = 0; i < static_cast<size_t>(DispatchX * DispatchY); i++) {
				this->resultStage.area += resultBuffer[i].area;
				this->resultStage.averageNormal += resultBuffer[i].averageNormal;
				this->resultStage.averagePosition += resultBuffer[i].averagePosition;
			}

			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			{
				/*	*/
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->framebuffer.framebuffer);
				glViewport(0, 0, this->framebuffer.attachmentSize[0].x, this->framebuffer.attachmentSize[0].y);

				/*	*/
				glClearColor(0.00f, 0.00f, 0.00f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				/*	Bind MVP Uniform Buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_mvp_buffer,
								  (this->getFrameCount() % this->nrUniformBuffers) * this->uniformSharedBufferSize,
								  this->uniformSharedBufferSize);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->windCoeffSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				glUseProgram(this->shaded_graphic_program);
				glDisable(GL_CULL_FACE);

				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				/*	Draw Instances.	*/
				glBindVertexArray(this->instanceGeometry.vao);

				glDrawElementsInstanced(GL_TRIANGLES, this->instanceGeometry.nrIndicesElements, GL_UNSIGNED_INT,
										nullptr, 1);

				glBindVertexArray(0);
			}

			/*	*/
			{
				glBeginQuery(GL_SAMPLES_PASSED, this->sample_query);

				glBindVertexArray(this->boundingMesh.vao);

				glColorMask(0, 0, 0, 0);
				glDepthMask(GL_FALSE);

				glDrawElementsInstanced(GL_TRIANGLES, this->boundingMesh.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										1);

				glColorMask(1, 1, 1, 1);
				glDepthMask(GL_TRUE);

				glBindVertexArray(0);

				glEndQuery(GL_SAMPLES_PASSED);
				glGetQueryObjectui64v(sample_query, GL_QUERY_RESULT, &bounding_sample_count);
			}

			glTextureBarrier();

			{
				int width = 0, height = 0;
				this->getSize(&width, &height);

				/*	Blit image targets to screen.	*/
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->framebuffer.framebuffer);

				glViewport(0, 0, width, height);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glBlitFramebuffer(0, 0, this->framebuffer.attachmentSize[0].x, this->framebuffer.attachmentSize[0].y, 0,
								  0, width, height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			{

				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, this->framebuffer.attachments[0]);

				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->framebuffer.attachments[this->framebuffer.depthIndex]);

				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_result_binding, this->result_buffer,
								  ((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->result_buffer_size,
								  this->result_buffer_size);

				glUseProgram(compute_program);

				DispatchX = std::ceil(this->framebuffer.attachmentSize[0].x / (float)this->localWorkGroupSize[0]);
				DispatchY = std::ceil(this->framebuffer.attachmentSize[0].y / (float)this->localWorkGroupSize[1]);

				glDispatchCompute(DispatchX, DispatchY, 1);
				glUseProgram(0);

				/*	Wait in till all data has been written.	*/
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}
		}

		void update() override {

			/*	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			this->uniformData.camera = this->camera;

			this->uniformData.bounding.bound.aabb.min[0] = this->boundingBox.getCenter().x();
			this->uniformData.bounding.bound.aabb.min[1] = this->boundingBox.getCenter().y();
			this->uniformData.bounding.bound.aabb.min[2] = this->boundingBox.getCenter().z();
			// this->uniformData.bounding.bound.aabb.max = E2GLM(this->boundingBox.getHalfSize());

			/*	*/
			this->uniformData.model = glm::translate(glm::mat4(1.0f), -E2GLM(this->boundingBox.getCenter()));
			/*	*/
			this->uniformData.model =
				glm::translate(this->uniformData.model, this->windCoeffSettingComponent->offsetPosition);

			/*	*/
			glm::quat quatRotation = glm::quat(this->windCoeffSettingComponent->offsetRotation);
			this->uniformData.model = this->uniformData.model * glm::toMat4(quatRotation);
			this->uniformData.model = glm::scale(this->uniformData.model, this->windCoeffSettingComponent->scaleFactor);

			/*	*/
			this->uniformData.proj = this->camera.getProjectionMatrix();
			this->uniformData.view = this->camera.getViewMatrix();

			if (windCoeffSettingComponent->focusCamera) {
				this->camera.setMode(Camera::CameraProjectionMode::Orthographic);

				this->camera.setOrth(-boundingBox.getHalfSize().x(), boundingBox.getHalfSize().x(),
									 -boundingBox.getHalfSize().y(), boundingBox.getHalfSize().y(),
									 -boundingBox.getHalfSize().z() * 2, boundingBox.getHalfSize().z() * 2);

				/*	*/
				this->camera.setPosition(
					E2GLM(boundingBox.getCenter()) +
					glm::normalize(glm::vec3(this->camera.getPosition().x, 0, this->camera.getPosition().z)));
				this->camera.lookAt(E2GLM(boundingBox.getCenter()));

				this->uniformData.view = this->camera.getRotationMatrix();

				this->uniformData.modelViewProjection =
					this->uniformData.proj * this->uniformData.view * this->uniformData.model;
			} else {
				this->camera.setMode(Camera::CameraProjectionMode::Perspective);
				this->camera.setNear(0.15f);
				this->camera.setFar(1000.0f);

				this->uniformData.modelViewProjection =
					this->uniformData.proj * this->uniformData.view * this->uniformData.model;
			}

			{
				/*	Update uniform.	*/
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
				void *uniformMVP = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformSharedBufferSize,
					this->uniformSharedBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformMVP, &this->uniformData, sizeof(this->uniformData));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class WindCoeffGLSample : public GLSample<WindCoeff> {
	  public:
		WindCoeffGLSample() : GLSample<WindCoeff>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::WindCoeffGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
