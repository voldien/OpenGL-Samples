#include "GLSampleWindow.h"

#include "ShaderLoader.h"
#include <GL/glew.h>

#include <iostream>
namespace glsample {

	class SampleComponent : public nekomimi::UIComponent {
	  private:
	  public:
		SampleComponent() { this->setName("Sample Window"); }
		virtual void draw() override {

			ImGui::ColorEdit4("color 1", color);
			if (ImGui::Button("Press me")) {
			}
		}
		float color[4];
	};

	class Triangle : public GLSampleWindow {
	  public:
		std::shared_ptr<SampleComponent> com;
		Triangle() : GLSampleWindow() {
			this->setTitle("Triangle");
			com = std::make_shared<SampleComponent>();
			this->addUIComponent(com);
		}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		unsigned int vbo;
		unsigned vao;
		unsigned int triangle_program;
		const std::string vertexShaderPath = "Shaders/triangle/triangle.vert";
		const std::string fragmentShaderPath = "Shaders/triangle/triangle.frag";

		const std::vector<Vertex> vertices = {{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, /*	Vertex (2), Color(3)	*/
											  {0.5f, 0.5f, 0.0f, 1.0f, 0.0f},  /*	Vertex (2), Color(3)	*/
											  {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f} /*	Vertex (2), Color(3)	*/};

		virtual void Release() override {
			glDeleteProgram(this->triangle_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			// vertex_source = fragcore::ShaderCompiler::convertSPIRV(vertex_source, fragcore::ShaderLanguage::GLSL);
			// fragment_source = fragcore::ShaderCompiler::convertSPIRV(fragment_source,
			// fragcore::ShaderLanguage::GLSL);

			/*	Load shader	*/
			this->triangle_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);
			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(8));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);
			printf("%d %d\n", width, height);

			/*	Set render viewport size in pixels.	*/
			glViewport(0, 0, width, height);

			/*	Clear default framebuffer color attachment.	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			/*	Bind shader pipeline.	*/
			glUseProgram(this->triangle_program);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		virtual void update() {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Triangle> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}