//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013 Lionel Fuentes
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

#ifndef HEADER_SOCCER_SETUP_SCREEN_HPP
#define HEADER_SOCCER_SETUP_SCREEN_HPP

#include "guiengine/screen.hpp"
#include "network/remote_kart_info.hpp"

namespace GUIEngine { class Widget; class LabelWidget; class ModelViewWidget; }

/**
  * \brief Screen with soccer setup options
  * \ingroup states_screens
  */
class SoccerSetupScreen : public GUIEngine::Screen, public GUIEngine::ScreenSingleton<SoccerSetupScreen>
{
    friend class GUIEngine::ScreenSingleton<SoccerSetupScreen>;

    SoccerSetupScreen();
    
    struct KartViewInfo
    {
        GUIEngine::ModelViewWidget* view;
        bool                        confirmed;
        int                         local_player_id;
        SoccerTeam                  team;
        
        KartViewInfo() : view(NULL), confirmed(false), local_player_id(-1), team(SOCCER_TEAM_NONE) {}
    };

    AlignedArray<KartViewInfo>  m_kart_view_info;
    
public:
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void loadedFromFile() OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void eventCallback(GUIEngine::Widget* widget, const std::string& name,
                               const int playerID) OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void beforeAddingWidget() OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void init() OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual void tearDown() OVERRIDE;
    
    /** \brief implement callback from parent class GUIEngine::Screen */
    virtual GUIEngine::EventPropagation filterActions( PlayerAction action,
                                                       int deviceID,
                                                       const unsigned int value,
                                                       Input::InputType type,
                                                       int playerId) OVERRIDE;
    
private:
    bool areAllKartsConfirmed() const;
    void updateKartViewsLayout();
};

#endif // HEADER_SOCCER_SETUP_SCREEN_HPP
