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

#include "graphics/shadow.hpp"
#include "graphics/irr_driver.hpp"

#include <IMesh.h>
#include <IMeshSceneNode.h>
#include <ISceneNode.h>

Shadow::Shadow(video::ITexture *texture, scene::ISceneNode *node, float scale = 1.0, float xOffset = 0.0, float yOffset = 0.0)
{
    video::SMaterial m;
    m.setTexture(0, texture);
    m.BackfaceCulling = false;
    m.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
    m_mesh   = irr_driver->createQuadMesh(&m, /*create_one_quad*/true);
    scene::IMeshBuffer *buffer = m_mesh->getMeshBuffer(0);
    irr::video::S3DVertex* v=(video::S3DVertex*)buffer->getVertices();
    v[0].Pos.X = -scale+xOffset; v[0].Pos.Z =  scale+yOffset; v[0].Pos.Y = 0.01f;
    v[1].Pos.X =  scale+xOffset; v[1].Pos.Z =  scale+yOffset; v[1].Pos.Y = 0.01f;
    v[2].Pos.X =  scale+xOffset; v[2].Pos.Z = -scale+yOffset; v[2].Pos.Y = 0.01f;
    v[3].Pos.X = -scale+xOffset; v[3].Pos.Z = -scale+yOffset; v[3].Pos.Y = 0.01f;
    v[0].TCoords = core::vector2df(0,0);
    v[1].TCoords = core::vector2df(1,0);
    v[2].TCoords = core::vector2df(1,1);
    v[3].TCoords = core::vector2df(0,1);
    core::vector3df normal(0, 0, 1.0f);
    v[0].Normal  = normal;
    v[1].Normal  = normal;
    v[2].Normal  = normal;
    v[3].Normal  = normal;
    buffer->recalculateBoundingBox();
    
    m_node   = irr_driver->addMesh(m_mesh);
#ifdef DEBUG
    m_node->setName("shadow");
#endif

    m_mesh->drop();   // the node grabs the mesh, so we can drop this reference
    m_node->setAutomaticCulling(scene::EAC_OFF);
    m_parent_kart_node = node;
    m_parent_kart_node->addChild(m_node);
}   // Shadow

// ----------------------------------------------------------------------------
Shadow::~Shadow()
{
    // Note: the mesh was not loaded from disk, so it is not cached,
    // and does not need to be removed. It's clean up when removing the node
    m_parent_kart_node->removeChild(m_node);
}   // ~Shadow

// ----------------------------------------------------------------------------
/** Removes the shadow, used for the simplified shadow when the kart is in
 *  the air.
 */
void Shadow::disableShadow()
{
    m_node->setVisible(false);
}
// ----------------------------------------------------------------------------
/** Enables the shadow again, after it was disabled with disableShadow().
 */
void Shadow::enableShadow()
{
    m_node->setVisible(true);
}
// ----------------------------------------------------------------------------
