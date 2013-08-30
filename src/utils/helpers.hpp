//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013 Lauri Kasanen
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HELPERS_H
#define HELPERS_H

#include <ITexture.h>

using irr::video::ITexture;

float smootherstep(float, float, float);
float clampf(float, float, float);

float mix(float x, float y, float a);

unsigned ispow(const unsigned in);
unsigned npow(unsigned in);

void savetex(ITexture *tex, const char *name = NULL);

float noise2d(float v1, float v2 = 0);

u8 shash8(const u8 * const data, const u16 size);

#endif
