#include "Util/Frustum.h"

namespace glsample {

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