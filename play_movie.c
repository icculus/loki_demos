/*
    Loki_Demos - A demo launching UI for games distributed by Loki
    Copyright (C) 2000  Loki Software, Inc.

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    info@lokigames.com
*/

#include "SDL.h"
#include "SDL_mixer.h"
#include <smpeg/smpeg.h>
#include "play_movie.h"

//#define DOUBLE_SIZE_MOVIE

void play_movie(const char *movie, int depth)
{
    SDL_Surface *screen;
    SMPEG *mpeg;
    SMPEG_Info info;
    SMPEG_Filter *filter;
    int done;
    SDL_AudioSpec audiofmt;
    Uint16 format;
    int freq, channels;

    /* Load the movie */
    mpeg = SMPEG_new(movie, &info, 0);
    if ( SMPEG_error(mpeg) ) {
        SMPEG_delete(mpeg);
        return;
    }

    /* Set the 2X fullscreen video mode */
    SDL_WM_GrabInput(SDL_GRAB_ON);
    SDL_ShowCursor(0);
#ifdef DOUBLE_SIZE_MOVIE
    screen = SDL_SetVideoMode(info.width*2,info.height*2,depth,SDL_FULLSCREEN);
#else
    screen = SDL_SetVideoMode(info.width, info.height, depth, SDL_FULLSCREEN);
#endif
    if ( ! screen ) {
        SMPEG_delete(mpeg);
        return;
    }

    /* Hook it up to the mixer */
    Mix_QuerySpec(&freq, &format, &channels);
    audiofmt.format = format;
    audiofmt.freq = freq;
    audiofmt.channels = channels;
    SMPEG_actualSpec(mpeg, &audiofmt);
    Mix_HookMusic(SMPEG_playAudioSDL, mpeg);

    /* Set up a bilinear filter for the video */
    filter = SMPEGfilter_bilinear();
    filter = SMPEG_filter( mpeg, filter );
    filter->destroy(filter);

    /* Play, waiting for the end or escape */
    SMPEG_setdisplay(mpeg, screen, NULL, NULL);
    SMPEG_scaleXY(mpeg, screen->w, screen->h);
    SMPEG_play(mpeg);
    done = 0;
    while ( !done && (SMPEG_status(mpeg) == SMPEG_PLAYING) ) {
        SDL_Event event;

        while ( SDL_PollEvent(&event) ) {
            switch (event.type) {
                case SDL_MOUSEBUTTONUP:
                case SDL_KEYUP:
                case SDL_QUIT:
                    done = 1;
                    break;
            }
        }
    }

    /* Stop the movie and unhook the music */
    SMPEG_stop(mpeg);
    Mix_HookMusic(NULL, NULL);
    SMPEG_delete(mpeg);

    /* Release the mouse */
    SDL_ShowCursor(1);
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}
