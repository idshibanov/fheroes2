/***************************************************************************
 *   Copyright (C) 2006 by Andrey Afletdinov                               *
 *   afletdinov@mail.dc.baikal.ru                                          *
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

#include <string>
#include "rect.h"
#include "types.h"
#include "error.h"
#include "display.h"

Display::Display()
{
    videosurface = true;
}

Display::~Display()
{
}

Display & Display::operator= (const Display & dp)
{
    surface = SDL_GetVideoSurface();

    return *this;
}

void Display::SetVideoMode(Display::resolution_t mode, bool fullscreen)
{
    u16 xres, yres;

    switch(mode)
    {
	default:
	case SMALL:
	    xres = 640;
	    yres = 480;
	    break;

	case MEDIUM:
	    xres = 800;
	    yres = 576;
	    break;

	case LARGE:
	    xres = 1024;
	    yres = 768;
	    break;

	case XLARGE:
	    xres = 1280;
	    yres = 1024;
	    break;
    }

    Display & display = Display::Get();

    if(display.valid() && display.w() == xres && display.h() == yres) return;

    u32 videoflags = SDL_HWPALETTE|SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_HWACCEL;

    if(fullscreen || (display.valid() && (display.flags() & SDL_FULLSCREEN))) videoflags |= SDL_FULLSCREEN;

    if(!SDL_SetVideoMode(xres, yres, 0, videoflags))
    {
	SDL_SetVideoMode(640, 480, 0, videoflags);

	Error::Warning(std::string(SDL_GetError()));
    }
}

/* flip */
void Display::Flip(void)
{
    Display & display = Display::Get();

    SDL_Flip(display.surface);
}

/* full screen */
void Display::FullScreen(void)
{
    Display & display = Display::Get();

    SDL_WM_ToggleFullScreen(display.surface);
}

/* set caption main window */
void Display::SetCaption(const std::string & caption)
{
    SDL_WM_SetCaption(caption.c_str(), NULL);
}

/* set icons window */
void Display::SetIcons(const Surface & icons)
{
    SDL_WM_SetIcon(const_cast<SDL_Surface *>(icons.GetSurface()), NULL);
}

/* hide system cursor */
void Display::HideCursor(void)
{
    SDL_ShowCursor(SDL_DISABLE);
}

/* show system cursor */
void Display::ShowCursor(void)
{
    SDL_ShowCursor(SDL_ENABLE);
}

/* get video display */
Display & Display::Get(void)
{
    static Display inside;

    if(! inside.surface) inside.surface = SDL_GetVideoSurface();

    return inside;
}
