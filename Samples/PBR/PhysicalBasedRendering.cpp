#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class PhysicalBasedRendering : public GLSampleWindow {
	  public:
		PhysicalBasedRendering() : GLSampleWindow() {
			this->setTitle(fmt::format("Physical Based Rendering: {}", "path"));

			this->physicalBasedRenderingSettingComponent =
				std::make_shared<PhysicalBasedRenderingSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(physicalBasedRenderingSettingComponent);
		}

		unsigned int reflection_texture;

		/*	*/
		GeometryObject instanceGeometry;
		GeometryObject skybox;

		unsigned int physical_based_rendering_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t skyboxUniformSize = 0;

		const NodeObject *rootNode;
		CameraController camera;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			glm::vec4 diffuseColor;

		} uniform_stage_buffer;

		typedef struct MaterialUniformBlock_t {

		} MaterialUniformBlock;

		/*	Skybox.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		/*	Simple	*/
		const std::string vertexPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.vert";
		const std::string fragmentPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.frag";

		/*	Advanced.	*/
		const std::string vertexShaderPath = "Shaders/pbr/physicalbasedrendering.vert";
		const std::string fragmentShaderPath = "Shaders/pbr/physicalbasedrendering.frag";
		const std::string ControlShaderPath = "Shaders/pbr/physicalbasedrendering.tesc";
		const std::string EvoluationShaderPath = "Shaders/pbr/physicalbasedrendering.tese";

		class PhysicalBasedRenderingSettingComponent : public nekomimi::UIComponent {

		  public:
			PhysicalBasedRenderingSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Physical Based Rendering Settings");
			}
			virtual void draw() override {
				/*	*/
				ImGui::TextUnformatted("Skybox");
				// ImGui::ColorEdit4("Tint", &this->uniform.skybox.tintColor[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::DragFloat("Exposure", &this->uniform.skybox.exposure);
				/*	*/
				ImGui::TextUnformatted("Debug");

				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};

		std::shared_ptr<PhysicalBasedRenderingSettingComponent> physicalBasedRenderingSettingComponent;

		virtual void Release() override { glDeleteProgram(this->physical_based_rendering_program); }

		virtual void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			/*	*/
			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());
			const std::vector<uint32_t> control_source =
				IOUtil::readFileData<uint32_t>(this->ControlShaderPath, this->getFileSystem());
			const std::vector<uint32_t> evolution_source =
				IOUtil::readFileData<uint32_t>(this->EvoluationShaderPath, this->getFileSystem());

			const std::vector<uint32_t> vertex_skybox_binary =
				IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_skybox_binary =
				IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->physical_based_rendering_program = ShaderLoader::loadGraphicProgram(
				compilerOptions, &vertex_source, &fragment_source, nullptr, &control_source, &evolution_source);
			this->skybox_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

			/*	Setup shader.	*/
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
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
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

			std::vector<ProceduralGeometry::Vertex> verticesCube;
			std::vector<unsigned int> indicesCube;
			ProceduralGeometry::generateCube(1, verticesCube, indicesCube);
			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox.vao);
			glBindVertexArray(this->skybox.vao);

			/*	*/
			glGenBuffers(1, &this->skybox.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCube.size() * sizeof(indicesCube[0]), indicesCube.data(),
						 GL_STATIC_DRAW);
			this->skybox.nrIndicesElements = indicesCube.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->skybox.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
			glBufferData(GL_ARRAY_BUFFER, verticesCube.size() * sizeof(ProceduralGeometry::Vertex), verticesCube.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(12));

			glBindVertexArray(0);
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

			/*	Skybox	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize + 0,
								  this->skyboxUniformSize);

				glUseProgram(this->skybox_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glStencilMask(GL_FALSE);
				glDepthFunc(GL_LEQUAL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glStencilMask(GL_TRUE);

				glUseProgram(0);
			}
		}

		virtual void update() override {
			/*	*/
			camera.update(getTimer().deltaTime());

			this->uniform_stage_buffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			this->uniform_stage_buffer.modelViewProjection = (this->uniform_stage_buffer.proj * camera.getViewMatrix());

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->skyboxUniformSize,
				this->skyboxUniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	class PhysicalBasedRenderingGLSample : public GLSample<PhysicalBasedRendering> {
	  public:
		PhysicalBasedRenderingGLSample() : GLSample<PhysicalBasedRendering>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"T,skybox-texture", "Skybox Texture Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
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
