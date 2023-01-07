#pragma once
#include "World.h"

class WorldBuilder {
  public:
	WorldBuilder(int x, int y, int z) {}

	World *buildWorld(){
		return new World();
	}
};
