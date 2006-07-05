/*
    freeHeroes2 engine
    turn-based game engine (clone of Heroes Of the Might and Magic II)
    Copyright (C) 2006

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    3DO, New World Computing, Heroes of Might and Magic, and the related
    3DO logos are trademarks and or registered trademarks of 3DO Company.
    All other trademarks belong to their respective owners.

    Web: http://sourceforge.net/projects/fheroes2

    Description:
*/

#include "SDL.h"
#include "gamedefs.h"
#include "config.h"
#include "heroes.h"
#include "kingdom.h"


static S_KINGDOM kingdom[KINGDOMMAX] = {
	{FALSE, NULL, NULL, { HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL } }, // BLUE
	{FALSE, NULL, NULL, { HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL } }, // RED
	{FALSE, NULL, NULL, { HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL } }, // GREEN
	{FALSE, NULL, NULL, { HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL } }, // YELLOW
	{FALSE, NULL, NULL, { HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL } }, // ORANGE
	{FALSE, NULL, NULL, { HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL, HEROESNULL } }, // PURPLE
};

BOOL	InitKingdom(void){

    if(GetIntValue(KINGDOMCOLORS) & BLUE)   kingdom[BLUE].play   = TRUE;
    if(GetIntValue(KINGDOMCOLORS) & GREEN)  kingdom[GREEN].play  = TRUE;
    if(GetIntValue(KINGDOMCOLORS) & RED)    kingdom[RED].play    = TRUE;
    if(GetIntValue(KINGDOMCOLORS) & YELLOW) kingdom[YELLOW].play = TRUE;
    if(GetIntValue(KINGDOMCOLORS) & ORANGE) kingdom[ORANGE].play = TRUE;
    if(GetIntValue(KINGDOMCOLORS) & PURPLE) kingdom[PURPLE].play = TRUE;

    if(GetIntValue(DEBUG)){
	fprintf(stderr, "InitKingdom:");
	if(GetIntValue(KINGDOMCOLORS) & BLUE)   fprintf(stderr, " BLUE");
	if(GetIntValue(KINGDOMCOLORS) & GREEN)  fprintf(stderr, " GREEN");
	if(GetIntValue(KINGDOMCOLORS) & RED)    fprintf(stderr, " RED");
	if(GetIntValue(KINGDOMCOLORS) & YELLOW) fprintf(stderr, " YELLOW");
	if(GetIntValue(KINGDOMCOLORS) & ORANGE) fprintf(stderr, " ORANGE");
	if(GetIntValue(KINGDOMCOLORS) & PURPLE) fprintf(stderr, " PURPLE");
	fprintf(stderr, "\n");
    }
    
    return TRUE;
}

void	FreeKingdom(void){

    Uint8 dom, name;
    
    for(dom = 0; dom < KINGDOMMAX; ++dom){

	kingdom[dom].play = FALSE;
	kingdom[dom].build = NULL;
	kingdom[dom].castle = NULL;

	for(name = 0; name < KINGDOMMAXHEROES; ++name) kingdom[dom].nameheroes[name] = HEROESNULL;
    }
}

void    KingdomAddHeroes(E_COLORS color, E_NAMEHEROES name){

    if(! kingdom[color].play) return;

    Uint8 i;
    for(i = 0; i < KINGDOMMAXHEROES; ++i)

	if(HEROESNULL == kingdom[color].nameheroes[i]){
	    kingdom[color].nameheroes[i] = name;
	    return;
	}

    if(GetIntValue(DEBUG)) fprintf(stderr, "KingdomAddHeroes: define KINGDOMMAXHEROES %d\n", KINGDOMMAXHEROES);
}

void    KingdomRemoveHeroes(E_COLORS color, E_NAMEHEROES name){

    Uint8 i;
    for(i = 0; i < KINGDOMMAXHEROES; ++i)

	if(name == kingdom[color].nameheroes[i]){
	    kingdom[color].nameheroes[i] = HEROESNULL;
	    return;
	}
}


void    KingdomAddCastle(E_COLORS color, S_CASTLE *castle){

    if(! kingdom[color].play) return;

    if(! castle){
	fprintf(stderr, "KingdomAddCastle: error: *castle NULL\n");
	return;
    }
    
    S_CASTLE *ptrCastle = kingdom[color].castle;

    while(ptrCastle) ptrCastle = (S_CASTLE *) ptrCastle->next;

    ptrCastle->next = castle;
}

void    KingdomRemoveCastle(E_COLORS color, S_CASTLE *castle){

    if(! castle){
	fprintf(stderr, "KingdomRemoveCastle: error: *castle NULL\n");
	return;
    }
    
    S_CASTLE *head = kingdom[color].castle;

    if(castle->pos.x == head->pos.x && castle->pos.y == head->pos.y){
	kingdom[color].castle = head->next;
	head->next = NULL;
	return;
    }

    S_CASTLE *tail = head->next;

    while(tail){
	
	if(castle->pos.x == tail->pos.x && castle->pos.y == tail->pos.y){
	    
	    head->next = tail->next;
	    tail= NULL;
	    return;
	}
	
	head = (S_CASTLE *) head->next;
	tail = (S_CASTLE *) tail->next;
    }
}
