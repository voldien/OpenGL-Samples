#include "ShaderLoader.h"
#include <GL/glew.h>

int ShaderLoader::loadProgram(const std::vector<char> *vertex, const std::vector<char> *fragment,
							  const std::vector<char> *geometry, const std::vector<char> *tesselationc,
							  const std::vector<char> *tesselatione) {

	int program = glCreateProgram();
	int shader_vertex, shader_fragment, shader_geometry;
	int lstatus;
	// 	checkError();

	if (vertex) {
		shader_vertex = loadShader(*vertex, GL_VERTEX_SHADER_ARB);
		glAttachShader(program, shader_vertex);
	}
	if (fragment) {
		shader_fragment = loadShader(*fragment, GL_VERTEX_SHADER_ARB);
		glAttachShader(program, shader_fragment);
	}

finished:
	glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
	if (lstatus != GL_TRUE) {
		char log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		// TODO FIXME
		fprintf(stderr, "%s.\n", log);
		return 0;
	}

	// 	/*	Remove shader resources not needed after linking.	*/
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
	// 	if (glIsShader(tesse)) {
	// 		glDetachShader(program, tesse);
	// 		glDeleteShader(tesse);
	// 	}
	// 	if (glIsShader(tessc)) {
	// 		glDetachShader(program, tessc);
	// 		glDeleteShader(tessc);
	// 	}
	// 	if (glIsShader(compute)) {
	// 		glDetachShader(program, compute);
	// 		glDeleteShader(compute);
	// 	}
}

int ShaderLoader::loadComputeProgram(const std::vector<char> &compute) {}

static void checkShaderError(int shader) {

	GLint lstatus;

	/*  */
	glGetShaderiv(shader, GL_COMPILE_STATUS, &lstatus);
	// checkError();

	/*	*/
	if (lstatus != GL_TRUE) {
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		// checkError();

		char log[maxLength];
		glGetShaderInfoLog(shader, sizeof(log), &maxLength, log);
		// checkError();
		//		throw RuntimeException(fmt::format("Shader compilation error\n{}", log));
	}
}

int ShaderLoader::loadShader(const std::vector<char> &source, int type) {

	const char *source_data = source.data();

	int shader = glCreateShader(type);
	glShaderSourceARB(shader, 1, (const GLchar **)&source_data, nullptr);
	glCompileShaderARB(shader);
	checkShaderError(shader);
	return shader;
}
