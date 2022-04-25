#include "ShaderLoader.h"
#include <GL/glew.h>
#include <GLHelper.h>
#include <GLRendererInterface.h>

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

	if (vertex) {
		shader_vertex = loadShader(*vertex, GL_VERTEX_SHADER_ARB);
		glAttachShader(program, shader_vertex);
		fragcore::checkError();
	}
	if (fragment) {
		shader_fragment = loadShader(*fragment, GL_FRAGMENT_SHADER_ARB);
		glAttachShader(program, shader_fragment);
		fragcore::checkError();
	}

	if (geometry) {
		shader_geometry = loadShader(*geometry, GL_GEOMETRY_SHADER_ARB);
		glAttachShader(program, shader_geometry);
		fragcore::checkError();
	}

	if (tesselationc) {
		shader_tesc = loadShader(*tesselationc, GL_TESS_CONTROL_SHADER);
		glAttachShader(program, shader_tesc);
		fragcore::checkError();
	}

	if (tesselatione) {
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
	int program = glCreateProgram();
	int lstatus;
	int shader_vcompute = loadShader(*computePaths[0], GL_COMPUTE_SHADER_BIT);
	glAttachShader(program, shader_vcompute);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
	if (lstatus != GL_TRUE) {
		char log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		// TODO FIXME
		return 0;
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

	const char *source_data = source.data();
	std::vector<const GLchar **> source_refs;

	int shader = glCreateShader(type);
	fragcore::checkError();
	glShaderSource(shader, 1, (const GLchar **)&source_data, nullptr);
	fragcore::checkError();
	glCompileShader(shader);
	fragcore::checkError();
	checkShaderError(shader);
	return shader;
}
