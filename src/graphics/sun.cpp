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

#include "graphics/sun.hpp"

#include "graphics/callbacks.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/material.hpp"
#include "graphics/rtts.hpp"
#include "graphics/screenquad.hpp"
#include "graphics/shaders.hpp"

using namespace video;
using namespace scene;
using namespace core;

SunNode::SunNode(scene::ISceneManager* mgr, float r, float g, float b):
                     LightNode(mgr, 0, r, g, b)
{

    sq = new screenQuad(irr_driver->getVideoDriver());

    SMaterial &m = sq->getMaterial();

    m.MaterialType = irr_driver->getShaders()->getShader(ES_SUNLIGHT);
    m.setTexture(0, irr_driver->getRTTs()->getRTT(RTT_NORMAL));
    m.setTexture(1, irr_driver->getRTTs()->getRTT(RTT_DEPTH));

    m.setFlag(EMF_BILINEAR_FILTER, false);
    m.MaterialTypeParam = pack_textureBlendFunc(EBF_ONE, EBF_ONE);
    m.BlendOperation = EBO_ADD;

    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
}

SunNode::~SunNode()
{
    delete sq;
}

void SunNode::render()
{
    SunLightProvider * const cb = (SunLightProvider *) irr_driver->getShaders()->
                                   m_callbacks[ES_SUNLIGHT];
    cb->setColor(m_color[0], m_color[1], m_color[2]);

    vector3df pos = getPosition();
    pos.normalize();
    cb->setPosition(pos.X, pos.Y, pos.Z);

    sq->render(false);
}
