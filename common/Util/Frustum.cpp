#include "Util/Frustum.h"

namespace glsample {

	Frustum::Frustum(const Frustum &other) : Node(other) {}

	void Frustum::calcFrustumPlanes(const Vector3 &position, const Vector3 &look_forward, const Vector3 &up,
									const Vector3 &right) {}

	Frustum::Intersection Frustum::checkPoint(const Vector3 &pos) const noexcept {

		/*	Iterate through each plane.	*/
		for (unsigned int x = 0; x < FrustumPlanes::NPLANES; x++) {
			if (fragcore::GeometryUtility::testPlanesPoint(this->planes[x], pos)) {
				return Intersection::Out;
			}
		}
		return In;
	}

	Frustum::Intersection Frustum::intersectionAABB(const Vector3 &min, const Vector3 &max) const noexcept {
		return Frustum::intersectionAABB(AABB::createMinMax(min, max));
	}

	Frustum::Intersection Frustum::intersectionAABB(const AABB &bounds) const noexcept {
		Frustum::Intersection result = Frustum::In;

		for (unsigned int i = 0; i < FrustumPlanes::NPLANES; i++) {

			if (!fragcore::GeometryUtility::testPlanesAABB(this->planes[i], bounds)) {
				return Intersection::Out;
			}
		}

		return result;
	}

	Frustum::Intersection Frustum::intersectionOBB(const Vector3 &u, const Vector3 &v,
												   const Vector3 &w) const noexcept {
		return Intersection::Out;
	}
	Frustum::Intersection Frustum::intersectionOBB(const OBB &obb) const noexcept { return Out; }

	Frustum::Intersection Frustum::intersectionSphere(const Vector3 &pos, float radius) const noexcept {
		return Frustum::intersectionSphere(BoundingSphere(pos, radius));
	}

	Frustum::Intersection Frustum::intersectionSphere(const BoundingSphere &sphere) const noexcept {

		for (unsigned int i = 0; i < (unsigned int)FrustumPlanes::NPLANES; i++) {
			if (!fragcore::GeometryUtility::testPlanesSphere(this->planes[i], sphere)) {
				return Intersection::Out;
			}
		}
		return Intersection::In;
	}

	Frustum::Intersection Frustum::intersectPlane(const Plane<float> &plane) const noexcept { return Intersection::In; }

	Frustum::Intersection Frustum::intersectionFrustum(const Frustum &frustum) const noexcept {
		return Intersection::In;
	}

} // namespace glsample