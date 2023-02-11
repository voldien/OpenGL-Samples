#include "GLSampleWindow.h"

#include "ImageImport.h"
#include "ModelImporter.h"
#include "ShaderLoader.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class PhysicalBasedRendering : public GLSampleWindow {
	  public:
		PhysicalBasedRendering() : GLSampleWindow() {
			this->setTitle(fmt::format("Physical Based Rendering: {}", "path"));
			this->skyboxSettingComponent =
				std::make_shared<SkyboxPanoramicSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(skyboxSettingComponent);
		}

		int skybox_panoramic;

		/*	*/
		GeometryObject instanceGeometry;

		unsigned int physical_based_rendering_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		const NodeObject *rootNode;
		CameraController camera;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			glm::vec4 diffuseColor;
			float delta;

			/*	*/
			float speed;
			float lifetime;
			float gravity;

		} uniform_stage_buffer;

		typedef struct MaterialUniformBlock_t {

		} MaterialUniformBlock;

		const std::string modelPath = "asset/bunny.obj";
		std::string panoramicPath = "asset/panoramic.jpg";

		const std::string vertexShaderPath = "Shaders/tessellation/tessellation.vert";
		const std::string fragmentShaderPath = "Shaders/tessellation/tessellation.frag";
		const std::string ControlShaderPath = "Shaders/tessellation/tessellation.tesc";
		const std::string EvoluationShaderPath = "Shaders/tessellation/tessellation.tese";

		class SkyboxPanoramicSettingComponent : public nekomimi::UIComponent {

		  public:
			SkyboxPanoramicSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("SkyBox Settings");
			}
			virtual void draw() override {
				//	ImGui::DragFloat("Exposure", &this->uniform.exposure);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SkyboxPanoramicSettingComponent> skyboxSettingComponent;

		virtual void Release() override { glDeleteProgram(this->physical_based_rendering_program); }

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load shader source.	*/
			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			this->physical_based_rendering_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->physical_based_rendering_program);
			this->uniform_buffer_index =
				glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "Diffuse"), 0);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "Normal"), 1);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "AmbientOcclusion"), 2);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "HightMap"), 3);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "Irradiance"), 4);
			glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			this->uniform_buffer_index =
				glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
			glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize = fragcore::Math::align(uniformSize, (size_t)minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			rootNode = modelLoader.getNodeRoot();
			const ModelSystemObject &modelRef = modelLoader.getModels()[0];

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->instanceGeometry.vao);
			glBindVertexArray(this->instanceGeometry.vao);

			for (size_t i = 0; i < modelLoader.getModels().size(); i++) {

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
				glEnableVertexAttribArrayARB(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, nullptr);

				/*	UVs	*/
				glEnableVertexAttribArrayARB(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(12));

				/*	Normals.	*/
				glEnableVertexAttribArrayARB(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArrayARB(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, modelRef.vertexStride, reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);
			// glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniform_buffer, sizeof(UniformBufferBlock),
			//				  sizeof(UniformBufferBlock));
			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);
			{

				//
				// glUseProgram(this->particle_compute_program);
				// glBindVertexArray(this->vao);
				// glDispatchCompute(nrParticles / localInvoke, 1, 1);
				//
				///*	*/
				// glViewport(0, 0, width, height);
				//
				// glUseProgram(this->particle_graphic_program);
				// glEnable(GL_BLEND);
				//// TODO add blend factor.
				//
				///*	Draw triangle*/
				// glBindVertexArray(this->vao);
				// glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
				// glBindVertexArray(0);
			}
		}

		virtual void update() override {
			/*	*/
			camera.update(getTimer().deltaTime());

			this->uniform_stage_buffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			this->uniform_stage_buffer.modelViewProjection = (this->uniform_stage_buffer.proj * camera.getViewMatrix());

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->uniformSize,
								 uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};
	class PhysicalBasedRenderingGLSample : public GLSample<PhysicalBasedRendering> {
	  public:
		PhysicalBasedRenderingGLSample() : GLSample<PhysicalBasedRendering>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PhysicalBasedRenderingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
