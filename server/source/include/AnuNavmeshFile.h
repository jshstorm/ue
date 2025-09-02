#pragma once

#define ANUNAVMESH_VERSION 2

#pragma pack(push, 1)

struct AnuNavmeshFileDataInfo
{
	char	agentName_[32];
	float	extend_[3];
	float	agentRadius_;
	float	agentHeight_;
	float	agentStepHeight_;
	float	navWalkingSearchHeightScale_;
	float	minBoundingBox[3];
	float	maxBoundingBox[3];
	unsigned char bCanCrouch_;
	unsigned char bCanJump_;
	unsigned char bCanWalk_;
	unsigned char bCanSwim_;
	unsigned char bCanFly_;
	unsigned int  tileCount_;
	unsigned int  dataOffset_;
	unsigned int  dataSize_;
};

struct AnuNavmeshFileHeader
{
	unsigned short int version_;
	unsigned short int dataCount_;
};

// AnuNavmeshFileHeader
// AnuNavmeshAgent * agentCount_
// AnuNavmeshData * agentCount_

#pragma pack(pop)