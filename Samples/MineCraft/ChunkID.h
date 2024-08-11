#pragma once

/**
 * @brief 
 * 
 */
class ChunkID {
  public:
	int X;
	int Y;
	int Z;

	ChunkID(int x, int y, int z) {
		this->X = x;
		this->Y = y;
		this->Z = z;
	}
	friend bool operator==(ChunkID &a, ChunkID &b) { return a.X == b.X && a.Y == b.Y && a.Z == b.Z; }
	unsigned int GetHashCode() const {

		unsigned hashCode = X;
		hashCode = (hashCode * 397) ^ Y;
		hashCode = (hashCode * 397) ^ Z;
		return hashCode;
	}

	friend bool operator!=(ChunkID &left, ChunkID &right) { return !(left == right); }

	static ChunkID FromWorldPos(const int x, const int y, const int z) { return ChunkID(x >> 4, y >> 4, z >> 4); }
};
