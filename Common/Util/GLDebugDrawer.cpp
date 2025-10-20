#include "GLDebugDrawer.h"
#include <ShaderCompiler.h>
#include <ShaderLoader.h>

using namespace glsample;

GLDebugDrawManager::GLDebugDrawManager(fragcore::IFileSystem *filesystem) : DebugDrawManager(filesystem) {

	/*  Load Programs.  */
	const char *debug_vertex_aabb_path = "Shaders/debug/debug_drawer_aabb.vert.spv";
	const char *debug_fragment_aabb_path = "Shaders/debug/debug_drawer_aabb.frag.spv";

	const char *debug_vertex_line_path = "Shaders/debug/debug_drawer_line.vert.spv";
	const char *debug_fragment_line_path = "Shaders/debug/debug_drawer_line.frag.spv";

	/*	*/
	fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
	compilerOptions.target = fragcore::ShaderLanguage::GLSL;
	compilerOptions.glslVersion = 330;

	const std::vector<uint32_t> vertex_debug_aabb_binary =
		fragcore::IOUtil::readFileData<uint32_t>(debug_vertex_aabb_path, filesystem);
	const std::vector<uint32_t> fragment_debug_aabb_binary =
		fragcore::IOUtil::readFileData<uint32_t>(debug_fragment_aabb_path, filesystem);

	ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_debug_aabb_binary, &fragment_debug_aabb_binary);

	/*  Setup buffers.  */
	for (unsigned int index = 0; index < (int)DrawType::MAX_DRAW_TYPE; index++) {
		this->commands[index] = Queue<DebugDrawCommand *>(2048);
	}

	/*	*/
	this->ubo_pool = CommonUtil::createBufferPool(GL_UNIFORM_BUFFER, static_cast<size_t>(1024 * 1024 * 2 * 8));

	/*	Load Meshes.	*/
}

void GLDebugDrawManager::draw(Camera *camera, FrameBuffer *frame) {

	this->updateBuffers();

	int width = 0;
	int height = 0;
	/*  */
	// glViewport(0, 0, width, height);

	/*	*/
	for (size_t i = 0; i < this->commands.size(); i++) {

		const Queue<DebugDrawCommand *> &commandQueue = this->commands[i];
		const size_t nrCommands = commandQueue.getSize();
		const MeshObject &mesh = this->debugGeometrys[i];

		/*	Bind Uniform Data.	*/

		/*	*/
		if (nrCommands > 0) {

			/*	Bind Mesh.	*/

			glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.nrIndicesElements, nrCommands);
		}
	}
	/*  */

	this->stackAllocator.clear();
}

void GLDebugDrawManager::updateBuffers() {

	/*	Transfer data.	*/
	for (size_t i = 0; i < this->commands.size(); i++) {
		const Queue<DebugDrawCommand *> &commandQueue = this->commands[i];
		const size_t nrCommands = commandQueue.getSize();

		if (nrCommands <= 0) {
		}

		DebugData *getDebugData = &this->StageBuffers.getBuffer(0);

		for (size_t j = 0; j < commandQueue.getSize(); j++) {
			const DebugDrawCommand *command = commandQueue[j];
			getDebugData->color = command->color;

			switch (command->type) {
			case DrawType::LINE:
				break;
			case DrawType::CROSS:
				break;
			case DrawType::SPHERE:
				break;
			case DrawType::CIRCLE:
				break;
			case DrawType::TRIANGLE:
				break;
			case DrawType::AABB:

				break;
			case DrawType::OBB:
				break;
			case DrawType::MAX_DRAW_TYPE:
				break;
			}
		}

		/*	Update Draw Parameters.	*/
	}
}