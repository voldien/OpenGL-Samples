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

	/*
	 *
	 */
	class FVDECLSPEC Frustum : public Camera<float> {
	  public:
		/**
		 *	Intersection.
		 */
		enum Intersection : unsigned int {
			Out = 0,	   /*	Outside frustum completly.*/
			In = 1,		   /*	Inside frustum completly.*/
			Intersect = 2, /*	Intresecting frustum planes.	*/
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
		 *	Comput the frustum planes.
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

	Frustum::Frustum() noexcept { /*  Default frustum value.  */
	}

	Frustum::Frustum(const Frustum &other) noexcept { /**this = other;*/
	}

	void Frustum::calcFrustumPlanes(const Vector3 &position, const Vector3 &look, const Vector3 &up,
									const Vector3 &right) {

		const float halfVSide = this->getFar() * tanf(this->getFOV() * .5f);
		const float halfHSide = halfVSide * getAspect();

		Vector3 frontMultFar = this->getFar() * look;

		this->planes[NEAR_PLANE] = {position + this->getNear() * look, look};
		this->planes[FAR_PLANE] = {position + frontMultFar, -look};

		this->planes[RIGHT_PLANE] = {position, (frontMultFar - right * halfHSide).cross(up)};
		this->planes[LEFT_PLANE] = {position, up.cross(frontMultFar + right * halfHSide)};
		this->planes[TOP_PLANE] = {position, right.cross(frontMultFar - up * halfVSide)};
		this->planes[BOTTOM_PLANE] = {position, (frontMultFar + up * halfVSide).cross(right)};
	}

	Frustum::Intersection Frustum::checkPoint(const Vector3 &pos) const noexcept {

		/*	Iterate through each plane.	*/
		for (int x = 0; x < 6; x++) {
			if (fragcore::GeometryUtility::TestPlanesPoint(planes[x], pos)) {
				return Out;
			}
		}
		return In;
	}

	Frustum::Intersection Frustum::intersectionAABB(const Vector3 &min, const Vector3 &max) const noexcept {

		Frustum::Intersection result = Frustum::In;

		for (int i = 0; i < 6; i++) {

			if (fragcore::GeometryUtility::TestPlanesAABB(this->planes[i], AABB::createMinMax(min, max))) {
				return Out;
			}
		}

		return result;
	}

	Frustum::Intersection Frustum::intersectionAABB(const AABB &bounds) const noexcept {
		return this->intersectionAABB(bounds.min(), bounds.max());
	}

	Frustum::Intersection Frustum::intersectionOBB(const Vector3 &u, const Vector3 &v,
												   const Vector3 &w) const noexcept {
		return Out;
	}
	Frustum::Intersection Frustum::intersectionOBB(const OBB &obb) const noexcept { return Out; }

	Frustum::Intersection Frustum::intersectionSphere(const Vector3 &pos, float radius) const noexcept {
		return Frustum::intersectionSphere(BoundingSphere(pos, radius));
	}

	Frustum::Intersection Frustum::intersectionSphere(const BoundingSphere &sphere) const noexcept {
		for (int i = 0; i < 6; i++) {
			if (fragcore::GeometryUtility::TestPlanesSphere(planes[i], sphere)) {
				return Out;
			}
		}
		return In;
	}

	Frustum::Intersection Frustum::intersectPlane(const Plane<float> &plane) const noexcept { return In; }

	Frustum::Intersection Frustum::intersectionFrustum(const Frustum &frustum) const noexcept { return In; }
} // namespace glsample