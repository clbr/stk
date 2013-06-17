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

#include "graphics/shaders.hpp"
#include "utils/log.hpp"

#include <assert.h>

Shaders::Shaders()
{

}

Shaders::~Shaders()
{

}

video::E_MATERIAL_TYPE Shaders::getShader(const E_SHADER num) const
{
    assert(num < ES_COUNT);
}

void Shaders::check(const int num) const
{
    if (num == -1)
    {
        Log::fatal("shaders", "Shader failed to load. Update your drivers, if the issue "
                              "persists, report a bug to us.");
    }
}
