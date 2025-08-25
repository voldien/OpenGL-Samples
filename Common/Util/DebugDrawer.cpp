#include "DebugDrawer.h"

#include "Common.h"
#include "DataStructure/StackBufferedAllocator.h"
#include "GLSampleSession.h"
#include "IO/IFileSystem.h"

#include <ShaderLoader.h>
#include <cstddef>

using namespace glsample;

DebugDrawManager::DebugDrawManager(fragcore::IFileSystem *filesystem)
	: stackAllocator(StackBufferedAllocator(static_cast<size_t>(1024 * 1024))) {

	/*  Load Programs.  */
	const char *debug_vertex_aabb_path = "Shaders/debug/debug_drawer_aabb.vert.spv";
	const char *debug_fragment_aabb_path = "Shaders/debug/debug_drawer_aabb.frag.spv";

	/*	*/
	fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
	compilerOptions.target = fragcore::ShaderLanguage::GLSL;
	compilerOptions.glslVersion = 430;

	const std::vector<uint32_t> vertex_debug_aabb_binary =
		fragcore::IOUtil::readFileData<uint32_t>(debug_vertex_aabb_path, filesystem);
	const std::vector<uint32_t> fragment_debug_aabb_binary =
		fragcore::IOUtil::readFileData<uint32_t>(debug_fragment_aabb_path, filesystem);

	ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_debug_aabb_binary, &fragment_debug_aabb_binary);

	/*  Setup buffers.  */
	for (unsigned int index = 0; index < (int)DrawType::MAX_DRAW_TYPE; index++) {
		this->commands[index] = Queue<DebugDrawCommand *>(2048);
	}

	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint *)&this->ubo_pool.buffer.alignment);
	this->ubo_pool.buffer.size = 1024 * 1024 * 2 * 8;
	this->ubo_pool.buffer.totalSize =
		fragcore::Math::align<size_t>(this->ubo_pool.buffer.size, (size_t)this->ubo_pool.buffer.alignment);
	this->ubo_pool.addresser = MemoryAddress(this->ubo_pool.buffer.totalSize, 0);

	/*	*/
	glGenBuffers(1, &this->ubo_pool.buffer.buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->ubo_pool.buffer.buffer);
	glBufferData(GL_UNIFORM_BUFFER, this->ubo_pool.buffer.totalSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void DebugDrawManager::draw(Camera *camera, FrameBuffer *frame) {

	/*	Transfer data.	*/
	for (size_t i = 0; i < this->commands.size(); i++) {
		const Queue<DebugDrawCommand *> &commandQueue = this->commands[i];
		const size_t nrCommands = commandQueue.getSize();
	}

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
			glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.nrIndicesElements, nrCommands);
		}
	}
	/*  */

	this->stackAllocator.clear();
}
void DebugDrawManager::addLine(const glm::vec3 &start, const glm::vec3 &end, const Color &color, float lineWidth,
							   float duration, bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->command.line.start = glm::vec4(start, 1);
	command->command.line.end = glm::vec4(end, 1);
	command->color = color;

	command->type = DrawType::LINE;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addCross(const glm::vec3 &position, const Color &color, float size, float duration,
								bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::CROSS;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addSphere(const Color &position, float radius, const Color &color, float duration,
								 bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::SPHERE;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addCircle(const glm::vec3 &centerPosition, const glm::vec3 &planeNormal, float radius,
								 const Color &color, float duration, bool depthEnabled) {}

void DebugDrawManager::addTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float duration,
								   bool depthEnabled) {}

void DebugDrawManager::addAABB(const AABB &aabb, const Color &color, float duration, bool depthEnabled) {
	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::AABB;
	this->commands[(int)command->type].enqueue(command);
}

void DebugDrawManager::addOBB(const OBB &obb, const Color &color, float duration, bool depthEnabled) {

	DebugDrawCommand *command = allocCommand();
	command->type = DrawType::OBB;
	this->commands[(int)command->type].enqueue(command);
}

DebugDrawManager::DebugDrawCommand *DebugDrawManager::allocCommand() {
	return (DebugDrawCommand *)this->stackAllocator.fetch(sizeof(DebugDrawCommand));
}
