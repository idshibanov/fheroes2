/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

#pragma once

#include <cstdint>
#include <vector>

#include "map_random_generator.h"
#include "map_random_generator_info.h"
#include "rand.h"

namespace Maps::Random_Generator
{

    struct PlacementTile final
    {
        int index{ 0 };
        float distance{ 0 };
        int ring{ 0 };
    };

    std::vector<std::vector<PlacementTile>> findOpenTilesSortedJittered( const Region & region, int mapWidth, Rand::PCG32 & randomGenerator );

    constexpr int32_t treeTypeFromGroundType( const int groundType );
    constexpr int32_t mountainTypeFromGroundType( const int groundType );
    int32_t localizeObjectToTerrain( const Maps::ObjectGroup groupType, const int32_t objectIndex, const int groundType );

    bool canFitObject( const NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction, const bool skipBorders );
    bool canFitObjectSet( const NodeCache & data, const ObjectSet & set, const fheroes2::Point & mainTilePos );
    void markObjectPlacement( NodeCache & data, const Maps::ObjectInfo & info, const fheroes2::Point & mainTilePos, const bool isAction );
    bool putObjectOnMap( Map_Format::MapFormat & mapFormat, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex );
    bool actionObjectPlacer( Map_Format::MapFormat & mapFormat, NodeCache & data, Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t type );
    bool placeCastle( Map_Format::MapFormat & mapFormat, NodeCache & data, const Region & region, const fheroes2::Point tilePos, const bool isCastle );
    bool placeMine( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, const int resource );
    bool placeObstacle( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & node, Rand::PCG32 & randomGenerator );
    bool placeSimpleObject( Map_Format::MapFormat & mapFormat, NodeCache & data, const Node & centerNode, const ObjectPlacement & placement );
}
