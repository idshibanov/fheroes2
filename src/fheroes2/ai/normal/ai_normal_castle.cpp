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

#include "ai_normal.h"
#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "race.h"
#include "world.h"

namespace AI
{
    struct buildOrder
    {
        building_t building;
        int priority;
    };

    const std::vector<buildOrder> & GetDefensiveStructures( int type )
    {
        static std::vector<buildOrder> defensive
            = {{BUILD_LEFTTURRET, 1}, {BUILD_RIGHTTURRET, 1}, {BUILD_MOAT, 1}, {BUILD_CAPTAIN, 1}, {BUILD_SPEC, 2}, {BUILD_TAVERN, 1}};

        return defensive;
    }

    const std::vector<buildOrder> & GetBuildOrder( int type )
    {
        static std::vector<buildOrder> genericBuildOrder
            = {{BUILD_CASTLE, 2},      {BUILD_STATUE, 1},      {DWELLING_UPGRADE7, 1}, {DWELLING_UPGRADE6, 1}, {DWELLING_MONSTER6, 1},  {DWELLING_UPGRADE5, 1},
               {DWELLING_MONSTER5, 1}, {DWELLING_UPGRADE4, 1}, {DWELLING_MONSTER4, 1}, {DWELLING_UPGRADE3, 2}, {DWELLING_MONSTER3, 2},  {DWELLING_UPGRADE2, 2},
               {DWELLING_MONSTER2, 3}, {DWELLING_MONSTER1, 4}, {BUILD_TAVERN, 2},      {BUILD_MAGEGUILD1, 2},  {BUILD_THIEVESGUILD, 3}, {BUILD_MAGEGUILD2, 3},
               {BUILD_SPEC, 5},        {BUILD_WEL2, 10},       {BUILD_MAGEGUILD3, 4},  {BUILD_MAGEGUILD4, 5},  {BUILD_MAGEGUILD5, 5}};

        // De-prioritizing dwelling 5, 1 and upgrades of 3 and 4
        // Well, tavern and Archery upgrade are more important
        static std::vector<buildOrder> knightBuildOrder
            = {{BUILD_CASTLE, 2},      {BUILD_STATUE, 1},      {DWELLING_UPGRADE6, 2}, {DWELLING_MONSTER6, 1},   {DWELLING_UPGRADE5, 2}, {DWELLING_MONSTER5, 2},
               {DWELLING_UPGRADE4, 2}, {DWELLING_MONSTER4, 1}, {DWELLING_UPGRADE3, 2}, {DWELLING_MONSTER3, 1},   {DWELLING_UPGRADE2, 1}, {DWELLING_MONSTER2, 3},
               {DWELLING_MONSTER1, 4}, {BUILD_WELL, 1},        {BUILD_TAVERN, 1},      {BUILD_MAGEGUILD1, 2},    {BUILD_MAGEGUILD2, 3},  {BUILD_MAGEGUILD3, 5},
               {BUILD_MAGEGUILD4, 5},  {BUILD_MAGEGUILD5, 5},  {BUILD_SPEC, 5},        {BUILD_THIEVESGUILD, 10}, {BUILD_WEL2, 20}};

        return ( type == Race::KNGT ) ? knightBuildOrder : genericBuildOrder;
    }

    bool Build( Castle & castle, const std::vector<buildOrder> & bList )
    {
        for ( std::vector<buildOrder>::const_iterator it = bList.begin(); it != bList.end(); ++it ) {
            if ( it->priority == 1 ) {
                if ( BuildIfAvailable( castle, it->building ) )
                    return true;
            }
            else {
                if ( BuildIfEnoughResources( castle, it->building, GetResourceMultiplier( castle, it->priority, it->priority + 1 ) ) )
                    return true;
            }
        }
        return false;
    }

    bool CastleDevelopment( Castle & castle )
    {
        if ( !castle.isBuild( BUILD_WELL ) && world.LastDay() ) {
            // return right away - if you can't buy Well you can't buy anything else
            return BuildIfAvailable( castle, BUILD_WELL );
        }

        if ( Build( castle, GetBuildOrder( castle.GetRace() ) ) ) {
            return true;
        }

        return Build( castle, GetDefensiveStructures( castle.GetRace() ) );
    }

    void Normal::CastleTurn( Castle & castle )
    {
        CastleDevelopment( castle );

        if ( world.LastDay() )
            castle.RecruitAllMonsters();
    }
}
