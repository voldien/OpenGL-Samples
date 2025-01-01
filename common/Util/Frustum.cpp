#include "Util/Frustum.h"

namespace glsample {

	Frustum::Frustum(const Frustum &other) : Camera(other) {}

	void Frustum::calcFrustumPlanes(const Vector3 &position, const Vector3 &look_forward, const Vector3 &up,
									const Vector3 &right) {

		/*	*/
		const float halfVSide = this->getFar() * ::tanf(Math::degToRad(this->getFOV()) * 0.5f);
		const float halfHSide = halfVSide * this->getAspect();

		/*	*/
		const Vector3 farDistance = this->getFar() * look_forward;

		/*	*/
		this->planes[NEAR_PLANE] = {position + this->getNear() * look_forward, look_forward};
		this->planes[FAR_PLANE] = {position + farDistance, -look_forward};

		this->planes[RIGHT_PLANE] = {position, (farDistance - right * halfHSide).cross(up)};
		this->planes[LEFT_PLANE] = {position, up.cross(farDistance + right * halfHSide)};

		this->planes[TOP_PLANE] = {position, right.cross(farDistance - up * halfVSide)};
		this->planes[BOTTOM_PLANE] = {position, (farDistance + up * halfVSide).cross(right)};
	}

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