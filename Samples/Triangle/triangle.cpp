#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>


namespace glsample {

	/**
	 * @brief
	 */
	class Triangle : public GLSampleWindow {
	  public:
		Triangle() : GLSampleWindow() { this->setTitle("Triangle"); }
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		unsigned int vbo;
		unsigned int vao;

		unsigned int triangle_program;

		const std::string vertexShaderPath = "Shaders/triangle/triangle.vert.spv";
		const std::string fragmentShaderPath = "Shaders/triangle/triangle.frag.spv";

		const std::vector<Vertex> vertices = {
			{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, /*	Vertex (2), Color(3)	*/
			{{0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}},	 /*	Vertex (2), Color(3)	*/
			{{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}} /*	Vertex (2), Color(3)	*/
		};

		void Release() override {
			glDeleteProgram(this->triangle_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
		}

		void Initialize() override {

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> triangle_vertex_binary =
					IOUtil::readFileData<uint32_t>(vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> triangle_fragment_binary =
					IOUtil::readFileData<uint32_t>(fragmentShaderPath, this->getFileSystem());

				/*	Setup compiler convert options.	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load graphic pipeline program.	*/
				this->triangle_program = ShaderLoader::loadGraphicProgram(compilerOptions, &triangle_vertex_binary,
																		  &triangle_fragment_binary);
			}

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	Create vertex buffer for the triangle vertices.	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	Setup vertex stream.	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	Setup vertex color stream.	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(8));

			glBindVertexArray(0);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	Set render viewport size in pixels.	*/
			glViewport(0, 0, width, height);

			/*	Clear default framebuffers' color attachment.	*/
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			{
				/*	Disable depth and culling of faces.	*/
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);

				/*	Bind shader pipeline.	*/
				glUseProgram(this->triangle_program);

				/*	Draw triangle.	*/
				glBindVertexArray(this->vao);
				glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}
		void update() override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Triangle> sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}