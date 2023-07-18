#include <GLSampleWindow.h>
#include <ShaderLoader.h>
#include <FontFactory.h>
#include <GL/glew.h>
#include <GLSample.h>
#include <iostream>

namespace glsample {

	class Font : public GLSampleWindow {
	  public:
		Font() : GLSampleWindow() {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		unsigned int vbo;
		unsigned int vao;

		unsigned int triangle_program;
		const std::string vertexShaderPath = "Shaders/font/font.vert";
		const std::string fragmentShaderPath = "Shaders/font/font.frag";

		class TessellationSettingComponent : public nekomimi::UIComponent {

		  public:
			TessellationSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Tessellation Settings");
			}
			virtual void draw() override {
				//				ImGui::DragFloat("Shadow Strength", &this->uniform.shadowStrength, 1, 0.0f, 1.0f);
				//				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 1.0f);
				//				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				//				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<TessellationSettingComponent> shadowSettingComponent;

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		void Release() override {
			glDeleteProgram(this->triangle_program);
			glDeleteBuffers(1, &this->vbo);
		}
		void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath);

			// Load font.
			// FontFactory::

			/*	Load shader	*/
			this->triangle_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			glUseProgram(this->triangle_program);
			this->triangle_program = glGetUniformLocation(this->triangle_program, "MVP");
			glUniform1i(glGetUniformLocation(this->triangle_program, "fontatlast"), 0);
			glUseProgram(0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	Vertex.	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	UV coordinate.	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glBindVertexArray(0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			// glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glEnable(GL_STENCIL);

			glUseProgram(this->triangle_program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->triangle_program);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		void update() override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Font> sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}