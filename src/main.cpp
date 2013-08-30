//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2006 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2011 Joerg Henrichs, Marianne Gagnon
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


/**
 * \mainpage SuperTuxKart developer documentation
 *
 * This document contains the developer documentation for SuperTuxKart,
 * including the list of modules, the list of classes, the API reference,
 * and some pages that describe in more depth some parts of the code/engine.
 *
 * \section Overview
 *
 * Here is an overview of the high-level interactions between modules :
 \dot
 digraph interaction {
 race -> modes
 race -> tracks
 race -> karts
 modes -> tracks
 modes -> karts
 tracks -> graphics
 karts -> graphics
 tracks -> items
 graphics -> irrlicht
 guiengine -> irrlicht
 states_screens -> guiengine
 states_screens -> input
 guiengine -> input
 karts->physics
 tracks->physics
 karts -> controller
 input->controller
 tracks -> animations
 physics -> animations
 }
 \enddot

 Note that this graph is only an approximation because the real one would be
 much too complicated :)


 \section Modules

 \li \ref addonsgroup :
   Handles add-ons that can be downloaded.
 \li \ref animations :
   This module manages interpolation-based animation (of position, rotation
   and/or scale)
 \li \ref audio :
   This module handles audio (sound effects and music).
 \li \ref challenges :
   This module handles the challenge system, which locks features (tracks, karts
   modes, etc.) until the user completes some task.
 \li \ref config :
   This module handles the user configuration, the supertuxkart configuration
   file (which contains options usually not edited by the player) and the input
   configuration file.
 \li \ref graphics :
   This module contains the core graphics engine, that is mostly a thin layer
   on top of irrlicht providing some additional features we need for STK
   (like particles, more scene node types, mesh manipulation tools, material
   management, etc...)
 \li \ref guiengine :
   Contains the generic GUI engine (contains the widgets and the backing logic
   for event handling, the skin, screens and dialogs). See module @ref states_screens
   for the actual STK GUI screens. Note that all input comes through this module
   too.
 \li \ref widgetsgroup :
   Contains the various types of widgets supported by the GUI engine.
 \li \ref input :
   Contains classes for input management (keyboard and gamepad)
 \li \ref io :
  Contains generic utility classes for file I/O (especially XML handling).
 \li \ref items :
   Defines the various collectibles and weapons of STK.
 \li \ref karts :
   Contains classes that deal with the properties, models and physics
   of karts.
 \li \ref controller :
   Contains kart controllers, which are either human players or AIs
   (this module thus contains the AIs)
 \li \ref modes :
   Contains the logic for the various game modes (race, follow the leader,
   battle, etc.)
 \li \ref physics :
   Contains various physics utilities.
 \li \ref race :
   Contains the race information that is conceptually above what you can find
   in group Modes. Handles highscores, grands prix, number of karts, which
   track was selected, etc.
 \li \ref states_screens :
   Contains the various screens and dialogs of the STK user interface,
   using the facilities of the guiengine module. Also contains the
   stack of menus and handles state management (in-game vs menu).
 \li \ref tracks :
   Contains information about tracks, namely drivelines, checklines and track
   objects.
 \li \ref tutorial :
   Work in progress
 */

#ifdef WIN32
#  ifdef __CYGWIN__
#    include <unistd.h>
#  endif
#  define _WINSOCKAPI_
#  include <windows.h>
#  ifdef _MSC_VER
#    include <direct.h>
#  endif
#else
#  include <unistd.h>
#endif
#include <time.h>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>

#include <IEventReceiver.h>

#include "main_loop.hpp"
#include "addons/addons_manager.hpp"
#include "addons/inetwork_http.hpp"
#include "addons/news_manager.hpp"
#include "audio/music_manager.hpp"
#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/stk_config.hpp"
#include "config/user_config.hpp"
#include "config/player.hpp"
#include "graphics/hardware_skinning.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/particle_kind_manager.hpp"
#include "graphics/referee.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/event_handler.hpp"
#include "input/input_manager.hpp"
#include "input/device_manager.hpp"
#include "input/wiimote_manager.hpp"
#include "io/file_manager.hpp"
#include "items/attachment_manager.hpp"
#include "items/item_manager.hpp"
#include "items/projectile_manager.hpp"
#include "karts/controller/ai_base_controller.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "modes/demo_world.hpp"
#include "modes/profile_world.hpp"
#include "network/network_manager.hpp"
#include "race/grand_prix_manager.hpp"
#include "race/highscore_manager.hpp"
#include "race/history.hpp"
#include "race/race_manager.hpp"
#include "replay/replay_play.hpp"
#include "replay/replay_recorder.hpp"
#include "states_screens/story_mode_lobby.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/dialogs/message_dialog.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/constants.hpp"
#include "utils/leak_check.hpp"
#include "utils/log.hpp"
#include "utils/translation.hpp"

static void cleanSuperTuxKart();

// ============================================================================
//                        gamepad visualisation screen
// ============================================================================

void gamepadVisualisation()
{

    core::array<SJoystickInfo>          irrlicht_gamepads;
    irr_driver->getDevice()->activateJoysticks(irrlicht_gamepads);


    struct Gamepad
    {
        s16   m_axis[SEvent::SJoystickEvent::NUMBER_OF_AXES];
        bool  m_button_state[SEvent::SJoystickEvent::NUMBER_OF_BUTTONS];
    };

    #define GAMEPAD_COUNT 8 // const won't work

    class EventReceiver : public IEventReceiver
    {
    public:
        Gamepad m_gamepads[GAMEPAD_COUNT];

        EventReceiver()
        {
            for (int n=0; n<GAMEPAD_COUNT; n++)
            {
                Gamepad& g = m_gamepads[n];
                for (int i=0; i<SEvent::SJoystickEvent::NUMBER_OF_AXES; i++)
                    g.m_axis[i] = 0;
                for (int i=0; i<SEvent::SJoystickEvent::NUMBER_OF_BUTTONS; i++)
                    g.m_button_state[i] = false;
            }
        }

        virtual bool OnEvent (const irr::SEvent &event)
        {
            switch (event.EventType)
            {
                case EET_JOYSTICK_INPUT_EVENT :
                {
                    const SEvent::SJoystickEvent& evt = event.JoystickEvent;
                    if (evt.Joystick >= GAMEPAD_COUNT) return true;

                    Gamepad& g = m_gamepads[evt.Joystick];
                    for (int i=0; i<SEvent::SJoystickEvent::NUMBER_OF_AXES;i++)
                    {
                        g.m_axis[i] = evt.Axis[i];
                    }
                    for (int i=0; i<SEvent::SJoystickEvent::NUMBER_OF_BUTTONS;
                         i++)
                    {
                        g.m_button_state[i] = evt.IsButtonPressed(i);
                    }
                    break;
                }

                case EET_KEY_INPUT_EVENT:
                {
                    const SEvent::SKeyInput& evt = event.KeyInput;

                    if (evt.PressedDown)
                    {
                        if (evt.Key == KEY_RETURN || evt.Key == KEY_ESCAPE ||
                            evt.Key == KEY_SPACE)
                        {
                            exit(0);
                        }
                    }

                }

                default:
                    // don't care about others
                    break;
            }
            return true;
        }
    };

    EventReceiver* events = new EventReceiver();
    irr_driver->getDevice()->setEventReceiver(events);

    while (true)
    {
        if (!irr_driver->getDevice()->run()) break;

        video::IVideoDriver* driver = irr_driver->getVideoDriver();
        const core::dimension2du size = driver ->getCurrentRenderTargetSize();

        driver->beginScene(true, true, video::SColor(255,0,0,0));

        for (int n=0; n<GAMEPAD_COUNT; n++)
        {
            Gamepad& g = events->m_gamepads[n];

            const int MARGIN = 10;
            const int x = (n & 1 ? size.Width/2 + MARGIN : MARGIN );
            const int w = size.Width/2 - MARGIN*2;
            const int h = size.Height/(GAMEPAD_COUNT/2) - MARGIN*2;
            const int y = size.Height/(GAMEPAD_COUNT/2)*(n/2) + MARGIN;

            driver->draw2DRectangleOutline( core::recti(x, y, x+w, y+h) );

            const int btn_y = y + 5;
            const int btn_x = x + 5;
            const int BTN_SIZE =
                (w - 10)/SEvent::SJoystickEvent::NUMBER_OF_BUTTONS;

            for (int b=0; b<SEvent::SJoystickEvent::NUMBER_OF_BUTTONS; b++)
            {
                core::position2di pos(btn_x + b*BTN_SIZE, btn_y);
                core::dimension2di size(BTN_SIZE, BTN_SIZE);

                if (g.m_button_state[b])
                {
                    driver->draw2DRectangle (video::SColor(255,255,0,0),
                                             core::recti(pos, size));
                }

                driver->draw2DRectangleOutline( core::recti(pos, size) );
            }

            const int axis_y = btn_y + BTN_SIZE + 5;
            const int axis_x = btn_x;
            const int axis_w = w - 10;
            const int axis_h = (h - BTN_SIZE - 15)
                            / SEvent::SJoystickEvent::NUMBER_OF_AXES;

            for (int a=0; a<SEvent::SJoystickEvent::NUMBER_OF_AXES; a++)
            {
                const float rate = g.m_axis[a] / 32767.0f;

                core::position2di pos(axis_x, axis_y + a*axis_h);
                core::dimension2di size(axis_w, axis_h);

                const bool deadzone = (abs(g.m_axis[a]) < DEADZONE_JOYSTICK);

                core::recti fillbar(core::position2di(axis_x + axis_w/2,
                                                      axis_y + a*axis_h),
                                    core::dimension2di( (int)(axis_w/2*rate),
                                                        axis_h)               );
                fillbar.repair(); // dimension may be negative
                driver->draw2DRectangle (deadzone ? video::SColor(255,255,0,0)
                                                  : video::SColor(255,0,255,0),
                                         fillbar);
                driver->draw2DRectangleOutline( core::recti(pos, size) );
            }
        }

        driver->endScene();
    }
}
// ============================================================================


void cmdLineHelp (char* invocation)
{
    Log::info("main",
    "Usage: %s [OPTIONS]\n\n"
    "Run SuperTuxKart, a racing game with go-kart that features"
    " the Tux and friends.\n\n"
    "Options:\n"
    "  -N,  --no-start-screen  Immediately start race without showing a "
                              "menu.\n"
    "  -R,  --race-now         Same as -N but also skip the ready-set-go phase"
                              " and the music.\n"
    "  -t,  --track NAME       Start at track NAME (see --list-tracks).\n"
    "       --gp NAME          Start the specified Grand Prix.\n"
    "       --stk-config FILE  use ./data/FILE instead of "
                              "./data/stk_config.xml\n"
    "  -l,  --list-tracks      Show available tracks.\n"
    "  -k,  --numkarts NUM     Number of karts on the racetrack.\n"
    "       --kart NAME        Use kart number NAME (see --list-karts).\n"
    "       --ai=a,b,...       Use the karts a, b, ... for the AI.\n"
    "       --list-karts       Show available karts.\n"
    "       --laps N           Define number of laps to N.\n"
    "       --mode N           N=1 novice, N=2 driver, N=3 racer.\n"
    "       --type N           N=0 Normal, N=1 Time trial, N=2 FTL\n"
    "       --reverse          Play track in reverse (if allowed)\n"
    // TODO: add back "--players" switch
    // "       --players n      Define number of players to between 1 and 4.\n"
    "  -f,  --fullscreen       Select fullscreen display.\n"
    "  -w,  --windowed         Windowed display (default).\n"
    "  -s,  --screensize WxH   Set the screen size (e.g. 320x200).\n"
    "  -v,  --version          Show version of SuperTuxKart.\n"
    "       --trackdir DIR     A directory from which additional tracks are "
                              "loaded.\n"
    "       --animations=n     Play karts' animations (All: 2, Humans only: 1,"
                              " Nobody: 0).\n"
    "       --gfx=n            Play other graphical effects like impact stars "
                              "dance,\n"
    "                            water animations or explosions (Enable: 1, "
                              "Disable: 0).\n"
    "       --weather=n        Show weather effects like rain or snow (0 or 1 "
                              "as --gfx).\n"
    "       --camera-style=n   Flexible (0) or hard like v0.6 (1) kart-camera "
                              "link.\n"
    "       --profile-laps=n   Enable automatic driven profile mode for n "
                              "laps.\n"
    "       --profile-time=n   Enable automatic driven profile mode for n "
                              "seconds.\n"
    "       --no-graphics      Do not display the actual race.\n"
    "       --demo-mode t      Enables demo mode after t seconds idle time in "
                               "main menu.\n"
    "       --demo-tracks t1,t2 List of tracks to be used in demo mode. No\n"
    "                          spaces are allowed in the track names.\n"
    "       --demo-laps n      Number of laps in a demo.\n"
    "       --demo-karts n     Number of karts to use in a demo.\n"
    "       --ghost            Replay ghost data together with one player kart.\n"
    // "       --history          Replay history file 'history.dat'.\n"
    // "       --history=n        Replay history file 'history.dat' using:\n"
    // "                            n=1: recorded positions\n"
    // "                            n=2: recorded key strokes\n"
    //"       --server[=port]    This is the server (running on the specified "
    //                          "port).\n"
    //"       --client=ip        This is a client, connect to the specified ip"
    //                          " address.\n"
    //"       --port=n           Port number to use.\n"
    //"       --numclients=n     Number of clients to wait for (server "
    //                          "only).\n"
    "       --no-console       Does not write messages in the console but to\n"
    "                          stdout.log.\n"
    "       --console          Write messages in the console and files\n"
    "  -h,  --help             Show this help.\n"
    "\n"
    "You can visit SuperTuxKart's homepage at "
    "http://supertuxkart.sourceforge.net\n\n", invocation
    );
}   // cmdLineHelp

//=============================================================================
/** For base options that don't need much to be inited (and, in some cases,
 *  that need to be read before initing stuff) - it only assumes that
 *  user config is loaded (necessary to check for blacklisted screen
 *  resolutions), but nothing else (esp. not kart_properties_manager and
 *  track_manager, since their search path might be extended by command
 *  line options).
 */
int handleCmdLinePreliminary(int argc, char **argv)
{
    int n;
    for(int i=1; i<argc; i++)
    {
        if(argv[i][0] != '-') continue;
        if (!strcmp(argv[i], "--help" ) ||
            !strcmp(argv[i], "-help"  ) ||
            !strcmp(argv[i], "-h"     )     )
        {
            cmdLineHelp(argv[0]);
            exit(0);
        }
        else if(!strcmp(argv[i], "--gamepad-visualisation") ||
                !strcmp(argv[i], "--gamepad-visualization")   )
        {
            UserConfigParams::m_gamepad_visualisation=true;
        }
        else if ( !strcmp(argv[i], "--debug=memory") )
        {
            UserConfigParams::m_verbosity |= UserConfigParams::LOG_MEMORY;
        }
        else if ( !strcmp(argv[i], "--debug=addons") )
        {
            UserConfigParams::m_verbosity |= UserConfigParams::LOG_ADDONS;
        }
        else if ( !strcmp(argv[i], "--debug=gui") )
        {
            UserConfigParams::m_verbosity |= UserConfigParams::LOG_GUI;
        }
        else if ( !strcmp(argv[i], "--debug=flyable") )
        {
            UserConfigParams::m_verbosity |= UserConfigParams::LOG_FLYABLE;
        }
        else if ( !strcmp(argv[i], "--debug=misc") )
        {
            UserConfigParams::m_verbosity |= UserConfigParams::LOG_MISC;
        }
        else if ( sscanf(argv[i], "--xmas=%d", &n) )
        {
            if (n)
            {
                UserConfigParams::m_xmas_enabled = true;
            }
            else
            {
                UserConfigParams::m_xmas_enabled = false;
            }
        }
        else if( !strcmp(argv[i], "--no-console"))
        {
            UserConfigParams::m_log_errors_to_console=false;
        }
        else if( !strcmp(argv[i], "--console"))
        {
            UserConfigParams::m_log_errors_to_console=true;
        }
        else if( !strcmp(argv[i], "--log=nocolor"))
        {
            Log::disableColor();
            Log::verbose("main", "Colours disabled.\n");
        }
        else if(sscanf(argv[i], "--log=%d",&n)==1)
        {
            Log::setLogLevel(n);
        }
        else if ( !strcmp(argv[i], "--debug=all") )
        {
            UserConfigParams::m_verbosity |= UserConfigParams::LOG_ALL;
        }
        else if( (!strcmp(argv[i], "--stk-config")) && i+1<argc )
        {
            stk_config->load(file_manager->getDataFile(argv[i+1]));
            Log::info("main", "STK config will be read from %s.\n",argv[i+1] );
            i++;
        }
        else if( !strcmp(argv[i], "--trackdir") && i+1<argc )
        {
            TrackManager::addTrackSearchDir(argv[i+1]);
            i++;
        }
        else if( !strcmp(argv[i], "--kartdir") && i+1<argc )
        {
            KartPropertiesManager::addKartSearchDir(argv[i+1]);
            i++;
        }
        else if( !strcmp(argv[i], "--no-graphics") || !strncmp(argv[i], "--list-", 7) ||
                 !strcmp(argv[i], "-l" ))
        {
            ProfileWorld::disableGraphics();
            UserConfigParams::m_log_errors_to_console=true;
        }
#if !defined(WIN32) && !defined(__CYGWIN)
        else if ( !strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f"))
        {
            // Check that current res is not blacklisted
            std::ostringstream o;
            o << UserConfigParams::m_width << "x"
              << UserConfigParams::m_height;
            std::string res = o.str();
            if (std::find(UserConfigParams::m_blacklist_res.begin(),
                          UserConfigParams::m_blacklist_res.end(),res)
                             == UserConfigParams::m_blacklist_res.end())
                UserConfigParams::m_fullscreen = true;
            else
                Log::warn("main", "Resolution %s has been blacklisted, so it "
                          "is not available!\n", res.c_str());
        }
        else if ( !strcmp(argv[i], "--windowed") || !strcmp(argv[i], "-w"))
        {
            UserConfigParams::m_fullscreen = false;
        }
#endif
        else if ( (!strcmp(argv[i], "--screensize") || !strcmp(argv[i], "-s") )
             && i+1<argc)
        {
            //Check if fullscreen and new res is blacklisted
            int width, height;
            if (sscanf(argv[i+1], "%dx%d", &width, &height) == 2)
            {
                std::ostringstream o;
                o << width << "x" << height;
                std::string res = o.str();
                if (!UserConfigParams::m_fullscreen ||
                    std::find(UserConfigParams::m_blacklist_res.begin(),
                              UserConfigParams::m_blacklist_res.end(),res) ==
                                  UserConfigParams::m_blacklist_res.end())
                {
                    UserConfigParams::m_prev_width =
                        UserConfigParams::m_width = width;
                    UserConfigParams::m_prev_height =
                        UserConfigParams::m_height = height;
                    Log::verbose("main", "You choose to use %dx%d.\n",
                             (int)UserConfigParams::m_width,
                             (int)UserConfigParams::m_height );
                }
                else
                    Log::warn("main", "Resolution %s has been blacklisted, so "
                                      "it is not available!\n", res.c_str());
                i++;
            }
            else
            {
                Log::fatal("main", "Error: --screensize argument must be "
                                   "given as WIDTHxHEIGHT");
            }
        }
        else if (strcmp(argv[i], "--version") == 0 ||
                 strcmp(argv[i], "-v"       ) == 0    )
        {
            Log::info("main", "==============================");
            Log::info("main", "SuperTuxKart, %s.", STK_VERSION ) ;
#ifdef SVNVERSION
            Log::info("main", "SuperTuxKart, SVN revision number '%s'.",
                      SVNVERSION ) ;
#endif

            // IRRLICHT_VERSION_SVN
            Log::info("main", "Irrlicht version %i.%i.%i (%s)",
                      IRRLICHT_VERSION_MAJOR , IRRLICHT_VERSION_MINOR,
                      IRRLICHT_VERSION_REVISION, IRRLICHT_SDK_VERSION );

            Log::info("main", "==============================");
        }   // --verbose or -v
    }
    return 0;
}

// ============================================================================
/** Handles command line options.
 *  \param argc Number of command line options
 *  \param argv Command line options.
 */
int handleCmdLine(int argc, char **argv)
{
    int n;
    char s[1024];

    for(int i=1; i<argc; i++)
    {

        if(!strcmp(argv[i], "--gamepad-debug"))
        {
            UserConfigParams::m_gamepad_debug=true;
        }
        else if (!strcmp(argv[i], "--wiimote-debug"))
        {
            UserConfigParams::m_wiimote_debug = true;
        }
        else if (!strcmp(argv[i], "--tutorial-debug"))
        {
            UserConfigParams::m_tutorial_debug = true;
        }
        else if(sscanf(argv[i], "--track-debug=%d",&n)==1)
        {
            UserConfigParams::m_track_debug=n;
        }
        else if(!strcmp(argv[i], "--track-debug"))
        {
            UserConfigParams::m_track_debug=1;
        }
        else if(!strcmp(argv[i], "--material-debug"))
        {
            UserConfigParams::m_material_debug = true;
        }
        else if(!strcmp(argv[i], "--ftl-debug"))
        {
            UserConfigParams::m_ftl_debug = true;
        }
        else if(UserConfigParams::m_artist_debug_mode &&
               !strcmp(argv[i], "--camera-debug"))
        {
            UserConfigParams::m_camera_debug=1;
        }
        else if(UserConfigParams::m_artist_debug_mode &&
               !strcmp(argv[i], "--physics-debug"))
        {
            UserConfigParams::m_physics_debug=1;
        }
        else if(!strcmp(argv[i], "--kartsize-debug"))
        {
            for(unsigned int i=0;
                i<kart_properties_manager->getNumberOfKarts(); i++)
            {
                const KartProperties *km =
                    kart_properties_manager->getKartById(i);
                 Log::info("main", "%s:\t%swidth: %f length: %f height: %f "
                                   "mesh-buffer count %d",
                        km->getIdent().c_str(),
                        (km->getIdent().size()<7) ? "\t" : "",
                        km->getMasterKartModel().getWidth(),
                        km->getMasterKartModel().getLength(),
                        km->getMasterKartModel().getHeight(),
                        km->getMasterKartModel().getModel()
                          ->getMeshBufferCount());
            }
        }
        else if(UserConfigParams::m_artist_debug_mode &&
                !strcmp(argv[i], "--check-debug"))
        {
            UserConfigParams::m_check_debug=true;
        }
        else if(!strcmp(argv[i], "--slipstream-debug"))
        {
            UserConfigParams::m_slipstream_debug=true;
        }
        else if(!strcmp(argv[i], "--rendering-debug"))
        {
            UserConfigParams::m_rendering_debug=true;
        }
        else if(!strcmp(argv[i], "--ai-debug"))
        {
            AIBaseController::enableDebug();
        }
        else if(sscanf(argv[i], "--server=%d",&n)==1)
        {
            network_manager->setMode(NetworkManager::NW_SERVER);
            UserConfigParams::m_server_port = n;
        }
        else if( !strcmp(argv[i], "--server") )
        {
            network_manager->setMode(NetworkManager::NW_SERVER);
        }
        else if( sscanf(argv[i], "--port=%d", &n) )
        {
            UserConfigParams::m_server_port=n;
        }
        else if( sscanf(argv[i], "--client=%1023s", s) )
        {
            network_manager->setMode(NetworkManager::NW_CLIENT);
            UserConfigParams::m_server_address=s;
        }
        else if ( sscanf(argv[i], "--gfx=%d", &n) )
        {
            if (n)
            {
                UserConfigParams::m_graphical_effects = true;
            }
            else
            {
                UserConfigParams::m_graphical_effects = false;
            }
        }
        else if ( sscanf(argv[i], "--weather=%d", &n) )
        {
            if (n)
            {
                UserConfigParams::m_weather_effects = true;
            }
            else
            {
                UserConfigParams::m_weather_effects = false;
            }
        }
        else if ( sscanf(argv[i], "--animations=%d", &n) )
        {
            UserConfigParams::m_show_steering_animations = n;
        }

        else if ( sscanf(argv[i], "--camera-style=%d", &n) )
        {
            UserConfigParams::m_camera_style = n;
        }
        else if( (!strcmp(argv[i], "--kart") && i+1<argc ))
        {
            unlock_manager->setCurrentSlot(UserConfigParams::m_all_players[0]
                                           .getUniqueID()                    );

            if (!unlock_manager->getCurrentSlot()->isLocked(argv[i+1]))
            {
                const KartProperties *prop =
                    kart_properties_manager->getKart(argv[i+1]);
                if(prop)
                {
                    UserConfigParams::m_default_kart = argv[i+1];

                    // if a player was added with -N, change its kart.
                    // Otherwise, nothing to do, kart choice will be picked
                    // up upon player creation.
                    if (StateManager::get()->activePlayerCount() > 0)
                    {
                        race_manager->setLocalKartInfo(0, argv[i+1]);
                    }
                    Log::verbose("main", "You chose to use kart '%s'.",
                                 argv[i+1] ) ;
                    i++;
                }
                else
                {
                    Log::warn("main", "Kart '%s' not found, ignored.",
                              argv[i+1]);
                    i++;  // ignore the next parameter, otherwise STK will abort
                }
            }
            else   // kart locked
            {
                Log::warn("main", "Kart '%s' has not been unlocked yet.",
                          argv[i+1]);
                Log::warn("main",
                          "Use --list-karts to list available karts.");
                return 0;
            }   // if kart locked
        }
        else if( sscanf(argv[i], "--ai=%1023s", s)==1)
        {
            const std::vector<std::string> l=
                StringUtils::split(std::string(s),',');
            race_manager->setDefaultAIKartList(l);
            // Add 1 for the player kart
            race_manager->setNumKarts(l.size()+1);
        }
        else if( (!strcmp(argv[i], "--mode") && i+1<argc ))
        {
            int n = atoi(argv[i+1]);
            if(n<0 || n>RaceManager::DIFFICULTY_LAST)
                Log::warn("main", "Invalid difficulty '%s' - ignored.\n",
                          argv[i+1]);
            else
                race_manager->setDifficulty(RaceManager::Difficulty(n));
            i++;
        }
        else if( (!strcmp(argv[i], "--type") && i+1<argc ))
        {
            switch (atoi(argv[i+1]))
            {
            case 0: race_manager
                        ->setMinorMode(RaceManager::MINOR_MODE_NORMAL_RACE);
                    break;
            case 1: race_manager
                        ->setMinorMode(RaceManager::MINOR_MODE_TIME_TRIAL);
                    break;
            case 2: race_manager
                        ->setMinorMode(RaceManager::MINOR_MODE_FOLLOW_LEADER);
                    break;
            default:
                Log::warn("main", "Invalid race type '%d' - ignored.",
                          atoi(argv[i+1]));
            }
            i++;
        }
        else if( !strcmp(argv[i], "--reverse"))
        {
            race_manager->setReverseTrack(true);
        }
        else if( (!strcmp(argv[i], "--track") || !strcmp(argv[i], "-t"))
                 && i+1<argc                                              )
        {
            unlock_manager->setCurrentSlot(UserConfigParams::m_all_players[0]
                                           .getUniqueID()                    );
            if (!unlock_manager->getCurrentSlot()->isLocked(argv[i+1]))
            {
                race_manager->setTrack(argv[i+1]);
                Log::verbose("main", "You choose to start in track '%s'.",
                             argv[i+1] );

                Track* t = track_manager->getTrack(argv[i+1]);
                if (t == NULL)
                {
                    Log::warn("main", "Can't find track named '%s'.",
                              argv[i+1]);
                }
                else if (t->isArena())
                {
                    //if it's arena, don't create ai karts
                    const std::vector<std::string> l;
                    race_manager->setDefaultAIKartList(l);
                    // Add 1 for the player kart
                    race_manager->setNumKarts(1);
                    race_manager->setMinorMode(RaceManager::MINOR_MODE_3_STRIKES);
                }
                else if(t->isSoccer())
                {
                    //if it's soccer, don't create ai karts
                    const std::vector<std::string> l;
                    race_manager->setDefaultAIKartList(l);
                    // Add 1 for the player kart
                    race_manager->setNumKarts(1);
                    race_manager->setMinorMode(RaceManager::MINOR_MODE_SOCCER);
                }
            }
            else
            {
                Log::warn("main", "Track '%s' has not been unlocked yet.",
                         argv[i+1]);
                Log::warn("main", "Use --list-tracks to list available "
                                  "tracks.");
                return 0;
            }
            i++;
        }
        else if( (!strcmp(argv[i], "--gp")) && i+1<argc)
        {
            race_manager->setMajorMode(RaceManager::MAJOR_MODE_GRAND_PRIX);
            const GrandPrixData *gp =
                grand_prix_manager->getGrandPrix(argv[i+1]);

            if (gp == NULL)
            {
                Log::warn("main", "There is no GP named '%s'.", argv[i+1]);
                return 0;
            }

            race_manager->setGrandPrix(*gp);
            i++;
        }
        else if( (!strcmp(argv[i], "--numkarts") || !strcmp(argv[i], "-k")) &&
                 i+1<argc )
        {
            UserConfigParams::m_num_karts = atoi(argv[i+1]);
            if(UserConfigParams::m_num_karts > stk_config->m_max_karts)
            {
                Log::warn("main",
                          "Number of karts reset to maximum number %d.",
                                  stk_config->m_max_karts);
                UserConfigParams::m_num_karts = stk_config->m_max_karts;
            }
            race_manager->setNumKarts( UserConfigParams::m_num_karts );
            Log::verbose("main", "%d karts will be used.",
                         (int)UserConfigParams::m_num_karts);
            i++;
        }
        else if( !strcmp(argv[i], "--list-tracks") || !strcmp(argv[i], "-l") )
        {

            Log::info("main", "  Available tracks:" );
            unlock_manager->setCurrentSlot(UserConfigParams::m_all_players[0]
                                           .getUniqueID()                    );

            for (size_t i = 0; i != track_manager->getNumberOfTracks(); i++)
            {
                const Track *track = track_manager->getTrack(i);
                const char * locked = "";
                if ( unlock_manager->getCurrentSlot()
                                   ->isLocked(track->getIdent()) )
                {
                    locked = " (locked)";
                }
                Log::info("main", "%-18s: %ls %s",
                              track->getIdent().c_str(),
                              track->getName(), locked);
                //}
            }

            Log::info("main", "Use --track N to choose track.");

            exit(0);
        }
        else if( !strcmp(argv[i], "--list-karts") )
        {
            Log::info("main", "  Available karts:");
            for (unsigned int i = 0;
                 i < kart_properties_manager->getNumberOfKarts(); i++)
            {
                const KartProperties* KP =
                    kart_properties_manager->getKartById(i);
                unlock_manager->setCurrentSlot(UserConfigParams::m_all_players[0]
                                              .getUniqueID()                    );
                const char * locked = "";
                if (unlock_manager->getCurrentSlot()->isLocked(KP->getIdent()))
                    locked = "(locked)";

                Log::info("main", " %-10s: %ls %s", KP->getIdent().c_str(),
                         KP->getName(), locked);
            }

            exit(0);
        }
        else if (    !strcmp(argv[i], "--no-start-screen")
                     || !strcmp(argv[i], "-N")                )
        {
            UserConfigParams::m_no_start_screen = true;
        }
        else if (    !strcmp(argv[i], "--race-now")
                     || !strcmp(argv[i], "-R")                )
        {
            UserConfigParams::m_no_start_screen = true;
            UserConfigParams::m_race_now = true;
        }
        else if ( !strcmp(argv[i], "--laps") && i+1<argc )
        {
            int laps = atoi(argv[i+1]);
            if (laps < 0)
            {
                Log::error("main", "Invalid number of laps: %s.\n", argv[i+1] );
                return 0;
            }
            else
            {
                Log::verbose("main", "You choose to have %d laps.", laps);
                race_manager->setNumLaps(laps);
                i++;
            }
        }
        else if( sscanf(argv[i], "--profile-laps=%d",  &n)==1)
        {
            if (n < 0)
            {
                Log::error("main", "Invalid number of profile-laps: %i.\n", n );
                return 0;
            }
            else
            {
                Log::verbose("main", "Profiling %d laps.",n);
                UserConfigParams::m_no_start_screen = true;
                ProfileWorld::setProfileModeLaps(n);
                race_manager->setNumLaps(n);
            }
        }
        else if( sscanf(argv[i], "--profile-time=%d",  &n)==1)
        {
            Log::verbose("main", "Profiling: %d seconds.", n);
            UserConfigParams::m_no_start_screen = true;
            ProfileWorld::setProfileModeTime((float)n);
            race_manager->setNumLaps(999999); // profile end depends on time
        }
        else if( !strcmp(argv[i], "--no-graphics") )
        {
            // Set default profile mode of 1 lap if we haven't already set one
            if (!ProfileWorld::isProfileMode()) {
                UserConfigParams::m_no_start_screen = true;
                ProfileWorld::setProfileModeLaps(1);
                race_manager->setNumLaps(1);
            }
        }
        else if( !strcmp(argv[i], "--ghost"))
        {
            ReplayPlay::create();
        }
        else if( sscanf(argv[i], "--history=%d",  &n)==1)
        {
            history->doReplayHistory( (History::HistoryReplayMode)n);
            // Force the no-start screen flag, since this initialises
            // the player structures correctly.
            UserConfigParams::m_no_start_screen = true;

        }
        else if( !strcmp(argv[i], "--history") )
        {
            history->doReplayHistory(History::HISTORY_POSITION);
            // Force the no-start screen flag, since this initialises
            // the player structures correctly.
            UserConfigParams::m_no_start_screen = true;

        }
        else if( !strcmp(argv[i], "--demo-mode") && i+1<argc)
        {
            unlock_manager->setCurrentSlot(UserConfigParams::m_all_players[0]
                                       .getUniqueID()                    );
            float t;
            StringUtils::fromString(argv[i+1], t);
            DemoWorld::enableDemoMode(t);
            // The default number of laps is taken from ProfileWorld and
            // is 0. So set a more useful default for demo mode.
            DemoWorld::setNumLaps(2);
            i++;
        }
        else if( !strcmp(argv[i], "--demo-laps") && i+1<argc)
        {
            // Note that we use a separate setting for demo mode to avoid the
            // problem that someone plays a game, and in further demos then
            // the wrong (i.e. last selected) number of laps would be used
            DemoWorld::setNumLaps(atoi(argv[i+1]));
            i++;
        }
        else if( !strcmp(argv[i], "--demo-karts") && i+1<argc)
        {
            // Note that we use a separate setting for demo mode to avoid the
            // problem that someone plays a game, and in further demos then
            // the wrong (i.e. last selected) number of karts would be used
            DemoWorld::setNumKarts(atoi(argv[i+1]));
            i++;
        }
        else if( !strcmp(argv[i], "--demo-tracks") && i+1<argc)
        {
            DemoWorld::setTracks(StringUtils::split(std::string(argv[i+1]),
                                                    ','));
            i++;
        }
#ifdef ENABLE_WIIUSE
        else if( !strcmp(argv[i], "--wii"))
        {
            WiimoteManager::enable();
        }
#endif
        // these commands are already processed in handleCmdLinePreliminary,
        // but repeat this just so that we don't get error messages about
        // unknown commands
        else if( !strcmp(argv[i], "--stk-config")&& i+1<argc ) { i++; }
        else if( !strcmp(argv[i], "--trackdir")  && i+1<argc ) { i++; }
        else if( !strcmp(argv[i], "--kartdir")   && i+1<argc ) { i++; }
        else if( !strcmp(argv[i], "--debug=memory" )                       ) {}
        else if( !strcmp(argv[i], "--debug=addons" )                       ) {}
        else if( !strcmp(argv[i], "--debug=gui"    )                       ) {}
        else if( !strcmp(argv[i], "--debug=flyable")                       ) {}
        else if( !strcmp(argv[i], "--debug=misc"   )                       ) {}
        else if( !strcmp(argv[i], "--debug=all"    )                       ) {}
        else if ( sscanf(argv[i], "--xmas=%d", &n) )                         {}
        else if( !strcmp(argv[i], "--log=nocolor"  )                       ) {}
        else if(  sscanf(argv[i], "--log=%d",&n    )==1                    ) {}
        else if( !strcmp(argv[i], "--no-console"   )                       ) {}
        else if( !strcmp(argv[i], "--console"      )                       ) {}
        else if( !strcmp(argv[i], "--screensize") ||
                 !strcmp(argv[i], "-s")            )                     {i++;}
        else if( !strcmp(argv[i], "--fullscreen") || !strcmp(argv[i], "-f")) {}
        else if( !strcmp(argv[i], "--windowed")   || !strcmp(argv[i], "-w")) {}
        else if( !strcmp(argv[i], "--version")    || !strcmp(argv[i], "-v")) {}
#ifdef __APPLE__
        // on OS X, sometimes the Finder will pass a -psn* something parameter
        // to the application
        else if( strncmp(argv[i], "-psn", 3) == 0) {}
#endif
        else
        {
            // invalid param needs to go to console
            UserConfigParams::m_log_errors_to_console = true;

            Log::error("main", "Invalid parameter: %s.\n", argv[i] );
            cmdLineHelp(argv[0]);
            cleanSuperTuxKart();
            return 0;
        }
    }   // for i <argc
    if(UserConfigParams::m_no_start_screen)
        unlock_manager->setCurrentSlot(UserConfigParams::m_all_players[0]
                                       .getUniqueID()                    );
    if(ProfileWorld::isProfileMode())
    {
        UserConfigParams::m_sfx = false;  // Disable sound effects
        UserConfigParams::m_music = false;// and music when profiling
    }

    return 1;
}   // handleCmdLine

//=============================================================================
/** Initialises the minimum number of managers to get access to user_config.
 */
void initUserConfig(char *argv[])
{
    irr_driver              = new IrrDriver();
    file_manager            = new FileManager(argv);
    user_config             = new UserConfig();     // needs file_manager
    const bool config_ok    = user_config->loadConfig();

    if (UserConfigParams::m_language.toString() != "system")
    {
#ifdef WIN32
        std::string s=std::string("LANGUAGE=")
                     +UserConfigParams::m_language.c_str();
        _putenv(s.c_str());
#else
        setenv("LANGUAGE", UserConfigParams::m_language.c_str(), 1);
#endif
    }

    translations            = new Translations();   // needs file_manager
    stk_config              = new STKConfig();      // in case of --stk-config
                                                    // command line parameters

    if (!config_ok || UserConfigParams::m_all_players.size() == 0)
    {
        user_config->addDefaultPlayer();
        user_config->saveConfig();
    }

}   // initUserConfig

//=============================================================================
void initRest()
{
    stk_config->load(file_manager->getDataFile("stk_config.xml"));

    // Now create the actual non-null device in the irrlicht driver
    irr_driver->initDevice();

    // Init GUI
    IrrlichtDevice* device = irr_driver->getDevice();
    video::IVideoDriver* driver = device->getVideoDriver();

    if (UserConfigParams::m_gamepad_visualisation)
    {
        gamepadVisualisation();
        exit(0);
    }

    GUIEngine::init(device, driver, StateManager::get());

    // This only initialises the non-network part of the addons manager. The
    // online section of the addons manager will be initialised from a
    // separate thread running in network http.
    news_manager            = new NewsManager();
    addons_manager          = new AddonsManager();

    INetworkHttp::create();

    // Note that the network thread must be started after the assignment
    // to network_http (since the thread might use network_http, otherwise
    // a race condition can be introduced resulting in a crash).
    INetworkHttp::get()->startNetworkThread();
    music_manager           = new MusicManager();
    sfx_manager             = new SFXManager();
    // The order here can be important, e.g. KartPropertiesManager needs
    // defaultKartProperties, which are defined in stk_config.
    history                 = new History              ();
    ReplayRecorder::create();
    material_manager        = new MaterialManager      ();
    track_manager           = new TrackManager         ();
    kart_properties_manager = new KartPropertiesManager();
    projectile_manager      = new ProjectileManager    ();
    powerup_manager         = new PowerupManager       ();
    attachment_manager      = new AttachmentManager    ();
    highscore_manager       = new HighscoreManager     ();
    network_manager         = new NetworkManager       ();
    KartPropertiesManager::addKartSearchDir(
                 file_manager->getAddonsFile("karts/"));
    track_manager->addTrackSearchDir(
                 file_manager->getAddonsFile("tracks/"));

    track_manager->loadTrackList();
    music_manager->addMusicToTracks();

    GUIEngine::addLoadingIcon(
        irr_driver->getTexture(file_manager->getTextureFile("notes.png")) );

    grand_prix_manager      = new GrandPrixManager     ();
    // Consistency check for challenges, and enable all challenges
    // that have all prerequisites fulfilled
    grand_prix_manager->checkConsistency();
    GUIEngine::addLoadingIcon( irr_driver->getTexture(
                               file_manager->getTextureFile("cup_gold.png")) );

    race_manager            = new RaceManager          ();
    // default settings for Quickstart
    race_manager->setNumLocalPlayers(1);
    race_manager->setNumLaps   (3);
    race_manager->setMajorMode (RaceManager::MAJOR_MODE_SINGLE);
    race_manager->setMinorMode (RaceManager::MINOR_MODE_NORMAL_RACE);
    race_manager->setDifficulty(
                 (RaceManager::Difficulty)(int)UserConfigParams::m_difficulty);

}   // initRest

//=============================================================================
/** Frees all manager and their associated memory.
 */
static void cleanSuperTuxKart()
{
    irr_driver->updateConfigIfRelevant();

    if(INetworkHttp::get())
        INetworkHttp::get()->stopNetworkThread();
    //delete in reverse order of what they were created in.
    //see InitTuxkart()
    Referee::cleanup();
    if(ReplayPlay::get())       ReplayPlay::destroy();
    if(race_manager)            delete race_manager;
    INetworkHttp::destroy();
    if(news_manager)            delete news_manager;
    if(addons_manager)          delete addons_manager;
    if(network_manager)         delete network_manager;
    if(grand_prix_manager)      delete grand_prix_manager;
    if(highscore_manager)       delete highscore_manager;
    if(attachment_manager)      delete attachment_manager;
    ItemManager::removeTextures();
    if(powerup_manager)         delete powerup_manager;
    if(projectile_manager)      delete projectile_manager;
    if(kart_properties_manager) delete kart_properties_manager;
    if(track_manager)           delete track_manager;
    if(material_manager)        delete material_manager;
    if(history)                 delete history;
    ReplayRecorder::destroy();
    if(sfx_manager)             delete sfx_manager;
    if(music_manager)           delete music_manager;
    delete ParticleKindManager::get();
    if(stk_config)              delete stk_config;

#ifndef WIN32
    if (user_config) //close logfiles
    {
        Log::closeOutputFiles();
#endif
        fclose(stderr);
        fclose(stdout);
#ifndef WIN32
    }
#endif


    if(user_config)             delete user_config;
    if(unlock_manager)          delete unlock_manager;
    if(translations)            delete translations;
    if(file_manager)            delete file_manager;
    if(irr_driver)              delete irr_driver;

    StateManager::deallocate();
    GUIEngine::EventHandler::deallocate();
}   // cleanSuperTuxKart

//=============================================================================
#ifdef BREAKPAD
bool ShowDumpResults(const wchar_t* dump_path,
                     const wchar_t* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded)
{
    wprintf(L"Path: %s id %s.\n", dump_path, minidump_id);
    return succeeded;
}
#endif

static bool checkXmasTime()
{
    time_t      rawtime;
    struct tm*  timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    return (timeinfo->tm_mon == 12-1);  // Xmas mode happens in December
}

#if defined(DEBUG) && defined(WIN32) && !defined(__CYGWIN__)
#pragma comment(linker, "/SUBSYSTEM:console")
#endif

int main(int argc, char *argv[] )
{
#ifdef BREAKPAD
    google_breakpad::ExceptionHandler eh(L"C:\\Temp", NULL, ShowDumpResults,
                                         NULL, google_breakpad::ExceptionHandler::HANDLER_ALL);
#endif
    srand(( unsigned ) time( 0 ));

    try {
        // Init the minimum managers so that user config exists, then
        // handle all command line options that do not need (or must
        // not have) other managers initialised:
        initUserConfig(argv); // argv passed so config file can be
                              // found more reliably

        UserConfigParams::m_xmas_enabled = checkXmasTime();

        handleCmdLinePreliminary(argc, argv);

        initRest();

        // Windows 32 always redirects output
#ifndef WIN32
        file_manager->redirectOutput();
#endif

        input_manager = new InputManager ();

#ifdef ENABLE_WIIUSE
        wiimote_manager = new WiimoteManager();
#endif

        // Get into menu mode initially.
        input_manager->setMode(InputManager::MENU);
        main_loop = new MainLoop();
        material_manager        -> loadMaterial    ();
        GUIEngine::addLoadingIcon( irr_driver->getTexture(
                           file_manager->getGUIDir() + "options_video.png") );
        kart_properties_manager -> loadAllKarts    ();
        unlock_manager          = new UnlockManager();
        //m_tutorial_manager      = new TutorialManager();
        GUIEngine::addLoadingIcon( irr_driver->getTexture(
                               file_manager->getTextureFile("gui_lock.png")) );
        projectile_manager      -> loadData        ();

        // Both item_manager and powerup_manager load models and therefore
        // textures from the model directory. To avoid reading the
        // materials.xml twice, we do this here once for both:
        file_manager->pushTextureSearchPath(file_manager->getModelFile(""));
        const std::string materials_file =
            file_manager->getModelFile("materials.xml");
        if(materials_file!="")
        {
            // Some of the materials might be needed later, so just add
            // them all permanently (i.e. as shared). Adding them temporary
            // will actually not be possible: powerup_manager adds some
            // permanent icon materials, which would (with the current
            // implementation) make the temporary materials permanent anyway.
            material_manager->addSharedMaterial(materials_file);
        }
        Referee::init();
        powerup_manager         -> loadAllPowerups ();
        ItemManager::loadDefaultItemMeshes();

        GUIEngine::addLoadingIcon( irr_driver->getTexture(
                                    file_manager->getGUIDir() + "gift.png") );

        file_manager->popTextureSearchPath();

        attachment_manager      -> loadModels      ();

        GUIEngine::addLoadingIcon( irr_driver->getTexture(
            file_manager->getGUIDir() + "banana.png") );

        //handleCmdLine() needs InitTuxkart() so it can't be called first
        if(!handleCmdLine(argc, argv)) exit(0);

        addons_manager->checkInstalledAddons();

        // Load addons.xml to get info about addons even when not
        // allowed to access the internet
        if (UserConfigParams::m_internet_status != INetworkHttp::IPERM_ALLOWED) {
            std::string xml_file = file_manager->getAddonsFile("addons.xml");
            if (file_manager->fileExists(xml_file)) {
                const XMLNode *xml = new XMLNode (xml_file);
                addons_manager->initOnline(xml);
            }
        }

        if(!UserConfigParams::m_no_start_screen)
        {
            StateManager::get()->pushScreen(StoryModeLobbyScreen::getInstance());
#ifdef ENABLE_WIIUSE
            // Show a dialog to allow connection of wiimotes. */
            if(WiimoteManager::isEnabled())
            {
                wiimote_manager->askUserToConnectWiimotes();
            }
#endif
            if(UserConfigParams::m_internet_status ==
                INetworkHttp::IPERM_NOT_ASKED)
            {
                class ConfirmServer :
                      public MessageDialog::IConfirmDialogListener
                {
                public:
                    virtual void onConfirm()
                    {
                        INetworkHttp::destroy();
                        UserConfigParams::m_internet_status =
                            INetworkHttp::IPERM_ALLOWED;
                        GUIEngine::ModalDialog::dismiss();
                        INetworkHttp::create();
                        // Note that the network thread must be started after
                        // the assignment to network_http (since the thread
                        // might use network_http, otherwise a race condition
                        // can be introduced resulting in a crash).
                        INetworkHttp::get()->startNetworkThread();

                    }   // onConfirm
                    // --------------------------------------------------------
                    virtual void onCancel()
                    {
                        INetworkHttp::destroy();
                        UserConfigParams::m_internet_status =
                            INetworkHttp::IPERM_NOT_ALLOWED;
                        GUIEngine::ModalDialog::dismiss();
                        INetworkHttp::create();
                        INetworkHttp::get()->startNetworkThread();
                    }   // onCancel
                };   // ConfirmServer

                new MessageDialog(_("SuperTuxKart may connect to a server "
                    "to download add-ons and notify you of updates. Would you "
                    "like this feature to be enabled? (To change this setting "
                    "at a later time, go to options, select tab "
                    "'User Interface', and edit \"Allow STK to connect to the "
                    "Internet\")."),
                    MessageDialog::MESSAGE_DIALOG_CONFIRM,
                    new ConfirmServer(), true);
            }
        }
        else
        {
            InputDevice *device;

            // Use keyboard 0 by default in --no-start-screen
            device = input_manager->getDeviceList()->getKeyboard(0);

            // Create player and associate player with keyboard
            StateManager::get()->createActivePlayer(
                    UserConfigParams::m_all_players.get(0), device );

            if (kart_properties_manager->getKart(UserConfigParams::m_default_kart) == NULL)
            {
                Log::warn("main", "Kart '%s' is unknown so will use the "
                          "default kart.",
                          UserConfigParams::m_default_kart.c_str());
                race_manager->setLocalKartInfo(0, UserConfigParams::m_default_kart.getDefaultValue());
            }
            else
            {
                // Set up race manager appropriately
                race_manager->setLocalKartInfo(0, UserConfigParams::m_default_kart);
            }

            // ASSIGN should make sure that only input from assigned devices
            // is read.
            input_manager->getDeviceList()->setAssignMode(ASSIGN);

            // Go straight to the race
            StateManager::get()->enterGameState();
        }


        // If an important news message exists it is shown in a popup dialog.
        const core::stringw important_message =
                                           news_manager->getImportantMessage();
        if(important_message!="")
        {
            new MessageDialog(important_message,
                              MessageDialog::MESSAGE_DIALOG_OK,
                              NULL, true);
        }   // if important_message


        // Replay a race
        // =============
        if(history->replayHistory())
        {
            // This will setup the race manager etc.
            history->Load();
            network_manager->setupPlayerKartInfo();
            race_manager->startNew(false);
            main_loop->run();
            // well, actually run() will never return, since
            // it exits after replaying history (see history::GetNextDT()).
            // So the next line is just to make this obvious here!
            exit(-3);
        }

        // Initialise connection in case that a command line option was set
        // configuring a client or server. Otherwise this function does nothing
        // here (and will be called again from the network gui).
        if(!network_manager->initialiseConnections())
        {
            Log::error("main", "Problems initialising network connections,\n"
                            "Running in non-network mode.");
        }
        // On the server start with the network information page for now
        if(network_manager->getMode()==NetworkManager::NW_SERVER)
        {
            // TODO - network menu
            //menu_manager->pushMenu(MENUID_NETWORK_GUI);
        }
        // Not replaying
        // =============
        if(!ProfileWorld::isProfileMode())
        {
            if(UserConfigParams::m_no_start_screen)
            {
                // Quickstart (-N)
                // ===============
                // all defaults are set in InitTuxkart()
                network_manager->setupPlayerKartInfo();
                race_manager->startNew(false);
            }
        }
        else  // profile
        {
            // Profiling
            // =========
            race_manager->setMajorMode (RaceManager::MAJOR_MODE_SINGLE);
            race_manager->setDifficulty(RaceManager::DIFFICULTY_HARD);
            network_manager->setupPlayerKartInfo();
            race_manager->startNew(false);
        }
        main_loop->run();

    }  // try
    catch (std::exception &e)
    {
        Log::error("main", "Exception caught : %s.",e.what());
        Log::error("main", "Aborting SuperTuxKart.");
    }

    /* Program closing...*/

    if(user_config)
    {
        // In case that abort is triggered before user_config exists
        if (UserConfigParams::m_crashed) UserConfigParams::m_crashed = false;
        user_config->saveConfig();
    }

#ifdef ENABLE_WIIUSE
    if(wiimote_manager)
        delete wiimote_manager;
#endif

    // If the window was closed in the middle of a race, remove players,
    // so we don't crash later when StateManager tries to access input devices.
    StateManager::get()->resetActivePlayers();
    if(input_manager) delete input_manager; // if early crash avoid delete NULL

    cleanSuperTuxKart();

#ifdef DEBUG
    MemoryLeaks::checkForLeaks();
#endif

    return 0 ;
}   // main

#ifdef WIN32
//routine for running under windows
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
