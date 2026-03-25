// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "CoreMinimal.h"

/**
 * 
 */



class WAVEFUNCCOLLAPSE_API WFCAlgorithm
{
private:
	static inline int TileSize = 9;
public:
	enum EPixelValues
	{
		Grass, Road, House
	};
	
	struct FTile
	{
		//A possible collapsed state of a tile (consisting of pixels) in the grid.
		//This tile will have possible rules etc
		int idx;
		EPixelValues pixels[TileSize];
		
		bool operator==(const FTile& other) const
		{
			return idx == other.idx;
		}
	};
private:
	struct FGridTile
	{
		//A representation of a tile inside of the current representation grid of the WFC.
		//TODO!: Use a bitmask instead of a vector of the possible options.
		std::vector<FTile> possible_options;
		bool collapsed;
	};
	
	struct FTileNeighbours
	{
		//TODO!: This should be bitmasks.
		std::vector<FTile> North;
		std::vector<FTile> East;
		std::vector<FTile> South;
		std::vector<FTile> West;
	};
	
	struct FBucketItem
	{
		//We use Buckets to keep track of the cells with a certain entropy, without having to iterate over the whole grid each step.
		//These items do not get updated if the entropy of a cell updates, so we need to verify whether the entropy is still what it claims to be when we use it.
		int XIndex;
		int YIndex;
	};
	
private:
	int Width;
	int Height;
	std::unique_ptr<FGridTile[]> Grid;
	std::vector<FTile> Possible_tileset;
	std::unordered_map<FTile, FTileNeighbours> TileRuleset;
	std::vector<std::vector<FBucketItem>> EntropyBuckets;
	
	void WFCAlgorithm::constructRuleset();
	
	static inline bool possibleNorth(const FTile& Curr, const FTile& Nb);
	static inline bool possibleEast(const FTile& Curr, const FTile& Nb);
	static inline bool possibleSouth(const FTile& Curr, const FTile& Nb);
	static inline bool possibleWest(const FTile& Curr, const FTile& Nb);

public:
	WFCAlgorithm(const std::vector<FTile> &possible_tiles, int width, int height);
	~WFCAlgorithm();
};
