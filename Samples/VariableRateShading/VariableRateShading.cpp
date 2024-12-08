#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ModelViewer.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {
	/**
	 * @brief
	 *
	 */
	class VariableRateShading : public ModelViewer {
	  public:
		VariableRateShading() : ModelViewer() { this->setTitle("Variable Rate Shading (VRS)"); }

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

		} uniformBuffer;

		unsigned int variable_rate_program;
		int localWorkGroupSize[3];
		unsigned int variable_rate_lut_texture;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;

		unsigned int nthTexture = 0;
		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		std::vector<unsigned int> multipass_textures;
		unsigned int depthTexture;

		/*	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		const std::string computeShaderPath = "Shaders/variablerateshading/variablerateshading.comp.spv";

		void Release() override {
			glDeleteProgram(this->variable_rate_program);
			/*	*/
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			ModelViewer::Initialize();

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> variable_rate_shading_compute_binary =
					IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->variable_rate_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &variable_rate_shading_compute_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->variable_rate_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->variable_rate_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->variable_rate_program, "texture0"), 0);
			glUniform1i(glGetUniformLocation(this->variable_rate_program, "variableRateLUT"), 1);
			glUniformBlockBinding(this->variable_rate_program, uniform_buffer_index, this->uniform_buffer_binding);
			glGetProgramiv(this->variable_rate_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			this->multipass_textures.resize(1);
			glGenTextures(this->multipass_textures.size(), this->multipass_textures.data());
			glGenTextures(1, &this->variable_rate_lut_texture);
			onResize(this->width(), this->height());

			/*	Setup and configure shading rate palette.	*/
			{
				GLint palSize;
				glGetIntegerv(GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV, &palSize);

				GLenum *palette = new GLenum[palSize];

				palette[0] = GL_SHADING_RATE_NO_INVOCATIONS_NV;
				palette[1] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
				palette[2] = GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
				palette[3] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;

				/* fill the rest	*/
				for (GLint i = 4; i < palSize; ++i) {
					palette[i] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
				}

				glShadingRateImagePaletteNV(0, 0, palSize, palette);
				delete[] palette;

				/*	 One palette with a constant rate	*/
				GLenum *paletteFullRate = new GLenum[palSize];
				for (GLint i = 0; i < palSize; ++i) {
					paletteFullRate[i] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
				}

				glShadingRateImagePaletteNV(1, 0, palSize, paletteFullRate);
				delete[] paletteFullRate;
			}
		}

		void onResize(int width, int height) override {
			ModelViewer::onResize(width, height);
			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(multipass_textures.size());
			for (size_t i = 0; i < this->multipass_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->multipass_texture_width,
							 this->multipass_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->multipass_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			/*	*/
			glGenTextures(1, &this->depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->multipass_texture_width,
						 this->multipass_texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture, 0);

			glDrawBuffers(drawAttach.size(), drawAttach.data());

			/*  Validate if created properly.*/
			const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			/*	Create and configure Variable Rate Shading configuration.	*/
			glEnable(GL_SHADING_RATE_IMAGE_PER_PRIMITIVE_NV);
			glEnable(GL_SHADING_RATE_IMAGE_NV);

			/*	Create and update */
			{
				int m_shadingRateImageTexelWidth;
				int m_shadingRateImageTexelHeight;

				glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV, &m_shadingRateImageTexelHeight);
				glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV, &m_shadingRateImageTexelWidth);

				const size_t lut_width = std::floor(width / (float)m_shadingRateImageTexelWidth);
				const size_t lut_height = std::floor(height / (float)m_shadingRateImageTexelHeight);

				glBindTexture(GL_TEXTURE_2D, this->variable_rate_lut_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, lut_width, lut_height, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE,
							 nullptr);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

		}

		void draw() override {

			/*	*/
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

				/*	Variable rate shading.	*/
				glBindShadingRateImageNV(this->variable_rate_lut_texture);

				glEnable(GL_SHADING_RATE_IMAGE_NV);

				ModelViewer::draw();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				glDisable(GL_SHADING_RATE_IMAGE_NV);
			}

			/*	*/
			int width, height;
			getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	Bind and Compute variable rate look up table Program.	*/
			{
				glUseProgram(this->variable_rate_program);

				/*	Previous game of life state.	*/
				glBindImageTexture(0, this->multipass_textures[this->nthTexture % this->multipass_textures.size()], 0,
								   GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

				/*	The resulting game of life state.	*/
				glBindImageTexture(1, this->variable_rate_lut_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

				const size_t workGroupX = std::ceil(this->multipass_texture_width / (float)this->localWorkGroupSize[0]);
				const size_t workGroupY =
					std::ceil(this->multipass_texture_height / (float)this->localWorkGroupSize[1]);

				glDispatchCompute(workGroupY, workGroupY, 1);

				/*	Wait in till image has been written.	*/
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, width, height);

			/*	Blit image targets to screen.	*/
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multipass_framebuffer);

				/*	*/
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height, 0, 0, width,
								  height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	const std::vector<const char *> required_extensions = {"GL_NV_shading_rate_image",
														   /*"GL_NV_primitive_shading_rate"*/};

	try {
		glsample::VariableRateShadingGLSample sample;
		sample.run(argc, argv, required_extensions);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}