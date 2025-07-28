#include "PostProcessing/PostProcessing.h"
#include "GLSampleSession.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IO/IOUtil.h>
#include <algorithm>

using namespace glsample;

PostProcessing::PostProcessing() : computeShaderSupported(glewIsExtensionSupported("GL_ARB_compute_shader")) {}

void PostProcessing::draw(glsample::FrameBuffer *framebuffer,
						  const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {

	/*	*/
	this->mapped_buffer.clear();
	for (const auto *it = render_targets.begin(); it != render_targets.end(); it++) {

		/*	*/
		const GBuffer target = std::get<0>(*it);
		const unsigned int texture_index = std::get<1>(*it);

		/*	*/
		const unsigned int texture = framebuffer->attachments[texture_index];
		this->mapped_buffer[target] = &framebuffer->attachments[texture_index];

		/*	*/
		if (!this->isBufferRequired(target)) {
			continue;
		}

		/*	*/
		if (glBindTextureUnit) {
			glBindTextureUnit(static_cast<unsigned int>(target), texture);
		} else {
			glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(target));
			glBindTexture(GL_TEXTURE_2D, texture);
		}
	}

	/*	*/
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

float PostProcessing::getIntensity() const noexcept { return this->intensity; }
void PostProcessing::setIntensity(const float intensity) { this->intensity = intensity; }

bool PostProcessing::isBufferRequired(const GBuffer required_data_buffer) const noexcept {
	return std::find(this->required_buffer.begin(), this->required_buffer.end(), required_data_buffer) !=
		   this->required_buffer.end();
}

const unsigned int &PostProcessing::getMappedBuffer(const GBuffer buffer_target) const noexcept {
	static unsigned int temp = 0;
	return this->mapped_buffer.find(buffer_target) != this->mapped_buffer.end() ? *this->mapped_buffer.at(buffer_target)
																				: temp;
}

void PostProcessing::addRequireBuffer(const GBuffer required_data_buffer) noexcept {
	this->required_buffer.push_back(required_data_buffer);
}
void PostProcessing::removeRequireBuffer(const GBuffer required_data_buffer) noexcept {}

int PostProcessing::createVAO() {
	static unsigned int vao = 0;
	if (!glIsVertexArray(vao)) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindVertexArray(0);
	}

	return vao;
}

int PostProcessing::createOverlayGraphicProgram(fragcore::IFileSystem *filesystem) {

	static int overlay_graphic_program = -1;

	if (overlay_graphic_program == -1) {
		/*	*/
		const std::string vertexOverlayShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
		const std::string fragmentOverlayTextureShaderPath = "Shaders/postprocessingeffects/overlay.frag.spv";

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		const std::vector<uint32_t> texture_vertex_binary =
			IOUtil::readFileData<uint32_t>(vertexOverlayShaderPath, filesystem);
		const std::vector<uint32_t> texture_fragment_binary =
			IOUtil::readFileData<uint32_t>(fragmentOverlayTextureShaderPath, filesystem);

		/*	Load shader	*/
		overlay_graphic_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &texture_vertex_binary, &texture_fragment_binary);
	}

	return overlay_graphic_program;
}