//
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

#ifndef SOCCER_WORLD_HPP
#define SOCCER_WORLD_HPP

#include "modes/world_with_rank.hpp"
#include "states_screens/race_gui_base.hpp"
#include "karts/abstract_kart.hpp"

#include <IMesh.h>

#include <string>

#define CLEAR_SPAWN_RANGE  5

class PhysicalObject;

/**
 * \brief An implementation of World, to provide the soccer game mode
 * \ingroup modes
 */
class SoccerWorld : public WorldWithRank
{
private:
    /** Number of goals each team scored
     */
    int m_team_goals[NB_SOCCER_TEAMS];
    
    /** Whether or not goals can be scored (they are disabled when a point is scored
    and re-enabled when the next game can be played)*/
    bool m_can_score_points;
    
public:
    
    SoccerWorld();
    virtual ~SoccerWorld() {}
    
    virtual void init();
    
    // clock events
    virtual bool isRaceOver();
    virtual void terminateRace();
    
    // overriding World methods
    virtual void reset();

    virtual bool useFastMusicNearEnd() const { return false; }
    virtual void getKartsDisplayInfo(
                          std::vector<RaceGUIBase::KartIconDisplayInfo> *info);
    virtual bool raceHasLaps(){ return false; }
    virtual void moveKartAfterRescue(AbstractKart* kart);
    
    virtual const std::string& getIdent() const;
    
    virtual void update(float dt);

    void onCheckGoalTriggered(bool first_goal);
    
private:
    void initKartList();
};   // SoccerWorld


#endif
