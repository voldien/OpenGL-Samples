#pragma once

#include "Core/Object.h"
#include "DataStructure/ITree.h"
#include <Math3D/Transform.h>

namespace glsample {

	class Node : public fragcore::Transform, fragcore::ITree<Node>, fragcore::Object {
	  public:
		 ~Node() override = default;
	};

} // namespace glsample