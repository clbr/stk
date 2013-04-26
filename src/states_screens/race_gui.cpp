//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2005 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006 Joerg Henrichs, SuperTuxKart-Team, Steve Baker
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

#include "states_screens/race_gui.hpp"

using namespace irr;

#include <algorithm>

#include "challenges/unlock_manager.hpp"
#include "config/user_config.hpp"
#include "graphics/camera.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/material_manager.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/modaldialog.hpp"
#include "guiengine/scalable_font.hpp"
#include "io/file_manager.hpp"
#include "input/input.hpp"
#include "input/input_manager.hpp"
#include "items/attachment.hpp"
#include "items/attachment_manager.hpp"
#include "items/powerup_manager.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/controller/controller.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "modes/follow_the_leader.hpp"
#include "modes/linear_world.hpp"
#include "modes/world.hpp"
#include "race/race_manager.hpp"
#include "tracks/track.hpp"
#include "utils/constants.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

/** The constructor is called before anything is attached to the scene node.
 *  So rendering to a texture can be done here. But world is not yet fully
 *  created, so only the race manager can be accessed safely.
 */
RaceGUI::RaceGUI()
{    
    m_enabled = true;
    
    // Originally m_map_height was 100, and we take 480 as minimum res
    const float scaling = irr_driver->getFrameSize().Height / 480.0f;
    // Marker texture has to be power-of-two for (old) OpenGL compliance
    m_marker_rendered_size  =  2 << ((int) ceil(1.0 + log(32.0 * scaling)));
    m_marker_ai_size        = (int)( 14.0f * scaling);
    m_marker_player_size    = (int)( 16.0f * scaling);
    m_map_width             = (int)(100.0f * scaling);
    m_map_height            = (int)(100.0f * scaling);
    m_map_left              = (int)( 10.0f * scaling);
    m_map_bottom            = (int)( 10.0f * scaling);
    
    // Minimap is also rendered bigger via OpenGL, so find power-of-two again
    const int map_texture   = 2 << ((int) ceil(1.0 + log(128.0 * scaling)));
    m_map_rendered_width    = map_texture;
    m_map_rendered_height   = map_texture;


    // special case : when 3 players play, use available 4th space for such things
    if (race_manager->getNumLocalPlayers() == 3)
    {
        m_map_left = UserConfigParams::m_width - m_map_width;
    }

    m_is_tutorial = (race_manager->getTrackName() == "tutorial");

    m_speed_meter_icon = material_manager->getMaterial("speedback.png");
    m_speed_bar_icon   = material_manager->getMaterial("speedfore.png");
    createMarkerTexture();
    
    // Translate strings only one in constructor to avoid calling
    // gettext in each frame.
    //I18N: Shown at the end of a race
    m_string_lap      = _("Lap");
    m_string_rank     = _("Rank");

    
    // Determine maximum length of the rank/lap text, in order to
    // align those texts properly on the right side of the viewport.
    gui::ScalableFont* font = GUIEngine::getFont(); 
    m_rank_lap_width = font->getDimension(m_string_lap.c_str()).Width;
    
    m_timer_width = font->getDimension(L"99:99:99").Width;

    font = (race_manager->getNumLocalPlayers() > 2 ? GUIEngine::getSmallFont()
                                                   : GUIEngine::getFont());
    
    int w;
    if (race_manager->getMinorMode()==RaceManager::MINOR_MODE_FOLLOW_LEADER ||
        race_manager->getMinorMode()==RaceManager::MINOR_MODE_3_STRIKES     ||
        race_manager->getNumLaps() > 9)
        w = font->getDimension(L"99/99").Width;
    else
        w = font->getDimension(L"9/9").Width;
    
    // In some split screen configuration the energy bar might be next 
    // to the lap display - so make the lap X/Y display large enough to
    // leave space for the energy bar (16 pixels) and 10 pixels of space
    // to the right (see drawEnergyMeter for details).
    w += 16 + 10;
    if(m_rank_lap_width < w) m_rank_lap_width = w;
    w = font->getDimension(m_string_rank.c_str()).Width;
    if(m_rank_lap_width < w) m_rank_lap_width = w;
    
}   // RaceGUI

//-----------------------------------------------------------------------------
RaceGUI::~RaceGUI()
{
}   // ~Racegui

//-----------------------------------------------------------------------------
/** Render all global parts of the race gui, i.e. things that are only 
 *  displayed once even in splitscreen.
 *  \param dt Timestep sized.
 */
void RaceGUI::renderGlobal(float dt)
{
    RaceGUIBase::renderGlobal(dt);
    cleanupMessages(dt);
        
    // Special case : when 3 players play, use 4th window to display such 
    // stuff (but we must clear it)
    if (race_manager->getNumLocalPlayers() == 3 && 
        !GUIEngine::ModalDialog::isADialogActive())
    {
        static video::SColor black = video::SColor(255,0,0,0);
        irr_driver->getVideoDriver()
            ->draw2DRectangle(black,
                              core::rect<s32>(UserConfigParams::m_width/2, 
                                              UserConfigParams::m_height/2,
                                              UserConfigParams::m_width, 
                                              UserConfigParams::m_height));
    }
    
    World *world = World::getWorld();
    assert(world != NULL);
    if(world->getPhase() >= WorldStatus::READY_PHASE &&
       world->getPhase() <= WorldStatus::GO_PHASE      )
    {
        drawGlobalReadySetGo();
    }

    // Timer etc. are not displayed unless the game is actually started.
    if(!world->isRacePhase()) return;
    if (!m_enabled) return;


    if (!m_is_tutorial)
    {
        drawGlobalTimer();
        if(world->getPhase() == WorldStatus::GO_PHASE ||
           world->getPhase() == WorldStatus::MUSIC_PHASE)
        {
            drawGlobalMusicDescription();
        }
    }
    
    drawGlobalMiniMap();

    if (!m_is_tutorial) drawGlobalPlayerIcons(m_map_height);
}   // renderGlobal

//-----------------------------------------------------------------------------
/** Render the details for a single player, i.e. speed, energy, 
 *  collectibles, ...
 *  \param kart Pointer to the kart for which to render the view.
 */
void RaceGUI::renderPlayerView(const Camera *camera, float dt)
{
    if (!m_enabled) return;
    
    const core::recti &viewport = camera->getViewport();
    
    core::vector2df scaling = camera->getScaling();
    const AbstractKart *kart = camera->getKart();
    if(!kart) return;
    
    drawPlungerInFace(camera, dt);
    
    scaling *= viewport.getWidth()/800.0f; // scale race GUI along screen size
    drawAllMessages     (kart, viewport, scaling);
    
    if(!World::getWorld()->isRacePhase()) return;

    drawPowerupIcons    (kart, viewport, scaling);
    drawSpeedAndEnergy  (kart, viewport, scaling);
    
    if (!m_is_tutorial)
        drawRankLap     (kart, viewport);

    RaceGUIBase::renderPlayerView(camera, dt);
}   // renderPlayerView

//-----------------------------------------------------------------------------
/** Displays the racing time on the screen.s
 */
void RaceGUI::drawGlobalTimer()
{
    assert(World::getWorld() != NULL);
        
    if (!World::getWorld()->shouldDrawTimer())
    {
        return;
    }

    core::stringw sw;
    video::SColor time_color = video::SColor(255, 255, 255, 255);
    int dist_from_right = 10 + m_timer_width;

    float elapsed_time = World::getWorld()->getTime();
    if (!race_manager->hasTimeTarget())
    {
        sw = core::stringw ( 
            StringUtils::timeToString(elapsed_time).c_str() );
    }
    else
    {
        float time_target = race_manager->getTimeTarget();
        if (elapsed_time < time_target)
        {
            sw = core::stringw (
              StringUtils::timeToString(time_target - elapsed_time).c_str());
        }
        else
        {
            sw = _("Challenge Failed");
            int string_width = 
                GUIEngine::getFont()->getDimension(_("Challenge Failed")).Width;
            dist_from_right = 10 + string_width;
            time_color = video::SColor(255,255,0,0);
        }
    }
    
    core::rect<s32> pos(UserConfigParams::m_width - dist_from_right, 10, 
                        UserConfigParams::m_width                  , 50);
    
    // special case : when 3 players play, use available 4th space for such things
    if (race_manager->getNumLocalPlayers() == 3)
    {
        pos += core::vector2d<s32>(0, UserConfigParams::m_height/2);
    }
        
    gui::ScalableFont* font = GUIEngine::getFont();
    font->draw(sw.c_str(), pos, time_color, false, false, NULL, 
               true /* ignore RTL */);
    
}   // drawGlobalTimer

//-----------------------------------------------------------------------------
/** Draws the mini map and the position of all karts on it.
 */
void RaceGUI::drawGlobalMiniMap()
{
    World *world = World::getWorld();
    // arenas currently don't have a map.
    if(world->getTrack()->isArena() || world->getTrack()->isSoccer()) return;

    const video::ITexture *mini_map = world->getTrack()->getMiniMap();
    
    int upper_y = UserConfigParams::m_height - m_map_bottom - m_map_height;
    int lower_y = UserConfigParams::m_height - m_map_bottom;
    
    if (mini_map != NULL)
    {
        core::rect<s32> dest(m_map_left,               upper_y, 
                             m_map_left + m_map_width, lower_y);
        core::rect<s32> source(core::position2di(0, 0), 
                               mini_map->getOriginalSize());
        irr_driver->getVideoDriver()->draw2DImage(mini_map, dest, source,
                                                  NULL, NULL, true);
    }
    
    for(unsigned int i=0; i<world->getNumKarts(); i++)
    {
        const AbstractKart *kart = world->getKart(i);
        if(kart->isEliminated()) continue;   // don't draw eliminated kart
        const Vec3& xyz = kart->getXYZ();
        Vec3 draw_at;
        world->getTrack()->mapPoint2MiniMap(xyz, &draw_at);
        // int marker_height = m_marker->getOriginalSize().Height;
        core::rect<s32> source(i    *m_marker_rendered_size,
                               0, 
                               (i+1)*m_marker_rendered_size, 
                               m_marker_rendered_size);
        int marker_half_size = (kart->getController()->isPlayerController() 
                                ? m_marker_player_size 
                                : m_marker_ai_size                        )>>1;
        core::rect<s32> position(m_map_left+(int)(draw_at.getX()-marker_half_size), 
                                 lower_y   -(int)(draw_at.getY()+marker_half_size),
                                 m_map_left+(int)(draw_at.getX()+marker_half_size), 
                                 lower_y   -(int)(draw_at.getY()-marker_half_size));
        irr_driver->getVideoDriver()->draw2DImage(m_marker, position, source, 
                                                  NULL, NULL, true);
    }   // for i<getNumKarts
}   // drawGlobalMiniMap

//-----------------------------------------------------------------------------
/** Energy meter that gets filled with nitro. This function is called from
 *  drawSpeedAndEnergy, which defines the correct position of the energy
 *  meter.
 *  \param x X position of the meter.
 *  \param y Y position of the meter.
 *  \param kart Kart to display the data for.
 *  \param scaling Scaling applied (in case of split screen)
 */
void RaceGUI::drawEnergyMeter(int x, int y, const AbstractKart *kart,              
                              const core::recti &viewport, 
                              const core::vector2df &scaling)
{
    float minRatio         = std::min(scaling.X, scaling.Y);
    const int GAUGEWIDTH   = 78;
    int gauge_width        = (int)(GAUGEWIDTH*minRatio);
    int gauge_height       = (int)(GAUGEWIDTH*minRatio);
    
    float state = (float)(kart->getEnergy()) 
                / kart->getKartProperties()->getNitroMax();
    if (state < 0.0f) state = 0.0f;
    else if (state > 1.0f) state = 1.0f;

    core::vector2df offset;
    offset.X = (float)(x-gauge_width) - 9.0f*scaling.X;
    offset.Y = (float)y-30.0f*scaling.Y;
    

    // Background
    irr_driver->getVideoDriver()->draw2DImage(m_gauge_empty, 
                                core::rect<s32>((int)offset.X,
                                                (int) offset.Y-gauge_height,
                                                (int) offset.X + gauge_width, 
                                                (int)offset.Y) /* dest rect */,
                             core::rect<s32>(0, 0, 256, 256) /* source rect */,
                             NULL /* clip rect */, NULL /* colors */,
                                              true /* alpha */);
  
    // Target
    
    if (race_manager->getCoinTarget() > 0)
    {
        float coin_target = (float)race_manager->getCoinTarget() 
                          / kart->getKartProperties()->getNitroMax();

        video::S3DVertex vertices[5];
        unsigned int count=2;

        // There are three different polygons used, depending on 
        // the target. Consider the nitro-display texture:
        //
        //   ----E-x--D       (position of v,w,x vary depending on  
        //            |        nitro)
        //       A    w 
        //            |
        //   -B--v----C
        // For nitro state <= r1 the triangle ABv is used, with v between B and C.
        // For nitro state <= r2 the quad ABCw is used, with w between C and D.
        // For nitro state >  r2 the poly ABCDx is used, with x between D and E.

        vertices[0].TCoords = core::vector2df(0.3f, 0.4f);
        vertices[0].Pos     = core::vector3df(offset.X+0.3f*gauge_width, 
                                              offset.Y-(1-0.4f)*gauge_height, 0);
        vertices[1].TCoords = core::vector2df(0, 1.0f);
        vertices[1].Pos     = core::vector3df(offset.X, offset.Y, 0);
        // The targets at which different polygons must be used.

        const float r1 = 0.4f;
        const float r2 = 0.65f;
        if(state<=r1)
        {
            count   = 3;
            float f = coin_target/r1;
            vertices[2].TCoords = core::vector2df(0.08f + (1.0f-0.08f)*f, 1.0f);
            vertices[2].Pos =  core::vector3df(offset.X + (0.08f*gauge_width)
                                                + (1.0f - 0.08f)*f*gauge_width,
                                               offset.Y,0);
                }
        else if(state<=r2)
        {
            count   = 4;
            float f = (coin_target - r1)/(r2-r1);
            vertices[2].TCoords = core::vector2df(1.0f, 1.0f);
            vertices[2].Pos     = core::vector3df(offset.X + gauge_width, 
                                                  offset.Y, 0);
            vertices[3].TCoords = core::vector2df(1.0f, (1.0f-f));
            vertices[3].Pos = core::vector3df(offset.X + gauge_width, 
                                              offset.Y-f*gauge_height,0);
        }
        else
        {
            count   = 5;
            float f = (coin_target - r2)/(1-r2);
            vertices[2].TCoords = core::vector2df(1.0, 1.0f);
            vertices[2].Pos     = core::vector3df(offset.X + gauge_width, 
                                                  offset.Y, 0);
            vertices[3].TCoords = core::vector2df(1.0,0);
            vertices[3].Pos     = core::vector3df(offset.X + gauge_width,
                                                  offset.Y-gauge_height, 0);
            vertices[4].TCoords = core::vector2df(1.0f - f*(1-0.61f), 0);
            vertices[4].Pos     = core::vector3df(offset.X + gauge_width 
                                                   - (1.0f-0.61f)*f*gauge_width,
                                                   offset.Y-gauge_height, 0);
        }
        short int index[5]={0};
        for(unsigned int i=0; i<count; i++)
        {
            index[i]=count-i-1;
            vertices[i].Color = video::SColor(255, 255, 255, 255);
        }
        
        video::SMaterial m;
        m.setTexture(0, m_gauge_goal);
        m.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
        irr_driver->getVideoDriver()->setMaterial(m);
        irr_driver->getVideoDriver()->draw2DVertexPrimitiveList(vertices, count,
        index, count-2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN);
             
    }
    
    

    // Filling (current state)

    if(state <=0) return; //Nothing to do

    if (state > 0.0f) 
    {
        video::S3DVertex vertices[5];
        unsigned int count=2;

        // There are three different polygons used, depending on 
        // the nitro state. Consider the nitro-display texture:
        //
        //   ----E-x--D       (position of v,w,x vary depending on  
        //            |        nitro)
        //       A    w 
        //            |
        //   -B--v----C
        // For nitro state <= r1 the triangle ABv is used, with v between B and C.
        // For nitro state <= r2 the quad ABCw is used, with w between C and D.
        // For nitro state >  r2 the poly ABCDx is used, with x between D and E.

        vertices[0].TCoords = core::vector2df(0.3f, 0.4f);
        vertices[0].Pos     = core::vector3df(offset.X+0.3f*gauge_width, 
                                              offset.Y-(1-0.4f)*gauge_height, 0);
        vertices[1].TCoords = core::vector2df(0, 1.0f);
        vertices[1].Pos     = core::vector3df(offset.X, offset.Y, 0);
        // The states at which different polygons must be used.

        const float r1 = 0.4f;
        const float r2 = 0.65f;
        if(state<=r1)
        {
            count   = 3;
            float f = state/r1;
            vertices[2].TCoords = core::vector2df(0.08f + (1.0f-0.08f)*f, 1.0f);
            vertices[2].Pos = core::vector3df(offset.X + (0.08f*gauge_width) 
                                                       + (1.0f - 0.08f)
                                                         *f*gauge_width, 
                                              offset.Y,0);
                }
        else if(state<=r2)
        {
            count   = 4;
            float f = (state - r1)/(r2-r1);
            vertices[2].TCoords = core::vector2df(1.0f, 1.0f);
            vertices[2].Pos     = core::vector3df(offset.X + gauge_width,
                                                  offset.Y, 0);
            vertices[3].TCoords = core::vector2df(1.0f, (1.0f-f));
            vertices[3].Pos = core::vector3df(offset.X + gauge_width,
                                              offset.Y-f*gauge_height,0);
        }
        else
        {
            count   = 5;
            float f = (state - r2)/(1-r2);
            vertices[2].TCoords = core::vector2df(1.0, 1.0f);
            vertices[2].Pos     = core::vector3df(offset.X + gauge_width, 
                                                  offset.Y, 0);
            vertices[3].TCoords = core::vector2df(1.0,0);
            vertices[3].Pos = core::vector3df(offset.X + gauge_width,
                                              offset.Y-gauge_height, 0);
            vertices[4].TCoords = core::vector2df(1.0f - f*(1-0.61f), 0);
            vertices[4].Pos     = 
                core::vector3df(offset.X + gauge_width - (1.0f-0.61f)*f*gauge_width,
                                offset.Y-gauge_height, 0);
        }
        short int index[5]={0};
        for(unsigned int i=0; i<count; i++)
        {
            index[i]=count-i-1;
            vertices[i].Color = video::SColor(255, 255, 255, 255);
        }
        
                
        video::SMaterial m;
        if(kart->getControls().m_nitro)
            m.setTexture(0, m_gauge_full_bright);
        else
            m.setTexture(0, m_gauge_full);
        m.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
        irr_driver->getVideoDriver()->setMaterial(m);
        irr_driver->getVideoDriver()->draw2DVertexPrimitiveList(vertices, count,
        index, count-2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN);
        

    }
    
    
}   // drawEnergyMeter

//-----------------------------------------------------------------------------
void RaceGUI::drawSpeedAndEnergy(const AbstractKart* kart, 
                                 const core::recti &viewport,
                                 const core::vector2df &scaling)
{

    float minRatio         = std::min(scaling.X, scaling.Y);
    const int SPEEDWIDTH   = 128;
    int meter_width        = (int)(SPEEDWIDTH*minRatio);
    int meter_height       = (int)(SPEEDWIDTH*minRatio);

    drawEnergyMeter(viewport.LowerRightCorner.X , 
                    (int)(viewport.LowerRightCorner.Y), 
                    kart, viewport, scaling);
    
    // First draw the meter (i.e. the background )
    // -------------------------------------------------------------------------
    
    
    core::vector2df offset;
    offset.X = (float)(viewport.LowerRightCorner.X-meter_width) - 24.0f*scaling.X;
    offset.Y = viewport.LowerRightCorner.Y-10.0f*scaling.Y;
    
    video::IVideoDriver *video = irr_driver->getVideoDriver();
    const core::rect<s32> meter_pos((int)offset.X,
                                    (int)(offset.Y-meter_height),
                                    (int)(offset.X+meter_width),
                                    (int)offset.Y);
    video::ITexture *meter_texture = m_speed_meter_icon->getTexture();
    const core::rect<s32> meter_texture_coords(core::position2d<s32>(0,0), 
                                               meter_texture->getOriginalSize());
    video->draw2DImage(meter_texture, meter_pos, meter_texture_coords, NULL, 
                       NULL, true);
    
    const float speed =  kart->getSpeed();
    if(speed <=0) return;  // Nothing to do if speed is negative.
    
    // Draw the actual speed bar (if the speed is >0)
    // ----------------------------------------------
    float speed_ratio = speed/KILOMETERS_PER_HOUR/110.0f;
    if(speed_ratio>1) speed_ratio = 1;

    video::ITexture   *bar_texture = m_speed_bar_icon->getTexture();
    core::dimension2du bar_size    = bar_texture->getOriginalSize();
    video::S3DVertex vertices[5];
    unsigned int count;

    // There are three different polygons used, depending on 
    // the speed ratio. Consider the speed-display texture:
    //
    //   D----x----D       (position of v,w,x vary depending on  
    //   |                 speed)
    //   w    A    
    //   |         
    //   C--v-B----E
    // For speed ratio <= r1 the triangle ABv is used, with v between B and C.
    // For speed ratio <= r2 the quad ABCw is used, with w between C and D.
    // For speed ratio >  r2 the poly ABCDx is used, with x between D and E.

    vertices[0].TCoords = core::vector2df(0.7f, 0.5f);
    vertices[0].Pos     = core::vector3df(offset.X+0.7f*meter_width, 
                                          offset.Y-0.5f*meter_height, 0);
    vertices[1].TCoords = core::vector2df(0.52f, 1.0f);
    vertices[1].Pos     = core::vector3df(offset.X+0.52f*meter_width, offset.Y, 0);
    // The speed ratios at which different triangles must be used.
    // These values should be adjusted in case that the speed display
    // is not linear enough. Mostly the speed values are below 0.7, it
    // needs some zipper to get closer to 1.
    const float r1 = 0.2f;
    const float r2 = 0.6f;
    if(speed_ratio<=r1)
    {
        count   = 3;
        float f = speed_ratio/r1;
        vertices[2].TCoords = core::vector2df(0.52f*(1-f), 1.0f);
        vertices[2].Pos = core::vector3df(offset.X+ (0.52f*(1.0f-f)*meter_width),
                                          offset.Y,0);
    }
    else if(speed_ratio<=r2)
    {
        count   = 4;
        float f = (speed_ratio - r1)/(r2-r1);
        vertices[2].TCoords = core::vector2df(0, 1.0f);
        vertices[2].Pos     = core::vector3df(offset.X, offset.Y, 0);
        vertices[3].TCoords = core::vector2df(0, (1-f));
        vertices[3].Pos = core::vector3df(offset.X, offset.Y-f*meter_height,0);
    }
    else
    {
        count   = 5;
        float f = (speed_ratio - r2)/(1-r2);
        vertices[2].TCoords = core::vector2df(0, 1.0f);
        vertices[2].Pos     = core::vector3df(offset.X, offset.Y, 0);
        vertices[3].TCoords = core::vector2df(0,0);
        vertices[3].Pos = core::vector3df(offset.X, offset.Y-meter_height, 0);
        vertices[4].TCoords = core::vector2df(f, 0);
        vertices[4].Pos     = core::vector3df(offset.X+f*meter_width,
                                              offset.Y-meter_height, 0);
    }
    short int index[5];
    for(unsigned int i=0; i<count; i++)
    {
        index[i]=i;
        vertices[i].Color = video::SColor(255, 255, 255, 255);
    }
    video::SMaterial m;
    m.setTexture(0, m_speed_bar_icon->getTexture());
    m.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
    irr_driver->getVideoDriver()->setMaterial(m);
    irr_driver->getVideoDriver()->draw2DVertexPrimitiveList(vertices, count,
        index, count-2, video::EVT_STANDARD, scene::EPT_TRIANGLE_FAN);


    // Draw Speed in Numbers

    core::recti pos;
    pos.UpperLeftCorner.X=(int)(offset.X + 0.5f*meter_width);
    pos.UpperLeftCorner.Y=(int)(offset.Y - 0.62f*meter_height);
    pos.LowerRightCorner.X=(int)(offset.X + 0.8f*meter_width);
    pos.LowerRightCorner.Y=(int)(offset.X - 0.5f*meter_height);
    
    gui::ScalableFont* font = GUIEngine::getLargeFont();

    static video::SColor color = video::SColor(255, 255, 255, 255);
    char str[256];
    sprintf(str, "%d", (int)(speed*10));
    
    font->draw(core::stringw(str).c_str(), pos, color);

} 

//-----------------------------------------------------------------------------
/** Displays the rank and the lap of the kart.
 *  \param info Info object c
*/
void RaceGUI::drawRankLap(const AbstractKart* kart,
                          const core::recti &viewport)
{
    // Don't display laps or ranks if the kart has already finished the race.
    if (kart->hasFinishedRace()) return;

    core::recti pos;
    pos.UpperLeftCorner.Y   = viewport.UpperLeftCorner.Y;
    // If the time display in the top right is in this viewport,
    // move the lap/rank display down a little bit so that it is
    // displayed under the time.
    if(viewport.UpperLeftCorner.Y==0 && 
        viewport.LowerRightCorner.X==UserConfigParams::m_width &&
        race_manager->getNumPlayers()!=3)
        pos.UpperLeftCorner.Y   += 40;
    pos.LowerRightCorner.Y  = viewport.LowerRightCorner.Y;
    pos.UpperLeftCorner.X   = viewport.LowerRightCorner.X 
                            - m_rank_lap_width - 10;
    pos.LowerRightCorner.X  = viewport.LowerRightCorner.X;

    gui::ScalableFont* font = (race_manager->getNumLocalPlayers() > 2
                            ? GUIEngine::getSmallFont()
                            : GUIEngine::getFont());
    int font_height         = (int)(font->getDimension(L"X").Height);
    static video::SColor color = video::SColor(255, 255, 255, 255);
    WorldWithRank *world    = (WorldWithRank*)(World::getWorld());

    if (world->displayRank())
    {
        const int rank = kart->getPosition();
            
        font->draw(m_string_rank.c_str(), pos, color);
        pos.UpperLeftCorner.Y  += font_height;
        pos.LowerRightCorner.Y += font_height;

        char str[256];
        const unsigned int kart_amount = world->getCurrentNumKarts();
        sprintf(str, "%d/%d", rank, kart_amount);
        font->draw(core::stringw(str).c_str(), pos, color);
        pos.UpperLeftCorner.Y  += font_height;
        pos.LowerRightCorner.Y += font_height;
    }
    
    // Don't display laps in follow the leader mode
    if(world->raceHasLaps())
    {
        const int lap = world->getKartLaps(kart->getWorldKartId());
    
        // don't display 'lap 0/...'
        if(lap>=0)
        {
            font->draw(m_string_lap.c_str(), pos, color);
            char str[256];
            sprintf(str, "%d/%d", lap+1, race_manager->getNumLaps());
            pos.UpperLeftCorner.Y  += font_height;
            pos.LowerRightCorner.Y += font_height;
            font->draw(core::stringw(str).c_str(), pos, color);
            pos.UpperLeftCorner.Y  += font_height;
            pos.LowerRightCorner.Y += font_height;
        }
    }

} // drawRankLap
