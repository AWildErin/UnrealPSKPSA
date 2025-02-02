#pragma once
#include <fstream>

#define CHUNK(ChunkName) (strncmp(Chunk.ChunkID, ChunkName, strlen(ChunkName)) == 0)

struct VChunkHeader
{
	char ChunkID[20];
	int TypeFlag;
	int DataSize;
	int DataCount;
};

struct VVertex
{
	int PointIndex;
	float U, V;
	char MatIndex;
	char Reserved;
	short Pad;
};

struct VTriangle
{
	int WedgeIndex[3];
	char MatIndex;
	char AuxMatIndex;
	unsigned SmoothingGroups;
};

struct VMaterial
{
	char MaterialName[64];
	int TextureIndex;
	unsigned PolyFlags;
	int AuxMaterial;
	unsigned AuxFlags;
	int LodBias;
	int LodStyle;
};

struct VJointPos
{
	FQuat4f Orientation;
	FVector3f Position;
	float Length;
	float XSize;
	float YSize;
	float ZSize;
};

struct VNamedBoneBinary
{
	char Name[64];
	int Flags;
	int NumChildren;
	int ParentIndex;
	VJointPos BonePos;
};

struct VRawBoneInfluence
{
	float Weight;
	int PointIdx;
	int BoneIdx;
};

struct Socket
{
	FString SocketName;
	FString BoneName;
	FVector RelativeLocation;
	FRotator RelativeRotation;
	FVector RelativeScale;
};

class PSKReader
{
	
public:
	PSKReader(const FString Filename, bool bLoadPropertiesFile = false);
	bool Read();

	// Switches
	bool bHasVertexNormals;
	bool bHasVertexColors;
	bool bHasExtraUVs;
	bool bLoadProperties;

	// PSKX
	TArray<FVector3f> Vertices;
	TArray<VVertex> Wedges;
	TArray<VTriangle> Faces;
	TArray<VMaterial> Materials;
	TArray<FVector3f> Normals;
	TArray<FColor> VertexColors;
	TArray<TArray<FVector2f>> ExtraUVs;

	// PSK
	TArray<VNamedBoneBinary> Bones;
	TArray<VRawBoneInfluence> Influences;

	// UModel Properties
	TArray<Socket> Sockets;

private:
	/** Handles loading UModel property files */
	bool ReadPropertiesFile();

	FString FileName;
	bool CheckHeader(const VChunkHeader Header) const;
	const char* HeaderBytes = "ACTRHEAD" + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00;
	std::ifstream Ar;
	
};
