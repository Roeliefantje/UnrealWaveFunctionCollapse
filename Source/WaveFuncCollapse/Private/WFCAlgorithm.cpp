// Fill out your copyright notice in the Description page of Project Settings.
#include "WFCAlgorithm.h"
#include <algorithm>

WFCAlgorithm::WFCAlgorithm(const std::vector<FTile> &PossibleTiles, int width, int height) : Width(width), Height(height), Possible_tileset(PossibleTiles)
{
	Grid = std::make_unique<FGridTile[]>(width * height);
	constructRuleset();
	
	//Initialize all the grid values with the same entropy, meaning they all can be all possible states.
	//TODO!: It would be fun if we could set an initial state through some functions instead of doing it here.
	//TODO!: So we would init all this, and then in the editor allow certain tiles to be set to a type.
	
	FGridTile BaseTile;
	BaseTile.collapsed = false;
	//TODO!: Could be more memory efficient to only store the Tile Ids instead of the full object.
	BaseTile.possible_options = Possible_tileset;
	
	//Use std::fill instead of a loop as memcpy is probably a bit more efficient.
	//Fill will deepcopy the struct, so the vectors are still seperate, we could look into using a pointer to a vec instead,
	//If we then have a shared_ptr for each different instance of possibilities, we could potentially save some memory.
	std::fill(Grid.get(), Grid.get() + Width * Height, BaseTile);
	
	//Init the entropy buckets and add all cells to highest bucket
	for (int i = 0; i <= Possible_tileset.size(); i++)
	{
		if (i == Possible_tileset.size())
		{
			std::vector<FBucketItem> final_bucket = std::vector<FBucketItem>();
			final_bucket.reserve(width * height);
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					final_bucket.emplace_back(x, y);
				}
			}
			EntropyBuckets.push_back(final_bucket);
		} else
		{
			EntropyBuckets.emplace_back(0);
		}
		
	}
}

//This function constructs a map of the ruleset to ensure
void WFCAlgorithm::constructRuleset()
{
	for (const auto &possibleTile : Possible_tileset)
	{
		auto nbRuleSet = FTileNeighbours{};
		for (const auto &nbTile : Possible_tileset)
		{
			if (possibleNorth(possibleTile, nbTile))
			{
				nbRuleSet.North.push_back(nbTile);
			}
			if (possibleEast(possibleTile, nbTile))
			{
				nbRuleSet.East.push_back(nbTile);
			}
			if (possibleWest(possibleTile, nbTile))
			{
				nbRuleSet.South.push_back(nbTile);
			}
			if (possibleSouth(possibleTile, nbTile))
			{
				nbRuleSet.West.push_back(nbTile);
			}
		}
		
		TileRuleset[possibleTile] = nbRuleSet;
	}
}

inline bool WFCAlgorithm::possibleNorth(const FTile& Curr, const FTile& Nb)
{
	//We need to compare the top row with the bottom row, which is done by calculating the dimensions of the tile and
	//then comparing them. The index for the bottom left value is the total size - the dimension, for example: 9 - 3 = 6
	int tileDimensions = std::sqrt(TileSize);
	for (int i = 0; i < tileDimensions; i++)
	{
		if (Curr.pixels[i] != Nb.pixels[TileSize - tileDimensions + i])
		{
			return false;
		}
	}
	
	return true;
}

inline bool WFCAlgorithm::possibleEast(const FTile& Curr, const FTile& Nb)
{
	//We need to compare the right side values with the left side values.
	int tileDimensions = std::sqrt(TileSize);
	for (int i = 0; i < tileDimensions; i++)
	{
		if (Curr.pixels[tileDimensions - 1 + tileDimensions * i] != Nb.pixels[tileDimensions * i])
		{
			return false;
		}
	}
	
	return true;
}

inline bool WFCAlgorithm::possibleSouth(const FTile& Curr, const FTile& Nb)
{
	//South is just north but flipped
	int tileDimensions = std::sqrt(TileSize);
	for (int i = 0; i < tileDimensions; i++)
	{
		if (Curr.pixels[TileSize - tileDimensions + i] != Nb.pixels[i])
		{
			return false;
		}
	}
	
	return true;
}

inline bool WFCAlgorithm::possibleWest(const FTile& Curr, const FTile& Nb)
{
	//West is East but flipped
	int tileDimensions = std::sqrt(TileSize);
	for (int i = 0; i < tileDimensions; i++)
	{
		if (Curr.pixels[tileDimensions * i] != Nb.pixels[tileDimensions - 1 + tileDimensions * i])
		{
			return false;
		}
	}
	
	return true;
}


WFCAlgorithm::~WFCAlgorithm()
{
}
