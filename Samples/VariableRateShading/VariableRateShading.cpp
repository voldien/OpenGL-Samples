#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ModelViewer.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {
	/**
	 * @brief
	 *
	 */
	class VariableRateShading : public ModelViewer {
	  public:
		VariableRateShading() : ModelViewer() {
			this->setTitle("Variable Rate Shading (VRS)");

			this->variableRateSettingComponent = std::make_shared<VariableRateShadingSettingComponent>(*this);
			this->addUIComponent(this->variableRateSettingComponent);
		}

		unsigned int variable_rate_color_program{};
		unsigned int variable_depth_edge_rate_program{};
		unsigned int variable_visual_edge_rate_program{};
		int localWorkGroupSize[3]{};
		unsigned int variable_rate_lut_texture = 0;
		unsigned int variable_rate_visual_texture = 0;

		unsigned int nthTexture = 0;

		const std::string VRS_color_edge_ShaderPath = "Shaders/variablerateshading/variablerateshading.comp.spv";
		const std::string VRS_depth_dege_ShaderPath = "Shaders/variablerateshading/variablerateshading_depth.comp.spv";
		const std::string VRS_visual_VShaderPath = "Shaders/variablerateshading/variablerateshading_visual.comp.spv";

		class VariableRateShadingSettingComponent : public GLUIComponent<VariableRateShading> {

		  public:
			VariableRateShadingSettingComponent(VariableRateShading &sample)
				: GLUIComponent(sample), uniform(sample.uniformStageBuffer) {
				this->setName("Variable Rate");
			}
			void draw() override {
				ImGui::Image(this->getRefSample().variable_rate_visual_texture, ImVec2(512, 512), ImVec2(0, 1),
							 ImVec2(1, 0));
				ImGui::Checkbox("Use Variable Rate Shading", &useVRS);
			}

			bool useVRS = true;
			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<VariableRateShadingSettingComponent> variableRateSettingComponent;

		void Release() override {
			ModelViewer::Release();

			glDeleteProgram(this->variable_rate_color_program);
		}

		void Initialize() override {

			ModelViewer::Initialize();

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> variable_rate_shading_edgecolor_compute_binary =
					IOUtil::readFileData<uint32_t>(this->VRS_color_edge_ShaderPath, this->getFileSystem());
				const std::vector<uint32_t> variable_rate_shading_visual_compute_binary =
					IOUtil::readFileData<uint32_t>(this->VRS_visual_VShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->variable_rate_color_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &variable_rate_shading_edgecolor_compute_binary);
				this->variable_visual_edge_rate_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &variable_rate_shading_visual_compute_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->variable_rate_color_program);
			glUniform1i(glGetUniformLocation(this->variable_rate_color_program, "ColorTexture"), 0);
			glUniform1i(glGetUniformLocation(this->variable_rate_color_program, "VariableRateLUT"), 1);
			glGetProgramiv(this->variable_rate_color_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->variable_visual_edge_rate_program);
			glUniform1i(glGetUniformLocation(this->variable_visual_edge_rate_program, "VariableRateLUT"), 0);
			glUniform1i(glGetUniformLocation(this->variable_visual_edge_rate_program, "ColorTexture"), 1);
			glGetProgramiv(this->variable_visual_edge_rate_program, GL_COMPUTE_WORK_GROUP_SIZE,
						   this->localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			glGenTextures(1, &variable_rate_visual_texture);
			this->onResize(this->width(), this->height());

			glShadingRateImageBarrierNV(GL_FALSE);
		}

		void onResize(int width, int height) override {
			ModelViewer::onResize(width, height);

			/*	Setup and configure shading rate palette.	*/
			{
				GLint palSize = 0;
				glGetIntegerv(GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV, &palSize);
				getLogger().debug("GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV {}", palSize);

				GLenum *palette = new GLenum[palSize];

				palette[0] = GL_SHADING_RATE_NO_INVOCATIONS_NV;
				palette[1] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
				palette[2] = GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
				palette[3] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;

				/* fill the rest	*/
				for (GLint index = 4; index < palSize; ++index) {
					palette[index] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;
				}

				glShadingRateImagePaletteNV(0, 0, palSize, palette);
				delete[] palette;
			}

			/*	Create and configure Variable Rate Shading configuration.	*/
			/*	Create and update */
			{
				int m_shadingRateImageTexelWidth = 0;
				int m_shadingRateImageTexelHeight = 0;

				glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV, &m_shadingRateImageTexelHeight);
				glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV, &m_shadingRateImageTexelWidth);

				getLogger().debug("Width: {}", m_shadingRateImageTexelWidth);
				getLogger().debug("Height: {}", m_shadingRateImageTexelHeight);

				const size_t lut_width = std::ceil(width / (float)m_shadingRateImageTexelWidth);
				const size_t lut_height = std::ceil(height / (float)m_shadingRateImageTexelHeight);

				/*	*/
				if (glIsTexture(this->variable_rate_lut_texture)) {
					glDeleteTextures(1, &this->variable_rate_lut_texture);
				}
				glGenTextures(1, &this->variable_rate_lut_texture);
				glBindTexture(GL_TEXTURE_2D, this->variable_rate_lut_texture);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, lut_width, lut_height);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glBindTexture(GL_TEXTURE_2D, 0);

				glBindTexture(GL_TEXTURE_2D, this->variable_rate_visual_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, lut_width, lut_height, 0, GL_RGBA, GL_FLOAT, nullptr);
				/*	Filtering.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

		void draw() override {

			/*	Draw scene.	*/
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());

				/*	Variable rate shading.	*/
				glBindShadingRateImageNV(this->variable_rate_lut_texture);

				if (this->variableRateSettingComponent->useVRS) {
					glEnable(GL_SHADING_RATE_IMAGE_NV);
				}

				ModelViewer::draw();

				glDisable(GL_SHADING_RATE_IMAGE_NV);

				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			/*	*/
			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	Bind and Compute variable rate look up table Program.	*/
			glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

			{
				glUseProgram(this->variable_rate_color_program);

				GLint width = 0;
				GLint height = 0;

				glBindTexture(GL_TEXTURE_2D, variable_rate_lut_texture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

				/*	Previous game of life state.	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->getFrameBuffer()->attachments[0]);
				/*	The resulting game of life state.	*/
				glBindImageTexture(1, this->variable_rate_lut_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

				const size_t workGroupX = std::ceil(width / (float)this->localWorkGroupSize[0]);
				const size_t workGroupY = std::ceil(height / (float)this->localWorkGroupSize[1]);

				glDispatchCompute(workGroupX, workGroupY, 1);

				/*	Wait in till image has been written.	*/
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				glUseProgram(this->variable_visual_edge_rate_program);

				glBindImageTexture(0, this->variable_rate_lut_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
				glBindImageTexture(1, this->variable_rate_visual_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glDispatchCompute(workGroupX, workGroupY, 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glUseProgram(0);
			}
		}

		void update() override { ModelViewer::update(); }
	};

	class VariableRateShadingGLSample : public GLSample<VariableRateShading> {
	  public:
		VariableRateShadingGLSample() : GLSample<VariableRateShading>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			GLSampleSession::customOptions(options);
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {

	const std::vector<const char *> required_extensions = {"GL_NV_shading_rate_image"};

	try {
		glsample::VariableRateShadingGLSample sample;
		sample.run(argc, argv, required_extensions);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}