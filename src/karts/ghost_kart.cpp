//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2012  Joerg Henrichs
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

#include "karts/ghost_kart.hpp"
#include "modes/world.hpp"

#include "LinearMath/btQuaternion.h"
#include "utils/log.hpp"

GhostKart::GhostKart(const std::string& ident)
             : Kart(ident, /*world kart id*/99999,
                    /*position*/-1, btTransform())
{
    m_current_transform = 0;
    m_next_event        = 0;
}   // GhostKart

// ----------------------------------------------------------------------------
void GhostKart::reset()
{
    m_node->setVisible(true);
    Kart::reset();
    m_current_transform = 0;
    m_next_event        = 0;
    // This will set the correct start position
    update(0);
}   // reset

// ----------------------------------------------------------------------------
/** Sets the next time and transform. The current time and transform becomes
 *  the previous time and transform.
 *  \param
 */
void GhostKart::addTransform(float time, const btTransform &trans)
{
    // FIXME: for now avoid that transforms for the same time are set
    // twice (to avoid division by zero in update). This should be
    // done when saving in replay
    if(m_all_times.size()>0 && m_all_times.back()==time)
        return;
    m_all_times.push_back(time);
    m_all_transform.push_back(trans);
}   // addTransform

// ----------------------------------------------------------------------------
/** Adds a replay event for this kart.
 */
void GhostKart::addReplayEvent(const ReplayBase::KartReplayEvent &kre)
{
    m_replay_events.push_back(kre);
}   // addReplayEvent

// ----------------------------------------------------------------------------
/** Updates the ghost data each time step. It uses interpolation to get a new
 *  position and rotation.
 *  \param dt Time step size.
 */
void GhostKart::update(float dt)
{
    float t = World::getWorld()->getTime();
    // Don't do anything at startup
    if(t==0) return;
    updateTransform(t, dt);
    while(m_next_event < m_replay_events.size() &&
          m_replay_events[m_next_event].m_time <= t)
    {
        Log::debug("Ghost_Kart", "Handling event %d", m_next_event);
        // Handle the next event now
        m_next_event++;
    }
}
// ----------------------------------------------------------------------------
/** Updates the current transform of the ghost kart using interpolation
 *  \param t Current world time.
 *  \param dt Time step size.
 */
void GhostKart::updateTransform(float t, float dt)
{

    // Find (if necessary) the next index to use
    while(m_current_transform+1 < m_all_times.size() &&
          t>=m_all_times[m_current_transform+1])
    {
          m_current_transform ++;
    }
    if(m_current_transform+1>=m_all_times.size())
    {
        m_node->setVisible(false);
        return;
    }

    float f =(t - m_all_times[m_current_transform])
           / (  m_all_times[m_current_transform+1]
              - m_all_times[m_current_transform]  );
    setXYZ((1-f)*m_all_transform[m_current_transform  ].getOrigin()
           + f  *m_all_transform[m_current_transform+1].getOrigin() );
    const btQuaternion q = m_all_transform[m_current_transform].getRotation()
                          .slerp(m_all_transform[m_current_transform+1]
                                 .getRotation(),
                                 f);
    setRotation(q);
    Moveable::updateGraphics(dt, Vec3(0,0,0), btQuaternion(0, 0, 0, 1));
}   // update
