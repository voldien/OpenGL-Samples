#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 *	Simple Alpha clipping sample, where fragments are discarded if they are below a certain alpha threashold.
	 */
	class AlphaClipping : public GLSampleWindow {
	  public:
		AlphaClipping() : GLSampleWindow() {
			this->setTitle("Alpha Clipping");

			/*	Setting Window.	*/
			this->alphaClippingSettingComponent =
				std::make_shared<AlphaClippingSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->alphaClippingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;
			/*	*/
			float clipping = 0.5f;
		} uniform_stage_buffer;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		GeometryObject planGeometry;

		/*	OpenGL Graphic Program.	*/
		int texture_program;
		/*	Texture.	*/
		int diffuse_texture;

		CameraController camera;

		class AlphaClippingSettingComponent : public nekomimi::UIComponent {
		  public:
			AlphaClippingSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Alpha Clipping Settings");
			}

			void draw() override {
				/*	*/
				ImGui::TextUnformatted("Alpha Clipping Setting");
				ImGui::DragFloat("Clipping", &this->uniform.clipping, 0.035f, 0.0f, 1.0f);
				/*	*/
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<AlphaClippingSettingComponent> alphaClippingSettingComponent;

		/*	Texture shaders paths.	*/
		const std::string vertexShaderPath = "Shaders/alphaclipping/alphaclipping.vert.spv";
		const std::string fragmentShaderPath = "Shaders/alphaclipping/alphaclipping.frag.spv";

		void Release() override {
			/*	Delete graphic pipeline.	*/
			glDeleteProgram(this->texture_program);

			/*	Delete texture.	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			/*	Delete uniform buffer.	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	Delete geometry data.	*/
			glDeleteVertexArrays(1, &this->planGeometry.vao);
			glDeleteBuffers(1, &this->planGeometry.vbo);
			glDeleteBuffers(1, &this->planGeometry.ibo);
		}

		void Initialize() override {

			/*	User command line option override.	*/
			std::string texturePath = this->getResult()["texture"].as<std::string>();
			this->uniform_stage_buffer.clipping = this->getResult()["clipping"].as<float>();

			{
				/*	*/
				const std::vector<uint32_t> alpha_clip_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> alpha_clip_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->texture_program = ShaderLoader::loadGraphicProgram(compilerOptions, &alpha_clip_vertex_binary,
																		 &alpha_clip_fragment_binary);
			}

			/*	Setup graphic program.	*/
			glUseProgram(this->texture_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->texture_program, "UniformBufferBlock");
			glUniformBlockBinding(this->texture_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->texture_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Load Texture	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(texturePath);

			/*	Align the uniform buffer size to hardware specific.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align(this->uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer, with round robin support.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Load geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateCube(1, vertices, indices);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->planGeometry.vao);
				glBindVertexArray(this->planGeometry.vao);

				/*	Create indices buffer.	*/
				glGenBuffers(1, &this->planGeometry.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planGeometry.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->planGeometry.nrIndicesElements = indices.size();

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->planGeometry.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, planGeometry.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
							 GL_STATIC_DRAW);

				/*	Set Vertex attribute location and size.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(0));

				/*	Set UV attribute location and size.	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				glBindVertexArray(0);
			}
		}
		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	Clear default framebuffer.	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, alphaClippingSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			{
				/*	Bind subset of the uniform buffer, that the graphic pipeline will use.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize,
								  this->uniformSize);

				/*	Activate texture graphic pipeline.	*/
				glUseProgram(this->texture_program);

				/*	Disable culling of faces, allows faces to be visable from both directions.	*/
				glDisable(GL_CULL_FACE);

				/*	active and bind texture to texture unit 0.	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Draw triangle*/
				glBindVertexArray(this->planGeometry.vao);
				glDrawElements(GL_TRIANGLES, this->planGeometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				/*	Unbound any graphic/compute pipeline.	*/
				glUseProgram(0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniform_stage_buffer.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.modelViewProjection =
				(this->uniform_stage_buffer.proj * this->camera.getViewMatrix());

			/*	Update uniform buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformMappedMemory = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSize,
				this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformMappedMemory, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class AlphaClippingGLSample : public GLSample<AlphaClipping> {
	  public:
		AlphaClippingGLSample() : GLSample<AlphaClipping>() {}

		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"))(
				"C,clipping", "Default Clipping Threshold", cxxopts::value<float>()->default_value("0.5"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::AlphaClippingGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
