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
#include "Node.h"
#include <FragCore.h>
#include <GeometryUtil.h>
#include <Math3D/BoundingSphere.h>
#include <Math3D/Plane.h>

namespace glsample {

	using namespace fragcore;

	/**
	 * @brief Positive
	 *
	 */
	class FVDECLSPEC Frustum : public Node {
	  public:
		~Frustum() override = default;

		/*	*/
		enum Intersection {
			Out = 0,	   /*	Outside frustum completely.*/
			In = 1,		   /*	Inside frustum completely.*/
			Intersect = 2, /*	Intersecting frustum planes.	*/
		};

		/**
		 *
		 */
		enum FrustumPlane : unsigned int {
			TOP_PLANE = 0,	  /*	*/
			BOTTOM_PLANE = 1, /*	*/
			LEFT_PLANE = 2,	  /*	*/
			RIGHT_PLANE = 3,  /*	*/
			NEAR_PLANE = 4,	  /*	*/
			FAR_PLANE = 5,	  /*	*/
			NPLANES = 6
		};

		/**
		 * @brief Get the Plane object
		 */
		Plane<float> &getPlane(const int index) { return this->planes[index]; }
		const Plane<float> &getPlane(const int index) const { return this->planes[index]; }
		unsigned int getNrPlanes() const noexcept { return NPLANES; }

		/**
		 *	Comput the frustum planes,
		 *	planes normal pointing positive towards the frustum volume.
		 */
		virtual void calcFrustumPlanes(const glm::vec3 &position, const glm::vec3 &look_forward, const glm::vec3 &up,
									   const glm::vec3 &right);

		/**
		 *	Check if point is inside the frustum.
		 *	@Return eIn if inside frustum, eOut otherwise.
		 */
		virtual Intersection checkPoint(const glm::vec3 &pos) const noexcept;

		/**
		 * Check if AABB intersects frustum.
		 * @param min
		 * @param max
		 * @return eIntersect if intersect the frustum, eOut otherwise.
		 */
		virtual Intersection intersectionAABB(const glm::vec3 &min, const glm::vec3 &max) const noexcept;

		virtual Intersection intersectionAABB(const AABB &bounds) const noexcept;

		virtual Intersection intersectionOBB(const glm::vec3 &u, const glm::vec3 &v, const glm::vec3 &w) const noexcept;
		virtual Intersection intersectionOBB(const OBB &obb) const noexcept;

		/**
		 *	Check if sphere intersects frustum.
		 *	@Return
		 */
		virtual Intersection intersectionSphere(const glm::vec3 &position, float radius) const noexcept;
		virtual Intersection intersectionSphere(const BoundingSphere &sphere) const noexcept;

		/**
		 *	Check if plane intersects frustum.
		 *	@Return
		 */
		virtual Intersection intersectPlane(const Plane<float> &plane) const noexcept;

		/**
		 *	Check if frustum intersects frustum.
		 *	@Return
		 */
		virtual Intersection intersectionFrustum(const Frustum &frustum) const noexcept;

	  protected: /*	Makes the object only inheritable .	*/
		Frustum() = default;
		Frustum(const Frustum &other);

	  protected:				/*	Attributes.	*/
		Plane<float> planes[6]; /*	*/
	};

} // namespace glsample
