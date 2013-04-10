
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010  Joerg Henrichs
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

#ifndef HEADER_CONTROLLER_HPP
#define HEADER_CONTROLLER_HPP

#include <irrString.h>
using namespace irr;

/**
  * \defgroup controller Karts/controller
  * Contains kart controllers, which are either human players or AIs
  * (this module thus contains the AIs)
  */

#include "input/input.hpp"
#include "states_screens/state_manager.hpp"

class AbstractKart;
class Item;
class KartControl;
class Material;

/** This is the base class for kart controller - that can be a player 
 *  or a a robot.
 * \ingroup controller
 */
class Controller
{
protected:
    /** Pointer to the kart that is controlled by this controller. */
    AbstractKart *m_kart;

    /** A pointer to the main controller, from which the kart takes
     *  it commands. */
    KartControl  *m_controls;

    /** If this belongs to a player, it stores the active player data
     *  structure. Otherwise it is 0. */
    StateManager::ActivePlayer *m_player;

    /** The name of the controller, mainly used for debugging purposes. */
    std::string  m_controller_name;
public:
                  Controller         (AbstractKart *kart, 
                                      StateManager::ActivePlayer *player=NULL);
    virtual      ~Controller         () {};
    virtual void  reset              () = 0;
    virtual void  update             (float dt) = 0;
    virtual void  handleZipper       (bool play_sound) = 0;
    virtual void  collectedItem      (const Item &item, int add_info=-1,
                                      float previous_energy=0) = 0;
    virtual void  crashed            (const AbstractKart *k) = 0;
    virtual void  crashed            (const Material *m) = 0;
    virtual void  setPosition        (int p) = 0;
    virtual bool  isPlayerController () const = 0;
    virtual bool  isNetworkController() const = 0;
    virtual bool  disableSlipstreamBonus() const = 0;
	// ---------------------------------------------------------------------------
    /** Sets the controller name for this controller. */
    virtual void setControllerName(const std::string &name) 
                                                 { m_controller_name = name; }
	// ---------------------------------------------------------------------------
    /** Returns the name of this controller. */
    const std::string &getControllerName() const { return m_controller_name; }
	// ---------------------------------------------------------------------------
    /** Returns the active player for this controller (NULL 
     *  if this controller does not belong to a player.    */
    StateManager::ActivePlayer *getPlayer () {return m_player;}
    
	// ---------------------------------------------------------------------------
	/** Returns the player object (or NULL if it's a computer controller). */
    const StateManager::ActivePlayer *getPlayer () const { return m_player; }
    
    // ------------------------------------------------------------------------
    /** Default: ignore actions. Only PlayerController get them. */
    virtual void action(PlayerAction action, int value) = 0;
    // ------------------------------------------------------------------------
    /** Callback whenever a new lap is triggered. Used by the AI
     *  to trigger a recomputation of the way to use.            */
    virtual void  newLap(int lap) = 0;
    // ------------------------------------------------------------------------
    virtual void  skidBonusTriggered() = 0;
    // ------------------------------------------------------------------------
    /** Called whan this controller's kart finishes the last lap. */
    virtual void  finishedRace(float time) = 0;
    // ------------------------------------------------------------------------
};   // Controller

#endif

/* EOF */
