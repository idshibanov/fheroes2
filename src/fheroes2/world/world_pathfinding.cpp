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

#include "world_pathfinding.h"
#include "ground.h"
#include "world.h"

void Pathfinder::reset()
{
    _cache.clear();
    _pathStart = -1;
    _pathfindingSkill = Skill::Level::NONE;
}

std::list<Route::Step> Pathfinder::buildPath( int from, int target, uint8_t skill )
{
    std::list<Route::Step> path;

    // check if we have to re-cache the map (new hero selected, etc)
    reEvaluateIfNeeded( from, skill );

    // trace the path from end point
    int currentNode = target;
    uint32_t cost = _cache[currentNode]._cost;
    while ( currentNode != from && currentNode != -1 ) {
        PathfindingNode & node = _cache[currentNode];

        path.emplace_front( node._from, Direction::Get( node._from, currentNode ), cost - node._cost );
        currentNode = node._from;
        cost = node._cost;
    }

    return path;
}

bool World::isTileBlocked( int tileIndex, bool fromWater ) const
{
    const Maps::Tiles & tile = world.GetTiles( tileIndex );
    const bool toWater = tile.isWater();
    const int object = tile.GetObject();

    if ( object == MP2::OBJ_HEROES || object == MP2::OBJ_MONSTER || object == MP2::OBJ_BOAT )
        return true;

    if ( MP2::isPickupObject( object ) || MP2::isActionObject( object, fromWater ) )
        return true;

    if ( fromWater && !toWater && object == MP2::OBJ_COAST )
        return true;

    return false;
}

bool World::isValidPath( int index, int direction ) const
{
    const Maps::Tiles & fromTile = GetTiles( index );
    const Maps::Tiles & toTile = GetTiles( Maps::GetDirectionIndex( index, direction ) );
    const bool fromWater = fromTile.isWater();

    // check corner water/coast
    if ( fromWater ) {
        switch ( direction ) {
        case Direction::TOP_LEFT:
            if ( !GetTiles( Maps::GetDirectionIndex( index, Direction::TOP ) ).isWater() || !GetTiles( Maps::GetDirectionIndex( index, Direction::LEFT ) ).isWater() )
                return false;
            break;

        case Direction::TOP_RIGHT:
            if ( !GetTiles( Maps::GetDirectionIndex( index, Direction::TOP ) ).isWater() || !GetTiles( Maps::GetDirectionIndex( index, Direction::RIGHT ) ).isWater() )
                return false;
            break;

        case Direction::BOTTOM_RIGHT:
            if ( !GetTiles( Maps::GetDirectionIndex( index, Direction::BOTTOM ) ).isWater() || !GetTiles( Maps::GetDirectionIndex( index, Direction::RIGHT ) ).isWater() )
                return false;
            break;

        case Direction::BOTTOM_LEFT:
            if ( !GetTiles( Maps::GetDirectionIndex( index, Direction::BOTTOM ) ).isWater() || !GetTiles( Maps::GetDirectionIndex( index, Direction::LEFT ) ).isWater() )
                return false;
            break;

        default:
            break;
        }
    }

    if ( !fromTile.isPassable( direction, fromWater, false ) )
        return false;

    return toTile.isPassable( Direction::Reflect( direction ), fromWater, false );
}

bool Pathfinder::isBlockedByObject( int from, int target, bool fromWater )
{
    int currentNode = target;
    while ( currentNode != from && currentNode != -1 ) {
        if ( world.isTileBlocked( currentNode, fromWater ) ) {
            return true;
        }
        currentNode = _cache[currentNode]._from;
    }
    return false;
}

uint32_t Pathfinder::getDistance( int from, int target, uint8_t skill )
{
    reEvaluateIfNeeded( from, skill );
    return _cache[target]._cost;
}

void Pathfinder::reEvaluateIfNeeded( int from, uint8_t skill )
{
    if ( _pathStart != from || _pathfindingSkill != skill ) {
        evaluateMap( from, skill );
    }
}

uint32_t Pathfinder::getMovementPenalty( int from, int target, int direction, uint8_t skill )
{
    const Maps::Tiles & tileTo = world.GetTiles( target );
    uint32_t penalty = ( world.GetTiles( from ).isRoad() && tileTo.isRoad() ) ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tileTo, skill );

    // diagonal move costs 50% extra
    if ( direction & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) )
        penalty = penalty * 3 / 2;

    return penalty;
}

// Destination is optional
void Pathfinder::evaluateMap( int start, uint8_t skill )
{
    const Directions directions = Direction::All();
    const bool fromWater = world.GetTiles( start ).isWater();
    const int width = world.w();
    const int height = world.h();

    _pathStart = start;
    _pathfindingSkill = skill;

    _cache.clear();
    _cache.resize( width * height );
    _cache[start] = PathfindingNode( -1, 0 );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( start );
    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        const int currentNodeIdx = nodesToExplore[lastProcessedNode];
        const MapsIndexes & monsters = Maps::GetTilesUnderProtection( currentNodeIdx );
        PathfindingNode & currentNode = _cache[currentNodeIdx];

        // check if current tile is protected, can move only to adjacent monster
        if ( !monsters.empty() ) {
            for ( int monsterIndex : monsters ) {
                const int direction = Direction::Get( currentNodeIdx, monsterIndex );

                if ( direction != Direction::UNKNOWN && direction != Direction::CENTER && world.isValidPath( currentNodeIdx, direction ) ) {
                    // add straight to cache, can't move further from the monster
                    const uint32_t moveCost = currentNode._cost + getMovementPenalty( currentNodeIdx, monsterIndex, direction, skill );
                    PathfindingNode & monsterNode = _cache[monsterIndex];
                    if ( monsterNode._from == -1 || monsterNode._cost > moveCost ) {
                        monsterNode._from = currentNodeIdx;
                        monsterNode._cost = moveCost;
                    }
                }
            }
        }
        else if ( currentNodeIdx == start || !world.isTileBlocked( currentNodeIdx, fromWater ) ) {
            for ( auto it = directions.begin(); it != directions.end(); ++it ) {
                if ( Maps::isValidDirection( currentNodeIdx, *it ) ) {
                    const int newIndex = Maps::GetDirectionIndex( currentNodeIdx, *it );

                    const uint32_t moveCost = currentNode._cost + getMovementPenalty( currentNodeIdx, newIndex, *it, skill );
                    PathfindingNode & newNode = _cache[newIndex];
                    if ( world.isValidPath( currentNodeIdx, *it ) && ( newNode._from == -1 || newNode._cost > moveCost ) ) {
                        newNode._from = currentNodeIdx;
                        newNode._cost = moveCost;

                        // duplicates are allowed if we find a cheaper way there
                        nodesToExplore.push_back( newIndex );
                    }
                }
            }
        }
    }
}
