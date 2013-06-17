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

#include "graphics/irr_driver.hpp"
#include "utils/helpers.hpp"

float clampf(float in, float low, float high) {
    return in > high ? high : in < low ? low : in;
}

float smootherstep(float e0, float e1, float x) {
    x = clampf((x - e0)/(e1 - e0), 0, 1);

    return x*x*x*(x*(x*6 - 15) + 10);
}

void savetex(ITexture *tex, const char *name) {

    using namespace core;
    using namespace video;

    IVideoDriver * const drv = irr_driver->getVideoDriver();

    IImage * const tmp = drv->createImage(tex, position2di(0,0), tex->getSize());

    if (!name)
    {
        stringc namec = tex->getName().getPath();
        namec += ".png";
        drv->writeImageToFile(tmp, namec.c_str());
    }
    else
    {
        drv->writeImageToFile(tmp, name);
    }

    tmp->drop();
}

float mix(float x, float y, float a) {
    return x * (1 - a) + y * a;
}

unsigned ispow(const unsigned in) {

    if (in < 2) return 0;

    return !(in & (in - 1));
}

unsigned npow(unsigned in) {

    if (ispow(in)) return in;

    in |= in >> 1;
    in |= in >> 2;
    in |= in >> 4;
    in |= in >> 8;
    in |= in >> 16;

    return in + 1;
}
