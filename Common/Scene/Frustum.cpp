#include "Scene/Frustum.h"

namespace glsample {

	Frustum::Frustum(const Frustum &other) : Node(other) {}

	void Frustum::calcFrustumPlanes(const glm::vec3 &position, const glm::vec3 &look_forward, const glm::vec3 &up,
									const glm::vec3 &right) noexcept {}

	Frustum::Intersection Frustum::checkPoint(const glm::vec3 &pos) const noexcept {

		/*	Iterate through each plane.	*/
		for (unsigned int plane_index = 0; plane_index < this->getNrPlanes(); plane_index++) {
			if (fragcore::GeometryUtility::testPlanesPoint(this->getPlane(plane_index), pos)) {
				return Intersection::Out;
			}
		}
		return In;
	}

	Frustum::Intersection Frustum::intersectionAABB(const glm::vec3 &min, const glm::vec3 &max) const noexcept {
		return Frustum::intersectionAABB(AABB::createMinMax(min, max));
	}

	Frustum::Intersection Frustum::intersectionAABB(const AABB &bounds) const noexcept {

		/*	Iterate through each plane.	*/
		for (unsigned int index_plane = 0; index_plane < this->getNrPlanes(); index_plane++) {

			if (!fragcore::GeometryUtility::testPlanesAABB(this->getPlane(index_plane), bounds)) {
				return Intersection::Out;
			}
		}

		return Frustum::In;
	}

	Frustum::Intersection Frustum::intersectionOBB(const glm::vec3 &u, const glm::vec3 &v,
												   const glm::vec3 &w) const noexcept {

		return Intersection::Out;
	}
	Frustum::Intersection Frustum::intersectionOBB(const OBB &obb) const noexcept { return Out; }

	Frustum::Intersection Frustum::intersectionSphere(const glm::vec3 &position, float radius) const noexcept {
		return Frustum::intersectionSphere(BoundingSphere(position, radius));
	}

	Frustum::Intersection Frustum::intersectionSphere(const BoundingSphere &sphere) const noexcept {

		/*	Iterate through each plane.	*/
		for (unsigned int index_plane = 0; index_plane < this->getNrPlanes(); index_plane++) {

			if (!fragcore::GeometryUtility::testPlanesSphere(this->getPlane(index_plane), sphere)) {
				return Intersection::Out;
			}
		}
		return Intersection::In;
	}

	Frustum::Intersection Frustum::intersectPlane(const Plane<float> &plane) const noexcept { return Intersection::In; }

	Frustum::Intersection Frustum::intersectionFrustum(const Frustum &frustum) const noexcept {
		/*	Iterate through each plane.	*/

		for (unsigned int index_plane0 = 0; index_plane0 < frustum.getNrPlanes(); index_plane0++) {

			for (unsigned int index_plane = 0; index_plane < this->getNrPlanes(); index_plane++) {

				if (!fragcore::GeometryUtility::testPlanesPlane(this->getPlane(index_plane),
																frustum.getPlane(index_plane0))) {
					return Intersection::Out;
				}
			}
		}
		return Intersection::In;
	}

} // namespace glsample