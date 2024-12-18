#include "Common.h"
#include "GLSampleSession.h"
#include "Math3D/Math3D.h"
#include "PhysicDesc.h"
#include "SampleHelper.h"
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
			this->camera.setPosition(glm::vec3(100.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		using UniformBufferBlock = struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

			glm::vec4 ambientColor = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 specularColor = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 viewPos = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			float shininess = 8;
		};

		UniformBufferBlock uniformStageBuffer;

		std::vector<fragcore::RigidBody *> rigidbodies_box;
		std::vector<fragcore::RigidBody *> rigidbodies_sphere;
		fragcore::RigidBody *planeRigibody;
		fragcore::RigidBody *sphere_camera_collider_rig;

		const std::array<size_t, 3> grid_aray = {8, 16, 8};

		/*	*/
		MeshObject hyerplane;
		MeshObject boxMesh;
		MeshObject sphereMesh;

		/*	White texture for each object.	*/
		unsigned int white_texture;
		/*	*/
		unsigned int graphic_program;
		unsigned int hyperplane_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;

		unsigned int uniform_buffer;
		unsigned int ssbo_instance_buffer;

		const size_t nrUniformBuffers = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t uniformLightBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;

		/*	*/
		CameraController camera;

		size_t instanceBatch = 0;

		PhysicInterface *physic_interface;

		/*	RigidBody Rendering Path.	*/
		const std::string vertexInstanceShaderPath = "Shaders/instance/instance_ssbo.vert.spv";
		const std::string fragmentInstanceShaderPath = "Shaders/instance/instance.frag.spv";

		/*	*/
		const std::string vertexHyperplaneShaderPath = "Shaders/svm/hyperplane_simple.vert.spv";
		const std::string geometryHyperplaneShaderPath = "Shaders/svm/hyperplane_simple.geom.spv";
		const std::string fragmentHyperplaneShaderPath = "Shaders/svm/hyperplane_simple.frag.spv";

		class RigidBodySettingComponent : public nekomimi::UIComponent {
		  public:
			RigidBodySettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("RigidBody Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Material Settings");
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Specular Color", &this->uniform.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Shininess", &this->uniform.shininess);

				ImGui::TextUnformatted("Physic Settings");
				ImGui::DragFloat("Speed", &this->speed);
				ImGui::DragFloat("Fixed Step", &this->fixedTimeStep);
				ImGui::DragInt("Max SubStep", &this->maxSubStep);
				ImGui::Checkbox("Use Gravity", &this->useGravity);
				ImGui::Checkbox("Simulate", &this->usePhysic);

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;
			bool RigidBody = true;
			bool useGravity = true;
			bool usePhysic = true;
			float speed = 1.0f;
			float fixedTimeStep = 1.0f / 60.0f;
			int maxSubStep = 1;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<RigidBodySettingComponent> rigidBodySettingComponent;

		void Release() override {

			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->hyperplane_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteBuffers(1, &this->ssbo_instance_buffer);
		}

		void Initialize() override {

			/*	Preallocate.	*/
			this->rigidbodies_box.resize(fragcore::Math::product<size_t>(grid_aray.data(), grid_aray.size()));
			this->rigidbodies_sphere.resize(fragcore::Math::product<size_t>(grid_aray.data(), grid_aray.size()));

			{

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				const std::vector<uint32_t> vertex_instance_binary =
					IOUtil::readFileData<uint32_t>(vertexInstanceShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_instance_binary =
					IOUtil::readFileData<uint32_t>(fragmentInstanceShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_blending_binary =
					IOUtil::readFileData<uint32_t>(this->vertexHyperplaneShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_blending_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentHyperplaneShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geometry_blending_binary =
					IOUtil::readFileData<uint32_t>(this->geometryHyperplaneShaderPath, this->getFileSystem());

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_instance_binary,
																		 &fragment_instance_binary);

				/*	Load shader	*/
				this->hyperplane_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_blending_binary, &fragment_blending_binary, &geometry_blending_binary);
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
			glUseProgram(this->hyperplane_program);
			glUseProgram(0);

			glGenVertexArrays(1, &this->hyerplane.vao);
			glBindVertexArray(this->hyerplane.vao);
			glBindVertexArray(0);

			/*	Create white texture.	*/
			this->white_texture = Common::createColorTexture(1, 1, Color::white());

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Setup instance buffer.	*/
			{
				/*	*/
				GLint storageMaxSize = 0;
				glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &storageMaxSize);
				GLint minStorageAlignSize = 0;
				glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageAlignSize);
				this->instanceBatch = this->rigidbodies_box.size() +
									  this->rigidbodies_sphere.size(); // storageMaxSize / sizeof(glm::mat4);

				this->uniformInstanceSize =
					fragcore::Math::align<size_t>(this->instanceBatch * sizeof(glm::mat4), (size_t)minStorageAlignSize);

				glGenBuffers(1, &this->ssbo_instance_buffer);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_instance_buffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, this->uniformInstanceSize * this->nrUniformBuffers, nullptr,
							 GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}

			/*	*/
			Common::loadCube(this->boxMesh, 1);
			Common::loadSphere(this->sphereMesh, 1);

			{
				this->physic_interface = new BulletPhysicInterface(nullptr);

				/*	Camera collision object.	*/
				CollisionDesc sphereDesc;
				sphereDesc.Primitive = CollisionDesc::ShapePrimitive::Sphere;
				sphereDesc.sphereshape.radius = 1.0f;
				Collision *sphereCollider = this->physic_interface->createCollision(&sphereDesc);

				/*	Sphere Camera Rigidbody.	*/
				RigidBodyDesc sphereRigiDesc;
				sphereRigiDesc.useGravity = false;
				sphereRigiDesc.isKinematic = true;
				sphereRigiDesc.collision = sphereCollider;
				sphereRigiDesc.position = Vector3(0, 0, 0);
				sphereRigiDesc.mass = 1000;

				this->sphere_camera_collider_rig = this->physic_interface->createRigibody(&sphereRigiDesc);
				this->physic_interface->addRigidBody(this->sphere_camera_collider_rig);

				CollisionDesc planDesc;
				planDesc.Primitive = CollisionDesc::ShapePrimitive::Box;

				planDesc.boxshape.boxsize[0] = 1000;
				planDesc.boxshape.boxsize[1] = 10;
				planDesc.boxshape.boxsize[2] = 1000;
				planDesc.center[0] = 0;
				planDesc.center[1] = 0;
				planDesc.center[2] = 0;
				Collision *planeCollider = this->physic_interface->createCollision(&planDesc);

				/*	Plane Rigidbody.	*/
				RigidBodyDesc planRigiDesc;
				planRigiDesc.useGravity = false;
				planRigiDesc.isKinematic = true;
				planRigiDesc.collision = planeCollider;
				planRigiDesc.position = Vector3(0, -20, 0);
				planRigiDesc.mass = 1000;

				this->planeRigibody = this->physic_interface->createRigibody(&planRigiDesc);
				this->physic_interface->addRigidBody(this->planeRigibody);

				/*	*/
				CollisionDesc collisionDesc;
				collisionDesc.Primitive = CollisionDesc::ShapePrimitive::Box;
				collisionDesc.boxshape.boxsize[0] = 1.0f;
				collisionDesc.boxshape.boxsize[1] = 1.0f;
				collisionDesc.boxshape.boxsize[2] = 1.0f;

				Collision *boxCollider = this->physic_interface->createCollision(&collisionDesc);

				size_t rig_index = 0;
				const float offset = 2.085f;

				/*	Boxes.	*/
				for (size_t x = 0; x < this->grid_aray[0]; x++) {
					for (size_t y = 0; y < this->grid_aray[1]; y++) {
						for (size_t z = 0; z < this->grid_aray[2]; z++) {
							RigidBodyDesc desc;
							desc.collision = boxCollider;
							desc.mass = 10;
							desc.useGravity = true;
							desc.drag = 0.005f;
							desc.position = Vector3(x * offset, 10 + y * offset, z * offset);
							desc.isKinematic = false;
							fragcore::RigidBody *rigidbody = this->physic_interface->createRigibody(&desc);
							this->rigidbodies_box[rig_index++] = rigidbody;
						}
					}
				}

				/*	Spheres	*/
				rig_index = 0;
				for (size_t x = 0; x < this->grid_aray[0]; x++) {
					for (size_t y = 0; y < this->grid_aray[1]; y++) {
						for (size_t z = 0; z < this->grid_aray[2]; z++) {
							RigidBodyDesc desc;
							desc.collision = sphereCollider;
							desc.mass = 10;
							desc.useGravity = true;
							desc.drag = 0.005f;
							desc.position = Vector3(x * offset, 10 + y * offset + 50, z * offset);
							desc.isKinematic = false;
							fragcore::RigidBody *rigidbody = this->physic_interface->createRigibody(&desc);
							this->rigidbodies_sphere[rig_index++] = rigidbody;
						}
					}
				}

				/*	*/
				for (size_t i = 0; i < this->rigidbodies_box.size(); i++) {
					this->physic_interface->addRigidBody(this->rigidbodies_box[i]);
					this->physic_interface->addRigidBody(this->rigidbodies_sphere[i]);
				}
			}
		}

		void onResize(int width, int height) override {

			/*	Update camera	*/
			this->camera.setFar(1500);
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			/*	*/
			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	Draw from main camera */
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Draw rigidbodies.	*/
			{

				glUseProgram(this->graphic_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->white_texture);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->rigidBodySettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer_binding,
								  this->ssbo_instance_buffer,
								  ((this->getFrameCount()) % this->nrUniformBuffers) * this->uniformInstanceSize,
								  this->uniformInstanceSize);

				/*	*/
				glBindVertexArray(this->boxMesh.vao);
				glDrawElementsInstanced(GL_TRIANGLES, this->boxMesh.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										this->rigidbodies_box.size());
				glBindVertexArray(0);

				/*	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer_binding,
								  this->ssbo_instance_buffer,
								  ((this->getFrameCount()) % this->nrUniformBuffers) * this->uniformInstanceSize +
									  this->rigidbodies_box.size() * sizeof(glm::mat4),
								  this->uniformInstanceSize);

				/*	*/
				glBindVertexArray(this->sphereMesh.vao);
				glDrawElementsInstanced(GL_TRIANGLES, this->sphereMesh.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										this->rigidbodies_sphere.size());
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Draw collision plane.	*/
			{
				glUseProgram(this->hyperplane_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->white_texture);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->rigidBodySettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				glUniformMatrix4fv(glGetUniformLocation(this->hyperplane_program, "ubo.viewProj"), 1, GL_FALSE,
								   &(this->uniformStageBuffer.proj * this->uniformStageBuffer.view)[0][0]);
				glm::vec4 hyerplane = glm::vec4(0, 1, 0, 0);
				glUniform4fv(glGetUniformLocation(this->hyperplane_program, "ubo.normalDistance"), 1, &hyerplane[0]);

				glBindVertexArray(this->hyerplane.vao);
				glDrawArraysInstanced(GL_POINTS, 0, 1, 1);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {

			if (this->rigidBodySettingComponent->usePhysic) {
				this->physic_interface->sync();
			}

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());
			/*	Update Camera collider.	*/
			this->sphere_camera_collider_rig->setPosition(
				Vector3(this->camera.getPosition().x, this->camera.getPosition().y, this->camera.getPosition().z));

			/*	*/
			{
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.model = glm::mat4(0.5f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

				this->uniformStageBuffer.viewPos = glm::vec4(camera.getPosition(), 0);

				/*	Update uniform buffer.	*/
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformMappedMemory = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformAlignBufferSize,
					this->uniformAlignBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformMappedMemory, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}

			/*	Update rigidbodies model matrix.	*/
			{
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_instance_buffer);
				uint8_t *uniform_instance_buffer_pointer = (uint8_t *)glMapBufferRange(
					GL_SHADER_STORAGE_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformInstanceSize,
					this->uniformInstanceSize, GL_MAP_WRITE_BIT);

				size_t offset = 0;
				for (size_t i = 0; i < rigidbodies_box.size(); i++) {
					glm::mat4 model = glm::mat4(1);
					model = glm::translate(model, glm::vec3(rigidbodies_box[i]->getPosition().x(),
															rigidbodies_box[i]->getPosition().y(),
															rigidbodies_box[i]->getPosition().z()));

					const glm::quat roat(
						rigidbodies_box[i]->getOrientation().w(), rigidbodies_box[i]->getOrientation().x(),
						rigidbodies_box[i]->getOrientation().y(), rigidbodies_box[i]->getOrientation().z());

					model = model * glm::toMat4(roat);

					memcpy(&uniform_instance_buffer_pointer[i * sizeof(glm::mat4)], &model[0][0], sizeof(model));
					offset++;
				}

				for (size_t i = 0; i < rigidbodies_sphere.size(); i++) {
					glm::mat4 model = glm::mat4(1);
					model = glm::translate(model, glm::vec3(rigidbodies_sphere[i]->getPosition().x(),
															rigidbodies_sphere[i]->getPosition().y(),
															rigidbodies_sphere[i]->getPosition().z()));

					const glm::quat roat(
						rigidbodies_sphere[i]->getOrientation().w(), rigidbodies_sphere[i]->getOrientation().x(),
						rigidbodies_sphere[i]->getOrientation().y(), rigidbodies_sphere[i]->getOrientation().z());

					model = model * glm::toMat4(roat);

					memcpy(&uniform_instance_buffer_pointer[(offset + i) * sizeof(glm::mat4)], &model[0][0],
						   sizeof(model));
				}

				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			}

			if (this->rigidBodySettingComponent->useGravity) {
				this->physic_interface->setGravity(Vector3(0, -9.82, 0));
			} else {
				this->physic_interface->setGravity(Vector3(0, 0, 0));
			}

			if (this->getInput().getMouseDown(Input::MouseButton::LEFT_BUTTON)) {
				for (size_t i = 0; i < rigidbodies_box.size(); i++) {
					const glm::vec3 force = camera.getLookDirection() * 150.0f;
					rigidbodies_box[i]->addForce(GLM2E(force));
				}
			}

			/*	Simulate.	*/
			if (this->rigidBodySettingComponent->usePhysic) {
				this->physic_interface->simulate(
					this->getTimer().deltaTime<float>() * this->rigidBodySettingComponent->speed,
					this->rigidBodySettingComponent->maxSubStep, this->rigidBodySettingComponent->fixedTimeStep);
			}
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