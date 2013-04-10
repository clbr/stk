//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010 SuperTuxKart-Team
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

#ifndef HEADER_GRAND_PRIX_WIN_HPP
#define HEADER_GRAND_PRIX_WIN_HPP

#include "guiengine/screen.hpp"
#include "karts/kart_model.hpp"

namespace irr { namespace scene { class ISceneNode; class ICameraSceneNode; class ILightSceneNode; class IMeshSceneNode; } }
namespace GUIEngine { class LabelWidget; }
class KartProperties;

/**
  * \brief Screen shown at the end of a Grand Prix
  * \ingroup states_screens
  */
class GrandPrixWin : public GUIEngine::Screen, public GUIEngine::ScreenSingleton<GrandPrixWin>
{
    friend class GUIEngine::ScreenSingleton<GrandPrixWin>;
    
    GrandPrixWin();
    
    /** sky angle, 0-360 */
    float m_sky_angle;
    
    /** Global evolution of time */
    double m_global_time;
    
    irr::scene::IMeshSceneNode* m_village;

    irr::scene::IMeshSceneNode* m_podium_step[3];
    irr::scene::ISceneNode* m_kart_node[3];

    /** A copy of the kart model for each kart used. */
    std::vector<KartModel*> m_all_kart_models;
    
    irr::scene::ISceneNode* m_sky;
    irr::scene::ICameraSceneNode* m_camera;

    irr::scene::ILightSceneNode* m_light;
    
    GUIEngine::LabelWidget* m_unlocked_label;
    
    int m_phase;
    
    float m_kart_x[3], m_kart_y[3], m_kart_z[3];
    float m_podium_x[3], m_podium_z[3];
    float m_kart_rotation[3];
    
    float m_camera_x, m_camera_y, m_camera_z;
    float m_camera_target_x, m_camera_target_z;
    
    MusicInformation* m_music;
    
public:

    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void loadedFromFile() OVERRIDE;
    
    /** \brief implement optional callback from parent class GUIEngine::Screen */
    void onUpdate(float dt, irr::video::IVideoDriver*) OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    void init() OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    void tearDown() OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    void eventCallback(GUIEngine::Widget* widget, const std::string& name,
                       const int playerID) OVERRIDE;
    
    /** \pre must be called after pushing the screen, but before onUpdate had the chance to be invoked */
    void setKarts(const std::string idents[3]);

    virtual MusicInformation* getMusic() const OVERRIDE { return m_music; }
};

#endif

