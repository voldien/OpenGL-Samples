/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
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
#pragma once

#include "Common.h"
#include "DataStructure/StackBufferedAllocator.h"
#include "Importer/ModelImporter.h"
#include "SampleHelper.h"

namespace glsample {
	/**
	 *
	 */
	class FVDECLSPEC DebugDrawManager : public fragcore::Object {
	  public:
		DebugDrawManager(fragcore::IFileSystem *filesystem);
		~DebugDrawManager() override = default;

		// TODO reduce argument.
		void draw(Camera *camera, FrameBuffer *frame);
		//	RenderQueue getSupportedQueue() const override; /*  Render as overlay only. */

		virtual void reset() {}

		virtual void addLine(const glm::vec3 &start, const glm::vec3 &end, const Color &color, float lineWidth = 1.0f,
							 float duration = 0.0f, bool depthEnabled = true);

		virtual void addCross(const glm::vec3 &position, const Color &color, float size, float duration = 0.0f,
							  bool depthEnabled = true);

		virtual void addSphere(const Color &position, float radius, const Color &color, float duration = 0.0f,
							   bool depthEnabled = true);

		virtual void addCircle(const glm::vec3 &centerPosition, const glm::vec3 &planeNormal, float radius,
							   const Color &color, float duration = 0.0f, bool depthEnabled = true);

		virtual void addTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float duration = 0.0f,
								 bool depthEnabled = true);

		virtual void addAABB(const AABB &aabb, const Color &color, float duration = 0.0f, bool depthEnabled = true);

		virtual void addOBB(const OBB &obb, const Color &color, float duration = 0.0f, bool depthEnabled = true);

	  protected:
	  private:
		enum class DrawType { LINE, CROSS, SPHERE, CIRCLE, TRIANGLE, AABB, OBB, MAX_DRAW_TYPE };

		using DebugDrawCommand = struct DebugDrawCommand_t {
			DrawType type;		   /*  */
			bool depthEnabled{};   /*  */
			float timeRemaining{}; /*  */
			float invokeTime{};	   /*  */
			Color color;		   /*	Common Color attribute.	*/

			union Command {
				struct {
					glm::vec4 start;
					glm::vec4 end;
				} line;

				struct {
					glm::vec4 pos;
				} cross;

				struct {
					glm::vec4 pos;
				} sphere;

				struct {
					glm::vec4 pos;
				} cirlcle;

				struct {
					glm::vec4 pos;
				} axes;

				struct {
					glm::vec4 a, b, c;
				} triangle;

				struct {
					glm::vec4 pos;
				} aabb;

				struct {
					glm::vec4 a, b, c;
				} obb;
				struct {
					glm::vec4 pos;
					int textIndex;
				} string;
			};

			Command command{};
		};

		DebugDrawCommand *allocCommand();

		using DebugData = struct debug_data_t {
			glm::mat4 model;
			glm::vec4 color;
		};

		std::map<unsigned int, Queue<DebugDrawCommand *>> commands; /*  */
		StackBufferedAllocator stackAllocator;
		std::vector<MeshObject> debugGeometrys; /*  Geometry of the debug objects. - multiple sub geometries.   */
		MaterialObject material;

		StageBuffer<DebugData, 3> StageBuffers;
		UBOPool ubo_pool;
	};
} // namespace glsample
