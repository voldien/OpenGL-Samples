#include "GLSampleBase.h"
#include "Common.h"
#include "GLRendererInterface.h"
#include "MIMIWindow.h"
#include <GL/glew.h>

#include <cstddef>

using namespace glsample;

GLSampleBase::GLSampleBase() : nekomimi::MIMIWindow(nekomimi::GfxBackEnd::ImGUI_OpenGL) {

	/*	Preallocate uniform buffer pool.	*/
	const size_t uniformPoolSize = static_cast<const size_t>(1024 * 1024 * 16);
	this->uniformPool = CommonUtil::createBufferPool(GL_UNIFORM_BUFFER, uniformPoolSize);
}

GLSampleBase::~GLSampleBase() { glDeleteBuffers(1, &this->uniformPool.buffer.buffer); }

std::string GLSampleBase::getResourcePath(const char *relativePath) const noexcept {

	/*	Copmute the resource path based on how the software is packaged.	*/
	if (getenv("APPDIR")) {
		/*	Extract it as if it is installed.	*/
		return fmt::format("{}/usr/lib/asset/{}", getenv("APPDIR"), relativePath);
	}
	return fmt::format("{}/asset/{}", ".", relativePath);
}

unsigned int GLSampleBase::getShaderVersion() const {
	const int glsl_version = this->getResult()["glsl-version"].as<int>();

	/*	Override glsl version.	*/
	if (glsl_version >= 0) {
		return glsl_version;
	}

	const fragcore::GLRendererInterface *interface = this->getGLRenderInterface();

	const char *shaderVersion = interface->getShaderVersion(fragcore::ShaderLanguage::GLSL);

	const unsigned int version = std::stoi(shaderVersion);
	return version;
}

bool GLSampleBase::supportSPIRV() const {
	const fragcore::GLRendererInterface *interface =
		dynamic_cast<const fragcore::GLRendererInterface *>(this->getRenderInterface());
	return (interface->getShaderLanguage() & (fragcore::ShaderLanguage::SPIRV != 0));
}