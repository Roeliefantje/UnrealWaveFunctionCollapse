// Fill out your copyright notice in the Description page of Project Settings.


#include "CityGenerator.h"

#include <cmath>

#include "ParticleEmitterInstances.h"
#include "WFCAlgorithm.h"
#include "Engine/StaticMeshActor.h"
#include "Serialization/AsyncPackageLoader.h"

// Sets default values
ACityGenerator::ACityGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();
	const auto Tiles = FTilesFromTileSetData();
	const int TileDim = TilesetData->TileDimensions;
	
	const int Height = GridHeight / TileDim;
	const int Width = GridWidth / TileDim;
	
	//Make sure the GridHeight and GridWith also get adjusted back so the resulting spawned grid is not wrong.
	GridHeight = Height * TileDim;
	GridWidth = Width * TileDim;
	
	auto Wfc = WFCAlgorithm(Tiles, Width, Height, TileDim);
	UE_LOG(LogTemp, Log, TEXT("Solving WFC algo"));
	auto Pixels = Wfc.Solve();
	
	
	for (int32 y = 0; y < GridHeight; y++)
	{
		FString RowString;
		for (int32 x = 0; x < GridWidth; x++)
		{
			int32 Index = y * GridWidth + x;
			// UE_LOG(LogTemp, Log, TEXT("%d "), Index);
			// Cast enum class to int32 for printing
			RowString += FString::Printf(TEXT("%d "), static_cast<int32>(Pixels[Index]));
		}
		UE_LOG(LogTemp, Log, TEXT("%s"), *RowString);
	}
	
	const std::vector<GridGroup> Groups = CreateGridGroups(Pixels);
	UE_LOG(LogTemp, Log, TEXT("Groups created: %llu"), Groups.size());
	SpawnMeshes(Groups);
}

void AddIfNotDuplicate(std::vector<WFCAlgorithm::FTile>& Vec, WFCAlgorithm::FTile Obj)
{
	for (const auto& ExistingTile : Vec)
	{
		if (ExistingTile.Pixels == Obj.Pixels)
		{
			return;
		}
	}
	
	Vec.push_back(Obj);
}

std::vector<WFCAlgorithm::FTile> ACityGenerator::FTilesFromTileSetData() const
{	
	auto FinalTiles = std::vector<WFCAlgorithm::FTile>();
	
	if (TilesetData == nullptr)
	{
		return FinalTiles;
	}
	
	for (auto TileTexture : TilesetData->Tiles) {
		if (TileTexture == nullptr)
		{
			continue;
		}
		UE_LOG(LogTemp, Log, TEXT("Reading tile texture."));
		auto &MipMap = TileTexture->GetPlatformMips()[0];
		
		if (TileTexture->GetPlatformData()->PixelFormat != PF_B8G8R8A8)
		{
			//Oodle texture plugin overrides the compression settings because that is lovely.
			//So make sure to disable the plugin in case changing the settings doesn't change anything.
			UE_LOG(LogTemp, Log, TEXT("Unsupported Pixel format!, make sure to change the compression settings."));
			const EPixelFormat& PixelFormat = TileTexture->GetPlatformData()->PixelFormat;
			static const UEnum* PixelFormatEnum = StaticEnum<EPixelFormat>();
			const FString PixelFormatName = PixelFormatEnum ? PixelFormatEnum->GetNameStringByValue((int64)PixelFormat) : TEXT("Unknown");
			UE_LOG(LogTemp, Log, TEXT("Pixel Format: %s (%d)"), *PixelFormatName, (int32)PixelFormat);
		}
		
		
		std::vector<EPixelValues> TilePixels = std::vector<EPixelValues>();
		TilePixels.reserve(TilesetData->TileDimensions * TilesetData->TileDimensions);
		
		const uint8* RawImageData = static_cast<const uint8*>(MipMap.BulkData.LockReadOnly());
		
		for (int y = 0; y < TilesetData->TileDimensions; y++)
		{
			for (int x = 0; x < TilesetData->TileDimensions; x++)
			{
				const size_t PixelCoord = (y * MipMap.SizeX + x) * 4;
				const FColor PixelColor = FColor(RawImageData[PixelCoord + 2],
										RawImageData[PixelCoord + 1],
										RawImageData[PixelCoord],
										RawImageData[PixelCoord + 3]);
				if (TilesetData->ColorToCellType.Contains(PixelColor))
				{
					TilePixels.push_back(TilesetData->ColorToCellType[PixelColor]);
					// UE_LOG(LogTemp, Log, TEXT("%d"), static_cast<int32>(TilesetData->ColorToCellType[PixelColor]));
				} else {
					UE_LOG(LogTemp, Log, TEXT("Texture not found, Color: %s"), *PixelColor.ToString());
					TilePixels.push_back(EPixelValues::Invalid);
				}
			}
		}
		MipMap.BulkData.Unlock();
		
		//Add it and its rotations to the set
		WFCAlgorithm::FTile Tile =  WFCAlgorithm::FTile{TilePixels, TilesetData->TileDimensions};
		AddIfNotDuplicate(FinalTiles, Tile);
		AddIfNotDuplicate(FinalTiles, Tile.GetCWRotatedTile(1));
		AddIfNotDuplicate(FinalTiles, Tile.GetCWRotatedTile(2));
		AddIfNotDuplicate(FinalTiles, Tile.GetCWRotatedTile(3));
		
	}
	
	UE_LOG(LogTemp, Log, TEXT("Tiles loaded: %llu"), FinalTiles.size());
	return FinalTiles;
}

inline void EmplaceInHasGroup(TMap<int, bool>& HasGroup, int StartIndex, int SizeX, int SizeY, int YStepSize)
{
	for (int y = 0; y < SizeY; y++)
	{
		for (int x = 0; x < SizeX; x++)
		{
			HasGroup.Emplace(StartIndex + y * YStepSize + x, true);	
		}
	}
}


void ACityGenerator::CreateHouseGroups(std::vector<ACityGenerator::GridGroup>& Groups,
	TMap<int, bool>& HasGroup, const int StartX, const int StartY, const int SizeX, const int SizeY) const
{
	const int FlattenedIndex = StartY * GridWidth + StartX;
	if ((SizeY < 2 || SizeX < 2))
	{
		//The group sizes should always be at least 2, if this is not the case we replace with grass instead.
		for (int OffsetY = 0; OffsetY < SizeY; OffsetY++)
		{
			for (int OffsetX = 0; OffsetX < SizeX; OffsetX++)
			{
				int StartIndex = FlattenedIndex + OffsetY * GridWidth + OffsetX;
				Groups.emplace_back(StartX + OffsetX, StartY + OffsetY, 1, 1, EPixelValues::Grass);
				HasGroup.Emplace(StartIndex, true);
			}
			
		}
	}
	
	
	//If both are divisible by 2, subdivide the groups into 2x2 houses
	if ( (SizeY & 1) == 0 && (SizeX & 1) == 0)
	{
		//This looks like a nested loop but in all cases either SizeY or SizeX should be 2
		for (int OffsetY = 0; OffsetY < SizeY; OffsetY +=2)
		{
			for (int OffsetX = 0; OffsetX < SizeX; OffsetX +=2)
			{
				int StartIndex = FlattenedIndex + OffsetY * GridWidth + OffsetX;
				//We want to force a rotation on a 2x2 house if the size of Y > 2, as this means the houses are along a road on the y-axis.
				Groups.emplace_back(StartX + OffsetX, StartY + OffsetY, 2, 2, EPixelValues::House, SizeX > 2);
				EmplaceInHasGroup(HasGroup, StartIndex, 2, 2, GridWidth);
				// HasGroup.Emplace(StartIndex, true);
				// HasGroup.Emplace(StartIndex + 1, true);
				// HasGroup.Emplace(StartIndex + GridWidth, true);
				// HasGroup.Emplace(StartIndex + GridWidth + 1, true);
			}
		}
	} else if ((SizeX == 3 && SizeY == 2) || (SizeY == 3 && SizeX == 2))
	{
		//3x2 houses.
		Groups.emplace_back(StartX, StartY, (SizeX == 3 ? 3 : 2), (SizeY == 3 ? 3 : 2), EPixelValues::House);
		for (int OffsetY = 0; OffsetY < SizeY; OffsetY++)
		{
			for (int OffsetX = 0; OffsetX < SizeX; OffsetX++)
			{
				HasGroup.Emplace(FlattenedIndex + OffsetY * GridWidth + OffsetX, true);
			}
		}
	} else if ((SizeY == 2) || (SizeX == 2)) {
		//In this case there is an uneven amount of house spaces, so we want to change either side to grass.
		//To figure out which side of the group we want to turn into grass, we see which side is the closest to the edge.
		//This only occurs when a tile is not connected, so this should handle the cases of tiles going off the side.
		const bool bVertical = (SizeY == 2);

		const int Limit     = bVertical ? GridWidth  : GridHeight;
		const int StartAxis = bVertical ? StartX     : StartY;

		if (StartAxis < (Limit / 2))
		{
			// Place the two 1x1 groups
			Groups.emplace_back(StartX, StartY, 1, 1, EPixelValues::Grass);
			HasGroup.Emplace(FlattenedIndex, true);

			if (bVertical)
			{
				Groups.emplace_back(StartX, StartY + 1, 1, 1, EPixelValues::Grass);
				HasGroup.Emplace(FlattenedIndex + GridWidth, true);
			}
			else
			{
				Groups.emplace_back(StartX + 1, StartY, 1, 1, EPixelValues::Grass);
				HasGroup.Emplace(FlattenedIndex + 1, true);
			}
		}
		else
		{
			// Recurse, trimming the long side
			CreateHouseGroups(Groups,HasGroup,
				StartX, StartY,
				bVertical ? SizeX - 1 : SizeX,
				bVertical ? SizeY     : SizeY - 1
			);
		}
	} else {
		//Shouldn't be able to get here
		UE_LOG(LogTemp, Warning, TEXT("Flattened index of type House has illegal group size: %d"), FlattenedIndex);
		Groups.emplace_back(StartX, StartY, 1, 1, EPixelValues::Grass);
		HasGroup.Emplace(FlattenedIndex, true);
	}
}
void ACityGenerator::CreateRoadGroups(std::vector<ACityGenerator::GridGroup>& Groups, TMap<int, bool>& HasGroup, const int StartX, const int StartY, const int SizeX, const int SizeY, const std::vector<EPixelValues>& Pixels) const
{
	const int FlattenedIndex = StartY * GridWidth + StartX;
	//Intersection, make a 2x2 group
	//Technically there could be a 2x2 road after an intersection as well, but I dont think its in our current tileset.
	if ((SizeY > 2 && SizeX > 2) || (SizeY == 2 && SizeX == 2))
	{
		Groups.emplace_back(StartX, StartY, 2, 2, EPixelValues::Road);
		EmplaceInHasGroup(HasGroup, FlattenedIndex, 2, 2, GridWidth);
		// HasGroup.Emplace(FlattenedIndex, true);
		// HasGroup.Emplace(FlattenedIndex + 1, true);
		// HasGroup.Emplace(FlattenedIndex + GridWidth, true);
		// HasGroup.Emplace(FlattenedIndex + GridWidth + 1, true);
	} else if ((SizeY == 2) || (SizeX == 2))
	{
		//In the case that Size X is 2, it is a road along the X direction, therefore we should check for intersections
		//in the X direction.
		int NeighbourOffset = (SizeX == 2) ? 1 : GridWidth;
		int StartIndexOffset = (SizeX == 2) ? GridWidth : 1;
		int LoopCount = (SizeX == 2) ? SizeY : SizeX;
		
		for (int i = 0; i < LoopCount; i++)
		{
			//Check if its not an intersection tile
			const int StartIndex = FlattenedIndex + StartIndexOffset * i;
			UE_LOG(LogTemp, Warning, TEXT("Iterating over road tiles: %d, %d"), StartIndex, StartIndex + NeighbourOffset);
			const int AboveNeighbourIndex = StartIndex - NeighbourOffset;
			const int BelowNeighbourIndex = StartIndex + NeighbourOffset * 2;
			
			if ((AboveNeighbourIndex >= 0 && Pixels[AboveNeighbourIndex] == EPixelValues::Road) ||
				(BelowNeighbourIndex < Pixels.size() && Pixels[BelowNeighbourIndex] == EPixelValues::Road))
			{
				if (i == 0)
				{
					//In the cases where we start the loop and we instantly see that there is a road as well the other way, this
					//means we are either in a turn or in an intersection, either way, we have to spawn a 2x2 tile.
					Groups.emplace_back(StartX, StartY, 2, 2, EPixelValues::Road);
					EmplaceInHasGroup(HasGroup, FlattenedIndex, 2, 2, GridWidth);
					// HasGroup.Emplace(FlattenedIndex, true);
					// HasGroup.Emplace(FlattenedIndex + 1, true);
					// HasGroup.Emplace(FlattenedIndex + GridWidth, true);
					// HasGroup.Emplace(FlattenedIndex + GridWidth + 1, true);
				}
				
				break;
			}
			//Group size in X and Y are 1 or 2 depending on whether we are iterating over Y or X
			//It can happen that a 2x2 tile is behind an intersection, in those cases,
			//to prevent an increment of both x and y we just use a ternary with sizeX.
			Groups.emplace_back((SizeX == 2 ? StartX : StartX + i),
							(SizeX == 2 ? StartY + i : StartY),
							(SizeX == 2 ? 2 : 1),
							(SizeX == 2 ? 1 : 2),
							EPixelValues::Road);
			HasGroup.Emplace(StartIndex, true);
			HasGroup.Emplace(StartIndex + NeighbourOffset, true);
		}
	} else
	{
		//Shouldn't be able to get here
		UE_LOG(LogTemp, Warning, TEXT("Flattened index of type Road has illegal group size: %d"), FlattenedIndex);
		Groups.emplace_back(StartX, StartY, 1, 1, EPixelValues::Road);
	}
}


std::vector<ACityGenerator::GridGroup> ACityGenerator::CreateGridGroups(const std::vector<EPixelValues>& Pixels) const
{
	std::vector<GridGroup> Groups;
	//Since most groups are either 2x1 or 2x2, we reserve accounting for that.
	Groups.reserve((GridWidth / 2) * (GridHeight / 2));
	TMap<int, bool> HasGroup;
	
	for (int y = 0; y < GridHeight; y++)
	{
		for (int x = 0; x < GridWidth; x++)
		{
			const int FlattenedIndex = y * GridWidth + x;
			//Check if Index is already ocntained in a group
			if (HasGroup.Contains(FlattenedIndex))
			{
				// UE_LOG(LogTemp, Warning, TEXT("Flattened index already encountered!"));
				continue;
			}
			
			const EPixelValues& GroupType = Pixels[FlattenedIndex];
			
			if (GroupType == EPixelValues::Grass || GroupType == EPixelValues::Pavement || GroupType == EPixelValues::Invalid)
			{
				//TODO!: Add bigger size groupings to grass as well to allow for trees and stuff
				Groups.emplace_back(x, y, 1, 1, GroupType);
				continue;
			}
			
			int SizeX = 1;
			int SizeY = 1;
			while (x + SizeX < GridWidth && Pixels[FlattenedIndex + SizeX] == GroupType)
			{
				SizeX++;
			}
			while (y + SizeY < GridHeight && Pixels[FlattenedIndex + GridWidth * SizeY] == GroupType)
			{
				SizeY++;
			}
			
			if (GroupType == EPixelValues::House)
			{
				CreateHouseGroups(Groups, HasGroup, x, y, SizeX, SizeY);
			} else if (GroupType == EPixelValues::Road)
			{
				CreateRoadGroups(Groups, HasGroup, x, y, SizeX, SizeY, Pixels);
			}
			
			
		}
	}
	
	return Groups;
}

UStaticMesh* ACityGenerator::GetSpawnMesh(EPixelValues PixelType, int SizeX, int SizeY) const
{
	switch (PixelType)
	{
	case EPixelValues::Grass:
		if (MeshsetData->Grass1x1Meshes.Num() > 0) {
			return MeshsetData->Grass1x1Meshes[0];
		}
		break;
	case EPixelValues::Road:
		//TODO!: Improve logic
		if (SizeX * SizeY == 2)
		{
			if (MeshsetData->Road2x1Meshes.Num() > 0)
			{
				return MeshsetData->Road2x1Meshes[0];
			}
		} else
		{
			if (MeshsetData->Road2x2Meshes.Num() > 0)
			{
				return MeshsetData->Road2x2Meshes[0];
			}
		}
		break;
	case EPixelValues::House:
		//TODO!: Improve logic :)
		if (SizeX * SizeY == 4)
		{
			if (MeshsetData->House2x2Meshes.Num() > 0) {
				return MeshsetData->House2x2Meshes[0];
			}
		} else if (SizeX * SizeY == 6)
		{
			if (MeshsetData->House3x2Meshes.Num() > 0) {
				return MeshsetData->House3x2Meshes[0];
			}
		} else if (SizeX * SizeY == 2)
		{
			if (MeshsetData->House3x2Meshes.Num() > 0) {
				return MeshsetData->House2x1Meshes[0];
			}
		} else
		{
			if (MeshsetData->House1x1Meshes.Num() > 0) {
				return MeshsetData->House1x1Meshes[0];
			}
		}
		
		break;
	case EPixelValues::Pavement:
		if (MeshsetData->Pavement1x1Meshes.Num() > 0)
		{
			return MeshsetData->Pavement1x1Meshes[0];
		}
		break;
	default:
		//Invalid mesh
		break;
	}
	
	return nullptr;
}

void ACityGenerator::SpawnMeshes(const std::vector<GridGroup>& Groups)
{
	const FVector Origin = GetActorLocation();
	
	for (const GridGroup& Group : Groups)
	{
		FVector SpawnLocation = Origin + FVector(Group.StartX * GridSpacing, Group.StartY * GridSpacing, 0);
		UStaticMesh* SpawnMesh = GetSpawnMesh(Group.GroupType, Group.SizeX, Group.SizeY);
		
		// Determine rotation based on size comparison
		FRotator SpawnRotation = FRotator::ZeroRotator;
		if (Group.SizeY > Group.SizeX || Group.ShouldRotate)
		{
			SpawnRotation = FRotator(0.f, 90.f, 0.f); // Rotate 90 degrees around yaw
			
			if (Group.SizeX > 1)
			{
				SpawnLocation = SpawnLocation + FVector(GridSpacing * (Group.SizeX - 1), 0, 0);
			}
		}
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
		AStaticMeshActor* SpawnedActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), 
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);
			
		if (SpawnedActor && SpawnedActor->GetStaticMeshComponent())
		{
			SpawnedActor->GetStaticMeshComponent()->SetStaticMesh(SpawnMesh);
			SpawnedActor->SetActorLabel(FString::Printf(TEXT("GridMesh_%d_%d"), Group.StartX, Group.StartY));
			// SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn mesh actor at grid [%d,%d]."), Group.StartX, Group.StartY);
		}
	}
}

// Called every frame
void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

