//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004 SuperTuxKart-Team
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

#ifndef HEADER_OVERWORLD_HPP
#define HEADER_OVERWORLD_HPP

#include <vector>

#include "modes/linear_world.hpp"
#include "utils/aligned_array.hpp"

#include "LinearMath/btTransform.h"

/*
 * The overworld map where challenges are played.
 * \note This mode derives from LinearWorld to get support for drivelines,
 *       minimap and rescue, even though this world is not technically
 *       linear.
 * \ingroup modes
 */
class OverWorld : public LinearWorld
{
protected:
    
    /** Override from base class */
    virtual void  createRaceGUI();
    
    bool m_return_to_garage;
    
    void moveKartAfterRescue(AbstractKart* kart, float angle);
    
    btTransform getClosestStartPoint(float currentKart_x, float currentKart_z);

public:
                  OverWorld();
    virtual      ~OverWorld();

    static void enterOverWorld();
    
    virtual void  update(float delta) OVERRIDE;
    
    // ------------------------------------------------------------------------
    /** Returns if this race mode has laps. */
    virtual bool  raceHasLaps() OVERRIDE { return false; }
    // ------------------------------------------------------------------------
    virtual void checkForWrongDirection(unsigned int i) OVERRIDE;
    // ------------------------------------------------------------------------
    /** The overworld is not a race per se so it's never over */
    virtual bool    isRaceOver() OVERRIDE { return false; }
    // ------------------------------------------------------------------------
    /** Implement base class method */
    virtual const std::string& 
                    getIdent() const OVERRIDE { return IDENT_OVERWORLD; }
    // ------------------------------------------------------------------------
    /** Override base class method */
    virtual bool shouldDrawTimer() const OVERRIDE { return false; }
    // ------------------------------------------------------------------------
    /** Override base class method */
    virtual void onFirePressed(Controller* who) OVERRIDE;
    // ------------------------------------------------------------------------
    /** Override settings from base class */
    virtual bool useChecklineRequirements() const OVERRIDE { return false; }
    // ------------------------------------------------------------------------
    void scheduleSelectKart() { m_return_to_garage = true; }
    // ------------------------------------------------------------------------
    virtual void moveKartAfterRescue(AbstractKart* kart) OVERRIDE;
    // ------------------------------------------------------------------------
    virtual void onMouseClick(int x, int y) OVERRIDE;
};

#endif
