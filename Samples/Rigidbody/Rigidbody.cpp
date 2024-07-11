#include "Core/Math3D.h"
#include "GLSampleSession.h"
#include "PhysicDesc.h"
#include "Scene.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <PhysicInterface.h>
#include <ShaderLoader.h>
#include <array>
#include <bulletPhysicInterface.h>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <queue>
#include <vector>

namespace glsample {

	/**
	 * @brief RigidBody
	 *
	 */
	class RigidBody : public GLSampleWindow {
	  public:
		RigidBody() : GLSampleWindow() {
			this->setTitle("RigidBody");

			/*	Setting Window.	*/
			this->rigidBodySettingComponent = std::make_shared<RigidBodySettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->rigidBodySettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		typedef struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			glm::vec4 ambientLight0 = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 ambientLight1 = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 ambientLight2 = glm::vec4(0.4, 0.4, 0.4, 1.0f);

		} UniformBufferBlock;

		UniformBufferBlock uniformStageBuffer;

		std::vector<fragcore::RigidBody *> rigidbodies;
		fragcore::RigidBody *planeRigibody;

		const std::array<int, 3> grid_aray = {8, 2, 8};

		/*	*/
		MeshObject boundingBox;
		MeshObject box;
		MeshObject plan;

		/*	White texture for each object.	*/
		unsigned int white_texture;

		/*	*/
		unsigned int graphic_program;
		unsigned int bounding_program;
		unsigned int wireframe_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;

		unsigned int uniform_buffer;
		unsigned int uniform_instance_buffer;

		const size_t nrUniformBuffers = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);
		size_t uniformLightBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;

		/*	*/
		CameraController camera;

		size_t instanceBatch = 0;

		PhysicInterface *interface;

		/*	RigidBody Rendering Path.	*/
		const std::string vertexInstanceShaderPath = "Shaders/instance/instance_ssbo.vert.spv";
		const std::string fragmentInstanceShaderPath = "Shaders/instance/instance.frag.spv";

		const std::string vertexBoundingShaderPath = "Shaders/bounding/boundingbox.vert.spv";
		const std::string fragmentBoundingShaderPath = "Shaders/bounding/boundingbox.frag.spv";

		const std::string vertexBlendShaderPath = "Shaders/blending/blending.vert.spv";
		const std::string fragmentBlendShaderPath = "Shaders/blending/blending.frag.spv";

		class RigidBodySettingComponent : public nekomimi::UIComponent {
		  public:
			RigidBodySettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("RigidBody Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;
			bool RigidBody = true;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<RigidBodySettingComponent> rigidBodySettingComponent;

		void Release() override {

			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->bounding_program);
			glDeleteProgram(this->wireframe_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteBuffers(1, &this->uniform_instance_buffer);
		}

		void Initialize() override {

			/*	Preallocate.	*/
			this->rigidbodies.resize(fragcore::Math::product<int>(grid_aray.data(), grid_aray.size()));

			{

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(vertexInstanceShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(fragmentInstanceShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_bound_binary =
					IOUtil::readFileData<uint32_t>(this->vertexBoundingShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_bound_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentBoundingShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_blending_binary =
					IOUtil::readFileData<uint32_t>(this->vertexBlendShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_blending_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentBlendShaderPath, this->getFileSystem());

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

				/*	Load shader	*/
				this->wireframe_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_bound_binary, &fragment_bound_binary);

				/*	Load shader	*/
				this->bounding_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_blending_binary,
																		  &fragment_blending_binary);
			}

			/*	Setup graphic program.	*/
			glUseProgram(this->graphic_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, uniform_buffer_binding);

			int instance_read_index =
				glGetProgramResourceIndex(this->graphic_program, GL_SHADER_STORAGE_BLOCK, "InstanceBlock");
			glShaderStorageBlockBinding(this->graphic_program, instance_read_index,
										this->uniform_instance_buffer_binding);
			glUniform1i(glGetUniformLocation(this->graphic_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Setup graphic program.	*/
			glUseProgram(this->wireframe_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->wireframe_program, "UniformBufferBlock");
			glUniformBlockBinding(this->wireframe_program, uniform_buffer_index, uniform_buffer_binding);
			glUniform1i(glGetUniformLocation(this->wireframe_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Setup graphic program.	*/
			glUseProgram(this->bounding_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->bounding_program, "UniformBufferBlock");
			int uniform_instance_buffer_index =
				glGetProgramResourceIndex(this->bounding_program, GL_SHADER_STORAGE_BLOCK, "InstanceBlock");
			glUniformBlockBinding(this->bounding_program, uniform_buffer_index, uniform_buffer_binding);

			glShaderStorageBlockBinding(this->bounding_program, instance_read_index,
										this->uniform_instance_buffer_binding);
			glUseProgram(0);

			/*	Create white texture.	*/
			glGenTextures(1, &this->white_texture);
			glBindTexture(GL_TEXTURE_2D, this->white_texture);
			const unsigned char white[] = {255, 255, 255, 255};
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			/*	No Mipmap.	*/
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			glBindTexture(GL_TEXTURE_2D, 0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffers, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Setup instance buffer.	*/
			{
				/*	*/
				GLint storageMaxSize;
				glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &storageMaxSize);
				GLint minStorageAlignSize;
				glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageAlignSize);
				this->instanceBatch = this->rigidbodies.size(); // storageMaxSize / sizeof(glm::mat4);

				this->uniformInstanceSize =
					fragcore::Math::align(this->instanceBatch * sizeof(glm::mat4), (size_t)minStorageAlignSize);

				glGenBuffers(1, &this->uniform_instance_buffer);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, this->uniformInstanceSize * this->nrUniformBuffers, nullptr,
							 GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}

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
				glGenVertexArrays(1, &this->box.vao);
				glBindVertexArray(this->box.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->box.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, box.vbo);
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
				glGenBuffers(1, &this->box.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, box.ibo);
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

				this->box.nrIndicesElements = CubeIndices.size();
				this->boundingBox.nrIndicesElements = wireCubeIndices.size();
				this->plan.nrIndicesElements = planIndices.size();

				/*	Share VAO.	*/
				this->boundingBox.vao = box.vao;
				this->plan.vao = box.vao;

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

			/*	TODO:Multiple thread*/
			{
				this->interface = new BulletPhysicInterface(nullptr);

				CollisionDesc planDesc;
				planDesc.Primitive = CollisionDesc::ShapePrimitive::Plane;
				planDesc.planeshape.d = -10;
				planDesc.planeshape.normal[0] = 0;
				planDesc.planeshape.normal[1] = 1;
				planDesc.planeshape.normal[2] = 0;
				Collision *planeCollider = this->interface->createCollision(&planDesc);

				/*	Plane Rigidbody.	*/
				RigidBodyDesc planRigiDesc;
				planRigiDesc.useGravity = false;
				planRigiDesc.isKinematic = true;
				planRigiDesc.collision = planeCollider;
				planRigiDesc.position = Vector3(0, 0, 0);

				this->planeRigibody = this->interface->createRigibody(&planRigiDesc);
				this->interface->addRigidBody(this->planeRigibody);

				/*	*/
				CollisionDesc collisionDesc;
				collisionDesc.Primitive = CollisionDesc::ShapePrimitive::Box;
				collisionDesc.boxshape.boxsize[0] = 1;
				collisionDesc.boxshape.boxsize[1] = 1;
				collisionDesc.boxshape.boxsize[2] = 1;

				Collision *boxCollider = this->interface->createCollision(&collisionDesc);

				for (size_t i = 0; i < this->rigidbodies.size(); i++) {
					RigidBodyDesc desc;
					desc.collision = boxCollider;
					desc.mass = 1;
					desc.useGravity = true;
					desc.drag = 0;
					desc.position = Vector3(i, i, i);
					desc.isKinematic = false;
					fragcore::RigidBody *rigidbody = this->interface->createRigibody(&desc);
					this->rigidbodies[i] = rigidbody;
				}

				for (size_t i = 0; i < this->rigidbodies.size(); i++) {
					this->interface->addRigidBody(this->rigidbodies[i]);
				}
			}
		}

		void onResize(int width, int height) override {

			/*	Update camera	*/
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			/*	*/
			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer_binding,
							  this->uniform_instance_buffer,
							  (this->getFrameCount() % this->nrUniformBuffers) * this->uniformInstanceSize,
							  this->uniformInstanceSize);

			/*	Draw from main camera */
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);
				glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(this->graphic_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->white_texture);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(box.vao);
				glDrawElementsInstanced(GL_TRIANGLES, this->box.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										this->rigidbodies.size());
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		std::queue<const NodeObject *> mainCameraNodeQueue;

		void update() override {

			this->interface->sync();

			const float elapsedTime = this->getTimer().getElapsed<float>();

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());
			/*	*/
			{
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

				/*	Update uniform buffer.	*/
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformMappedMemory = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformBufferSize,
					this->uniformBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformMappedMemory, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}

			/*	Update rigidbody model matrix.	*/
			{
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer);
				uint8_t *uniform_instance_buffer_pointer = (uint8_t *)glMapBufferRange(
					GL_SHADER_STORAGE_BUFFER,
					((this->getFrameCount()) % this->nrUniformBuffers) * this->uniformInstanceSize,
					this->uniformInstanceSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

				for (size_t i = 0; i < rigidbodies.size(); i++) {
					glm::mat4 model = glm::mat4(1);
					model = glm::translate(model, glm::vec3(rigidbodies[i]->getPosition().x(),
															rigidbodies[i]->getPosition().y(),
															rigidbodies[i]->getPosition().z()));
					glm::quat roat(rigidbodies[i]->getOrientation().w(), rigidbodies[i]->getOrientation().x(),
								   rigidbodies[i]->getOrientation().y(), rigidbodies[i]->getOrientation().z());
					model = model * glm::toMat4(roat);
					memcpy(&uniform_instance_buffer_pointer[i * sizeof(glm::mat4)], &model[0][0], sizeof(model));
				}
				/*	*/

				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			}

			/*	Simulate.	*/
			this->interface->simulate(this->getTimer().deltaTime<float>(), 1);
		}
	};

	class RigidBodyGLSample : public GLSample<RigidBody> {
	  public:
		RigidBodyGLSample() : GLSample<RigidBody>() {}
		void customOptions(cxxopts::OptionAdder &options) override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::RigidBodyGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}