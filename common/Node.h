#pragma once

#include "Core/Object.h"
#include "DataStructure/ITree.h"
#include <Math3D/Transform.h>

namespace glsample {

	class Node : public fragcore::Transform, public fragcore::ITree<Node>, public fragcore::Object {
	  public:
		~Node() override = default;
	};

} // namespace glsample