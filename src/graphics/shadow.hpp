//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009  Joerg Henrichs
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

#ifndef HEADER_SHADOW_HPP
#define HEADER_SHADOW_HPP

#include "utils/no_copy.hpp"

#include <string>

namespace irr
{
    namespace scene { class ISceneNode; class IMesh; }
    namespace video { class ITexture; }
}
using namespace irr;

/**
 * \brief This class is used to enable a shadow for a kart.
 * For now it uses a simple texture to simulate the shadow, real time shadows might
 * be added later.
 * \ingroup graphics
 */
class Shadow : public NoCopy
{
private:
    /** The scene node for the shadow. */
    scene::ISceneNode  *m_node;
    /** The mesh of the shadow. */
    scene::IMesh       *m_mesh;
    /** The scene node of the kart to which this shadow belongs. */
    scene::ISceneNode  *m_parent_kart_node;
public:
         Shadow(video::ITexture *texture,
                scene::ISceneNode *node,
                float scale, float xOffset, float yOffset);
        ~Shadow();
    void enableShadow();
    void disableShadow();
};   // Shadow
#endif

/* EOF */


