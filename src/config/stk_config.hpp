//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 Joerg Henrichs
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

#ifndef HEADER_STK_CONFIG_HPP
#define HEADER_STK_CONFIG_HPP

/**
 * \defgroup config
 * This module handles the user configuration, the supertuxkart configuration
 * file (which contains options usually not edited by the player) and the input
 * configuration file.
 */

#include "utils/no_copy.hpp"

#include <vector>
#include <string>

class KartProperties;
class MusicInformation;
class XMLNode;

/**
 * \brief Global STK configuration information.
 * Parameters here can be tuned without recompilation, but the user shouldn't actually modify
 * them. It also includes the list of default kart physics parameters which are used for
 * each kart (but which can be overwritten for each kart, too).
 * \ingroup config
 */
class STKConfig : public NoCopy
{
protected:
    /** Default kart properties. */
    KartProperties  *m_default_kart_properties;

public:
    /** What to do if a kart already has a powerup when it hits a bonus box:
     *  - NEW:  give it a random new bonx box.
     *  - SAME: give it one more item of the type it currently has.
     *  - ONLY_IF_SAME: only give it one more item if the randomly chosen item
     *          has the same type as the currently held item. */
    enum {POWERUP_MODE_NEW,
          POWERUP_MODE_SAME,
          POWERUP_MODE_ONLY_IF_SAME}
          m_same_powerup_mode;

    static float UNDEFINED;
    float m_anvil_weight;            /**<Additional kart weight if anvil is
                                         attached.                           */
    float m_anvil_speed_factor;      /**<Speed decrease when attached first. */
    float m_parachute_friction;      /**<Increased parachute air friction.   */
    float m_parachute_done_fraction; /**<Fraction of speed when lost will
                                         detach parachute.                   */
    float m_parachute_time;          /**<Time a parachute is active.         */
    float m_parachute_time_other;    /**<Time a parachute attached to other
                                         karts is active.                    */
    float m_bomb_time;               /**<Time before a bomb explodes.        */
    float m_bomb_time_increase;      /**<Time added to bomb timer when it's
                                         passed on.                          */
    float m_anvil_time;              /**<Time an anvil is active.            */
    float m_item_switch_time;        /**< Time items will be switched.       */
    int   m_bubblegum_counter;       /**< How many times bubble gums must be
                                          driven over before they disappear. */
    float m_bubblegum_shield_time;   /**<How long a bubble gum shield lasts. */
    bool  m_shield_restrict_weapos;  /**<Wether weapon usage is punished. */
    float m_explosion_impulse_objects;/**<Impulse of explosion on moving
                                          objects, e.g. road cones, ...      */
    float m_penalty_time;            /**< Penalty time when starting too
                                          early.                             */
    float m_delay_finish_time;       /**<Delay after a race finished before
                                         the results are displayed.          */
    float m_music_credit_time;       /**<Time the music credits are
                                         displayed.                          */
    int   m_max_karts;               /**<Maximum number of karts.            */
    int   m_max_history;             /**<Maximum number of frames to save in
                                         a history files.                    */
    bool  m_smooth_normals;          /**< If normals for raycasts for wheels
                                         should be interpolated.             */
    /** If the angle between a normal on a vertex and the normal of the
     *  triangle are more than this value, the physics will use the normal
     *  of the triangle in smoothing normal. */
    float m_smooth_angle_limit;
    int   m_max_skidmarks;           /**<Maximum number of skid marks/kart.  */
    float m_skid_fadeout_time;       /**<Time till skidmarks fade away.      */
    float m_near_ground;             /**<Determines when a kart is not near
                                      *  ground anymore and the upright
                                      *  constraint is disabled to allow for
                                      *  more violent explosions.            */
    int   m_min_kart_version,        /**<The minimum and maximum .kart file  */
          m_max_kart_version;        /**<version supported by this binary.   */
    int   m_min_track_version,       /**<The minimum and maximum .track file */
          m_max_track_version;       /**<version supported by this binary.   */
    int   m_max_display_news;        /**<How often a news message is displayed
                                         before it is ignored. */
    bool  m_enable_networking;

    /** Disable steering if skidding is stopped. This can help in making
     *  skidding more controllable (since otherwise when trying to steer while
     *  steering is reset to match the graphics it often results in the kart
     *  crashing). */
    bool m_disable_steer_while_unskid;

    /** If true the camera will stay behind the kart, potentially making it
     *  easier to see where the kart is going to after the skid. */
    bool m_camera_follow_skid;

    float m_ai_acceleration;         /**<Between 0 and 1, default being 1, can be
                                         used to give a handicap to AIs */

    std::vector<float>
          m_leader_intervals;        /**<Interval in follow the leader till
                                         last kart is reomved.               */
    float m_leader_time_per_kart;    /**< Additional time to each leader
                                          interval for each additional kart. */
    std::vector<int> m_switch_items; /**< How to switch items.               */
    /** The number of points a kart on position X has more than the
     *  next kart. From this the actual number of points for each
     *  position is computed. */
    std::vector<int> m_score_increase;

    /** Filename of the title music to play.*/
    MusicInformation
         *m_title_music;

    /** Minimum time between consecutive saved tranform events.  */
    float m_replay_dt;

    /** Maximum difference between interpolated and actual position. If the
     *  difference is larger than this, a new event is generated. */
    float m_replay_delta_pos2;

    /** A heading difference of more than that will trigger a new event to
     *  be generated. */
    float m_replay_delta_angle;

private:
    /** True if stk_config has been loaded. This is necessary if the
     *  --stk-config command line parameter has been specified to avoid
     *  that stk loads the default configuration after already having
     *  loaded a user specified config file. */
    bool  m_has_been_loaded;

public:
         STKConfig();
        ~STKConfig();
    void init_defaults    ();
    void getAllData       (const XMLNode * root);
    void load             (const std::string &filename);
    const std::string &getMainMenuPicture(int n);
    const std::string &getBackgroundPicture(int n);
    void  getAllScores(std::vector<int> *all_scores, int num_karts);
    // ------------------------------------------------------------------------
    /** Returns the default kart properties for each kart. */
    const KartProperties &
         getDefaultKartProperties() const {return *m_default_kart_properties; }
}
;   // STKConfig

extern STKConfig* stk_config;
#endif
