#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	// TODO add support for batching.
	class Instance : public GLSampleWindow {
	  public:
		Instance() : GLSampleWindow() {
			this->setTitle("Instance");
			this->instanceSettingComponent = std::make_shared<InstanceSettingComponent>(this->uniformData);
			this->addUIComponent(this->instanceSettingComponent);

			this->camera.setPosition(glm::vec3(0));
			this->camera.lookAt(glm::vec3(1));
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	Light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f);
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 viewPos;

			float shininess = 16.0f;
		} uniformData;

		/*  */
		size_t rows = 8;
		size_t cols = 8;

		size_t instanceBatch = 64;
		const size_t nrInstances = (rows * cols);
		std::vector<glm::mat4> instance_model_matrices;

		/*	*/
		GeometryObject instanceGeometry;

		/*	*/
		unsigned int diffuse_texture;
		/*	*/
		unsigned int instance_program;

		/*  Instance buffer model matrix.   */
		unsigned int instance_model_buffer;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;
		unsigned int uniform_mvp_buffer;
		unsigned int uniform_instance_buffer;
		const size_t nrUniformBuffers = 3;

		size_t uniformSize = sizeof(UniformBufferBlock);
		size_t uniformInstanceSize = 0;

		CameraController camera;

		class InstanceSettingComponent : public nekomimi::UIComponent {

		  public:
			InstanceSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Instance Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular", &this->uniform.specularColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<InstanceSettingComponent> instanceSettingComponent;

		const std::string vertexInstanceShaderPath = "Shaders/instance/instance.vert.spv";
		const std::string fragmentInstanceShaderPath = "Shaders/instance/instance.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->instance_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_mvp_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->instanceGeometry.vao);
			glDeleteBuffers(1, &this->instanceGeometry.vbo);
		}

		void Initialize() override {

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			/*	Load shader source.	*/
			std::vector<uint32_t> instance_vertex_binary =
				IOUtil::readFileData<uint32_t>(this->vertexInstanceShaderPath, this->getFileSystem());
			std::vector<uint32_t> instance_fragment_binary =
				IOUtil::readFileData<uint32_t>(this->fragmentInstanceShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->instance_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &instance_vertex_binary, &instance_fragment_binary);

			/*	Setup instance graphic pipeline.	*/
			glUseProgram(this->instance_program);
			glUniform1i(glGetUniformLocation(this->instance_program, "DiffuseTexture"), 0);
			/*	*/
			int uniform_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformBufferBlock");
			glUniformBlockBinding(this->instance_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			/*	*/
			int uniform_instance_buffer_index = glGetUniformBlockIndex(this->instance_program, "UniformInstanceBlock");
			glUniformBlockBinding(this->instance_program, this->uniform_instance_buffer_index,
								  this->uniform_instance_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = fragcore::Math::align(this->uniformSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_mvp_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffers, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			GLint uniformMaxSize;
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uniformMaxSize);
			this->instanceBatch = 64; ////:; uniformMaxSize / sizeof(glm::mat4);

			/*	*/
			this->uniformInstanceSize =
				fragcore::Math::align(this->instanceBatch * sizeof(glm::mat4), (size_t)minMapBufferSize);
			this->instance_model_matrices.resize(this->instanceBatch);
			glGenBuffers(1, &this->uniform_instance_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformInstanceSize * this->nrUniformBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			const ModelSystemObject &modelRef = modelLoader.getModels()[0];

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);

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

		void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniformData.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Bind MVP Uniform Buffer.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_mvp_buffer,
							  (this->getFrameCount() % this->nrUniformBuffers) * this->uniformSize, this->uniformSize);

			/*	Bind Model Instance Uniform Buffer.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_instance_buffer_binding, this->uniform_instance_buffer,
							  (this->getFrameCount() % this->nrUniformBuffers) * this->uniformInstanceSize,
							  this->uniformInstanceSize);

			// TODO add support for batching for limit amount of uniform buffers.

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->instanceSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			glUseProgram(this->instance_program);
			glDisable(GL_CULL_FACE);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	Draw Instances.	*/
			glBindVertexArray(this->instanceGeometry.vao);
			glDrawElementsInstanced(GL_TRIANGLES, this->instanceGeometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
									this->nrInstances);
			glBindVertexArray(0);
		}

		void update() override {
			/*	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	Update instance model matrix.	*/
			for (size_t i = 0; i < rows; i++) {
				for (size_t j = 0; j < cols; j++) {
					const size_t index = i * cols + j;

					glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(i * 10.0f, 0, j * 10.0f));
					model = glm::rotate(model, glm::radians(elapsedTime * 45.0f + index * 11.5f),
										glm::vec3(0.0f, 1.0f, 0.0f));
					model = glm::scale(model, glm::vec3(1.95f));

					instance_model_matrices[index] = model;
				}
			}

			/*	*/
			this->uniformData.model = glm::mat4(1.0f);
			this->uniformData.view = camera.getViewMatrix();
			this->uniformData.modelViewProjection = this->uniformData.model * camera.getViewMatrix();
			this->uniformData.viewPos = glm::vec4(this->camera.getPosition(), 0);

			/*	Update uniform.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_mvp_buffer);
			void *uniformMVP = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformSize,
				this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformMVP, &this->uniformData, sizeof(this->uniformData));
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			/*	Update instance buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			void *uniformInstance = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformInstanceSize,
				this->uniformInstanceSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformInstance, this->instance_model_matrices.data(),
				   sizeof(this->instance_model_matrices[0]) * this->instance_model_matrices.size());

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class InstanceGLSample : public GLSample<Instance> {
	  public:
		InstanceGLSample() : GLSample<Instance>() {}

		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny.obj"))(
				"B,batch", "Bath Size", cxxopts::value<int>()->default_value("64"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::InstanceGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}