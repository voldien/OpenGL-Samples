#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Texture : public GLSampleWindow {
	  public:
		Texture() : GLSampleWindow() {
			this->setTitle("Texture");

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

		} uniform_stage_buffer{};

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(uniform_buffer_block);

		MeshObject cubeGeometry;

		/*	*/
		int texture_program{};
		int diffuse_texture{};

		CameraController camera;

		/*	Texture shaders paths.	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->texture_program);
			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			/*	*/
			glDeleteVertexArrays(1, &this->cubeGeometry.vao);
			glDeleteBuffers(1, &this->cubeGeometry.vbo);
			glDeleteBuffers(1, &this->cubeGeometry.ibo);
		}

		void onResize(int width, int height) override {
			/*	*/
			this->camera.setAspect((float)width / (float)height);
		}

		void Initialize() override {

			/*	*/
			const std::string texturePath = getResult()["texture"].as<std::string>();

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
				this->texture_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &texture_vertex_binary, &texture_fragment_binary);
			}

			/*	Setup graphic program.	*/
			glUseProgram(this->texture_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->texture_program, "UniformBufferBlock");
			glUniformBlockBinding(this->texture_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->texture_program, "DiffuseTexture"), 0);
			glUseProgram(0);

			/*	Load Texture	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(texturePath, ColorSpace::SRGB);

			/*	Align the uniform buffer size to hardware specific.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align<size_t>(this->uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			Common::loadCube(this->cubeGeometry, 1);
		}

		void draw() override {

			/*	Bind subset of the uniform buffer, that the graphic pipeline will use.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize, this->uniformSize);

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	Clear default framebuffer.	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				/*	Activate texture graphic pipeline.	*/
				glUseProgram(this->texture_program);

				/*	Disable culling of faces, allows faces to be visible from both directions.	*/
				glDisable(GL_CULL_FACE);

				/*	active and bind texture to texture unit 0.	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Draw triangle*/
				glBindVertexArray(this->cubeGeometry.vao);
				glDrawElements(GL_TRIANGLES, this->cubeGeometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniform stage buffer values.	*/
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

	class TextureGLSample : public GLSample<Texture> {
	  public:
		TextureGLSample() : GLSample<Texture>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::TextureGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
