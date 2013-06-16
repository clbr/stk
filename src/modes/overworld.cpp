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

#include "audio/music_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "graphics/irr_driver.hpp"
#include "input/device_manager.hpp"
#include "input/input.hpp"
#include "input/input_manager.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "karts/rescue_animation.hpp"
#include "modes/overworld.hpp"
#include "physics/physics.hpp"
#include "network/network_manager.hpp"
#include "states_screens/dialogs/select_challenge.hpp"
#include "states_screens/kart_selection.hpp"
#include "states_screens/race_gui_overworld.hpp"
#include "tracks/track.hpp"

//-----------------------------------------------------------------------------
OverWorld::OverWorld() : LinearWorld()
{
    m_return_to_garage = false;
    m_stop_music_when_dialog_open = false;
}   // Overworld

//-----------------------------------------------------------------------------
OverWorld::~OverWorld()
{
    Vec3 kart_xyz = getKart(0)->getXYZ();
    race_manager->setKartLastPositionOnOverworld(kart_xyz);
}   // ~OverWorld

//-----------------------------------------------------------------------------
/** Function to simplify the start process */
void OverWorld::enterOverWorld()
{

    race_manager->setNumLocalPlayers(1);
    race_manager->setMajorMode (RaceManager::MAJOR_MODE_SINGLE);
    race_manager->setMinorMode (RaceManager::MINOR_MODE_OVERWORLD);
    race_manager->setNumKarts( 1 );
    race_manager->setTrack( "overworld" );
    race_manager->setDifficulty(RaceManager::DIFFICULTY_HARD);

    // Use keyboard 0 by default (FIXME: let player choose?)
    InputDevice* device = input_manager->getDeviceList()->getKeyboard(0);

    // Create player and associate player with keyboard
    StateManager::get()->createActivePlayer(unlock_manager->getCurrentPlayer(),
                                            device);

    if (!kart_properties_manager->getKart(UserConfigParams::m_default_kart))
    {
        Log::warn("overworld", "cannot find kart '%s', "
                  "will revert to default\n",
                  UserConfigParams::m_default_kart.c_str());

        UserConfigParams::m_default_kart.revertToDefaults();
    }
    race_manager->setLocalKartInfo(0, UserConfigParams::m_default_kart);

    // ASSIGN should make sure that only input from assigned devices
    // is read.
    input_manager->getDeviceList()->setAssignMode(ASSIGN);
    input_manager->getDeviceList()
        ->setSinglePlayer( StateManager::get()->getActivePlayer(0) );

    StateManager::get()->enterGameState();
    network_manager->setupPlayerKartInfo();
    race_manager->startNew(false);
    if(race_manager->haveKartLastPositionOnOverworld()){
			OverWorld *ow = (OverWorld*)World::getWorld();
			ow->getKart(0)->setXYZ(race_manager->getKartLastPositionOnOverworld());
			ow->moveKartAfterRescue(ow->getKart(0));
		}
    irr_driver->showPointer(); // User should be able to click on the minimap

}   // enterOverWorld

//-----------------------------------------------------------------------------
/** General update function called once per frame.
 *  \param dt Time step size.
 */
void OverWorld::update(float dt)
{
    // Skip annoying waiting without a purpose
    // Make sure to do all things that would normally happen in the
    // update() method of the base classes.
    if (getPhase() < GO_PHASE)
    {
        setPhase(RACE_PHASE);
        // Normally done in WorldStatus::update(), during phase SET_PHASE,
        // so we have to start music 'manually', since we skip all phases.
        World::getWorld()->getTrack()->startMusic();

        if (music_manager->getCurrentMusic() != NULL &&
            UserConfigParams::m_music)
            music_manager->getCurrentMusic()->startMusic();
        m_karts[0]->startEngineSFX();
    }
    LinearWorld::update(dt);
    const unsigned int kart_amount  = m_karts.size();

    // isn't it cool, on the overworld nitro is free!
    for(unsigned int n=0; n<kart_amount; n++)
    {
        m_karts[n]->setEnergy(100.0f);
    }

    if (m_return_to_garage)
    {
        m_return_to_garage = false;
        delayedSelfDestruct();
        race_manager->exitRace(false);
        KartSelectionScreen* s = KartSelectionScreen::getInstance();
        s->setMultiplayer(false);
        s->setFromOverworld(true);
        StateManager::get()->resetAndGoToScreen(s);
    }
}   // update

//-----------------------------------------------------------------------------
/** Override the base class method to change behavior. We don't want wrong
 *  direction messages in the overworld since there is no direction there.
 *  \param i Kart id.
 */
void OverWorld::checkForWrongDirection(unsigned int i)
{
}   // checkForWrongDirection

//-----------------------------------------------------------------------------

void OverWorld::createRaceGUI()
{
    m_race_gui = new RaceGUIOverworld();
}   // createRaceGUI

//-----------------------------------------------------------------------------

void OverWorld::onFirePressed(Controller* who)
{
    const std::vector<OverworldChallenge>& challenges =
                                                  m_track->getChallengeList();

    AbstractKart* k = getKart(0);
    Vec3 kart_xyz = k->getXYZ();
    if (dynamic_cast<RescueAnimation*>(k->getKartAnimation()) != NULL)
    {
        // you can't start a race while being rescued
        return;
    }

    for (unsigned int n=0; n<challenges.size(); n++)
    {
        if ( challenges[n].isForceFieldSet() &&
             challenges[n].getForceField().m_is_locked )
            continue;

        if ( (kart_xyz - Vec3(challenges[n].m_position)).length2_2d()
              < CHALLENGE_DISTANCE_SQUARED)
        {
            if (challenges[n].m_challenge_id == "tutorial")
            {
                scheduleTutorial();
                return;
            }
            else
            {
                race_manager->setKartLastPositionOnOverworld(kart_xyz);
                new SelectChallengeDialog(0.8f, 0.8f,
                                          challenges[n].m_challenge_id);
            }
        } // end if
    } // end for
}   // onFirePressed

//-----------------------------------------------------------------------------

btTransform OverWorld::getClosestStartPoint(float currentKart_x,
                                            float currentKart_z)
{
    // find closest point to drop kart on
    World *world = World::getWorld();
    const int start_spots_amount =
        world->getTrack()->getNumberOfStartPositions();
    assert(start_spots_amount > 0);


    int closest_id = -1;
    float closest_distance = 999999999.0f;

    for (int n=0; n<start_spots_amount; n++)
    {
        // no need for the overhead to compute exact distance with sqrt(),
        // so using the 'manhattan' heuristic which will do fine enough.
        const btTransform &s = world->getTrack()->getStartTransform(n);
        const Vec3 &v = s.getOrigin();

        float absDistance = fabs(currentKart_x - v.getX()) +
                    fabs(currentKart_z - v.getZ());

        if (absDistance < closest_distance)
        {
            closest_distance = absDistance;
            closest_id = n;
        }
    }

    assert(closest_id != -1);
    return world->getTrack()->getStartTransform(closest_id);
}   // getClosestStartPoint

//-----------------------------------------------------------------------------
/** Moves a kart to its rescue position.
 *  \param kart The kart that was rescued.
 */
void OverWorld::moveKartAfterRescue(AbstractKart* kart)
{
    moveKartAfterRescue(kart, 0);
}   // moveKartAfterRescue(AbstractKart*)

//-----------------------------------------------------------------------------

void OverWorld::moveKartAfterRescue(AbstractKart* kart, float angle)
{
    // find closest point to drop kart on
    World *world = World::getWorld();
    const int start_spots_amount =
        world->getTrack()->getNumberOfStartPositions();
    assert(start_spots_amount > 0);

    const float currentKart_x = kart->getXYZ().getX();
    const float currentKart_z = kart->getXYZ().getZ();

    const btTransform& s = getClosestStartPoint(currentKart_x, currentKart_z);
    const Vec3 &xyz = s.getOrigin();
    kart->setXYZ(xyz);
    kart->setRotation(s.getRotation());

    //position kart from same height as in World::resetAllKarts
    btTransform pos;
    pos.setOrigin( kart->getXYZ()
                  +btVector3(0, 0.5f*kart->getKartHeight(), 0.0f) );
    pos.setRotation( btQuaternion(btVector3(0.0f, 1.0f, 0.0f), angle) );

    kart->getBody()->setCenterOfMassTransform(pos);

    //project kart to surface of track
    bool kart_over_ground = m_track->findGround(kart);

    if (kart_over_ground)
    {
        //add vertical offset so that the kart starts off above the track
        float vertical_offset =
              kart->getKartProperties()->getVertRescueOffset()
            * kart->getKartHeight();
        kart->getBody()->translate(btVector3(0, vertical_offset, 0));
    }
    else
    {
        Log::warn("overworld", "Invalid position after rescue for kart %s "
        						"on track %s.", (kart->getIdent().c_str()),
                                m_track->getIdent().c_str());
    }
}   // moveKartAfterRescue

//-----------------------------------------------------------------------------
/** Called when a mouse click happens. If the click happened while the mouse
 *  was hovering on top of a challenge, the kart will be teleported to
 *  the challenge.
 *  \param x,y Mouse coordinates.
 */
void OverWorld::onMouseClick(int x, int y)
{
    const OverworldChallenge *challenge =
        ((RaceGUIOverworld*)getRaceGUI())->getCurrentChallenge();

    if(challenge)
    {
        AbstractKart* kart = getKart(0);
        const btTransform& s = getClosestStartPoint(challenge->m_position.X,
                                                    challenge->m_position.Z);
        const Vec3 &xyz = s.getOrigin();
        float angle = atan2(challenge->m_position.X - xyz[0],
                            challenge->m_position.Z - xyz[2]);
        kart->setXYZ(xyz);
        moveKartAfterRescue(kart, angle);
        return;
    }
}  // onMouseClick
