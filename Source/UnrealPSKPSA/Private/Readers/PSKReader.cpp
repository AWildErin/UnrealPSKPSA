#include "Readers/PSKReader.h"

PSKReader::PSKReader(const FString Filename, bool bLoadPropertiesFile /*= false*/)
{
	Ar.open(ToCStr(Filename), std::ios::binary);
	FileName = Filename;
	bLoadProperties = bLoadPropertiesFile;
}

bool PSKReader::Read()
{
	// Load properties first if we want them
	if (bLoadProperties)
	{
		if (!ReadPropertiesFile())
		{
			return false;
		}
	}

	VChunkHeader Header;
	Ar.read(reinterpret_cast<char*>(&Header), sizeof(VChunkHeader));

	if (!CheckHeader(Header))
		return false;

	VChunkHeader Chunk;
	while (!Ar.eof())
	{
		Ar.read(reinterpret_cast<char*>(&Chunk), sizeof(VChunkHeader));
		const auto DataCount = Chunk.DataCount;

		if (CHUNK("PNTS0000"))
		{
			Vertices.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Vertices[i]), sizeof(FVector3f));
			}
		}
		else if (CHUNK("VTXW0000"))
		{
			Wedges.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Wedges[i]), sizeof(VVertex));
				if (DataCount <= 65536)
				{
					Wedges[i].PointIndex &= 0xFFFF;
				}
			}
		}
		else if (CHUNK("FACE0000"))
		{
			Faces.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				for (auto j = 0; j < 3; j++)
				{
					Ar.read(reinterpret_cast<char*>(&Faces[i].WedgeIndex[j]), sizeof(short));
					Faces[i].WedgeIndex[j] &= 0xFFFF;
				}
	                
				Ar.read(&Faces[i].MatIndex, sizeof(char));
				Ar.read(&Faces[i].AuxMatIndex, sizeof(char));
				Ar.read(reinterpret_cast<char*>(&Faces[i].SmoothingGroups), sizeof(unsigned));
			}
		}
		else if (CHUNK("FACE3200"))
		{
			Faces.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Faces[i].WedgeIndex), sizeof Faces[i].WedgeIndex);
				Ar.read(&Faces[i].MatIndex, sizeof(char));
                Ar.read(&Faces[i].AuxMatIndex, sizeof(char));
                Ar.read(reinterpret_cast<char*>(&Faces[i].SmoothingGroups), sizeof(unsigned));
			}
		}
		else if (CHUNK("MATT0000"))
		{
			Materials.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Materials[i]), sizeof(VMaterial));
			}
		}
		else if (CHUNK("VTXNORMS"))
		{
			Normals.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Normals[i]), sizeof(FVector3f));
			}
		}
		else if (CHUNK("VERTEXCOLOR"))
		{
			VertexColors.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&VertexColors[i]), sizeof(FColor));
			}
		}
		else if (CHUNK("EXTRAUVS"))
		{
			TArray<FVector2f> UVData;
			UVData.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&UVData[i]), sizeof(FVector2f));
			}

			ExtraUVs.Add(UVData);
		}
		else if (CHUNK("REFSKELT") || CHUNK("REFSKEL0"))
		{
			Bones.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Bones[i].Name), sizeof(Bones[i].Name));
				Ar.read(reinterpret_cast<char*>(&Bones[i].Flags), sizeof(int));
				Ar.read(reinterpret_cast<char*>(&Bones[i].NumChildren), sizeof(int));
				Ar.read(reinterpret_cast<char*>(&Bones[i].ParentIndex), sizeof(int));
				Ar.read(reinterpret_cast<char*>(&Bones[i].BonePos.Orientation), sizeof(FQuat4f));
				Ar.read(reinterpret_cast<char*>(&Bones[i].BonePos.Position), sizeof(FVector3f));
	            	
				Ar.read(reinterpret_cast<char*>(&Bones[i].BonePos.Length), sizeof(float));
				Ar.read(reinterpret_cast<char*>(&Bones[i].BonePos.XSize), sizeof(float));
				Ar.read(reinterpret_cast<char*>(&Bones[i].BonePos.YSize), sizeof(float));
				Ar.read(reinterpret_cast<char*>(&Bones[i].BonePos.ZSize), sizeof(float));
	            	
			}
		}
		else if (CHUNK("RAWWEIGHTS") || CHUNK("RAWW0000"))
		{
			Influences.SetNum(DataCount);
			for (auto i = 0; i < DataCount; i++)
			{
				Ar.read(reinterpret_cast<char*>(&Influences[i]), sizeof(VRawBoneInfluence));
			}
		}
		else
		{
			Ar.ignore(Chunk.DataSize*DataCount); 
		}
	}

	bHasVertexNormals = Normals.Num() > 0;
	bHasVertexColors = VertexColors.Num() > 0;
	bHasExtraUVs = ExtraUVs.Num() > 0;
	Ar.close();
	return true;
}


/// @todo Move to a separate class as PSA can make use of this too
/// @todo I'm not a fan of using std stuff, it's generally frowned upon as per the UE code guide
bool PSKReader::ReadPropertiesFile()
{
	FString PropsFile = FPaths::ChangeExtension(FileName, "props.txt");

	UE_LOG(LogTemp, Log, TEXT("Loading properties: %s"), *PropsFile);

	if (!FPaths::FileExists(PropsFile))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to open properties file at: %s"), *PropsFile);
		return false;
	}

	std::ifstream PropStream;
	PropStream.open(ToCStr(PropsFile), std::ios::in);
	FRegexPattern PropArrayPattern(TEXT("(\\w+)\\[(\\d+)\\] ="));
	FRegexPattern KVPattern(TEXT("(\\w+)\\s*=\\s*(.*)"));

	// Helper lambda just to skip lines
	auto SkipLine = [](std::ifstream& Stream) { Stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); };

	char CurrentLineBuffer[512] {};
	auto ReadLine = [&](std::ifstream& Stream) {
		Stream.getline(CurrentLineBuffer, 512);
		FString Line(ANSI_TO_TCHAR(CurrentLineBuffer));
		return Line;
	};

	auto SanitiseMatch = [&](FString Line) {
		FString CleanLine = Line;
		CleanLine = CleanLine.TrimChar('{');
		CleanLine = CleanLine.TrimChar('}');
		CleanLine = CleanLine.TrimStartAndEnd();

		return CleanLine;
	};

	auto GetKeyValue = [&](FString Line) {
		FRegexMatcher Matcher(KVPattern, Line);
		Matcher.FindNext();

		FString Key = Matcher.GetCaptureGroup(1);
		FString Value = Matcher.GetCaptureGroup(2);

		check(!Key.IsEmpty() || !Value.IsEmpty());

		// Clean our Value
		Value = SanitiseMatch(Value);

		TPair<FString, FString> Return(Key, Value);
		return Return;
	};

	// This breaks for static meshes as they dont have the bone KV
	// Lets just change this to FParse
	while (!PropStream.eof())
	{
		FString Line = ReadLine(PropStream);

		FRegexMatcher ArrayMatcher(PropArrayPattern, Line);
		ArrayMatcher.FindNext();

		UE_LOG(LogTemp, Log, TEXT("Line: %s"), *Line);

		// Indicate the start of a socket struct
		if (Line.StartsWith("Sockets["))
		{
			FString Name = ArrayMatcher.GetCaptureGroup(1);
			FString Count = ArrayMatcher.GetCaptureGroup(2);
			int32 SocketCount = FCString::Atoi(*Count);

			SkipLine(PropStream);

			for (int32 i = 0; i < SocketCount; i++)
			{
				Socket NewSocket;

				// Skip some junk we dont care aboutr
				SkipLine(PropStream);
				SkipLine(PropStream);

				auto SocketNameKV = GetKeyValue(ReadLine(PropStream));
				auto BoneNameKV = GetKeyValue(ReadLine(PropStream));
				auto LocationKV = GetKeyValue(ReadLine(PropStream));
				auto RotationKV = GetKeyValue(ReadLine(PropStream));
				auto ScaleKV = GetKeyValue(ReadLine(PropStream));

				FVector Location;
				Location.InitFromString(LocationKV.Value);

				// UModel exports these weirdly so we need to replace the keys
				FRotator Rotation;
				FString RotVal = RotationKV.Value;
				RotVal = RotVal.Replace(TEXT("Pitch"), TEXT("P"));
				RotVal = RotVal.Replace(TEXT("Yaw"), TEXT("Y"));
				RotVal = RotVal.Replace(TEXT("Roll"), TEXT("R"));

				Rotation.InitFromString(RotVal);

				FVector Scale;
				Scale.InitFromString(ScaleKV.Value);

				NewSocket.SocketName = SocketNameKV.Value;
				NewSocket.BoneName = BoneNameKV.Value;
				NewSocket.RelativeLocation = Location;
				NewSocket.RelativeRotation = Rotation;
				NewSocket.RelativeScale = Scale;
				
				Sockets.Add(NewSocket);

				SkipLine(PropStream);
			}
		}
	}

	return true;
}

bool PSKReader::CheckHeader(const VChunkHeader Header) const
{
	return std::strcmp(Header.ChunkID, HeaderBytes) == 0;
}


