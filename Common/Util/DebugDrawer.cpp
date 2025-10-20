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
	: stackAllocator(StackBufferedAllocator(static_cast<size_t>(1024 * 1024))) {}

void DebugDrawManager::reset() {
	for (unsigned int index = 0; index < (int)DrawType::MAX_DRAW_TYPE; index++) {
		this->commands[index].clear();
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

	command->command.aabb.pos = glm::vec4(aabb.getCenter(), 1.0f);
	command->command.aabb.size = glm::vec4(aabb.getHalfSize(), 1.0f);

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
