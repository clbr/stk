//  SuperTuxKart - a fun racing game with go-kart
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

#include "graphics/screenquad.hpp"

// Just the static parts to our screenquad

const u16 ScreenQuad::indices[4] = {0, 1, 2, 3};

static const SColor white(255, 255, 255, 255);

const S3DVertex ScreenQuad::vertices[4] = {
		S3DVertex(-1, 1, 0, 0, 1, 0, white, 0, 1),
		S3DVertex(1, 1, 0, 0, 1, 0, white, 1, 1),
		S3DVertex(-1, -1, 0, 0, 1, 0, white, 0, 0),
		S3DVertex(1, -1, 0, 0, 1, 0, white, 1, 0),
		};
