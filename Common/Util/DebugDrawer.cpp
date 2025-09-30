#include "DebugDrawer.h"

#include "Common.h"
#include "DataStructure/StackBufferedAllocator.h"
#include "GLSampleSession.h"
#include "IO/IFileSystem.h"
#include "SampleHelper.h"

#include <ShaderLoader.h>
#include <cstddef>

using namespace glsample;

DebugDrawManager::DebugDrawManager(fragcore::IFileSystem *filesystem)
	: stackAllocator(StackBufferedAllocator(static_cast<size_t>(1024 * 1024))) {

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

void DebugDrawManager::draw(Camera *camera, FrameBuffer *frame) {

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
void DebugDrawManager::reset() {
	for (unsigned int index = 0; index < (int)DrawType::MAX_DRAW_TYPE; index++) {
		this->commands[index].clear();
	}
}

void DebugDrawManager::updateBuffers() {

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

void DebugDrawManager::addLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec4 &color, float lineWidth,
							   float duration, bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();

	command->command.line.start = glm::vec4(start, 1);
	command->command.line.end = glm::vec4(end, 1);
	command->color = color;

	command->type = DrawType::LINE;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addCross(const glm::vec3 &position, const glm::vec4 &color, float size, float duration,
								bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::CROSS;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addSphere(const glm::vec4 &position, float radius, const glm::vec4 &color, float duration,
								 bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::SPHERE;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addCircle(const glm::vec3 &centerPosition, const glm::vec3 &planeNormal, float radius,
								 const glm::vec4 &color, float duration, bool depthEnabled) {}

void DebugDrawManager::addTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float duration,
								   bool depthEnabled) {}

void DebugDrawManager::addAABB(const AABB &aabb, const glm::vec4 &color, float duration, bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::AABB;
	command->color = color;

	command->command.aabb.pos = glm::vec4(E2GLM(aabb.getCenter()), 1.0f);
	command->command.aabb.size = glm::vec4(E2GLM(aabb.getHalfSize()), 1.0f);

	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addOBB(const OBB &obb, const glm::vec4 &color, float duration, bool depthEnabled) {

	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::OBB;
	command->color = color;
	this->commands[(int)command->type].enqueue(command);
}

DebugDrawManager::DebugDrawCommand *DebugDrawManager::allocCommand() {
	return (DebugDrawCommand *)this->stackAllocator.fetch(sizeof(DebugDrawCommand));
}
