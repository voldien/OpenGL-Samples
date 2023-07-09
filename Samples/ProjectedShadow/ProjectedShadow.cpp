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

	class ProjectedShadow : public GLSampleWindow {
	  public:
		ProjectedShadow() : GLSampleWindow() {
			this->setTitle("MultiPass");

			this->alphaClippingSettingComponent = std::make_shared<AlphaClippingSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->alphaClippingSettingComponent);

			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

		} uniformBuffer;

		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		/*	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		class AlphaClippingSettingComponent : public nekomimi::UIComponent {
		  public:
			AlphaClippingSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("NormalMap Settings");
			}

			virtual void draw() override {

				//ImGui::TextUnformatted("BillBoarding Setting");
				//ImGui::DragFloat("Clipping", &this->uniform.clipping, 0.035f, 0.0f, 1.0f);
				//ImGui::TextUnformatted("Debug Setting");
				//ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<AlphaClippingSettingComponent> alphaClippingSettingComponent;

		virtual void Release() override {
			///*	*/
			// glDeleteProgram(this->texture_program);
			//
			///*	*/
			// glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			//
			///*	*/
			// glDeleteVertexArrays(1, &this->planGeometry.vao);
			// glDeleteBuffers(1, &this->planGeometry.vbo);
			// glDeleteBuffers(1, &this->planGeometry.ibo);
		}
		virtual void Initialize() override {}

		virtual void draw() override {
			{
				/*	Bind subset of the uniform buffer, that the graphic pipeline will use.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				int width, height;
				this->getSize(&width, &height);

				/*	*/
				glViewport(0, 0, width, height);

				/*	Clear default framebuffer.	*/
				glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
		}
		virtual void update() override {}
	};

	class ProjectedShadowGLSample : public GLSample<ProjectedShadow> {
	  public:
		ProjectedShadowGLSample() : GLSample<ProjectedShadow>() {}

		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::ProjectedShadowGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
