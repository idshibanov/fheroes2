/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <set>

#include "world.h"

namespace
{
    // Aliases to make data structures easier to work with
    // TileData.first is index, TileData.second is payload
    using TileData = std::pair<int, int>;
    using TileDataVector = std::vector<std::pair<int, int> >;

    static const Directions directions = Direction::All();

    enum
    {
        BLOCKED = 0,
        OPEN = 1,
        BORDER = 2,
        REGION = 3
    };

    struct MapRegionNode
    {
        int index = -1;
        uint16_t type = BLOCKED;
        uint16_t passable = 0;
        bool isWater = false;

        MapRegionNode() {}
        MapRegionNode( int index )
            : index( index )
            , type( OPEN )
        {}
        MapRegionNode( int index, uint16_t pass, bool water )
            : index( index )
            , type( OPEN )
            , passable( pass )
            , isWater( water )
        {}
    };

    struct MapRegion
    {
        int id = REGION;
        bool isWater = false;
        std::vector<MapRegion *> neighbours;
        std::vector<MapRegionNode> nodes;
        std::vector<MapRegionNode> edgeNodes;
        size_t lastProcessedNode = 0;

        MapRegion( int regionIndex, int mapIndex, bool water )
            : id( REGION + regionIndex )
            , isWater( water )
        {
            nodes.push_back( { mapIndex } );
            nodes[0].type = id;
        }
    };

    struct RegionLinkRoute : std::list<int>
    {
        int _indexFrom;
        int _indexTo;
        size_t _length;
        uint32_t _basePenalty;
        uint32_t _roughTerrainPenalty;
    };

    int ConvertExtendedIndex( int index, uint32_t width )
    {
        const uint32_t originalWidth = width - 2;
        return ( index / originalWidth + 1 ) * width + ( index % originalWidth ) + 1;
    }

    bool AppendIfFarEnough( std::vector<int> & dataSet, int value, uint32_t distance )
    {
        bool match = true;
        for ( int & current : dataSet ) {
            if ( Maps::GetApproximateDistance( current, value ) < distance )
                match = false;
        }

        if ( match )
            dataSet.push_back( value );

        return match;
    }

    void CheckAdjacentTiles( std::vector<MapRegionNode> & rawData, uint32_t rawDataWidth, MapRegion & region )
    {
        static const Directions directions = Direction::All();

        // region.nodes will be modified, so have to copy the node here
        MapRegionNode node = region.nodes[region.lastProcessedNode];
        const int extIDX = ConvertExtendedIndex( node.index, rawDataWidth );

        uint8_t neighbourCount = 0;
        for ( const int direction : directions ) {
            const int newIndex = Direction::GetDirectionIndex( extIDX, direction, rawDataWidth );
            MapRegionNode & newTile = rawData[newIndex];
            if ( newTile.passable & Direction::Reflect( direction ) ) {
                if ( newTile.type == OPEN && newTile.isWater == region.isWater ) {
                    newTile.type = region.id;
                    region.nodes.push_back( newTile );
                }
                else if ( newTile.type > REGION && newTile.type != region.id ) {
                    ++neighbourCount;
                }
            }
        }

        if ( neighbourCount > 0 ) {
            region.nodes[region.lastProcessedNode].type = BORDER;
            region.edgeNodes.push_back( node );
        }
    }

    void RegionExpansion( std::vector<MapRegionNode> & rawData, uint32_t rawDataWidth, MapRegion & region )
    {
        // Process only "open" nodes that exist at the start of the loop and ignore what's added
        const size_t nodesEnd = region.nodes.size();

        while ( region.lastProcessedNode < nodesEnd ) {
            CheckAdjacentTiles( rawData, rawDataWidth, region );
            ++region.lastProcessedNode;
        }
    }
}

void FindMissingRegions( std::vector<MapRegionNode> & rawData, const Size & mapSize, std::vector<MapRegion> & regions )
{
    const uint32_t extendedWidth = mapSize.w + 2;

    MapRegionNode * currentTile = rawData.data() + extendedWidth + 1;
    MapRegionNode * mapEnd = rawData.data() + extendedWidth * ( mapSize.h + 1 );

    for ( ; currentTile != mapEnd; ++currentTile ) {
        if ( currentTile->type == OPEN ) {
            const size_t currentPosition = currentTile - rawData.data();

            MapRegion region( regions.size(), currentTile->index, currentTile->isWater );

            do {
                CheckAdjacentTiles( rawData, extendedWidth, region );
                ++region.lastProcessedNode;
            } while ( region.lastProcessedNode != region.nodes.size() );

            regions.push_back( region );
        }
    }
}

void World::ComputeStaticAnalysis()
{
    int terrainTotal = 0;
    const int width = w();
    const int height = h();
    const int mapSize = std::max( width, height );

    const Directions directions = Direction::All();
    TileDataVector obstacles[4];
    TileDataVector castleCenters;
    std::vector<int> regionCenters;

    const uint32_t castleRegionSize = 17;
    const uint32_t extraRegionSize = 18;
    const uint32_t emptyLineFrequency = 7;

    for ( int x = 0; x < width; ++x ) {
        obstacles[0].emplace_back( x, 0 ); // water, columns
        obstacles[2].emplace_back( x, 0 ); // ground, columns
    }
    for ( int y = 0; y < height; ++y ) {
        obstacles[1].emplace_back( y, 0 ); // water, rows
        obstacles[3].emplace_back( y, 0 ); // ground, rows
    }

    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const int index = rowIndex + x;
            Maps::Tiles & tile = vec_tiles[index];
            if ( tile.GetPassable() == 0 ) {
                ++terrainTotal;
                ++obstacles[0][x].second;
                ++obstacles[1][y].second;
                ++obstacles[2][x].second;
                ++obstacles[3][y].second;
            }
            else if ( tile.isWater() ) {
                ++obstacles[2][x].second;
                ++obstacles[3][y].second;
            }
            else {
                ++obstacles[0][x].second;
                ++obstacles[1][y].second;
            }
        }
    }

    auto smallerThanSort = []( const TileData & left, TileData & right ) { return left.second < right.second; };
    for ( int i = 0; i < 4; ++i )
        std::sort( obstacles[i].begin(), obstacles[i].end(), smallerThanSort );

    std::vector<int> emptyLines[4]; // 0,1 is water; 2,3 is ground
    for ( int i = 0; i < 4; ++i ) {
        for ( const TileData & line : obstacles[i] ) {
            AppendIfFarEnough( emptyLines[i], line.first, emptyLineFrequency );
        }
    }

    for ( Castle * castle : vec_castles ) {
        castleCenters.emplace_back( castle->GetIndex(), castle->GetColor() );
    }
    std::sort( castleCenters.begin(), castleCenters.end(), []( const TileData & left, const TileData & right ) {
        // Sort castles by color primarily (NONE is last)
        // If same color then compare map index
        if ( left.second == right.second )
            return left.first < right.first;
        return left.second > right.second;
    } );

    for ( const TileData & castleTile : castleCenters ) {
        // Check if a lot of players next to each other? (Slugfest map)
        // GetCastle( Point( val % width, val / width ) )->GetColor();
        const int castleIndex = castleTile.first + width;
        AppendIfFarEnough( regionCenters, ( castleIndex > vec_tiles.size() ) ? castleTile.first : castleIndex, castleRegionSize );
    }

    for ( int waterOrGround = false; waterOrGround < 2; ++waterOrGround ) {
        for ( const int rowID : emptyLines[waterOrGround * 2] ) {
            for ( const int colID : emptyLines[waterOrGround * 2 + 1] ) {
                int centerIndex = -1;

                const int tileIndex = rowID * width + colID;
                const Maps::Tiles & tile = vec_tiles[tileIndex];
                if ( tile.GetPassable() && tile.isWater() ) {
                    centerIndex = tileIndex;
                }
                else {
                    for ( const int direction : directions ) {
                        if ( Maps::isValidDirection( tileIndex, direction ) ) {
                            const int newIndex = Maps::GetDirectionIndex( tileIndex, direction );
                            const Maps::Tiles & newTile = vec_tiles[newIndex];
                            if ( newTile.GetPassable() && tile.isWater() == waterOrGround ) {
                                centerIndex = newIndex;
                                break;
                            }
                        }
                    }
                }

                if ( centerIndex >= 0 ) {
                    AppendIfFarEnough( regionCenters, centerIndex, extraRegionSize );
                }
            }
        }
    }

    const uint32_t extendedWidth = width + 2;
    std::vector<MapRegionNode> data( extendedWidth * ( height + 2 ) );
    for ( int y = 0; y < height; ++y ) {
        const int rowIndex = y * width;
        for ( int x = 0; x < width; ++x ) {
            const size_t index = rowIndex + x;
            const Maps::Tiles & tile = vec_tiles[index];
            MapRegionNode & node = data[ConvertExtendedIndex( index, extendedWidth )];

            node.index = index;
            node.passable = tile.GetPassable();
            node.isWater = tile.isWater();
            if ( node.passable != 0 ) {
                node.type = OPEN;
            }
        }
    }

    std::vector<MapRegion> regions;
    for ( size_t regionID = 0; regionID < regionCenters.size(); ++regionID ) {
        const int tileIndex = regionCenters[regionID];
        regions.push_back( { static_cast<int>( regionID ), tileIndex, vec_tiles[tileIndex].isWater() } );
        data[ConvertExtendedIndex( tileIndex, extendedWidth )].type = REGION + regionID;
    }

    for ( int i = 0; i < mapSize / 2; ++i ) {
        for ( size_t regionID = 0; regionID < regionCenters.size(); ++regionID ) {
            RegionExpansion( data, extendedWidth, regions[regionID] );
        }
    }

    FindMissingRegions( data, Size( width, height ), regions );

    for ( auto reg : regions ) {
        for ( auto node : reg.nodes ) {
            vec_tiles[node.index]._metadata = node.type;
        }
    }

    // Create region connection clusters
    TileDataVector regionLinks;

    // while ( !connectionMap.empty() ) {
    //    // begin() should be always valid if map is not empty
    //    TileData link = *connectionMap.begin();
    //    bool isRoad = vec_tiles[link.first].isRoad();

    //    std::set<int> openTiles;
    //    openTiles.insert( link.first );

    //    // Loop to find all tiles in a cluster, we only need 1 to make it a region link
    //    while ( !openTiles.empty() ) {
    //        // pop_first() for std::set
    //        const int tileIndex = *openTiles.begin();
    //        openTiles.erase( openTiles.begin() );

    //        std::unordered_map<int, int>::iterator currentTile = connectionMap.find( tileIndex );
    //        if ( currentTile != connectionMap.end() ) {
    //            const bool isCurrentTileRoad = vec_tiles[currentTile->first].isRoad();
    //            if ( ( isRoad == isCurrentTileRoad && currentTile->second > link.second ) || ( !isRoad && isCurrentTileRoad ) ) {
    //                link = *currentTile;
    //                isRoad = isCurrentTileRoad;
    //            }
    //            connectionMap.erase( currentTile );
    //        }

    //        // find if there's more tiles around
    //        for ( int direction : directions ) {
    //            if ( Maps::isValidDirection( tileIndex, direction ) ) {
    //                const int newIndex = Maps::GetDirectionIndex( tileIndex, direction );

    //                if ( connectionMap.find( newIndex ) != connectionMap.end() )
    //                    openTiles.insert( newIndex );
    //            }
    //        }
    //    }

    //    regionLinks.push_back( link );
    //}

    // for ( int y = 0; y < height; ++y ) {
    //    const int rowIndex = y * width;
    //    for ( int x = 0; x < width; ++x ) {
    //        const size_t index = rowIndex + x;
    //        vec_tiles[index]._metadata = data[index + extendedWidth + 1].type;
    //    }
    //}

    // DEBUG: view the hot spots
    // for ( const int center : regionCenters ) {
    //    vec_tiles[center]._metadata = vec_tiles[center]._region;
    //}
    // for ( const TileData & link : regionLinks ) {
    //    vec_tiles[link.first]._metadata = vec_tiles[link.first]._region;
    //}
}
