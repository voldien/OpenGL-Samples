#include "DebugDrawer.h"
#include "DataStructure/StackBufferedAllocator.h"
#include "GLSampleSession.h"
#include "IO/IFileSystem.h"

using namespace glsample;

DebugDrawManager::DebugDrawManager(fragcore::IFileSystem *filesystem)
	: stackAllocator(StackBufferedAllocator(1024 * 1024)) {

	/*  Load Programs.  */
	const char *guassian_vertical_blur_compute_path = "Shaders/postprocessingeffects/guassian_blur_vertical.comp.spv";
	const char *guassian_horizontal_blur_compute_path =
		"Shaders/postprocessingeffects/guassian_blur_horizontal.comp.spv";

	const char *box_blur_compute_path = "Shaders/postprocessingeffects/box_blur.comp.spv";

	/*  Setup buffers.  */
	for (unsigned int index = 0; index < (int)DrawType::MAX_DRAW_TYPE; index++) {
		this->commands[index] = Queue<DebugDrawCommand *>();
	}
}

void DebugDrawManager::draw(FrameBuffer *frame) {

	/*	Transfer data.	*/

	int width = 0;
	int height = 0;
	/*  */
	glViewport(0, 0, width, height);

	for (size_t i = 0; i < this->commands.size(); i++) {
		if (this->commands[i].getSize() > 0) {
			
		}
	}
	/*  */
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
