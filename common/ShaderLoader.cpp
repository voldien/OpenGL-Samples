/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <GLHelper.h>
#include <GLRendererInterface.h>

using namespace glsample;

int ShaderLoader::loadGraphicProgram(const fragcore::ShaderCompiler::CompilerConvertOption &compilerOptions,
									 const std::vector<uint32_t> *vertex, const std::vector<uint32_t> *fragment,
									 const std::vector<uint32_t> *geometry,
									 const std::vector<uint32_t> *tessela1ion_control,
									 const std::vector<uint32_t> *tesselation_evolution) {

	if (glSpecializeShaderARB) {
		/*	Load SPIRV.	*/
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

	std::vector<char> tesse_control_source;
	if (tessela1ion_control) {

		const std::vector<char> geometry_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*tessela1ion_control, compilerOptions);
		tesse_control_source.insert(tesse_control_source.end(), geometry_source_code.begin(),
									geometry_source_code.end());
	}

	std::vector<char> tesse_evolution_source;
	if (tesselation_evolution) {

		const std::vector<char> geometry_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*tesselation_evolution, compilerOptions);
		tesse_evolution_source.insert(tesse_evolution_source.end(), geometry_source_code.begin(),
									  geometry_source_code.end());
	}

	return ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, &geometry_source, &tesse_control_source,
											&tesse_evolution_source);
}

int ShaderLoader::loadGraphicProgram(const std::vector<char> *vertex, const std::vector<char> *fragment,
									 const std::vector<char> *geometry, const std::vector<char> *tesselation_control,
									 const std::vector<char> *tesselation_evolution) {
	fragcore::resetErrorFlag();

	const int program = glCreateProgram();
	fragcore::checkError();

	int shader_vertex = 0;
	int shader_fragment = 0;
	int shader_geometry = 0;
	int shader_tese = 0;
	int shader_tesc = 0;
	int lstatus = 0;

	if (program < 0) {
		return 0;
	}

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

	if (tesselation_control && tesselation_control->size() > 0) {
		shader_tesc = loadShader(*tesselation_control, GL_TESS_CONTROL_SHADER);
		glAttachShader(program, shader_tesc);
		fragcore::checkError();
	}

	if (tesselation_evolution && tesselation_evolution->size() > 0) {
		shader_tese = loadShader(*tesselation_evolution, GL_TESS_EVALUATION_SHADER);
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

		throw cxxexcept::RuntimeException("Failed to link program: {}", log);
	}

	/*	Remove shader resources not needed after linking.	*/
	if (glIsShader(shader_vertex)) {
		glDetachShader(program, shader_vertex);
		fragcore::checkError();
		glDeleteShader(shader_vertex);
		fragcore::checkError();
	}
	if (glIsShader(shader_fragment)) {
		glDetachShader(program, shader_fragment);
		fragcore::checkError();
		glDeleteShader(shader_fragment);
		fragcore::checkError();
	}
	if (glIsShader(shader_geometry)) {
		glDetachShader(program, shader_geometry);
		fragcore::checkError();
		glDeleteShader(shader_geometry);
		fragcore::checkError();
	}
	if (glIsShader(shader_tesc)) {
		glDetachShader(program, shader_tesc);
		fragcore::checkError();
		glDeleteShader(shader_tesc);
		fragcore::checkError();
	}
	if (glIsShader(shader_tese)) {
		glDetachShader(program, shader_tese);
		fragcore::checkError();
		glDeleteShader(shader_tese);
		fragcore::checkError();
	}

	return program;
}

int ShaderLoader::loadComputeProgram(const fragcore::ShaderCompiler::CompilerConvertOption &compilerOptions,
									 const std::vector<uint32_t> *compute_binary) {

	std::vector<char> compute_source;
	if (glSpecializeShaderARB) {
		/*	Load SPIRV.	*/
		// if (compute_binary) {
		// 	compute_source.resize(compute_binary[0].size() * sizeof(uint32_t));
		// 	std::memcpy(compute_source.data(), compute_binary[0].data(), compute_source.size());
		// }
	} else {
	}

	if (compute_binary) {

		const std::vector<char> vertex_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*compute_binary, compilerOptions);
		compute_source.insert(compute_source.end(), vertex_source_code.begin(), vertex_source_code.end());
	}

	return ShaderLoader::loadComputeProgram({&compute_source});
}

int ShaderLoader::loadComputeProgram(const std::vector<const std::vector<char> *> &computePaths) {

	fragcore::resetErrorFlag();

	const int program = glCreateProgram();
	fragcore::checkError();

	int lstatus = 0;
	const int shader_compute = ShaderLoader::loadShader(*computePaths[0], GL_COMPUTE_SHADER);

	/*	*/
	glAttachShader(program, shader_compute);
	fragcore::checkError();

	/*	*/
	glLinkProgram(program);
	fragcore::checkError();

	glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
	fragcore::checkError();

	/*	*/
	if (lstatus != GL_TRUE) {
		char log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		fragcore::checkError();
		throw cxxexcept::RuntimeException("Failed to link program: {}", log);
	}

	if (glIsShader(shader_compute)) {
		glDetachShader(program, shader_compute);
		fragcore::checkError();
		glDeleteShader(shader_compute);
		fragcore::checkError();
	}

	return program;
}

int ShaderLoader::loadMeshProgram(const fragcore::ShaderCompiler::CompilerConvertOption &compilerOptions,
								  const std::vector<uint32_t> *meshs, const std::vector<uint32_t> *tasks,
								  const std::vector<uint32_t> *fragment) {
	std::vector<char> mesh_source;
	if (meshs) {

		const std::vector<char> mesh_source_code = fragcore::ShaderCompiler::convertSPIRV(*meshs, compilerOptions);
		mesh_source.insert(mesh_source.end(), mesh_source_code.begin(), mesh_source_code.end());
	}

	std::vector<char> task_source;
	if (tasks) {

		const std::vector<char> task_source_code = fragcore::ShaderCompiler::convertSPIRV(*tasks, compilerOptions);
		task_source.insert(task_source.end(), task_source_code.begin(), task_source_code.end());
	}

	std::vector<char> fragment_source;
	if (fragment) {

		const std::vector<char> fragment_source_code =
			fragcore::ShaderCompiler::convertSPIRV(*fragment, compilerOptions);
		fragment_source.insert(fragment_source.end(), fragment_source_code.begin(), fragment_source_code.end());
	}

	return ShaderLoader::loadMeshProgram(&mesh_source, &task_source, &fragment_source);
}

int ShaderLoader::loadMeshProgram(const std::vector<char> *meshs, const std::vector<char> *tasks,
								  const std::vector<char> *fragment) {
	fragcore::resetErrorFlag();

	const int program = glCreateProgram();
	fragcore::checkError();

	int shader_mesh = 0;
	int shader_task = 0;
	int shader_frag = 0;
	int lstatus = 0;

	/*	*/
	shader_mesh = loadShader(*meshs, GL_MESH_SHADER_NV);
	glAttachShader(program, shader_mesh);
	fragcore::checkError();

	/*	*/
	shader_task = loadShader(*tasks, GL_TASK_SHADER_NV);
	glAttachShader(program, shader_task);
	fragcore::checkError();

	/*	*/
	shader_frag = loadShader(*fragment, GL_FRAGMENT_SHADER);
	glAttachShader(program, shader_frag);
	fragcore::checkError();

	/*	*/
	glLinkProgram(program);
	fragcore::checkError();

	glGetProgramiv(program, GL_LINK_STATUS, &lstatus);
	fragcore::checkError();
	if (lstatus != GL_TRUE) {
		GLchar log[4096];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		fragcore::checkError();
		throw cxxexcept::RuntimeException("Failed to link program: {}", log);
	}

	if (glIsShader(shader_mesh)) {
		glDetachShader(program, shader_mesh);
		glDeleteShader(shader_mesh);
	}

	if (glIsShader(shader_task)) {
		glDetachShader(program, shader_task);
		glDeleteShader(shader_task);
	}

	if (glIsShader(shader_frag)) {
		glDetachShader(program, shader_frag);
		glDeleteShader(shader_frag);
	}

	return program;
}

static void checkShaderError(int shader) {

	GLint cstatus = 0;

	/*  */
	glGetShaderiv(shader, GL_COMPILE_STATUS, &cstatus);
	fragcore::checkError();

	/*	*/
	if (cstatus != GL_TRUE) {
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		fragcore::checkError();

		GLchar log[4096];
		glGetShaderInfoLog(shader, sizeof(log), &maxLength, log);
		fragcore::checkError();
		std::cout << log << std::endl;
	}
}

int ShaderLoader::loadShader(const std::vector<char> &source, const int type) {

	/*	*/
	const unsigned int spirv_magic_number = 0x07230203;
	const unsigned int magic_number = (source[3] << 24) | (source[2] << 16) | (source[1] << 8) | source[0];

	const char *source_data = source.data();
	std::vector<const GLchar **> source_refs;

	const unsigned int shader = glCreateShader(type);
	fragcore::checkError();

	/*	Load as Spirv if data is Spirv file and supported.	*/
	if (spirv_magic_number == magic_number && glSpecializeShaderARB) {

		glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, source.data(), source.size());
		fragcore::checkError();
		glSpecializeShaderARB(shader, "main", 0, nullptr, nullptr);
		fragcore::checkError();
	} else {
		glShaderSource(shader, 1, (const GLchar **)&source_data, nullptr);
		fragcore::checkError();
		/*	*/
		glCompileShader(shader);
		fragcore::checkError();
	}

	/*	*/
	checkShaderError(shader);
	return shader;
}
