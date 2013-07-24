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

#ifndef HEADER_SHADERS_HPP
#define HEADER_SHADERS_HPP

#include <irrlicht.h>
#include <vector>
using namespace irr;

enum E_SHADER
{
    ES_NORMAL_MAP = 0,
    ES_NORMAL_MAP_LIGHTMAP,
    ES_SPLATTING,
    ES_WATER,
    ES_SPHERE_MAP,
    ES_GRASS,
    ES_BUBBLES,
    ES_RAIN,
    ES_SNOW,
    ES_MOTIONBLUR,
    ES_GAUSSIAN3H,
    ES_GAUSSIAN3V,
    ES_MIPVIZ,
    ES_FLIP,
    ES_FLIP_ADDITIVE,
    ES_BLOOM,
    ES_GAUSSIAN6H,
    ES_GAUSSIAN6V,
    ES_COLORIZE,
    ES_PASS,
    ES_PASS_ADDITIVE,
    ES_GLOW,
    ES_OBJECTPASS,
    ES_OBJECTPASS_REF,
    ES_LIGHTBLEND,
    ES_POINTLIGHT,
    ES_SUNLIGHT,
    ES_OBJECTPASS_RIMLIT,
    ES_MLAA_COLOR1,
    ES_MLAA_BLEND2,
    ES_MLAA_NEIGH3,
    ES_SSAO,

    ES_COUNT
};

class Shaders
{
public:
    Shaders();
    ~Shaders();

    video::E_MATERIAL_TYPE getShader(const E_SHADER num) const;

    video::IShaderConstantSetCallBack * m_callbacks[ES_COUNT];

private:
    void check(const int num) const;

    int m_shaders[ES_COUNT];
};

#endif
