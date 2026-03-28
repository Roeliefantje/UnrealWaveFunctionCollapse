// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>
#include <vector>

#include "CoreMinimal.h"

/**
 * 
 */

//While this is an Enum to be used by the WFC only, in order to expose it to blueprints, it needs to be outside of the class.
UENUM(BlueprintType)
enum class EPixelValues: uint8
{
	Grass   UMETA(DisplayName = "Grass"),
	Road    UMETA(DisplayName = "Road"),
	House   UMETA(DisplayName = "House"),
	Invalid UMETA(DisplayName = "Invalid")
};

class WAVEFUNCCOLLAPSE_API WFCAlgorithm
{
private:
	
public:

	struct FTile
	{
		//A possible collapsed state of a tile (consisting of pixels) in the grid.
		//This tile will have possible rules etc
		std::vector<EPixelValues> Pixels;
		int Dim;
		// EPixelValues pixels[TileSize];
		
		FTile GetCWRotatedTile(int Rotations) const;
	};
private:
	struct FGridTile
	{
		//A representation of a tile inside of the current representation grid of the WFC.
		//TODO!: Use a bitmask instead of a vector of the possible options.
		// std::vector<FTile> possible_options;
		uint32_t PossibleOptions;
		bool Collapsed;
	};
	
	struct FTileNeighbours
	{
		uint32_t North;
		uint32_t East;
		uint32_t South;
		uint32_t West;
		
		void AddPossibleNeighbours(const FTileNeighbours& Other)
		{
			North |= Other.North;
			East  |= Other.East;
			South |= Other.South;
			West  |= Other.West;
		}
	};
	
	struct FBucketItem
	{
		//We use Buckets to keep track of the cells with a certain entropy, without having to iterate over the whole grid each step.
		//These items do not get updated if the entropy of a cell updates, so we need to verify whether the entropy is still what it claims to be when we use it.
		int XIndex;
		int YIndex;
	};
	
private:
	int TileDimensions;
	int Width;
	int Height;
	std::unique_ptr<FGridTile[]> Grid;
	std::vector<FTile> Possible_tileset;
	std::vector<FTileNeighbours> TileRuleset;
	std::vector<std::vector<FBucketItem>> EntropyBuckets;
	
	void constructRuleset();
	
	inline bool possibleNorth(const FTile& Curr, const FTile& Nb) const;
	inline bool possibleEast(const FTile& Curr, const FTile& Nb) const;
	inline bool possibleSouth(const FTile& Curr, const FTile& Nb) const;
	inline bool possibleWest(const FTile& Curr, const FTile& Nb) const;
	
	FBucketItem GetLowestEntropyGridTile();
	void UpdateNeighbour(int x, int y, uint32_t bitMask);
	
	static void LogNeighbourBitmasks(FTileNeighbours NBInfo);
public:
	WFCAlgorithm(const std::vector<FTile> &possible_tiles, int width, int height, int TileDimensions);
	std::vector<EPixelValues> Solve();
	bool Step();
	~WFCAlgorithm();
};
