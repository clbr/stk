//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2011 Joerg Henrichs
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

#ifndef HEADER_STK_DYNAMICS_WORLD_HPP
#define HEADER_STK_DYNAMICS_WORLD_HPP

#include "btBulletDynamicsCommon.h"

class STKDynamicsWorld : public btDiscreteDynamicsWorld
{
public:
	/** The standard constructor which just created a btDiscreteDynamicsWorld. */
    STKDynamicsWorld(btDispatcher*             dispatcher,
                     btBroadphaseInterface*    pairCache,
                     btConstraintSolver*       constraintSolver,
                     btCollisionConfiguration* collisionConfiguration)

                   : btDiscreteDynamicsWorld(dispatcher, pairCache,
                                             constraintSolver,
                                             collisionConfiguration)
    {
    }

    /** Resets m_localTime to 0. This allows more precise replay of
     *  physics, which is important for replaying histories. */
    virtual void resetLocalTime() { m_localTime = 0; }

};   // STKDynamicsWorld
#endif
/* EOF */

