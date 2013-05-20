//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2005 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006-2007 Eduardo Hernandez Munoz
//  Copyright (C) 2008-2012 Joerg Henrichs
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


//The AI debugging works best with just 1 AI kart, so set the number of karts
//to 2 in main.cpp with quickstart and run supertuxkart with the arg -N.
#ifdef DEBUG
   // Enable AI graphical debugging
#  undef AI_DEBUG
   // Shows left and right lines when using new findNonCrashing function
#  undef AI_DEBUG_NEW_FIND_NON_CRASHING
   // Show the predicted turn circles
#  undef AI_DEBUG_CIRCLES
   // Show the heading of the kart
#  undef AI_DEBUG_KART_HEADING
   // Shows line from kart to its aim point
#  undef AI_DEBUG_KART_AIM
#endif

#include "karts/controller/skidding_ai.hpp"

#ifdef AI_DEBUG
#  include "graphics/irr_driver.hpp"
#endif
#include "graphics/show_curve.hpp"
#include "graphics/slip_stream.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/controller/kart_control.hpp"
#include "karts/controller/ai_properties.hpp"
#include "karts/kart_properties.hpp"
#include "karts/max_speed.hpp"
#include "karts/rescue_animation.hpp"
#include "karts/skidding.hpp"
#include "karts/skidding_properties.hpp"
#include "items/attachment.hpp"
#include "items/item_manager.hpp"
#include "items/powerup.hpp"
#include "modes/linear_world.hpp"
#include "modes/profile_world.hpp"
#include "network/network_manager.hpp"
#include "race/race_manager.hpp"
#include "tracks/quad_graph.hpp"
#include "tracks/track.hpp"
#include "utils/constants.hpp"
#include "utils/log.hpp"

#ifdef AI_DEBUG
#  include "irrlicht.h"
   using namespace irr;
#endif

#if defined(WIN32) && !defined(__CYGWIN__)  && !defined(__MINGW32__)
#  define isnan _isnan
#else
#  include <math.h>
#endif

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>

SkiddingAI::SkiddingAI(AbstractKart *kart) 
                   : AIBaseController(kart)
{
    reset();
    // Determine if this AI has superpowers, which happens e.g.
    // for the final race challenge against nolok.
    m_superpower = race_manager->getAISuperPower();

    m_point_selection_algorithm = PSA_DEFAULT;
    setControllerName("Skidding");

    // Use this define in order to compare the different algorithms that
    // select the next point to steer at.
#undef COMPARE_AIS
#ifdef COMPARE_AIS
    std::string name("");
    m_point_selection_algorithm = m_kart->getWorldKartId() % 2
                                ? PSA_DEFAULT : PSA_FIXED;
    switch(m_point_selection_algorithm)
    {
    case PSA_FIXED   : name = "Fixed";   break;
    case PSA_NEW     : name = "New";     break;
    case PSA_DEFAULT : name = "Default"; break;
    }
    setControllerName(name);
#endif


    // Draw various spheres on the track for an AI
#ifdef AI_DEBUG
    for(unsigned int i=0; i<4; i++)
    {
        video::SColor col_debug(128, i==0 ? 128 : 0, 
                                     i==1 ? 128 : 0,
                                     i==2 ? 128 : 0);
        m_debug_sphere[i] = irr_driver->addSphere(1.0f, col_debug);
        m_debug_sphere[i]->setVisible(false);
    }
    m_debug_sphere[m_point_selection_algorithm]->setVisible(true);
    m_item_sphere  = irr_driver->addSphere(1.0f);

#define CURVE_PREDICT1   0
#define CURVE_KART       1
#define CURVE_LEFT       2
#define CURVE_RIGHT      3
#define CURVE_AIM        4
#define CURVE_QG         5
#define NUM_CURVES (CURVE_QG+1)

    m_curve   = new ShowCurve*[NUM_CURVES];
    for(unsigned int i=0; i<NUM_CURVES; i++)
        m_curve[i] = NULL;
#ifdef AI_DEBUG_CIRCLES
    m_curve[CURVE_PREDICT1]  = new ShowCurve(0.05f, 0.5f, 
                                   irr::video::SColor(128,   0,   0, 128));
#endif
#ifdef AI_DEBUG_KART_HEADING
    irr::video::SColor c;
    c = irr::video::SColor(128,   0,   0, 128);
    m_curve[CURVE_KART]      = new ShowCurve(0.5f, 0.5f, c);
#endif
#ifdef AI_DEBUG_NEW_FIND_NON_CRASHING
    m_curve[CURVE_LEFT]      = new ShowCurve(0.5f, 0.5f, 
                                            video::SColor(128, 128,   0,   0));
    m_curve[CURVE_RIGHT]     = new ShowCurve(0.5f, 0.5f, 
                                            video::SColor(128,   0, 128,   0));
#endif
    m_curve[CURVE_QG]        = new ShowCurve(0.5f, 0.5f, 
                                            video::SColor(128,   0, 128,   0));
#ifdef AI_DEBUG_KART_AIM
    irr::video::SColor c1;
    c1 = irr::video::SColor(128,   0,   0, 128);

    m_curve[CURVE_AIM]       = new ShowCurve(0.5f, 0.5f, c1);
#endif
#endif

}   // SkiddingAI

//-----------------------------------------------------------------------------
/** The destructor deletes the shared TrackInfo objects if no more SkiddingAI
 *  instances are around.
 */
SkiddingAI::~SkiddingAI()
{
#ifdef AI_DEBUG
    for(unsigned int i=0; i<3; i++)
        irr_driver->removeNode(m_debug_sphere[i]);
    irr_driver->removeNode(m_item_sphere);
    for(unsigned int i=0; i<NUM_CURVES; i++)
    {
        delete m_curve[i];
    }
    delete m_curve;
#endif
}   // ~SkiddingAI

//-----------------------------------------------------------------------------
/** Resets the AI when a race is restarted.
 */
void SkiddingAI::reset()
{
    m_time_since_last_shot       = 0.0f;
    m_start_kart_crash_direction = 0;
    m_start_delay                = -1.0f;
    m_time_since_stuck           = 0.0f;
    m_kart_ahead                 = NULL;
    m_distance_ahead             = 0.0f;
    m_kart_behind                = NULL;
    m_distance_behind            = 0.0f;
    m_current_curve_radius       = 0.0f;
    m_curve_center               = Vec3(0,0,0);
    m_current_track_direction    = GraphNode::DIR_STRAIGHT;
    m_item_to_collect            = NULL;
    m_avoid_item_close           = false;
    m_skid_probability_state     = SKID_PROBAB_NOT_YET;
    m_last_item_random           = NULL;

    AIBaseController::reset();
    m_track_node               = QuadGraph::UNKNOWN_SECTOR;
    QuadGraph::get()->findRoadSector(m_kart->getXYZ(), &m_track_node);
    if(m_track_node==QuadGraph::UNKNOWN_SECTOR)
    {
        Log::error("SkiddingAI", 
                "Invalid starting position for '%s' - not on track"
                " - can be ignored.\n",
                m_kart->getIdent().c_str());
        m_track_node = QuadGraph::get()->findOutOfRoadSector(m_kart->getXYZ());
    }

	AIBaseController::reset();
}   // reset

//-----------------------------------------------------------------------------
/** Returns a name for the AI.
 *  This is used in profile mode when comparing different AI implementations
 *  to be able to distinguish them from each other.
 */
const irr::core::stringw& SkiddingAI::getNamePostfix() const 
{
    // Static to avoid returning the address of a temporary string.
    static irr::core::stringw name="(default)";
    return name;
}   // getNamePostfix

//-----------------------------------------------------------------------------
/** Returns the pre-computed successor of a graph node.
 *  \param index The index of the graph node for which the successor
 *               is searched.
 */
unsigned int SkiddingAI::getNextSector(unsigned int index)
{
    return m_successor_index[index];
}   // getNextSector

//-----------------------------------------------------------------------------
/** This is the main entry point for the AI.
 *  It is called once per frame for each AI and determines the behaviour of
 *  the AI, e.g. steering, accelerating/braking, firing.
 */
void SkiddingAI::update(float dt)
{
    // This is used to enable firing an item backwards.
    m_controls->m_look_back = false;
    m_controls->m_nitro     = false;

    // Don't do anything if there is currently a kart animations shown.
    if(m_kart->getKartAnimation())
        return;

    if (m_superpower == RaceManager::SUPERPOWER_NOLOK_BOSS)
    {
        if (m_kart->getPowerup()->getType()==PowerupManager::POWERUP_NOTHING)
        {
            if (m_kart->getPosition() > 1)
            {
                int r = rand() % 5;
                if (r == 0 || r == 1)
                    m_kart->setPowerup(PowerupManager::POWERUP_ZIPPER, 1);
                else if (r == 2 || r == 3)
                    m_kart->setPowerup(PowerupManager::POWERUP_BUBBLEGUM, 1);
                else
                    m_kart->setPowerup(PowerupManager::POWERUP_SWATTER, 1);
            }
            else if (m_kart->getAttachment()->getType() == Attachment::ATTACH_SWATTER)
            {
                int r = rand() % 4;
                if (r < 3)
                    m_kart->setPowerup(PowerupManager::POWERUP_BUBBLEGUM, 1);
                else
                    m_kart->setPowerup(PowerupManager::POWERUP_BOWLING, 1);
            }
            else
            {
                int r = rand() % 5;
                if (r == 0 || r == 1)
                    m_kart->setPowerup(PowerupManager::POWERUP_BUBBLEGUM, 1);
                else if (r == 2 || r == 3)
                    m_kart->setPowerup(PowerupManager::POWERUP_SWATTER, 1);
                else
                    m_kart->setPowerup(PowerupManager::POWERUP_BOWLING, 1);
            }
            
            // also give him some free nitro
            if (m_kart->getPosition() > 1)
                m_kart->setEnergy(m_kart->getEnergy() + 7);
            else
                m_kart->setEnergy(m_kart->getEnergy() + 4);
        }
    }

    // Having a non-moving AI can be useful for debugging, e.g. aiming
    // or slipstreaming.
#undef AI_DOES_NOT_MOVE_FOR_DEBUGGING
#ifdef AI_DOES_NOT_MOVE_FOR_DEBUGGING
    m_controls->m_accel     = 0;
    m_controls->m_steer     = 0;
    return;
#endif

    // The client does not do any AI computations.
    if(network_manager->getMode()==NetworkManager::NW_CLIENT) 
    {
        AIBaseController::update(dt);
        return;
    }

    // If the kart needs to be rescued, do it now (and nothing else)
    if(isStuck() && !m_kart->getKartAnimation())
    {
        new RescueAnimation(m_kart);
        AIBaseController::update(dt);
        return;
    }

    if( m_world->isStartPhase() )
    {
        handleRaceStart();
        AIBaseController::update(dt);
        return;
    }

    // Get information that is needed by more than 1 of the handling funcs
    computeNearestKarts();

    m_kart->setSlowdown(MaxSpeed::MS_DECREASE_AI, 
                        m_ai_properties->getSpeedCap(m_distance_to_player),
                        /*fade_in_time*/0.0f);
    //Detect if we are going to crash with the track and/or kart
    checkCrashes(m_kart->getXYZ());
    determineTrackDirection();
    
    // Special behaviour if we have a bomb attach: try to hit the kart ahead 
    // of us.
    bool commands_set = false;
    if(m_ai_properties->m_handle_bomb && 
        m_kart->getAttachment()->getType()==Attachment::ATTACH_BOMB && 
        m_kart_ahead )
    {
        // Use nitro if the kart is far ahead, or faster than this kart
        m_controls->m_nitro = m_distance_ahead>10.0f || 
                             m_kart_ahead->getSpeed() > m_kart->getSpeed();
        // If we are close enough, try to hit this kart
        if(m_distance_ahead<=10)
        {
            Vec3 target = m_kart_ahead->getXYZ();

            // If we are faster, try to predict the point where we will hit
            // the other kart
            if(m_kart_ahead->getSpeed() < m_kart->getSpeed())
            {
                float time_till_hit = m_distance_ahead
                                    / (m_kart->getSpeed()-m_kart_ahead->getSpeed());
                target += m_kart_ahead->getVelocity()*time_till_hit;
            }
            float steer_angle = steerToPoint(target);
            setSteering(steer_angle, dt);
            commands_set = true;
        }
        handleRescue(dt);
    }
    if(!commands_set)
    {
        /*Response handling functions*/
        handleAcceleration(dt);
        handleSteering(dt);
        handleItems(dt);
        handleRescue(dt);
        handleBraking();
        // If a bomb is attached, nitro might already be set.
        if(!m_controls->m_nitro)
            handleNitroAndZipper();
    }
    // If we are supposed to use nitro, but have a zipper, 
    // use the zipper instead (unless there are items to avoid cloe by)
    if(m_controls->m_nitro && 
        m_kart->getPowerup()->getType()==PowerupManager::POWERUP_ZIPPER && 
        m_kart->getSpeed()>1.0f && 
        m_kart->getSpeedIncreaseTimeLeft(MaxSpeed::MS_INCREASE_ZIPPER)<=0 &&
        !m_avoid_item_close)
    {
        // Make sure that not all AI karts use the zipper at the same
        // time in time trial at start up, so during the first 5 seconds
        // this is done at random only.
        if(race_manager->getMinorMode()!=RaceManager::MINOR_MODE_TIME_TRIAL ||
            (m_world->getTime()<3.0f && rand()%50==1) )
        {
            m_controls->m_nitro = false;
            m_controls->m_fire  = true;
        }
    }

    /*And obviously general kart stuff*/
    AIBaseController::update(dt);
}   // update

//-----------------------------------------------------------------------------
/** This function decides if the AI should brake.
 *  The decision can be based on race mode (e.g. in follow the leader the AI
 *  will brake if it is ahead of the leader). Otherwise it will depend on
 *  the direction the AI is facing (if it's not facing in the track direction
 *  it will brake in order to make it easier to re-align itself), and 
 *  estimated curve radius (brake to avoid being pushed out of a curve).
 */
void SkiddingAI::handleBraking()
{
    m_controls->m_brake = false;
    // In follow the leader mode, the kart should brake if they are ahead of
    // the leader (and not the leader, i.e. don't have initial position 1)
    if(race_manager->getMinorMode() == RaceManager::MINOR_MODE_FOLLOW_LEADER &&
        m_kart->getPosition() < m_world->getKart(0)->getPosition()           &&
        m_kart->getInitialPosition()>1                                         )
    {
#ifdef DEBUG
    if(m_ai_debug)
        Log::debug("SkiddingAI", "braking: %s ahead of leader.\n",
               m_kart->getIdent().c_str());
#endif

        m_controls->m_brake = true;
        return;
    }
    
    // A kart will not brake when the speed is already slower than this 
    // value. This prevents a kart from going too slow (or even backwards)
    // in tight curves.
    const float MIN_SPEED = 5.0f;

    // If the kart is not facing roughly in the direction of the track, brake
    // so that it is easier for the kart to turn in the right direction.
    if(m_current_track_direction==GraphNode::DIR_UNDEFINED &&
        m_kart->getSpeed() > MIN_SPEED)
    {
#ifdef DEBUG
        if(m_ai_debug)
            Log::debug("SkiddingAI", "%s not aligned with track.\n",
                   m_kart->getIdent().c_str());
#endif
        m_controls->m_brake = true;
        return;
    }
    if(m_current_track_direction==GraphNode::DIR_LEFT ||
       m_current_track_direction==GraphNode::DIR_RIGHT   )
    {
        float max_turn_speed = 
            m_kart->getKartProperties()
                   ->getSpeedForTurnRadius(m_current_curve_radius);

        if(m_kart->getSpeed() > 1.5f*max_turn_speed  && 
            m_kart->getSpeed()>MIN_SPEED             &&
            fabsf(m_controls->m_steer) > 0.95f          )
        {
            m_controls->m_brake = true;
#ifdef DEBUG
            if(m_ai_debug)
                Log::debug("SkiddingAI", 
                       "speed %f too tight curve: radius %f ",
                       m_kart->getSpeed(),
                       m_kart->getIdent().c_str(),
                       m_current_curve_radius);
#endif
        }
        return;
    }

    return;

}   // handleBraking

//-----------------------------------------------------------------------------
/** Decides in which direction to steer. If the kart is off track, it will
 *  steer towards the center of the track. Otherwise it will call one of
 *  the findNonCrashingPoint() functions to determine a point to aim for. Then
 *  it will evaluate items to see if it should aim for any items or try to 
 *  avoid item, and potentially adjust the aim-at point, before computing the
 *  steer direction to arrive at the currently aim-at point.
 *  \param dt Time step size.
 */
void SkiddingAI::handleSteering(float dt)
{
    const int next = m_next_node_index[m_track_node];
    
    float steer_angle = 0.0f;

    /*The AI responds based on the information we just gathered, using a
     *finite state machine.
     */
    //Reaction to being outside of the road
	float side_dist = 
		m_world->getDistanceToCenterForKart( m_kart->getWorldKartId() );

    if( fabsf(side_dist)  >
       0.5f* QuadGraph::get()->getNode(m_track_node).getPathWidth()+0.5f )
    {
        steer_angle = steerToPoint(QuadGraph::get()->getQuadOfNode(next)
                                                    .getCenter());

#ifdef AI_DEBUG
        m_debug_sphere[0]->setPosition(QuadGraph::get()->getQuadOfNode(next)
                       .getCenter().toIrrVector());
        Log::debug("skidding_ai","-Outside of road: steer to center point.\n");
#endif
    }
    //If we are going to crash against a kart, avoid it if it doesn't
    //drives the kart out of the road
    else if( m_crashes.m_kart != -1 && !m_crashes.m_road )
    {
        //-1 = left, 1 = right, 0 = no crash.
        if( m_start_kart_crash_direction == 1 )
        {
            steer_angle = steerToAngle(next, -M_PI*0.5f );
            m_start_kart_crash_direction = 0;
        }
        else if(m_start_kart_crash_direction == -1)
        {
            steer_angle = steerToAngle(next, M_PI*0.5f);
            m_start_kart_crash_direction = 0;
        }
        else
        {
            if(m_world->getDistanceToCenterForKart( m_kart->getWorldKartId() ) >
               m_world->getDistanceToCenterForKart( m_crashes.m_kart ))
            {
                steer_angle = steerToAngle(next, -M_PI*0.5f );
                m_start_kart_crash_direction = 1;
            }
            else
            {
                steer_angle = steerToAngle(next, M_PI*0.5f );
                m_start_kart_crash_direction = -1;
            }
        }

#ifdef AI_DEBUG
        Log::debug("skidding_ai",  "- Velocity vector crashes with kart "
                   "and doesn't crashes with road : steer 90 " 
                   "degrees away from kart.\n");
#endif

    }
    else
    {
        m_start_kart_crash_direction = 0;
        Vec3 aim_point;
        int last_node = QuadGraph::UNKNOWN_SECTOR;

        switch(m_point_selection_algorithm)
        {
        case PSA_FIXED : findNonCrashingPointFixed(&aim_point, &last_node);
                         break;
        case PSA_NEW:    findNonCrashingPointNew(&aim_point, &last_node);
                         break;
        case PSA_DEFAULT:findNonCrashingPoint(&aim_point, &last_node);
                         break;
        }
#ifdef AI_DEBUG
        m_debug_sphere[m_point_selection_algorithm]->setPosition(aim_point.toIrrVector());
#endif
#ifdef AI_DEBUG_KART_AIM
        const Vec3 eps(0,0.5f,0);
        m_curve[CURVE_AIM]->clear();
        m_curve[CURVE_AIM]->addPoint(m_kart->getXYZ()+eps);
        m_curve[CURVE_AIM]->addPoint(aim_point);
#endif

        // Potentially adjust the point to aim for in order to either
        // aim to collect item, or steer to avoid a bad item.
        if(m_ai_properties->m_collect_avoid_items)
            handleItemCollectionAndAvoidance(&aim_point, last_node);

        steer_angle = steerToPoint(aim_point);
    }  // if m_current_track_direction!=LEFT/RIGHT

    setSteering(steer_angle, dt);
}   // handleSteering

//-----------------------------------------------------------------------------
/** Decides if the currently selected aim at point (as determined by 
 *  handleSteering) should be changed in order to collect/avoid an item.
 *  There are 5 potential phases:
 *  1. Collect all items close by and sort them by items-to-avoid and 
 *     items-to-collect. 'Close by' are all items between the current
 *     graph node the kart is on and the graph node the aim_point is on.
 *     The function evaluateItems() filters those items: atm all items-to-avoid
 *     are collected, and all items-to-collect that are not too far away from
 *     the intended driving direction (i.e. don't require a too sharp steering
 *     change).
 *  2. If a pre-selected item (see phase 5) exists, and items-to-avoid which
 *     might get hit if the pre-selected item is collected, the pre-selected
 *     item is unselected. This can happens if e.g. items-to-avoid are behind
 *     the pre-selected items on a different graph node and were therefore not
 *     evaluated then the now pre-selected item was selected initially.
 *  3. If a pre-selected item exists, the kart will steer towards it. The AI
 *     does a much better job of collecting items if after selecting an item
 *     it tries to collect this item even if it doesn't fulfill the original
 *     conditions to be selected in the first place anymore. Example: An item
 *     was selected to be collected because the AI was hitting it anyway. Then
 *     the aim_point changes, and the selected item is not that close anymore.
 *     In many cases it is better to keep on aiming for the items (otherwise
 *     the aiming will not have much benefit and mostly only results in 
 *     collecting items that are on long straights).
 *     At this stage because of phase 2) it is certain that no item-to-avoid 
 *     will be hit. The function handleSelectedItem() evaluates if it is still
 *     feasible to collect them item (if the kart has already passed the item
 *     it won't reverse to collect it). If the item is still to be aimed for,
 *     adjust the aim_point and return.
 *  4. Make sure to avoid any items-to-avoid. The function steerToAvoid
 *     selects a new aim point if otherwise an item-to-avoid would be hit.
 *     If this is the case, aim_point is adjused and control returns to the
 *     caller.
 *  5. Try to collect an item-to-collect. Select the closest item to the
 *     kart (which in case of a row of items will be the item the kart
 *     is roughly driving towards to anyway). It is then tested if the kart
 *     would hit any item-to-avoid when driving towards this item - if so
 *     the driving direction is not changed and the function returns. 
 *     Otherwise, i.e. no items-to-avoid will be hit, it is evaluated if
 *     the kart is (atm) going to hit it anyway. In this case, the item is
 *     selected (see phase 2 and 3). If on the other hand the item is not 
 *     too far our of the way, aim_point is adjusted to drive towards
 *     this item (but it is not selected, so the item will be discarded if
 *     the kart is getting too far away from it). Ideally later as the
 *     kart is steering towards this item the item will be selected.
 *
 *  \param aim_point Currently selected point to aim at. If the AI should
 *         try to collect an item, this value will be changed.
 *  \param last_node Index of the graph node on which the aim_point is.
 */
void SkiddingAI::handleItemCollectionAndAvoidance(Vec3 *aim_point, 
                                                  int last_node)
{
#ifdef AI_DEBUG
    m_item_sphere->setVisible(false);
#endif
    // Angle of line from kart to aim_point
    float kart_aim_angle = atan2(aim_point->getX()-m_kart->getXYZ().getX(),
                                 aim_point->getZ()-m_kart->getXYZ().getZ());

    // Make sure we have a valid last_node
    if(last_node==QuadGraph::UNKNOWN_SECTOR)
        last_node = m_next_node_index[m_track_node];

    int node = m_track_node;
    float distance = 0;
    std::vector<const Item *> items_to_collect;
    std::vector<const Item *> items_to_avoid;

    // 1) Filter and sort all items close by
    // -------------------------------------
    const float max_item_lookahead_distance = 30.f;
    while(distance < max_item_lookahead_distance)
    {
        int q_index= QuadGraph::get()->getNode(node).getQuadIndex();
        const std::vector<Item *> &items_ahead = 
            ItemManager::get()->getItemsInQuads(q_index);
        for(unsigned int i=0; i<items_ahead.size(); i++)
        {
            evaluateItems(items_ahead[i],  kart_aim_angle, 
                          &items_to_avoid, &items_to_collect);
        }   // for i<items_ahead;
        distance += QuadGraph::get()->getDistanceToNext(node, 
                                                      m_successor_index[node]);
        node = m_next_node_index[node];
        // Stop when we have reached the last quad
        if(node==last_node) break;
    }   // while (distance < max_item_lookahead_distance)

    m_avoid_item_close = items_to_avoid.size()>0;

    core::line2df line_to_target(aim_point->getX(), 
                                 aim_point->getZ(),
                                 m_kart->getXYZ().getX(), 
                                 m_kart->getXYZ().getZ());

    // 2) If the kart is aiming for an item, but (suddenly) detects 
    //    some close-by items to avoid (e.g. behind the item, which was too
    //    far away to be considered earlier), the kart cancels collecting
    //    the item if this could cause the item-to-avoid to be collected.
    // --------------------------------------------------------------------
    if(m_item_to_collect && items_to_avoid.size()>0)
    {
        for(unsigned int i=0; i< items_to_avoid.size(); i++)
        {
            Vec3 d = items_to_avoid[i]->getXYZ()-m_item_to_collect->getXYZ();
            if( d.length2_2d()>m_ai_properties->m_bad_item_closeness_2)
                continue;
            // It could make sense to also test if the bad item would 
            // actually be hit, not only if it is close (which can result
            // in false positives, i.e. stop collecting an item though
            // it is not actually necessary). But in (at least) one case 
            // steering after collecting m_item_to_collect causes it to then
            // collect the bad item (it's too close to avoid it at that
            // time). So for now here is no additional test, if we see
            // false positives we can handle it here.
            m_item_to_collect = NULL;
            break;
        }   // for i<items_to_avoid.size()
    }   // if m_item_to_collect && items_to_avoid.size()>0


    // 3) Steer towards a pre-selected item
    // -------------------------------------
    if(m_item_to_collect)
    {
        if(handleSelectedItem(kart_aim_angle, aim_point))
        {
            // Still aim at the previsouly selected item.
            *aim_point = m_item_to_collect->getXYZ();
#ifdef AI_DEBUG
            m_item_sphere->setVisible(true);
            m_item_sphere->setPosition(aim_point->toIrrVector());
#endif
            return;
        }

        if(m_ai_debug)
            Log::debug("SkiddingAI", "%s unselects item.\n", 
                        m_kart->getIdent().c_str());
        // Otherwise remove the pre-selected item (and start
        // looking for a new item).
        m_item_to_collect = NULL;
    }   // m_item_to_collect

    // 4) Avoid items-to-avoid
    // -----------------------
    if(items_to_avoid.size()>0)
    {
        // If we need to steer to avoid an item, this takes priority,
        // ignore items to collect and return the new aim_point.
        if(steerToAvoid(items_to_avoid, line_to_target, aim_point))
        {
#ifdef AI_DEBUG
            m_item_sphere->setVisible(true);
            m_item_sphere->setPosition(aim_point->toIrrVector());
#endif
            return;
        }
    }

    // 5) We are aiming for a new item. If necessary, determine
    // randomly if this item sshould actually be collected.
    // --------------------------------------------------------
    if(items_to_collect.size()>0)
    {
        if(items_to_collect[0] != m_last_item_random)
        {
            int p = (int)(100.0f*m_ai_properties->
                          getItemCollectProbability(m_distance_to_player));
            m_really_collect_item = m_random_collect_item.get(100)<p;
            m_last_item_random = items_to_collect[0];
        }
        if(!m_really_collect_item)
        {
            // The same item was selected previously, but it was randomly
            // decided not to collect it - so keep on ignoring this item.
            return;
        }
    }

    // Reset the probability if a different (or no) item is selected.
    if(items_to_collect.size()==0 || items_to_collect[0]!=m_last_item_random)
        m_last_item_random = NULL;

    // 6) Try to aim for items-to-collect
    // ----------------------------------
    if(items_to_collect.size()>0)
    {
        const Item *item_to_collect = items_to_collect[0];
        // Test if we would hit a bad item when aiming at this good item.
        // If so, don't change the aim. In this case it has already been
        // ensured that we won't hit the bad item (otherwise steerToAvoid
        // would have detected this earlier).
        if(!hitBadItemWhenAimAt(item_to_collect, items_to_avoid))
        {
            // If the item is hit (with the current steering), it means
            // it's on a good enough driveline, so make this item a permanent
            // target. Otherwise only try to get closer (till hopefully this
            // item s on our driveline)
            if(item_to_collect->hitLine(line_to_target, m_kart))
            {
#ifdef AI_DEBUG
                m_item_sphere->setVisible(true);
                m_item_sphere->setPosition(item_to_collect->getXYZ()
                                                           .toIrrVector());
#endif
                if(m_ai_debug)
                    Log::debug("SkiddingAI", "%s selects item type '%d'.\n",
                           m_kart->getIdent().c_str(),
                           item_to_collect->getType());
                m_item_to_collect = item_to_collect;
            }
            else
            {
                // Kart will not hit item, try to get closer to this item
                // so that it can potentially become a permanent target.
                Vec3 xyz = item_to_collect->getXYZ();
                float item_angle = atan2(xyz.getX() - m_kart->getXYZ().getX(),
                                         xyz.getZ() - m_kart->getXYZ().getZ());
                float angle = normalizeAngle(kart_aim_angle - item_angle);

                if(fabsf(angle) < 0.3)
                {
                    *aim_point = item_to_collect->getXYZ();
#ifdef AI_DEBUG
                    m_item_sphere->setVisible(true);
                    m_item_sphere->setPosition(item_to_collect->getXYZ()
                                                               .toIrrVector());
#endif
                    if(m_ai_debug)                
                        Log::debug("SkiddingAI", 
                               "%s adjusts to hit type %d angle %f.\n",
                               m_kart->getIdent().c_str(),
                               item_to_collect->getType(), angle);
                }
                else
                {
                    if(m_ai_debug)
                        Log::debug("SkiddingAI", 
                               "%s won't hit '%d', angle %f.\n",
                               m_kart->getIdent().c_str(),
                               item_to_collect->getType(), angle);
                }
            }   // kart will not hit item
        }   // does hit hit bad item
    }   // if item to consider was found

}   // handleItemCollectionAndAvoidance

//-----------------------------------------------------------------------------
/** Returns true if the AI would hit any of the listed bad items when trying
 *  to drive towards the specified item.
 *  \param item The item the AI is evaluating for collection.
 *  \param items_to_aovid A list of bad items close by. All of these needs
 *        to be avoided.
 *  \return True if it would hit any of the bad items.
*/
bool SkiddingAI::hitBadItemWhenAimAt(const Item *item, 
                              const std::vector<const Item *> &items_to_avoid)
{
    core::line2df to_item(m_kart->getXYZ().getX(), m_kart->getXYZ().getZ(),
                          item->getXYZ().getX(),   item->getXYZ().getZ()   );
    for(unsigned int i=0; i<items_to_avoid.size(); i++)
    {
        if(items_to_avoid[i]->hitLine(to_item, m_kart))
            return true;
    }
    return false;
}   // hitBadItemWhenAimAt

//-----------------------------------------------------------------------------
/** This function is called when the AI is trying to hit an item that is 
 *  pre-selected to be collected. The AI only evaluates if it's still
 *  feasible/useful to try to collect this item, or abandon it (and then 
 *  look for a new item). At item is unselected if the kart has passed it 
 *  (so collecting it would require the kart to reverse).
 *  \pre m_item_to_collect is defined
 *  \param kart_aim_angle The angle towards the current aim_point.
 *  \param aim_point The current aim_point.
 *  \param last_node
 *  \return True if th AI should still aim for the pre-selected item.
 */
bool SkiddingAI::handleSelectedItem(float kart_aim_angle, Vec3 *aim_point)
{
    // If the item is unavailable or has been switched into a bad item
    // stop aiming for it.
    if(m_item_to_collect->getDisableTime()>0 ||
        m_item_to_collect->getType() == Item::ITEM_BANANA ||
        m_item_to_collect->getType() == Item::ITEM_BANANA   )
        return false;

    const Vec3 &xyz = m_item_to_collect->getXYZ();
    float item_angle = atan2(xyz.getX() - m_kart->getXYZ().getX(),
                             xyz.getZ() - m_kart->getXYZ().getZ());
    
    float angle = normalizeAngle(kart_aim_angle - item_angle);
    if(fabsf(angle)>1.5)
    {
        // We (likely) have passed the item we were aiming for
        return false;
    }
    else
    {
        // Keep on aiming for last selected item
#ifdef AI_DEBUG
        m_item_sphere->setVisible(true);
        m_item_sphere->setPosition(m_item_to_collect->getXYZ().toIrrVector());
#endif
        *aim_point = m_item_to_collect->getXYZ();
    }
    return true;
}   // handleSelectedItem

//-----------------------------------------------------------------------------
/** Decides if steering is necessary to avoid bad items. If so, it modifies
 *  the aim_point and returns true.
 *  \param items_to_avoid List of items to avoid.
 *  \param line_to_target The 2d line from the current kart position to
 *         the current aim point.
 *  \param aim_point The point the AI is steering towards (not taking items
 *         into account).
 *  \return True if steering is necessary to avoid an item.
 */
bool SkiddingAI::steerToAvoid(const std::vector<const Item *> &items_to_avoid,
                              const core::line2df &line_to_target,
                              Vec3 *aim_point)
{
    // First determine the left-most and right-most item. 
    float left_most        = items_to_avoid[0]->getDistanceFromCenter();
    float right_most       = items_to_avoid[0]->getDistanceFromCenter();
    int   index_left_most  = 0;
    int   index_right_most = 0;

    for(unsigned int i=1; i<items_to_avoid.size(); i++)
    {
        float dist = items_to_avoid[i]->getDistanceFromCenter();
        if (dist<left_most)
        {
            left_most       = dist;
            index_left_most = i;
        }
        if(dist>right_most)
        {
            right_most       = dist;
            index_right_most = i;
        }
    }

    // Check if we would drive left of the leftmost or right of the
    // rightmost point - if so, nothing to do.
    core::vector2df left(items_to_avoid[index_left_most]->getXYZ().getX(),
                         items_to_avoid[index_left_most]->getXYZ().getZ());
    int item_index = -1;
    bool is_left    = false;

    // >=0 means the point is to the right of the line, or the line is
    // to the left of the point.
    if(line_to_target.getPointOrientation(left)>=0)
    {
        // Left of leftmost point
        item_index = index_left_most;            
        is_left       = true;
    }
    else
    {
        core::vector2df right(items_to_avoid[index_right_most]->getXYZ().getX(),
                              items_to_avoid[index_right_most]->getXYZ().getZ());
        if(line_to_target.getPointOrientation(right)<=0)
        {
            // Right of rightmost point
            item_index = index_right_most;
            is_left    = false;
        }
    }
    if(item_index>-1)
    {
        // Even though we are on the side, we must make sure 
        // that we don't hit that item
        // If we don't hit the item on the side, no more tests are necessary
        if(!items_to_avoid[item_index]->hitLine(line_to_target, m_kart))
            return false;

        // See if we can avoid this item by driving further to the side
        const Vec3 *avoid_point = items_to_avoid[item_index]
                                ->getAvoidancePoint(is_left);
        // If avoid_point is NULL, it means steering more to the sides 
        // brings us off track. In this case just try to steer to the
        // other side (e.g. when hitting a left-most item and the kart can't
        // steer further left, steer a bit to the right of the left-most item
        // (without further tests if we might hit anything else).
        if(!avoid_point)
            avoid_point = items_to_avoid[item_index]
                        ->getAvoidancePoint(!is_left);
        *aim_point = *avoid_point;
        return true;
    }
    
    // At this stage there must be at least two items - if there was
    // only a single item, the 'left of left-most' or 'right of right-most'
    // tests above are had been and an appropriate steering point was already
    // determined.

    // Try to identify two items we are driving between (if the kart is not 
    // driving between two items, one of the 'left of left-most' etc.
    // tests before applied and this point would not be reached).

    float min_distance[2] = {99999.9f, 99999.9f};
    int   index[2] = {-1, -1};
    core::vector2df closest2d[2];
    for(unsigned int i=0; i<items_to_avoid.size(); i++)
    {
        const Vec3 &xyz         = items_to_avoid[i]->getXYZ();
        core::vector2df item2d  = xyz.toIrrVector2d();
        core::vector2df point2d = line_to_target.getClosestPoint(item2d);
        float d = (xyz.toIrrVector2d() - point2d).getLengthSQ();
        float direction = line_to_target.getPointOrientation(item2d);
        int ind = direction<0 ? 0 : 1;
        if(d<min_distance[ind])
        {
            min_distance[ind] = d;
            index[ind]        = i;
            closest2d[ind]    = point2d;
        }
    }
    
    assert(index[0]!= index[1]);
    assert(index[0]!=-1       );
    assert(index[1]!=-1       );

    // We are driving between item_to_avoid[index[0]] and ...[1].
    // If we don't hit any of them, just keep on driving as normal
    bool hit_left  = items_to_avoid[index[0]]->hitKart(closest2d[0], m_kart);
    bool hit_right = items_to_avoid[index[1]]->hitKart(closest2d[1], m_kart);
    if( !hit_left && !hit_right)
        return false;
    
    // If we hit the left item, aim at the right avoidance point 
    // of the left item. We might still hit the right item ... this might
    // still be better than going too far off track
    if(hit_left)
    {
        *aim_point = 
            *(items_to_avoid[index[0]]->getAvoidancePoint(/*left*/false));
        return true;
    }

    // Now we must be hitting the right item, so try aiming at the left
    // avoidance point of the right item.
    *aim_point = *(items_to_avoid[index[1]]->getAvoidancePoint(/*left*/true));
    return true;
}   // steerToAvoid

//-----------------------------------------------------------------------------
/** This subroutine decides if the specified item should be collected, 
 *  avoided, or ignored. It can potentially use the state of the
 *  kart to make this decision, e.g. depending on what item the kart currently
 *  has, how much nitro it has etc. Though atm it picks the first good item,
 *  and tries to avoid any bad items on the track.
 *  \param item The item which is considered for picking/avoidance.
 *  \param kart_aim_angle The angle of the line from the kart to the aim point.
 *         If aim_angle==kart_heading then the kart is driving towards the 
 *         item.
 *  \param item_to_avoid A pointer to a previously selected item to avoid
 *         (NULL if no item was avoided so far).
 *  \param item_to_collect A pointer to a previously selected item to collect.
 */
void SkiddingAI::evaluateItems(const Item *item, float kart_aim_angle, 
                               std::vector<const Item *> *items_to_avoid, 
                               std::vector<const Item *> *items_to_collect)
{
    // Ignore items that are currently disabled
    if(item->getDisableTime()>0) return;

    // If the item type is not handled here, ignore it
    Item::ItemType type = item->getType();
    if( type!=Item::ITEM_BANANA    && type!=Item::ITEM_BUBBLEGUM &&
        type!=Item::ITEM_BONUS_BOX &&
        type!=Item::ITEM_NITRO_BIG && type!=Item::ITEM_NITRO_SMALL  )
        return;

    bool avoid = false;
    switch(type)
    {
        // Negative items: avoid them
        case Item::ITEM_BUBBLEGUM: // fallthrough
        case Item::ITEM_BANANA: avoid = true;  break;

        // Positive items: try to collect
        case Item::ITEM_NITRO_BIG:   
            // Only collect nitro, if it can actually be stored.
            if(m_kart->getEnergy() + 
                    m_kart->getKartProperties()->getNitroBigContainer() 
                  > m_kart->getKartProperties()->getNitroMax())
                  return;
            // fall through: if we have enough space to store a big
            // container, we can also store a small container, and
            // finally fall through to the bonus box code.
        case Item::ITEM_NITRO_SMALL: avoid = false; break;
            // Only collect nitro, if it can actually be stored.
            if (m_kart->getEnergy() + 
                    m_kart->getKartProperties()->getNitroSmallContainer() 
                  > m_kart->getKartProperties()->getNitroMax())
                  return;
        case Item::ITEM_BONUS_BOX:
            break;
        case Item::ITEM_TRIGGER: return; break;

        default: assert(false); break;
    }    // switch


    // Ignore items to be collected that are out of our way (though all items
    // to avoid are collected).
    if(!avoid)
    {
        // item_angle The angle of the item (relative to the forward axis,
        // so 0 means straight ahead in world coordinates!).
        const Vec3 &xyz = item->getXYZ();
        float item_angle = atan2(xyz.getX() - m_kart->getXYZ().getX(),
                                 xyz.getZ() - m_kart->getXYZ().getZ());

        float diff = normalizeAngle(kart_aim_angle-item_angle);

        // The kart is driving at high speed, when the current max speed 
        // is higher than the max speed of the kart (which is caused by 
        // any powerups etc)
        // Otherwise check for skidding. If the kart is still collecting
        // skid bonus, currentMaxSpeed is not affected yet, but it might
        // be if the kart would need to turn sharper, therefore stops
        // skidding, and will get the bonus speed.
        bool high_speed = (m_kart->getCurrentMaxSpeed() > 
                             m_kart->getKartProperties()->getMaxSpeed() ) ||
                          m_kart->getSkidding()->getSkidBonusReady();
        float max_angle = high_speed 
                        ? m_ai_properties->m_max_item_angle_high_speed
                        : m_ai_properties->m_max_item_angle;

        if(fabsf(diff) > max_angle)
            return;

    }   // if !avoid

    // Now insert the item into the sorted list of items to avoid 
    // (or to collect). The lists are (for now) sorted by distance
    std::vector<const Item*> *list;
    if(avoid)
        list = items_to_avoid;
    else
        list = items_to_collect;
    
    float new_distance = (item->getXYZ() - m_kart->getXYZ()).length2_2d();

    // This list is usually very short, so use a simple bubble sort
    list->push_back(item);
    int i;
    for(i=list->size()-2; i>=0; i--)
    {
        float d = ((*list)[i]->getXYZ() - m_kart->getXYZ()).length2_2d();
        if(d<=new_distance) 
        {
            break;
        }
        (*list)[i+1] = (*list)[i];
    }
    (*list)[i+1] = item;

}   // evaluateItems

//-----------------------------------------------------------------------------
/** Handle all items depending on the chosen strategy.
 *  Either (low level AI) just use an item after 10 seconds, or do a much
 *  better job on higher level AI - e.g. aiming at karts ahead/behind, wait an
 *  appropriate time before using multiple items etc.
 *  \param dt Time step size.
 */
void SkiddingAI::handleItems(const float dt)
{
    m_controls->m_fire = false;
    if(m_kart->getKartAnimation() || 
        m_kart->getPowerup()->getType() == PowerupManager::POWERUP_NOTHING ) 
        return;

    m_time_since_last_shot += dt;
    
    if (m_superpower == RaceManager::SUPERPOWER_NOLOK_BOSS)
    {
        m_controls->m_look_back = (m_kart->getPowerup()->getType() == PowerupManager::POWERUP_BOWLING);
        
        if( m_time_since_last_shot > 3.0f )
        {
            m_controls->m_fire = true;
            if (m_kart->getPowerup()->getType() == PowerupManager::POWERUP_SWATTER)
                m_time_since_last_shot = 3.0f;
            else
                m_time_since_last_shot = (rand() % 1000) / 1000.0f * 3.0f - 2.0f; // to make things less predictable :)
        }
        else
        {
            m_controls->m_fire = false;
        }
        return;
    }

    // Tactic 1: wait ten seconds, then use item
    // -----------------------------------------
    if(!m_ai_properties->m_item_usage_non_random)
    {
        if( m_time_since_last_shot > 10.0f )
        {
            m_controls->m_fire = true;
            m_time_since_last_shot = 0.0f;
        }
        return;
    }

    // Tactic 2: calculate
    // -------------------
    switch( m_kart->getPowerup()->getType() )
    {
    case PowerupManager::POWERUP_BUBBLEGUM:
        // Avoid dropping all bubble gums one after another
        if( m_time_since_last_shot <3.0f) break;

        // Either use the bubble gum after 10 seconds, or if the next kart 
        // behind is 'close' but not too close (too close likely means that the
        // kart is not behind but more to the side of this kart and so won't 
        // be hit by the bubble gum anyway). Should we check the speed of the
        // kart as well? I.e. only drop if the kart behind is faster? Otoh 
        // this approach helps preventing an overtaken kart to overtake us 
        // again.
        m_controls->m_fire = (m_distance_behind < 15.0f &&
                              m_distance_behind > 3.0f    );
        break;   // POWERUP_BUBBLEGUM

    // All the thrown/fired items might be improved by considering the angle
    // towards m_kart_ahead.
    case PowerupManager::POWERUP_CAKE:
        {
            // Leave some time between shots
            if(m_time_since_last_shot<3.0f) break;
            // Since cakes can be fired all around, just use a sane distance
            // with a bit of extra for backwards, as enemy will go towards cake
            bool fire_backwards = (m_kart_behind && m_kart_ahead &&
                                   m_distance_behind < m_distance_ahead) ||
                                  !m_kart_ahead;
            float distance = fire_backwards ? m_distance_behind
                                            : m_distance_ahead;
            m_controls->m_fire = (fire_backwards && distance < 25.0f)  ||
                                 (!fire_backwards && distance < 20.0f);
            if(m_controls->m_fire)
                m_controls->m_look_back = fire_backwards;
            break;
        }   // POWERUP_CAKE

    case PowerupManager::POWERUP_BOWLING:
        {
            // Leave more time between bowling balls, since they are 
            // slower, so it should take longer to hit something which
            // can result in changing our target.
            if(m_time_since_last_shot < 5.0f) break;
            // Bowling balls are slower, so only fire on closer karts - but when
            // firing backwards, the kart can be further away, since the ball
            // acts a bit like a mine (and the kart is racing towards it, too)
            bool fire_backwards = (m_kart_behind && m_kart_ahead && 
                                   m_distance_behind < m_distance_ahead) ||
                                  !m_kart_ahead;
            float distance = fire_backwards ? m_distance_behind 
                                            : m_distance_ahead;
            m_controls->m_fire = ( (fire_backwards && distance < 30.0f)  ||
                                   (!fire_backwards && distance <10.0f)    ) &&
                                m_time_since_last_shot > 3.0f;
            if(m_controls->m_fire)
                m_controls->m_look_back = fire_backwards;
            break;
        }   // POWERUP_BOWLING

    case PowerupManager::POWERUP_ZIPPER:
        // Do nothing. Further up a zipper is used if nitro should be selected,
        // saving the (potential more valuable nitro) for later
        break;   // POWERUP_ZIPPER

    case PowerupManager::POWERUP_PLUNGER:
        {
            // Leave more time after a plunger, since it will take some
            // time before a plunger effect becomes obvious.
            if(m_time_since_last_shot < 5.0f) break;

            // Plungers can be fired backwards and are faster,
            // so allow more distance for shooting.
            bool fire_backwards = (m_kart_behind && m_kart_ahead && 
                                   m_distance_behind < m_distance_ahead) ||
                                  !m_kart_ahead;
            float distance      = fire_backwards ? m_distance_behind 
                                                 : m_distance_ahead;
            m_controls->m_fire  = distance < 30.0f                 || 
                                  m_time_since_last_shot > 10.0f;
            if(m_controls->m_fire)
                m_controls->m_look_back = fire_backwards;
            break;
        }   // POWERUP_PLUNGER

    case PowerupManager::POWERUP_SWITCH:
        // For now don't use a switch if this kart is first (since it's more 
        // likely that this kart then gets a good iteam), otherwise use it 
        // after a waiting an appropriate time
        if(m_kart->getPosition()>1 && 
            m_time_since_last_shot > stk_config->m_item_switch_time+2.0f)
            m_controls->m_fire = true;
        break;   // POWERUP_SWITCH

    case PowerupManager::POWERUP_PARACHUTE:
        // Wait one second more than a previous parachute
        if(m_time_since_last_shot > stk_config->m_parachute_time_other+1.0f)
            m_controls->m_fire = true;
        break;   // POWERUP_PARACHUTE

    case PowerupManager::POWERUP_ANVIL:
        // Wait one second more than a previous anvil
        if(m_time_since_last_shot < stk_config->m_anvil_time+1.0f) break;

        if(race_manager->getMinorMode()==RaceManager::MINOR_MODE_FOLLOW_LEADER)
        {
            m_controls->m_fire = m_world->getTime()<1.0f && 
                                 m_kart->getPosition()>2;
        }
        else
        {
            m_controls->m_fire = m_time_since_last_shot > 3.0f && 
                                 m_kart->getPosition()>1;
        }
        break;   // POWERUP_ANVIL

    case PowerupManager::POWERUP_SWATTER:
        {
            // Squared distance for which the swatter works
            float d2 = m_kart->getKartProperties()->getSwatterDistance2();
            // Fire if the closest kart ahead or to the back is not already 
            // squashed and close enough.
            // FIXME: this can be improved on, since more than one kart might 
            //        be hit, and a kart ahead might not be at an angle at 
            //        which the glove can be used.
            if(  ( m_kart_ahead && !m_kart_ahead->isSquashed()             &&
                    (m_kart_ahead->getXYZ()-m_kart->getXYZ()).length2()<d2 &&
                    m_kart_ahead->getSpeed() < m_kart->getSpeed()            ) ||
                 ( m_kart_behind && !m_kart_behind->isSquashed() &&
                    (m_kart_behind->getXYZ()-m_kart->getXYZ()).length2()<d2) )
                    m_controls->m_fire = true;
            break;
        }
    case PowerupManager::POWERUP_RUBBERBALL:
        // Perhaps some more sophisticated algorithm might be useful.
        // For now: fire if there is a kart ahead (which means that
        // this kart is certainly not the first kart)
        m_controls->m_fire = m_kart_ahead != NULL;
        break;
    default:
        Log::error("SkiddingAI", 
                "Invalid or unhandled powerup '%d' in default AI.\n",
                m_kart->getPowerup()->getType());
        assert(false);
    }
    if(m_controls->m_fire)  m_time_since_last_shot = 0.0f;
}   // handleItems

//-----------------------------------------------------------------------------
/** Determines the closest karts just behind and in front of this kart. The
 *  'closeness' is for now simply based on the position, i.e. if a kart is
 *  more than one lap behind or ahead, it is not considered to be closest.
 */
void SkiddingAI::computeNearestKarts()
{
    int my_position    = m_kart->getPosition();

    // If we are not the first, there must be another kart ahead of this kart
    if( my_position>1 )
    {
        m_kart_ahead = m_world->getKartAtPosition(my_position-1);
        if(m_kart_ahead && 
              ( m_kart_ahead->isEliminated() || m_kart_ahead->hasFinishedRace()))
              m_kart_ahead = NULL;
    }
    else
        m_kart_ahead = NULL;

    if( my_position<(int)m_world->getCurrentNumKarts())
    {
        m_kart_behind = m_world->getKartAtPosition(my_position+1);
        if(m_kart_behind && 
            (m_kart_behind->isEliminated() || m_kart_behind->hasFinishedRace()))
            m_kart_behind = NULL;
    }
    else
        m_kart_behind = NULL;

    m_distance_ahead = m_distance_behind = 9999999.9f;
    float my_dist = m_world->getOverallDistance(m_kart->getWorldKartId());
    if(m_kart_ahead)
    {
        m_distance_ahead = 
            m_world->getOverallDistance(m_kart_ahead->getWorldKartId())
            -my_dist;
    }
    if(m_kart_behind)
    {
        m_distance_behind = my_dist
            -m_world->getOverallDistance(m_kart_behind->getWorldKartId());
    }

    // Compute distance to nearest player kart
    float max_overall_distance = 0.0f;
    unsigned int n = ProfileWorld::isProfileMode() 
                   ? 0 : race_manager->getNumPlayers();
    for(unsigned int i=0; i<n; i++)
    {
        unsigned int kart_id = 
            m_world->getPlayerKart(i)->getWorldKartId();
        if(m_world->getOverallDistance(kart_id)>max_overall_distance)
            max_overall_distance = m_world->getOverallDistance(kart_id);
    }
    if(max_overall_distance==0.0f)
        max_overall_distance = 999999.9f;   // force best driving 
    // Now convert 'maximum overall distance' to distance to player.
    m_distance_to_player = 
                m_world->getOverallDistance(m_kart->getWorldKartId())
                - max_overall_distance;
}   // computeNearestKarts

//-----------------------------------------------------------------------------
/** Determines if the AI should accelerate or not.
 *  \param dt Time step size.
 */
void SkiddingAI::handleAcceleration( const float dt)
{
    //Do not accelerate until we have delayed the start enough
    if( m_start_delay > 0.0f )
    {
        m_start_delay -= dt;
        m_controls->m_accel = 0.0f;
        return;
    }

    if( m_controls->m_brake )
    {
        m_controls->m_accel = 0.0f;
        return;
    }

    if(m_kart->getBlockedByPlungerTime()>0)
    {
        if(m_kart->getSpeed() < m_kart->getCurrentMaxSpeed() / 2)
            m_controls->m_accel = 0.05f;
        else 
            m_controls->m_accel = 0.0f;
        return;
    }
    
    m_controls->m_accel = stk_config->m_ai_acceleration;

}   // handleAcceleration

//-----------------------------------------------------------------------------
void SkiddingAI::handleRaceStart()
{
    if( m_start_delay <  0.0f )
    {
        // Each kart starts at a different, random time, and the time is
        // smaller depending on the difficulty.
        m_start_delay = m_ai_properties->m_min_start_delay 
                      + (float) rand() / RAND_MAX 
                      * (m_ai_properties->m_max_start_delay -
                         m_ai_properties->m_min_start_delay);

        float false_start_probability = 
               m_superpower == RaceManager::SUPERPOWER_NOLOK_BOSS
               ? 0.0f  : m_ai_properties->m_false_start_probability;

        // Now check for a false start. If so, add 1 second penalty time.
        if(rand() < RAND_MAX * false_start_probability)
        {
            m_start_delay+=stk_config->m_penalty_time;
            return;
        }
    }
}   // handleRaceStart

//-----------------------------------------------------------------------------
//TODO: if the AI is crashing constantly, make it move backwards in a straight
//line, then move forward while turning.

void SkiddingAI::handleRescue(const float dt)
{
    // check if kart is stuck
    if(m_kart->getSpeed()<2.0f && !m_kart->getKartAnimation() && 
        !m_world->isStartPhase())
    {
        m_time_since_stuck += dt;
        if(m_time_since_stuck > 2.0f)
        {
            new RescueAnimation(m_kart);
            m_time_since_stuck=0.0f;
        }   // m_time_since_stuck > 2.0f
    }
    else
    {
        m_time_since_stuck = 0.0f;
    }
}   // handleRescue

//-----------------------------------------------------------------------------
/** Decides wether to use nitro or not.
 */
void SkiddingAI::handleNitroAndZipper()
{
    m_controls->m_nitro = false;
    // If we are already very fast, save nitro.
    if(m_kart->getSpeed() > 0.95f*m_kart->getCurrentMaxSpeed())
        return;
    // Don't use nitro when the AI has a plunger in the face!
    if(m_kart->getBlockedByPlungerTime()>0) return;
    
    // Don't use nitro if we are braking
    if(m_controls->m_brake) return;

    // Don't use nitro if the kart is not on ground or has finished the race
    if(!m_kart->isOnGround() || m_kart->hasFinishedRace()) return;
    
    // Don't compute nitro usage if we don't have nitro or are not supposed
    // to use it, and we don't have a zipper or are not supposed to use
    // it (calculated).
    if( (m_kart->getEnergy()==0 || 
        m_ai_properties->m_nitro_usage==AIProperties::NITRO_NONE)  &&
        (m_kart->getPowerup()->getType()!=PowerupManager::POWERUP_ZIPPER ||
         !m_ai_properties->m_item_usage_non_random )                         )
        return;

    // If there are items to avoid close, and we only have zippers, don't
    // use them (since this make it harder to avoid items).
    if(m_avoid_item_close &&
        (m_kart->getEnergy()==0 ||
         m_ai_properties->m_nitro_usage==AIProperties::NITRO_NONE) )
        return;
    // If a parachute or anvil is attached, the nitro doesn't give much
    // benefit. Better wait till later.
    const bool has_slowdown_attachment = 
        m_kart->getAttachment()->getType()==Attachment::ATTACH_PARACHUTE ||
        m_kart->getAttachment()->getType()==Attachment::ATTACH_ANVIL;
    if(has_slowdown_attachment) return;

    // If the kart is very slow (e.g. after rescue), use nitro
    if(m_kart->getSpeed()<5)
    {
        m_controls->m_nitro = true;
        return;
    }

    // If this kart is the last kart, and we have enough 
    // (i.e. more than 2) nitro, use it.
    // -------------------------------------------------
    const unsigned int num_karts = m_world->getCurrentNumKarts();
    if(m_kart->getPosition()== (int)num_karts && 
        num_karts>1 && m_kart->getEnergy()>2.0f)
    {
        m_controls->m_nitro = true;
        return;
    }

    // On the last track shortly before the finishing line, use nitro 
    // anyway. Since the kart is faster with nitro, estimate a 50% time
    // decrease (additionally some nitro will be saved when top speed
    // is reached).
    if(m_world->getLapForKart(m_kart->getWorldKartId())
                        ==race_manager->getNumLaps()-1 &&
       m_ai_properties->m_nitro_usage == AIProperties::NITRO_ALL)
    {
        float finish = 
            m_world->getEstimatedFinishTime(m_kart->getWorldKartId());
        if( 1.5f*m_kart->getEnergy() >= finish - m_world->getTime() )
        {
            m_controls->m_nitro = true;
            return;
        }
    }

    // A kart within this distance is considered to be overtaking (or to be
    // overtaken).
    const float overtake_distance = 10.0f;

    // Try to overtake a kart that is close ahead, except 
    // when we are already much faster than that kart
    // --------------------------------------------------
    if(m_kart_ahead                                       && 
        m_distance_ahead < overtake_distance              &&
        m_kart_ahead->getSpeed()+5.0f > m_kart->getSpeed()   )
    {
            m_controls->m_nitro = true;
            return;
    }

    if(m_kart_behind                                   &&
        m_distance_behind < overtake_distance          &&
        m_kart_behind->getSpeed() > m_kart->getSpeed()    )
    {
        // Only prevent overtaking on highest level
        m_controls->m_nitro = m_ai_properties->m_nitro_usage
                              == AIProperties::NITRO_ALL;
        return;
    }
    
    if(m_kart->getPowerup()->getType()==PowerupManager::POWERUP_ZIPPER &&
        m_kart->getSpeed()>1.0f && 
        m_kart->getSpeedIncreaseTimeLeft(MaxSpeed::MS_INCREASE_ZIPPER)<=0)
    {
        GraphNode::DirectionType dir;
        unsigned int last;
        const GraphNode &gn = QuadGraph::get()->getNode(m_track_node);
        gn.getDirectionData(m_successor_index[m_track_node], &dir, &last);
        if(dir==GraphNode::DIR_STRAIGHT)
        {
            float diff = QuadGraph::get()->getDistanceFromStart(last)
                       - QuadGraph::get()->getDistanceFromStart(m_track_node);
            if(diff<0) diff+=World::getWorld()->getTrack()->getTrackLength();
            if(diff>m_ai_properties->m_straight_length_for_zipper)
                m_controls->m_fire = true;
        }
        
    }
}   // handleNitroAndZipper

//-----------------------------------------------------------------------------
void SkiddingAI::checkCrashes(const Vec3& pos )
{
    int steps = int( m_kart->getVelocityLC().getZ() / m_kart_length );
    if( steps < 2 ) steps = 2;

    // The AI drives significantly better with more steps, so for now
    // add 5 additional steps.
    steps+=5;

    //Right now there are 2 kind of 'crashes': with other karts and another
    //with the track. The sight line is used to find if the karts crash with
    //each other, but the first step is twice as big as other steps to avoid
    //having karts too close in any direction. The crash with the track can
    //tell when a kart is going to get out of the track so it steers.
    m_crashes.clear();

    // If slipstream should be handled actively, trigger overtaking the
    // kart which gives us slipstream if slipstream is ready
    const SlipStream *slip=m_kart->getSlipstream();
    if(m_ai_properties->m_make_use_of_slipstream && 
        slip->isSlipstreamReady() &&
        slip->getSlipstreamTarget())
    {
        //Log::debug("skidding_ai", "%s overtaking %s\n",
        //           m_kart->getIdent().c_str(),
        //           m_kart->getSlipstreamKart()->getIdent().c_str());
        // FIXME: we might define a minimum distance, and if the target kart
        // is too close break first - otherwise the AI hits the kart when
        // trying to overtake it, actually speeding the other kart up.
        m_crashes.m_kart = slip->getSlipstreamTarget()->getWorldKartId();
    }

    const size_t NUM_KARTS = m_world->getNumKarts();

    //Protection against having vel_normal with nan values
    const Vec3 &VEL = m_kart->getVelocity();
    Vec3 vel_normal(VEL.getX(), 0.0, VEL.getZ());
    float speed=vel_normal.length();
    // If the velocity is zero, no sense in checking for crashes in time
    if(speed==0) return;

    // Time it takes to drive for m_kart_length units.
    float dt = m_kart_length / speed; 
    vel_normal/=speed;

    int current_node = m_track_node;
    if(steps<1 || steps>1000)
    {
        Log::warn("SkiddingAI", 
            "Incorrect STEPS=%d. kart_length %f velocity %f\n",
            steps, m_kart_length, m_kart->getVelocityLC().getZ());
        steps=1000;
    }
    for(int i = 1; steps > i; ++i)
    {
        Vec3 step_coord = pos + vel_normal* m_kart_length * float(i);

        /* Find if we crash with any kart, as long as we haven't found one
         * yet
         */
        if( m_crashes.m_kart == -1 )
        {
            for( unsigned int j = 0; j < NUM_KARTS; ++j )
            {
                const AbstractKart* kart = m_world->getKart(j);
                // Ignore eliminated karts
                if(kart==m_kart||kart->isEliminated()) continue;
                const AbstractKart *other_kart = m_world->getKart(j);
                // Ignore karts ahead that are faster than this kart.
                if(m_kart->getVelocityLC().getZ() < other_kart->getVelocityLC().getZ())
                    continue;
                Vec3 other_kart_xyz = other_kart->getXYZ() + other_kart->getVelocity()*(i*dt);
                float kart_distance = (step_coord - other_kart_xyz).length_2d();

                if( kart_distance < m_kart_length)
                    m_crashes.m_kart = j;
            }
        }

        /*Find if we crash with the drivelines*/
        if(current_node!=QuadGraph::UNKNOWN_SECTOR &&
            m_next_node_index[current_node]!=-1)
            QuadGraph::get()->findRoadSector(step_coord, &current_node,
                        /* sectors to test*/ &m_all_look_aheads[current_node]);

        if( current_node == QuadGraph::UNKNOWN_SECTOR)
        {
            m_crashes.m_road = true;
            return;
        }
    }
}   // checkCrashes

//-----------------------------------------------------------------------------
/** This is a new version of findNonCrashingPoint, which at this stage is
 *  slightly inferior (though faster and more correct) than the original 
 *  version - the original code cuts corner more aggressively than this
 *  version (and in most cases cuting the corner does not end in a 
 *  collision, so it's actually faster).
 *  This version find the point furthest ahead which can be reached by
 *  travelling in a straight direction from the current location of the
 *  kart. This is done by using two lines: one from the kart to the
 *  lower left side of the next quad, and one from the kart to the
 *  lower right side of the next quad. The area between those two lines
 *  can be reached by the kart in a straight line, and will not go off
 *  track (assuming that the kart is on track). Then the next quads are 
 *  tested: New left/right lines are computed. If the new left line is to 
 *  the right of the old left line, the new left line becomes the current 
 *  left line:
 *
 *            X      The new left line connecting kart to X will be to the right
 *        \        / of the old left line, so the available area for the kart
 *         \      /  will be dictated by the new left line.
 *          \    /
 *           kart
 *
 *  Similarly for the right side. This will narrow down the available area
 *  the kart can aim at, till finally the left and right line overlap.
 *  All points between the connection of the two end points of the left and 
 *  right line can be reached without getting off track. Which point the
 *  kart aims at then depends on the direction of the track: if there is
 *  a left turn, the kart will aim to the left point (and vice versa for
 *  right turn) - slightly offset by the width of the kart to avoid that 
 *  the kart is getting off track.
 *  \param aim_position The point to aim for, i.e. the point that can be
 *         driven to in a straight line.
 *  \param last_node The graph node index in which the aim_position is.
*/
void SkiddingAI::findNonCrashingPointNew(Vec3 *result, int *last_node)
{    
    *last_node = m_next_node_index[m_track_node];
    const core::vector2df xz = m_kart->getXYZ().toIrrVector2d();

    const Quad &q = QuadGraph::get()->getQuadOfNode(*last_node);

    // Index of the left and right end of a quad.
    const unsigned int LEFT_END_POINT  = 0;
    const unsigned int RIGHT_END_POINT = 1;
    core::line2df left (xz, q[LEFT_END_POINT ].toIrrVector2d());
    core::line2df right(xz, q[RIGHT_END_POINT].toIrrVector2d());

#if defined(AI_DEBUG) && defined(AI_DEBUG_NEW_FIND_NON_CRASHING)
    const Vec3 eps1(0,0.5f,0);
    m_curve[CURVE_LEFT]->clear();
    m_curve[CURVE_LEFT]->addPoint(m_kart->getXYZ()+eps1);
    m_curve[CURVE_LEFT]->addPoint(q[LEFT_END_POINT]+eps1);
    m_curve[CURVE_LEFT]->addPoint(m_kart->getXYZ()+eps1);
    m_curve[CURVE_RIGHT]->clear();
    m_curve[CURVE_RIGHT]->addPoint(m_kart->getXYZ()+eps1);
    m_curve[CURVE_RIGHT]->addPoint(q[RIGHT_END_POINT]+eps1);
    m_curve[CURVE_RIGHT]->addPoint(m_kart->getXYZ()+eps1);
#endif
#ifdef AI_DEBUG_KART_HEADING
    const Vec3 eps(0,0.5f,0);
    m_curve[CURVE_KART]->clear();
    m_curve[CURVE_KART]->addPoint(m_kart->getXYZ()+eps);
    Vec3 forw(0, 0, 50);
    m_curve[CURVE_KART]->addPoint(m_kart->getTrans()(forw)+eps);
#endif
    while(1)
    {
        unsigned int next_sector = m_next_node_index[*last_node];
        const Quad &q_next = QuadGraph::get()->getQuadOfNode(next_sector);
        // Test if the next left point is to the right of the left
        // line. If so, a new left line is defined.
        if(left.getPointOrientation(q_next[LEFT_END_POINT].toIrrVector2d())
            < 0 )
        {
            core::vector2df p = q_next[LEFT_END_POINT].toIrrVector2d();
            // Stop if the new point is to the right of the right line
            if(right.getPointOrientation(p)<0)
                break;
            left.end = p;
#if defined(AI_DEBUG) && defined(AI_DEBUG_NEW_FIND_NON_CRASHING)
            Vec3 ppp(p.X, m_kart->getXYZ().getY(), p.Y);
            m_curve[CURVE_LEFT]->addPoint(ppp+eps);
            m_curve[CURVE_LEFT]->addPoint(m_kart->getXYZ()+eps);
#endif
        }
        else
            break;

        // Test if new right point is to the left of the right line. If
        // so, a new right line is defined.
        if(right.getPointOrientation(q_next[RIGHT_END_POINT].toIrrVector2d())
            > 0 )
        {
            core::vector2df p = q_next[RIGHT_END_POINT].toIrrVector2d();
            // Break if new point is to the left of left line
            if(left.getPointOrientation(p)>0)
                break;
#if defined(AI_DEBUG) && defined(AI_DEBUG_NEW_FIND_NON_CRASHING)

            Vec3 ppp(p.X, m_kart->getXYZ().getY(), p.Y);
            m_curve[CURVE_RIGHT]->addPoint(ppp+eps);
            m_curve[CURVE_RIGHT]->addPoint(m_kart->getXYZ()+eps);
#endif
            right.end = p;
        }
        else
            break;
        *last_node = next_sector;
    }   // while

    //Vec3 ppp(0.5f*(left.end.X+right.end.X),
    //         m_kart->getXYZ().getY(),
    //         0.5f*(left.end.Y+right.end.Y));
    //*result = ppp;

    *result = QuadGraph::get()->getQuadOfNode(*last_node).getCenter();
}   // findNonCrashingPointNew

//-----------------------------------------------------------------------------
/** Find the sector that at the longest distance from the kart, that can be
 *  driven to without crashing with the track, then find towards which of
 *  the two edges of the track is closest to the next curve afterwards,
 *  and return the position of that edge.
 *  \param aim_position The point to aim for, i.e. the point that can be
 *         driven to in a straight line.
 *  \param last_node The graph node index in which the aim_position is.
 */
void SkiddingAI::findNonCrashingPointFixed(Vec3 *aim_position, int *last_node)
{    
#ifdef AI_DEBUG_KART_HEADING
    const Vec3 eps(0,0.5f,0);
    m_curve[CURVE_KART]->clear();
    m_curve[CURVE_KART]->addPoint(m_kart->getXYZ()+eps);
    Vec3 forw(0, 0, 50);
    m_curve[CURVE_KART]->addPoint(m_kart->getTrans()(forw)+eps);
#endif
    *last_node = m_next_node_index[m_track_node];
    int target_sector;

    Vec3 direction;
    Vec3 step_track_coord;

    // The original while(1) loop is replaced with a for loop to avoid
    // infinite loops (which we had once or twice). Usually the number
    // of iterations in the while loop is less than 7.
    for(unsigned int j=0; j<100; j++)
    {
        // target_sector is the sector at the longest distance that we can 
        // drive to without crashing with the track.
        target_sector = m_next_node_index[*last_node];

        //direction is a vector from our kart to the sectors we are testing
        direction = QuadGraph::get()->getQuadOfNode(target_sector).getCenter()
                  - m_kart->getXYZ();

        float len=direction.length_2d();
        unsigned int steps = (unsigned int)( len / m_kart_length );
        if( steps < 3 ) steps = 3;

        // That shouldn't happen, but since we had one instance of
        // STK hanging, add an upper limit here (usually it's at most
        // 20 steps)
        if( steps>1000) steps = 1000;

        // Protection against having vel_normal with nan values
        if(len>0.0f) {
            direction*= 1.0f/len;
        }

        Vec3 step_coord;
        //Test if we crash if we drive towards the target sector
        for(unsigned int i = 2; i < steps; ++i )
        {
            step_coord = m_kart->getXYZ()+direction*m_kart_length * float(i);

            QuadGraph::get()->spatialToTrack(&step_track_coord, step_coord,
                                             *last_node );
 
            float distance = fabsf(step_track_coord[0]);

            //If we are outside, the previous node is what we are looking for
            if ( distance + m_kart_width * 0.5f 
                 > QuadGraph::get()->getNode(*last_node).getPathWidth()*0.5f )
            {
                *aim_position = QuadGraph::get()->getQuadOfNode(*last_node)
                                                 .getCenter();
                return;
            }
        }
        *last_node = target_sector;
    }   // for i<100
    *aim_position = QuadGraph::get()->getQuadOfNode(*last_node).getCenter();
}   // findNonCrashingPointFixed

//-----------------------------------------------------------------------------
/** This is basically the original AI algorithm. It is clearly buggy:
 *  1. the test:
 *
 *         distance + m_kart_width * 0.5f 
 *                  > QuadGraph::get()->getNode(*last_node).getPathWidth() )
 *
 *     is incorrect, it should compare with getPathWith*0.5f (since distance
 *     is the distance from the center, i.e. it is half the path width if
 *     the point is at the edge).
 *  2. the test:
 *
 *         QuadGraph::get()->spatialToTrack(&step_track_coord, step_coord,
 *                                          *last_node );
 *     in the for loop tests always against distance from the same 
 *     graph node (*last_node), while de-fact the loop will test points
 *     on various graph nodes.
 *
 *  This results in this algorithm often picking points to aim at that
 *  would actually force the kart off track. But in reality the kart has
 *  to turn (and does not immediate in one frame change its direction)
 *  which takes some time - so it is actually mostly on track.
 *  Since this algoritm (so far) ends up with by far the best AI behaviour,
 *  it is for now the default).
 *  \param aim_position On exit contains the point the AI should aim at.
 *  \param last_node On exit contais the graph node the AI is aiming at.
*/ 
 void SkiddingAI::findNonCrashingPoint(Vec3 *aim_position, int *last_node)
{    
#ifdef AI_DEBUG_KART_HEADING
    const Vec3 eps(0,0.5f,0);
    m_curve[CURVE_KART]->clear();
    m_curve[CURVE_KART]->addPoint(m_kart->getXYZ()+eps);
    Vec3 forw(0, 0, 50);
    m_curve[CURVE_KART]->addPoint(m_kart->getTrans()(forw)+eps);
#endif
    *last_node = m_next_node_index[m_track_node];
    float angle = QuadGraph::get()->getAngleToNext(m_track_node, 
                                              m_successor_index[m_track_node]);
    int target_sector;

    Vec3 direction;
    Vec3 step_track_coord;

    float angle1;
    // The original while(1) loop is replaced with a for loop to avoid
    // infinite loops (which we had once or twice). Usually the number
    // of iterations in the while loop is less than 7.
    for(unsigned int j=0; j<100; j++)
    {
        // target_sector is the sector at the longest distance that we can 
        // drive to without crashing with the track.
        target_sector = m_next_node_index[*last_node];
        angle1 = QuadGraph::get()->getAngleToNext(target_sector, 
                                                m_successor_index[target_sector]);
        // In very sharp turns this algorithm tends to aim at off track points,
        // resulting in hitting a corner. So test for this special case and 
        // prevent a too-far look-ahead in this case
        float diff = normalizeAngle(angle1-angle);
        if(fabsf(diff)>1.5f)
        {
            *aim_position = QuadGraph::get()->getQuadOfNode(target_sector)
                                                 .getCenter();
            return;
        }

        //direction is a vector from our kart to the sectors we are testing
        direction = QuadGraph::get()->getQuadOfNode(target_sector).getCenter()
                  - m_kart->getXYZ();

        float len=direction.length_2d();
        unsigned int steps = (unsigned int)( len / m_kart_length );
        if( steps < 3 ) steps = 3;

        // That shouldn't happen, but since we had one instance of
        // STK hanging, add an upper limit here (usually it's at most
        // 20 steps)
        if( steps>1000) steps = 1000;

        // Protection against having vel_normal with nan values
        if(len>0.0f) {
            direction*= 1.0f/len;
        }

        Vec3 step_coord;
        //Test if we crash if we drive towards the target sector
        for(unsigned int i = 2; i < steps; ++i )
        {
            step_coord = m_kart->getXYZ()+direction*m_kart_length * float(i);

            QuadGraph::get()->spatialToTrack(&step_track_coord, step_coord,
                                             *last_node );
 
            float distance = fabsf(step_track_coord[0]);

            //If we are outside, the previous node is what we are looking for
            if ( distance + m_kart_width * 0.5f 
                 > QuadGraph::get()->getNode(*last_node).getPathWidth() )
            {
                *aim_position = QuadGraph::get()->getQuadOfNode(*last_node)
                                                 .getCenter();
                return;
            }
        }
        angle = angle1;
        *last_node = target_sector;
    }   // for i<100
    *aim_position = QuadGraph::get()->getQuadOfNode(*last_node).getCenter();
}   // findNonCrashingPoint

//-----------------------------------------------------------------------------
/** Determines the direction of the track ahead of the kart: 0 indicates 
 *  straight, +1 right turn, -1 left turn.
 */
void SkiddingAI::determineTrackDirection()
{
    const QuadGraph *qg = QuadGraph::get();
    unsigned int succ   = m_successor_index[m_track_node];
    float angle_to_track = qg->getNode(m_track_node).getAngleToSuccessor(succ)
                         - m_kart->getHeading();
    angle_to_track = normalizeAngle(angle_to_track);

    // In certain circumstances (esp. S curves) it is possible that the
    // kart is not facing in the direction of the track. In this case
    // determining the curve radius based on the direction the kart is
    // facing results in very incorrect results (example: if the kart is
    // in a tight curve, but already facing towards the last point of the
    // curve - in this case a huge curve radius will be computes (since
    // the kart is nearly going straight), while in fact the kart would
    // go across the circle and not along, bumping into the track).
    // To avoid this we set the direction to undefined in this case,
    // which causes the kart to brake (which will allow the kart to
    // quicker be aligned with the track again).
    if(fabsf(angle_to_track) > 0.22222f * M_PI)
    {
        m_current_track_direction = GraphNode::DIR_UNDEFINED;
        return;
    }

    unsigned int next   = qg->getNode(m_track_node).getSuccessor(succ);

    qg->getNode(next).getDirectionData(m_successor_index[next], 
                                       &m_current_track_direction, 
                                       &m_last_direction_node);

#ifdef AI_DEBUG
    m_curve[CURVE_QG]->clear();
    for(unsigned int i=m_track_node; i<=m_last_direction_node; i++)
    {
        m_curve[CURVE_QG]->addPoint(qg->getNode(i).getCenter());
    }
#endif

    if(m_current_track_direction==GraphNode::DIR_LEFT  ||
       m_current_track_direction==GraphNode::DIR_RIGHT   )
    {
        handleCurve();
    }   // if(m_current_track_direction == DIR_LEFT || DIR_RIGHT   )


    return;
}   // determineTrackDirection

// ----------------------------------------------------------------------------
/** If the kart is at/in a curve, determine the turn radius.
 */
void SkiddingAI::handleCurve()
{
    // Ideally we would like to have a circle that:
    // 1) goes through the kart position
    // 2) has the current heading of the kart as tangent in that point
    // 3) goes through the last point
    // 4) has a tangent at the last point that faces towards the next node
    // Unfortunately conditions 1 to 3 already fully determine the circle,
    // i.e. it is not always possible to find an appropriate circle.
    // Using the first three conditions is mostly a good choice (since the
    // kart will already point towards the direction of the circle), and
    // the case that the kart is facing wrong was already tested for before

    const QuadGraph *qg = QuadGraph::get();
    Vec3 xyz            = m_kart->getXYZ();
    Vec3 tangent        = m_kart->getTrans()(Vec3(0,0,1)) - xyz;
    Vec3 last_xyz       = qg->getNode(m_last_direction_node).getCenter();

    determineTurnRadius(xyz, tangent, last_xyz,
                        &m_curve_center, &m_current_curve_radius);
    assert(!isnan(m_curve_center.getX()));
    assert(!isnan(m_curve_center.getY()));
    assert(!isnan(m_curve_center.getZ()));

#undef ADJUST_TURN_RADIUS_TO_AVOID_CRASH_INTO_TRACK
#ifdef ADJUST_TURN_RADIUS_TO_AVOID_CRASH_INTO_TRACK
    // NOTE: this can deadlock if the AI is going on a shortcut, since
    // m_last_direction_node is based on going on the main driveline :(
    unsigned int i= m_track_node;
    while(1)
    {
        i = m_next_node_index[i];
        // Pick either the lower left or right point:
        int index = m_current_track_direction==GraphNode::DIR_LEFT
                  ? 0 : 1;
        float r = (m_curve_center - qg->getQuadOfNode(i)[index]).length();
        if(m_current_curve_radius < r)
        {
            last_xyz = qg->getQuadOfNode(i)[index];
            determineTurnRadius(xyz, tangent, last_xyz,
                                &m_curve_center, &m_current_curve_radius);
            m_last_direction_node = i;
            break;
        }
        if(i==m_last_direction_node)
            break;
    }
#endif
#if defined(AI_DEBUG) && defined(AI_DEBUG_CIRCLES)
    m_curve[CURVE_PREDICT1]->makeCircle(m_curve_center, 
                                        m_current_curve_radius);
    m_curve[CURVE_PREDICT1]->addPoint(last_xyz);
    m_curve[CURVE_PREDICT1]->addPoint(m_curve_center);
    m_curve[CURVE_PREDICT1]->addPoint(xyz);
#endif

}   // handleCurve
// ----------------------------------------------------------------------------
/** Determines if the kart should skid. The base implementation enables
 *  skidding 
 *  \param steer_fraction The steering fraction as computed by the 
 *          AIBaseController.
 *  \return True if the kart should skid.
 */
bool SkiddingAI::doSkid(float steer_fraction)
{
    if(fabsf(steer_fraction)>1.5f)
    {
        // If the kart has to do a sharp turn, but is already skidding, find
        // a good time to release the skid button, since this will turn the
        // kart more sharply:
        if(m_controls->m_skid)
        {
#ifdef DEBUG
            if(m_ai_debug)
            {
                if(fabsf(steer_fraction)>=2.5f)
                    Log::debug("SkiddingAI", "%s stops skidding (%f).\n",
                           m_kart->getIdent().c_str(), steer_fraction);
            }
#endif
            // If the current turn is not sharp enough, delay releasing 
            // the skid button.
            return fabsf(steer_fraction)<2.5f;
        }

        // If the kart is not skidding, now is not a good time to start
        return false;
    }

    // No skidding on straights
    if(m_current_track_direction==GraphNode::DIR_STRAIGHT ||
       m_current_track_direction==GraphNode::DIR_UNDEFINED  )
    {
#ifdef DEBUG
        if(m_controls->m_skid && m_ai_debug)
        {
            Log::debug("SkiddingAI", "%s stops skidding on straight.\n",
                m_kart->getIdent().c_str());
        }
#endif
        return false;
    }

    const float MIN_SKID_SPEED = 5.0f;
    const QuadGraph *qg = QuadGraph::get();
    Vec3 last_xyz       = qg->getNode(m_last_direction_node).getCenter();

    // Only try skidding when a certain minimum speed is reached.
    if(m_kart->getSpeed()<MIN_SKID_SPEED) return false;

    // Estimate how long it takes to finish the curve
    Vec3 diff_kart = m_kart->getXYZ() - m_curve_center;
    Vec3 diff_last = last_xyz         - m_curve_center;
    float angle_kart = atan2(diff_kart.getX(), diff_kart.getZ());
    float angle_last = atan2(diff_last.getX(), diff_last.getZ());
    float angle = m_current_track_direction == GraphNode::DIR_RIGHT
                ? angle_last - angle_kart
                : angle_kart - angle_last;
    angle = normalizeAngle(angle);
    float length = m_current_curve_radius*fabsf(angle);
    float duration = length / m_kart->getSpeed();
    // The estimated skdding time is usually too short - partly because
    // he speed of the kart decreases during the turn, partly because
    // the actual path is adjusted during the turn. So apply an
    // experimentally found factor in to get better estimates.
    duration *= 1.5f;
    const Skidding *skidding = m_kart->getSkidding();

    // If the remaining estimated time for skidding is too short, stop
    // it. This code will mostly trigger the bonus at the end of a skid.
    if(m_controls->m_skid && duration < 1.0f)
    {
        if(m_ai_debug)
            Log::debug("SkiddingAI", "'%s' too short, stop skid.\n",
                    m_kart->getIdent().c_str());
        return false;
    }
    // Test if the AI is trying to skid against track direction. This 
    // can happen if the AI is adjusting steering somewhat (e.g. in a 
    // left turn steer right to avoid getting too close to the left
    // vorder). In this case skidding will be useless.
    else if( (steer_fraction > 0 && 
              m_current_track_direction==GraphNode::DIR_LEFT) ||
             (steer_fraction < 0 && 
              m_current_track_direction==GraphNode::DIR_RIGHT)  )
        {
#ifdef DEBUG
            if(m_controls->m_skid && m_ai_debug)
                Log::debug("SkiddingAI",
                        "%s skidding against track direction.\n",
                        m_kart->getIdent().c_str());
#endif
            return false;
        }
    // If there is a skidding bonus, try to get it.
    else if(skidding->getNumberOfBonusTimes()>0 &&
            skidding->getTimeTillBonus(0) < duration)
    {
#ifdef DEBUG
        if(!m_controls->m_skid && m_ai_debug)
            Log::debug("SkiddingAI", "%s start skid, duration %f.\n",
                        m_kart->getIdent().c_str(), duration);
#endif
        return true;

    }  // if curve long enough for skidding

#ifdef DEBUG
        if(m_controls->m_skid && m_ai_debug)
            Log::debug("SkiddingAI", "%s has no reasons to skid anymore.\n",
                   m_kart->getIdent().c_str());
#endif
    return false;
}   // doSkid

//-----------------------------------------------------------------------------
/** Converts the steering angle to a lr steering in the range of -1 to 1. 
 *  If the steering angle is too great, it will also trigger skidding. This 
 *  function uses a 'time till full steer' value specifying the time it takes
 *  for the wheel to reach full left/right steering similar to player karts 
 *  when using a digital input device. The parameter is defined in the kart 
 *  properties and helps somewhat to make AI karts more 'pushable' (since
 *  otherwise the karts counter-steer to fast).
 *  It also takes the effect of a plunger into account by restricting the
 *  actual steer angle to 50% of the maximum.
 *  \param angle Steering angle.
 *  \param dt Time step.
 */
void SkiddingAI::setSteering(float angle, float dt)
{
    float steer_fraction = angle / m_kart->getMaxSteerAngle();

    // Use a simple finite state machine to make sure to randomly decide
    // whether to skid or not only once per skid section. See docs for
    // m_skid_probability_state for more details.
    if(!doSkid(steer_fraction))
    {
        m_skid_probability_state = SKID_PROBAB_NOT_YET;
        m_controls->m_skid       = KartControl::SC_NONE;
    }
    else
    {
        KartControl::SkidControl sc = steer_fraction > 0 
                                    ? KartControl::SC_RIGHT 
                                    : KartControl::SC_LEFT; 
        if(m_skid_probability_state==SKID_PROBAB_NOT_YET)
        {
            int prob = (int)(100.0f*m_ai_properties
                               ->getSkiddingProbability(m_distance_to_player));
            int r = m_random_skid.get(100);
            m_skid_probability_state = (r<prob)
                                     ? SKID_PROBAB_SKID 
                                     : SKID_PROBAB_NO_SKID;
#undef PRINT_SKID_STATS
#ifdef PRINT_SKID_STATS
            Log::info("SkiddingAI", "%s distance %f prob %d skidding %s\n", 
                   m_kart->getIdent().c_str(), distance, prob, 
                   sc= ? "no" : sc==KartControl::SC_LEFT ? "left" : "right");
#endif
        }
        m_controls->m_skid = m_skid_probability_state == SKID_PROBAB_SKID 
                           ? sc : KartControl::SC_NONE;
    }

    // Adjust steer fraction in case to be in [-1,1]
    if     (steer_fraction >  1.0f) steer_fraction =  1.0f;
    else if(steer_fraction < -1.0f) steer_fraction = -1.0f;

    // Restrict steering when a plunger is in the face
    if(m_kart->getBlockedByPlungerTime()>0)
    {
        if     (steer_fraction >  0.5f) steer_fraction =  0.5f;
        else if(steer_fraction < -0.5f) steer_fraction = -0.5f;
    }
    
    const Skidding *skidding = m_kart->getSkidding();

    // If we are supposed to skid, but the current steering is still 
    // in the wrong direction, don't start to skid just now, since then
    // we can't turn into the direction we want to anymore (see 
    // Skidding class)
    Skidding::SkidState ss = skidding->getSkidState();
    if((ss==Skidding::SKID_ACCUMULATE_LEFT  && steer_fraction>0.2f ) ||
       (ss==Skidding::SKID_ACCUMULATE_RIGHT && steer_fraction<-0.2f)    )
    {
        m_controls->m_skid = KartControl::SC_NONE;
#ifdef DEBUG
        if(m_ai_debug)
            Log::info("SkiddingAI", "'%s' wrong steering, stop skid.\n",
                    m_kart->getIdent().c_str());
#endif
    }

    if(m_controls->m_skid!=KartControl::SC_NONE && 
            ( ss==Skidding::SKID_ACCUMULATE_LEFT ||
              ss==Skidding::SKID_ACCUMULATE_RIGHT  )  )
    {
        steer_fraction = 
            skidding->getSteeringWhenSkidding(steer_fraction);
        if(fabsf(steer_fraction)>1.8)
        {
#ifdef DEBUG
            if(m_ai_debug)
                Log::info("SkiddingAI", "%s steering too much (%f).\n",
                       m_kart->getIdent().c_str(), steer_fraction);
#endif
            m_controls->m_skid = KartControl::SC_NONE;
        }
        if(steer_fraction<-1.0f)
            steer_fraction = -1.0f;
        else if(steer_fraction>1.0f)
            steer_fraction = 1.0f;
    }

    float old_steer      = m_controls->m_steer;

    // The AI has its own 'time full steer' value (which is the time
    float max_steer_change = dt/m_ai_properties->m_time_full_steer;
    if(old_steer < steer_fraction)
    {
        m_controls->m_steer = (old_steer+max_steer_change > steer_fraction) 
                           ? steer_fraction : old_steer+max_steer_change;
    }
    else
    {
        m_controls->m_steer = (old_steer-max_steer_change < steer_fraction) 
                           ? steer_fraction : old_steer-max_steer_change;
    }


}   // setSteering

// ----------------------------------------------------------------------------
/** Determine the center point and radius of a circle given two points on
 *  the ccircle and the tangent at the first point. This is done as follows:
 *  1. Determine the line going through the center point start+end, which is 
 *     orthogonal to the vector from start to end. This line goes through the
 *     center of the circle.
 *  2. Determine the line going through the first point and is orthogonal
 *     to the given tangent.
 *  3. The intersection of these two lines is the center of the circle.
 *  \param[in] start First point.
 *  \param[in] tangent Tangent at first point.
 *  \param[in] end Second point on circle.
 *  \param[out] center Center point of the circle.
 *  \param[out] radius Radius of the circle.
 */
void  SkiddingAI::determineTurnRadius(const Vec3 &start,
                                      const Vec3 &tangent,
                                      const Vec3 &end,
                                      Vec3 *center,
                                      float *radius)
{
    // 1) Line through middle of start+end
    Vec3 mid = 0.5f*(start+end);
    Vec3 direction = end-start;

    Vec3 orthogonal(direction.getZ(), 0, -direction.getX());
    Vec3  q1 = mid + orthogonal;
    irr::core::line2df line1(mid.getX(), mid.getZ(),
                             q1.getX(),  q1.getZ()  );

    Vec3 ortho_tangent(tangent.getZ(), 0, -tangent.getX());
    Vec3  q2 = start + ortho_tangent;
    irr::core::line2df line2(start.getX(), start.getZ(),
                             q2.getX(),    q2.getZ());


    irr::core::vector2df result;
    if(line1.intersectWith(line2, result, /*checkOnlySegments*/false))
    {
        *center = Vec3(result.X, start.getY(), result.Y);
        *radius = (start - *center).length();
    }
    else
    {
        // No intersection. In this case assume that the two points are
        // on a semicircle, in which case the center is at 0.5*(start+end):
        *center = 0.5f*(start+end);
        *radius = 0.5f*(end-start).length();
    }
    return;
}   // determineTurnRadius
