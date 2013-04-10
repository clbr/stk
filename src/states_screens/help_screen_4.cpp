//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009 Marianne Gagnon
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

#include "states_screens/help_screen_4.hpp"

#include "guiengine/widget.hpp"
#include "guiengine/widgets/ribbon_widget.hpp"
#include "modes/world.hpp"
#include "states_screens/help_screen_1.hpp"
#include "states_screens/help_screen_2.hpp"
#include "states_screens/help_screen_3.hpp"
#include "states_screens/state_manager.hpp"

using namespace GUIEngine;

DEFINE_SCREEN_SINGLETON( HelpScreen4 );

// -----------------------------------------------------------------------------

HelpScreen4::HelpScreen4() : Screen("help4.stkgui")
{
}   // HelpScreen4

// -----------------------------------------------------------------------------

void HelpScreen4::loadedFromFile()
{
}   // loadedFromFile

// -----------------------------------------------------------------------------

void HelpScreen4::eventCallback(Widget* widget, const std::string& name, const int playerID)
{
    if (name == "category")
    {
        std::string selection = ((RibbonWidget*)widget)->getSelectionIDString(PLAYER_ID_GAME_MASTER).c_str();
        
        if (selection == "page1") StateManager::get()->replaceTopMostScreen(HelpScreen1::getInstance());
        else if (selection == "page2") StateManager::get()->replaceTopMostScreen(HelpScreen2::getInstance());
        else if(selection == "page3") StateManager::get()->replaceTopMostScreen(HelpScreen3::getInstance());
    }
    else if (name == "back")
    {
        StateManager::get()->escapePressed();
    }
}   // eventCallback

// -----------------------------------------------------------------------------

void HelpScreen4::init()
{
    Screen::init();
    RibbonWidget* w = this->getWidget<RibbonWidget>("category");
    
    if (w != NULL) w->select( "page4", PLAYER_ID_GAME_MASTER );
}   // init

// -----------------------------------------------------------------------------
