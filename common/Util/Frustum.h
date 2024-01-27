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
#include <Core/math3D/Plane.h>
#include <FragCore.h>

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
			TOPP = 0,	 /*	*/
			BOTTOMP = 1, /*	*/
			LEFTP = 2,	 /*	*/
			RIGHTP = 3,	 /*	*/
			NEARP = 4,	 /*	*/
			FARP = 5,	 /*	*/
		};

		Plane<float> &getPlane(int index) { return this->planes[index]; }
		const Plane<float> &getPlane(int index) const { return this->planes[index]; }

		/**
		 *	Update camera matrix perspective.
		 */
		virtual void updatePerspective();

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
		virtual Intersection intersectionAABB(const Vector3 &min, const Vector3 &max) noexcept;

		virtual Intersection intersectionAABB(const AABB &bounds) noexcept;

		virtual Intersection intersectionOBB(const Vector3 &u, const Vector3 &v, const Vector3 &w) noexcept;
		virtual Intersection intersectionOBB(const OBB &obb) noexcept;

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
		Frustum();
		Frustum(const Frustum &other);

	  private:					/*	Attributes.	*/
		Plane<float> planes[6]; /*	*/
		float tang;				/*	*/
		float nw, nh, fh, fw;	/*	*/
	};

	Frustum::Frustum() { /*  Default frustum value.  */
	}

	Frustum::Frustum(const Frustum &other) { /**this = other;*/
	}

	void Frustum::updatePerspective() {
		// this->percmatrix = Matrix4x4::perspective(this->getFov(), this->getRatio(), this->getNear(), this->getFar());

		/*	*/
		// calcFrustumPlanes(transform.getPosition(), transform.getRotation().getVector(),
		//                   transform.getRotation().getVector(Vector3::up()),
		//                   transform.getRotation().getVector(Vector3::right()));
	}

	void Frustum::calcFrustumPlanes(const Vector3 &position, const Vector3 &look, const Vector3 &up,
									const Vector3 &right) {
		Vector3 dir, nc, fc, X, Y, Z;

		planes[NEARP].setNormalAndPoint(-Z, nc);
		planes[FARP].setNormalAndPoint(Z, fc);

		Vector3 aux, normal;

		aux = (nc + Y * nh) - position;
		aux.normalize();
		normal = aux.cross(X);
		planes[TOPP].setNormalAndPoint(normal, nc + Y * nh);

		aux = (nc - Y * nh) - position;
		aux.normalize();
		normal = X.cross(aux);
		planes[BOTTOMP].setNormalAndPoint(normal, nc - Y * nh);

		aux = (nc - X * nw) - position;
		aux.normalize();
		normal = aux.cross(Y);
		planes[LEFTP].setNormalAndPoint(normal, nc - X * nw);

		aux = (nc + X * nw) - position;
		aux.normalize();
		normal = Y.cross(aux);
		planes[RIGHTP].setNormalAndPoint(normal, nc + X * nw);
	}

	Frustum::Intersection Frustum::checkPoint(const Vector3 &pos) const noexcept {
		int x;

		/*	Iterate through each plane.	*/
		for (x = 0; x < 6; x++) {
			if (planes[x].distance(pos) < 0) {
				return Out;
			}
		}
		return In;
	}

	Frustum::Intersection Frustum::intersectionAABB(const Vector3 &min, const Vector3 &max) noexcept {

		Frustum::Intersection result = Frustum::In;
		int i;

		for (i = 0; i < 6; i++) {

			Vector3 p = min;
			Vector3 n = max;
			Vector3 N = planes[i].getNormal();

			if (N.x() >= 0) {
				p[0] = max.x();
				n[0] = min.x();
			}
			if (N.y() >= 0) {
				p[1] = max.y();
				n[1] = min.y();
			}
			if (N.z() >= 0) {
				p[2] = max.z();
				n[2] = min.z();
			}

			if (planes[i].distance(p) < 0.0f) {
				return Out;
			} else if (planes[i].distance(n) < 0.0f) {
				result = Intersection::Intersect;
			}
		}

		return result;
	}

	Frustum::Intersection Frustum::intersectionAABB(const AABB &bounds) noexcept {
		return In;
		return intersectionAABB(bounds.min(), bounds.max());
	}

	Frustum::Intersection Frustum::intersectionOBB(const Vector3 &u, const Vector3 &v, const Vector3 &w) noexcept {
		return Out;
	}
	Frustum::Intersection Frustum::intersectionOBB(const OBB &obb) noexcept { return Out; }

	Frustum::Intersection Frustum::intersectionSphere(const Vector3 &pos, float radius) const noexcept { return Out; }

	Frustum::Intersection Frustum::intersectionSphere(const BoundingSphere &sphere) const noexcept { return Out; }

	Frustum::Intersection Frustum::intersectPlane(const Plane<float> &plane) const noexcept { return In; }

	Frustum::Intersection Frustum::intersectionFrustum(const Frustum &frustum) const noexcept { return In; }
} // namespace glsample