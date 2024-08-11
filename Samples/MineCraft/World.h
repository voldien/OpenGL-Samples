#pragma once
#include "Chunk.h"
#include "ChunkID.h"
#include <map>
#include <vector>

class World {
  public:
	void getChunk(std::vector<Chunk> &chunks);

	uint32_t getChunkID(int x, int y, int z) {
		// Chunk& chunk = this->Chunks[ChunkID::FromWorldPos(x, y, z).GetHashCode()];
		// return chunk.getData(x, y, z); // [x & 0xF, y & 0xF, z & 0xF];
		return 0;
	}
	
	// uint32_t operator[](int x, int y, int z) {
	//
	//	var chunk = Chunks[ChunkId.FromWorldPos(x, y, z)];
	//	return chunk[x & 0xF, y & 0xF, z & 0xF];
	//
	//	// set
	//	//{
	//	//    var chunk = Chunks[ChunkId.FromWorldPos(x, y, z)];
	//	//    chunk[x & 0xF, y & 0xF, z & 0xF] = value;
	//	//}
	//}

  private:
	std::map<unsigned int, Chunk> Chunks;
};
