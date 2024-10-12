#pragma once
#include <Core/Ref.h>
#include <IO/IO.h>

class Chunk {
  public:
	Chunk(int x, int y, int z) {}

	int getData(int x, int y, int z) { return 0; }

	void writeToIO(fragcore::Ref<fragcore::IO> &io);

  private:
	void *voxel;
};
