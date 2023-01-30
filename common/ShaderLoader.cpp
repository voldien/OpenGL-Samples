#include "ShaderLoader.h"
#include <GL/glew.h>
#include <GLHelper.h>
#include <GLRendererInterface.h>

using namespace glsample;
int ShaderLoader::loadGraphicProgram(const fragcore::ShaderCompiler::CompilerConvertOption &compilerOptions,
									 const std::vector<uint32_t> *vertex, const std::vector<uint32_t> *fragment,
									 const std::vector<uint32_t> *geometry, const std::vector<uint32_t> *tesselationc,
									 const std::vector<uint32_t> *tesselatione) {

	if (glSpecializeShaderARB) {
	} else {
	}

	std::vector<char> vertex_source;
	if (vertex) {

		const std::vector<char> vertex_source_code = fragcore::ShaderCompiler::convertSPIRV(*vertex, compilerOptions);
		vertex_source.insert(vertex_source.end(), vertex_source_code.begin(), vertex_source_code.end());
	}

	std::vector<char> fragment_source;
	if (fragment) {

		const std::vector<char> fragment_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*fragment, compilerOptions);
		fragment_source.insert(fragment_source.end(), fragment_source_code.begin(), fragment_source_code.end());
	}

	std::vector<char> geometry_source;
	if (geometry) {

		const std::vector<char> geometry_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*geometry, compilerOptions);
		geometry_source.insert(geometry_source.end(), geometry_source_code.begin(), geometry_source_code.end());
	}

	std::vector<char> tesse_c_source;
	if (tesselationc) {

		const std::vector<char> geometry_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*tesselationc, compilerOptions);
		tesse_c_source.insert(tesse_c_source.end(), geometry_source_code.begin(), geometry_source_code.end());
	}

	std::vector<char> tesse_e_source;
	if (tesselatione) {

		const std::vector<char> geometry_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*tesselatione, compilerOptions);
		tesse_e_source.insert(tesse_e_source.end(), geometry_source_code.begin(), geometry_source_code.end());
	}

	return loadGraphicProgram(&vertex_source, &fragment_source, &geometry_source, &tesse_c_source, &tesse_e_source);
}

int ShaderLoader::loadGraphicProgram(const std::vector<char> *vertex, const std::vector<char> *fragment,
									 const std::vector<char> *geometry, const std::vector<char> *tesselationc,
									 const std::vector<char> *tesselatione) {
	fragcore::resetErrorFlag();

	int program = glCreateProgram();
	fragcore::checkError();
	int shader_vertex = 0;
	int shader_fragment = 0;
	int shader_geometry = 0;
	int shader_tese = 0;
	int shader_tesc = 0;
	int lstatus;

	if (program < 0)
		return 0;

	if (vertex && vertex->size() > 0) {
		shader_vertex = loadShader(*vertex, GL_VERTEX_SHADER_ARB);
		glAttachShader(program, shader_vertex);
		fragcore::checkError();
	}
	if (fragment && fragment->size() > 0) {
		shader_fragment = loadShader(*fragment, GL_FRAGMENT_SHADER_ARB);
		glAttachShader(program, shader_fragment);
		fragcore::checkError();
	}

	if (geometry && geometry->size() > 0) {
		shader_geometry = loadShader(*geometry, GL_GEOMETRY_SHADER_ARB);
		glAttachShader(program, shader_geometry);
		fragcore::checkError();
	}

	if (tesselationc && tesselationc->size() > 0) {
		shader_tesc = loadShader(*tesselationc, GL_TESS_CONTROL_SHADER);
		glAttachShader(program, shader_tesc);
		fragcore::checkError();
	}

	if (tesselatione && tesselatione->size() > 0) {
		shader_tese = loadShader(*tesselatione, GL_TESS_EVALUATION_SHADER);
		glAttachShader(program, shader_tese);
		fragcore::checkError();
	}

	glLinkProgram(program);
	fragcore::checkError();

finished:
	glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
	fragcore::checkError();

	if (lstatus != GL_TRUE) {
		char log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		fragcore::checkError();
		// TODO FIXME
		throw cxxexcept::RuntimeException("Failed to link program: {}", log);
	}

	/*	Remove shader resources not needed after linking.	*/
	if (glIsShader(shader_vertex)) {
		glDetachShader(program, shader_vertex);
		glDeleteShader(shader_vertex);
	}
	if (glIsShader(shader_fragment)) {
		glDetachShader(program, shader_fragment);
		glDeleteShader(shader_fragment);
	}
	if (glIsShader(shader_geometry)) {
		glDetachShader(program, shader_geometry);
		glDeleteShader(shader_geometry);
	}
	if (glIsShader(shader_tesc)) {
		glDetachShader(program, shader_tesc);
		glDeleteShader(shader_tesc);
	}
	if (glIsShader(shader_tese)) {
		glDetachShader(program, shader_tese);
		glDeleteShader(shader_tese);
	}

	return program;
}

int ShaderLoader::loadComputeProgram(const std::vector<std::vector<char> *> &computePaths) {

	fragcore::resetErrorFlag();

	int program = glCreateProgram();
	int lstatus;
	int shader_vcompute = loadShader(*computePaths[0], GL_COMPUTE_SHADER);

	/*	*/
	glAttachShader(program, shader_vcompute);
	fragcore::checkError();

	/*	*/
	glLinkProgram(program);
	fragcore::checkError();

	glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
	fragcore::checkError();
	if (lstatus != GL_TRUE) {
		char log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		fragcore::checkError();

		throw cxxexcept::RuntimeException("Failed to link program: {}", log);
	}

	if (glIsShader(shader_vcompute)) {
		glDetachShader(program, shader_vcompute);
		glDeleteShader(shader_vcompute);
	}

	return program;
}

static void checkShaderError(int shader) {

	GLint lstatus;

	/*  */
	glGetShaderiv(shader, GL_COMPILE_STATUS, &lstatus);
	fragcore::checkError();

	/*	*/
	if (lstatus != GL_TRUE) {
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		fragcore::checkError();

		char log[maxLength];
		glGetShaderInfoLog(shader, sizeof(log), &maxLength, log);
		fragcore::checkError();
		//		throw RuntimeException(fmt::format("Shader compilation error\n{}", log));
	}
}

int ShaderLoader::loadShader(const std::vector<char> &source, int type) {

	const unsigned int spirv_magic_number = 0x07230203;
	const unsigned int magic_number = (source[3] << 24) | (source[2] << 16) | (source[1] << 8) | source[0];

	const char *source_data = source.data();
	std::vector<const GLchar **> source_refs;

	unsigned int shader = glCreateShader(type);
	fragcore::checkError();
	if (spirv_magic_number == magic_number && glSpecializeShaderARB) {
		glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, source.data(), source.size());
		glSpecializeShaderARB(shader, "main", 0, 0, 0);
	} else {
		glShaderSource(shader, 1, (const GLchar **)&source_data, nullptr);
	}

	fragcore::checkError();
	glCompileShader(shader);
	fragcore::checkError();
	checkShaderError(shader);
	return shader;
}
