/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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
#include "Camera.h"
#include "Core/math3D/BoundingSphere.h"
#include <Core/math3D/Plane.h>
#include <FragCore.h>
#include <GeometryUtil.h>

namespace glsample {

	using namespace fragcore;

	/**
	 * @brief Positive
	 *
	 */
	class FVDECLSPEC Frustum : public Camera<float> {
	  public:
		/**
		 *	Intersection.
		 */
		enum Intersection : unsigned int {
			Out = 0,	   /*	Outside frustum completly - Positive.*/
			In = 1,		   /*	Inside frustum completly - Negative.*/
			Intersect = 2, /*	Intresecting frustum planes- Partially.	*/
		};

		/**
		 *
		 */
		enum FrustumPlanes : unsigned int {
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
		 *
		 * @param index
		 * @return Plane<float>&
		 */
		Plane<float> &getPlane(int index) { return this->planes[index]; }
		const Plane<float> &getPlane(int index) const { return this->planes[index]; }

		/**
		 *	Comput the frustum planes,
		 *	planes normal pointing positive towards the frustum volume.
		 */
		virtual void calcFrustumPlanes(const Vector3 &position, const Vector3 &look, const Vector3 &up,
									   const Vector3 &right);

		/**
		 *	Check if point is inside the frustum.
		 *	@Return eIn if inside frustum, eOut otherwise.
		 */
		virtual Intersection checkPoint(const Vector3 &pos) const noexcept;

		/**
		 * Check if AABB intersects frustum.
		 * @param min
		 * @param max
		 * @return eIntersect if intersect the frustum, eOut otherwise.
		 */
		virtual Intersection intersectionAABB(const Vector3 &min, const Vector3 &max) const noexcept;

		virtual Intersection intersectionAABB(const AABB &bounds) const noexcept;

		virtual Intersection intersectionOBB(const Vector3 &u, const Vector3 &v, const Vector3 &w) const noexcept;
		virtual Intersection intersectionOBB(const OBB &obb) const noexcept;

		/**
		 *	Check if sphere intersects frustum.
		 *	@Return
		 */
		virtual Intersection intersectionSphere(const Vector3 &pos, float radius) const noexcept;
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
		Frustum() noexcept;
		Frustum(const Frustum &other) noexcept;

	  private:					/*	Attributes.	*/
		Plane<float> planes[6]; /*	*/
		float nw, nh, fh, fw;	/*	*/
	};

} // namespace glsample