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

#include "challenges/unlock_manager.hpp"
#include "graphics/irr_driver.hpp"
#include "guiengine/widget.hpp"
#include "guiengine/widgets/dynamic_ribbon_widget.hpp"
#include "guiengine/widgets/icon_button_widget.hpp"
#include "io/file_manager.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/arenas_screen.hpp"
#include "states_screens/dialogs/track_info_dialog.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/random_generator.hpp"
#include "utils/translation.hpp"

#include <iostream>

using namespace GUIEngine;
using namespace irr::core;
using namespace irr::video;

DEFINE_SCREEN_SINGLETON( ArenasScreen );

const char* ALL_ARENA_GROUPS_ID = "all";


// -----------------------------------------------------------------------------

ArenasScreen::ArenasScreen() : Screen("arenas.stkgui")
{
}

// -----------------------------------------------------------------------------

void ArenasScreen::loadedFromFile()
{
}

// -----------------------------------------------------------------------------

void ArenasScreen::beforeAddingWidget()
{
    
    // Dynamically add tabs
    RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
    assert( tabs != NULL );
    
    tabs->clearAllChildren();
    
    bool soccer_mode = race_manager->getMinorMode() == RaceManager::MINOR_MODE_SOCCER;
    const std::vector<std::string>& groups = track_manager->getAllArenaGroups(soccer_mode);
    const int group_amount = groups.size();
    
    if (group_amount > 1)
    {
        //I18N: name of the tab that will show arenas from all groups
        tabs->addTextChild( _("All"), ALL_ARENA_GROUPS_ID );
    }
    
    // Make group names being picked up by gettext
#define FOR_GETTEXT_ONLY(x)
    //I18N: arena group name
    FOR_GETTEXT_ONLY( _("standard") )
    //I18N: arena group name
    FOR_GETTEXT_ONLY( _("Add-Ons") )
    
    // add others after
    for (int n=0; n<group_amount; n++)
    {
        // try to translate the group name
        tabs->addTextChild( _(groups[n].c_str()), groups[n] );
    }
    
    int num_of_arenas=0;
    for (unsigned int n=0; n<track_manager->getNumberOfTracks(); n++) //iterate through tracks to find how many are arenas
    {
        Track* temp = track_manager->getTrack(n);
        if (soccer_mode)
        {
            if(temp->isSoccer())
                num_of_arenas++;
        }
        else
        {
            if(temp->isArena())
                num_of_arenas++;
        }
    }
    
    DynamicRibbonWidget* tracks_widget = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( tracks_widget != NULL );
    tracks_widget->setItemCountHint(num_of_arenas); //set the item hint to that number to prevent weird formatting
}

// -----------------------------------------------------------------------------

void ArenasScreen::init()
{
    Screen::init();
    buildTrackList();
    DynamicRibbonWidget* w = this->getWidget<DynamicRibbonWidget>("tracks");
    // select something by default for the game master
    assert( w != NULL );
    w->setSelection(w->getItems()[0].m_code_name, PLAYER_ID_GAME_MASTER, true); 
}   // init

// -----------------------------------------------------------------------------

void ArenasScreen::eventCallback(Widget* widget, const std::string& name, const int playerID)
{
    if (name == "tracks")
    { 
        DynamicRibbonWidget* w2 = dynamic_cast<DynamicRibbonWidget*>(widget);
        if (w2 == NULL) return;

        const std::string selection = w2->getSelectionIDString(PLAYER_ID_GAME_MASTER);
        if (UserConfigParams::logGUI())
            std::cout << "Clicked on arena " << selection.c_str() << std::endl;

        
        if (selection == "random_track")
        {
            RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
            assert( tabs != NULL );
            
            bool soccer_mode = race_manager->getMinorMode() == RaceManager::MINOR_MODE_SOCCER;

            std::vector<int> curr_group;
            if (tabs->getSelectionIDString(PLAYER_ID_GAME_MASTER) == ALL_ARENA_GROUPS_ID)
            {
                const std::vector<std::string>& groups = track_manager->getAllArenaGroups();
                for (unsigned int i = 0; i < groups.size(); i++)
                {
                    const std::vector<int>& tmp_group = track_manager->getArenasInGroup(groups[i], soccer_mode);
                    // Append to our main vector
                    curr_group.insert(curr_group.end(), tmp_group.begin(), tmp_group.end());
                }
            } // if on tab "all"
            else
            {
                curr_group = track_manager->getArenasInGroup(
                        tabs->getSelectionIDString(PLAYER_ID_GAME_MASTER), soccer_mode );
            }
            
            RandomGenerator random;
            const int randomID = random.get(curr_group.size());
            
            Track* clickedTrack = track_manager->getTrack( curr_group[randomID] );
            if (clickedTrack != NULL)
            {
                ITexture* screenshot = irr_driver->getTexture( clickedTrack->getScreenshotFile().c_str() );
                
                new TrackInfoDialog(selection, clickedTrack->getIdent(), clickedTrack->getName(),
                                    screenshot, 0.8f, 0.7f);
            }
            
        }    
        else if (selection == "locked")
        {
            unlock_manager->playLockSound();
        }
        else if (selection == RibbonWidget::NO_ITEM_ID)
        {
        }
        else
        {
            Track* clickedTrack = track_manager->getTrack(selection);
            if (clickedTrack != NULL)
            {
                ITexture* screenshot = irr_driver->getTexture( clickedTrack->getScreenshotFile().c_str() );

                new TrackInfoDialog(selection, clickedTrack->getIdent(), clickedTrack->getName(),
                                    screenshot, 0.8f, 0.7f);
            }   // clickedTrack !=  NULL
        }   // if random_track
        
    }
    else if (name == "trackgroups")
    {
        buildTrackList();
    }
    else if (name == "back")
    {
        StateManager::get()->escapePressed();
    }
    
}   // eventCallback

// ------------------------------------------------------------------------------------------------------

void ArenasScreen::buildTrackList()
{
    DynamicRibbonWidget* w = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( w != NULL );
    
    // Re-build track list everytime (accounts for locking changes, etc.)
    w->clearItems();
    
    RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
    assert( tabs != NULL );
    const std::string curr_group_name = tabs->getSelectionIDString(0);
    
    bool soccer_mode = race_manager->getMinorMode() == RaceManager::MINOR_MODE_SOCCER;
    
    if (curr_group_name == ALL_ARENA_GROUPS_ID)
    {
        const int trackAmount = track_manager->getNumberOfTracks();
        
        for (int n=0; n<trackAmount; n++)
        {
            Track* curr = track_manager->getTrack(n);
            if (soccer_mode)
            {
                if(!curr->isSoccer()) continue;
            }
            else
            {
                if(!curr->isArena()) continue;
            }
            
            if (unlock_manager->getCurrentSlot()->isLocked(curr->getIdent()))
            {
                w->addItem( _("Locked : solve active challenges to gain access to more!"),
                           "locked", curr->getScreenshotFile(), LOCKED_BADGE );
            }
            else
            {
                w->addItem( curr->getName(), curr->getIdent(), curr->getScreenshotFile(), 0,
                           IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE );
            }
        }
        
    }
    else
    {
        const std::vector<int>& currArenas = track_manager->getArenasInGroup(curr_group_name, soccer_mode);
        const int trackAmount = currArenas.size();

        for (int n=0; n<trackAmount; n++)
        {
            Track* curr = track_manager->getTrack(currArenas[n]);
            if (soccer_mode)
            {
                if(!curr->isSoccer()) continue;
            }
            else
            {
                if(!curr->isArena()) continue;
            }
            
            if (unlock_manager->getCurrentSlot()->isLocked(curr->getIdent()))
            {
                w->addItem( _("Locked : solve active challenges to gain access to more!"),
                           "locked", curr->getScreenshotFile(), LOCKED_BADGE );
            }
            else
            {
                w->addItem( curr->getName(), curr->getIdent(), curr->getScreenshotFile(), 0,
                           IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE );
            }
        }
    }
    w->addItem(_("Random Arena"), "random_track", "/gui/track_random.png");
    w->updateItemDisplay();  
    
    assert(w->getItems().size() > 0);
}

// ------------------------------------------------------------------------------------------------------

void ArenasScreen::setFocusOnTrack(const std::string& trackName)
{
    DynamicRibbonWidget* w = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( w != NULL );
    
    w->setSelection(trackName, PLAYER_ID_GAME_MASTER, true); 
}   // setFOxuOnTrack

// ------------------------------------------------------------------------------------------------------

