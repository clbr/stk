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


#include "karts/abstract_kart.hpp"

#include "items/powerup.hpp"
#include "karts/abstract_kart_animation.hpp"
#include "karts/kart_model.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "utils/log.hpp"

/** Creates a kart.
 *  \param ident The identifier of the kart.
 *  \param world_kart_id  The world index of this kart.
 *  \param position The start position of the kart (1<=position<=n).
 *  \param init_transform The start position of the kart.
 */
AbstractKart::AbstractKart(const std::string& ident,
                           int world_kart_id, int position,
                           const btTransform& init_transform)
             : Moveable()
{
    m_world_kart_id   = world_kart_id;
    m_kart_properties = kart_properties_manager->getKart(ident);
    m_kart_animation  = NULL;
    assert(m_kart_properties != NULL);

    // We have to take a copy of the kart model, since otherwise
    // the animations will be mixed up (i.e. different instances of
    // the same model will set different animation frames).
    // Technically the mesh in m_kart_model needs to be grab'ed and
    // released when the kart is deleted, but since the original
    // kart_model is stored in the kart_properties all the time,
    // there is no risk of a mesh being deleted to early.
    m_kart_model  = m_kart_properties->getKartModelCopy();
    m_kart_width  = m_kart_model->getWidth();
    m_kart_height = m_kart_model->getHeight();
    m_kart_length = m_kart_model->getLength();
}   // AbstractKart

// ----------------------------------------------------------------------------
AbstractKart::~AbstractKart()
{
    delete m_kart_model;
    if(m_kart_animation)
        delete m_kart_animation;
}   // ~AbstractKart

// ----------------------------------------------------------------------------
void AbstractKart::reset()
{
    Moveable::reset();
    if(m_kart_animation)
    {
        delete m_kart_animation;
        m_kart_animation = NULL;
    }
}   // reset

// ----------------------------------------------------------------------------
/** Returns a name to be displayed for this kart. */
const wchar_t* AbstractKart::getName() const
{
    return m_kart_properties->getName();
}   // getName;
// ----------------------------------------------------------------------------
/** Returns a unique identifier for this kart (name of the directory the
 *  kart was loaded from). */
const std::string& AbstractKart::getIdent() const
{
    return m_kart_properties->getIdent();
}   // getIdent
// ----------------------------------------------------------------------------
bool AbstractKart::isWheeless() const
{
    return m_kart_model->getWheelModel(0)==NULL;
}   // isWheeless

// ----------------------------------------------------------------------------
/** Sets a new kart animation. This function should either be called to
 *  remove an existing kart animation (ka=NULL), or to set a new kart
 *  animation, in which case the current kart animation must be NULL.
 *  \param ka The new kart animation, or NULL if the current kart animation
 *            is to be stopped.
 */
void AbstractKart::setKartAnimation(AbstractKartAnimation *ka)
{
#ifdef DEBUG
    if( ( (ka!=NULL) ^ (m_kart_animation!=NULL) ) ==0)
    {
        if(ka) Log::debug("Abstract_Kart", "Setting kart animation to '%s'.",
                          ka->getName().c_str());
        else   Log::debug("Abstract_Kart", "Setting kart animation to NULL.");
        if(m_kart_animation) Log::info("Abstract_Kart", "Current kart"
                                       "animation is '%s'.\n",
                                        m_kart_animation->getName().c_str());
        else   Log::debug("Abstract_Kart", "Current kart animation is NULL.");
    }
#endif
    // Make sure that the either the current animation is NULL and a new (!=0)
    // is set, or there is a current animation, then it must be set to 0. This
    // makes sure that the calling logic of this function is correct.
    assert( (ka!=NULL) ^ (m_kart_animation!=NULL) );
    m_kart_animation = ka;
}   // setKartAnimation
