#pragma once

#pragma pack(push, 1)

struct AnuNavVoxelFileHeader
{
	float locationX;
	float locationY;
	float locationZ;
	unsigned int cellSize;
	unsigned int gridSizeX;
	unsigned int gridSizeY;
	unsigned int gridSizeZ;
	unsigned int graphCount;
	unsigned int voxelCount;
};

struct AnuNavVoxelGraphHeader
{
	unsigned int uid;
	unsigned int voxelCount;
};

struct AnuNavVoxelFileData
{
	constexpr static float Stride = 0.2f;
	constexpr static float Multiplier = 1.0f / Stride;

	unsigned short x;
	unsigned short y;
	unsigned short z;
	short height;
	unsigned char mask;

	void PushHeight(float value)
	{
		height = (short)(value * Multiplier);
	}

	float PopHeight() const
	{
		return (float)height * Stride;
	}
};

#pragma pack(pop)