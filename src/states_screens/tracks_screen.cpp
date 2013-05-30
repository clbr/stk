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
#include "race/grand_prix_data.hpp"
#include "race/grand_prix_manager.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/tracks_screen.hpp"
#include "states_screens/dialogs/gp_info_dialog.hpp"
#include "states_screens/dialogs/track_info_dialog.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/translation.hpp"

#include <iostream>

using namespace GUIEngine;
using namespace irr::core;
using namespace irr::video;

static const char ALL_TRACK_GROUPS_ID[] = "all";

DEFINE_SCREEN_SINGLETON( TracksScreen );

// -----------------------------------------------------------------------------

TracksScreen::TracksScreen() : Screen("tracks.stkgui")
{
}

// -----------------------------------------------------------------------------

void TracksScreen::loadedFromFile()
{
}

// -----------------------------------------------------------------------------

void TracksScreen::eventCallback(Widget* widget, const std::string& name, const int playerID)
{
    // -- track selection screen
    if (name == "tracks")
    {
        DynamicRibbonWidget* w2 = dynamic_cast<DynamicRibbonWidget*>(widget);
        if (w2 != NULL)
        {
            const std::string selection = w2->getSelectionIDString(PLAYER_ID_GAME_MASTER);
            if(UserConfigParams::logGUI())
                std::cout << "Clicked on track " << selection.c_str()
                          << std::endl;

            UserConfigParams::m_last_track = selection;

            if (selection == "random_track")
            {
                RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
                assert( tabs != NULL );

                if (m_random_track_list.empty()) return;

                std::string track = m_random_track_list.front();
                m_random_track_list.pop_front();
                m_random_track_list.push_back(track);
                Track* clickedTrack = track_manager->getTrack( track );


                if (clickedTrack != NULL)
                {
                    ITexture* screenshot = irr_driver->getTexture( clickedTrack->getScreenshotFile().c_str() );

                    new TrackInfoDialog(selection, clickedTrack->getIdent(),
                                        translations->fribidize(clickedTrack->getName()),
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

                    new TrackInfoDialog(selection, clickedTrack->getIdent(),
                                        translations->fribidize(clickedTrack->getName()),
                                        screenshot, 0.8f, 0.7f);
                }
            }
        }
    }
    else if (name == "gps")
    {
        DynamicRibbonWidget* gps_widget = dynamic_cast<DynamicRibbonWidget*>(widget);
        if (gps_widget != NULL)
        {
            std::string selection = gps_widget->getSelectionIDString(PLAYER_ID_GAME_MASTER);

            if (selection == "locked")
            {
                unlock_manager->playLockSound();
            }
            else
            {
                new GPInfoDialog( selection, 0.8f, 0.7f );
            }
        }
        else
        {
            assert(false);
        }
    }
    else if (name == "trackgroups")
    {
        RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
        assert( tabs != NULL );
        UserConfigParams::m_last_used_track_group = tabs->getSelectionIDString(0);
        buildTrackList();
    }
    else if (name == "back")
    {
        StateManager::get()->escapePressed();
    }
}

// -----------------------------------------------------------------------------

void TracksScreen::beforeAddingWidget()
{
    Screen::init();
    // Dynamically add tabs
    RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
    assert( tabs != NULL );

    tabs->clearAllChildren();

    const std::vector<std::string>& groups = track_manager->getAllTrackGroups();
    const int group_amount = groups.size();

    if (group_amount > 1)
    {
        //I18N: name of the tab that will show tracks from all groups
        tabs->addTextChild( _("All"), ALL_TRACK_GROUPS_ID );
    }

    // Make group names being picked up by gettext
#define FOR_GETTEXT_ONLY(x)
    //I18N: track group name
    FOR_GETTEXT_ONLY( _("standard") )
    //I18N: track group name
    FOR_GETTEXT_ONLY( _("Add-Ons") )

    // add others after
    for (int n=0; n<group_amount; n++)
    {
        // try to translate the group name
        tabs->addTextChild( _(groups[n].c_str()), groups[n] );
    }

    DynamicRibbonWidget* tracks_widget = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( tracks_widget != NULL );
    tracks_widget->setItemCountHint( track_manager->getNumberOfTracks() );
}

// -----------------------------------------------------------------------------

void TracksScreen::init()
{
    DynamicRibbonWidget* gps_widget = this->getWidget<DynamicRibbonWidget>("gps");
    assert( gps_widget != NULL );

    DynamicRibbonWidget* tracks_widget = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( tracks_widget != NULL );

    // Reset GP list everytime (accounts for locking changes, etc.)
    gps_widget->clearItems();

    // Build GP list
    const int gpAmount = grand_prix_manager->getNumberOfGrandPrix();
    for (int n=0; n<gpAmount; n++)
    {
        const GrandPrixData* gp = grand_prix_manager->getGrandPrix(n);

        const std::vector<std::string> &tracks = gp->getTrackNames();

        std::vector<std::string> sshot_files;
        for (unsigned int t=0; t<tracks.size(); t++)
        {
            Track* curr = track_manager->getTrack(tracks[t]);
            if (curr == NULL)
            {
                std::cerr << "/!\\ WARNING: Grand Prix '" << gp->getId() << "' refers to track '"
                          << tracks[t] << "', which does not exist.\n";
            }
            else
            {
                sshot_files.push_back(curr->getScreenshotFile());
            }
        }
        if (sshot_files.size() == 0)
        {
            std::cerr << "/!\\ WARNING: Grand Prix '" << gp->getId()
                      << "' does not contain any valid track.\n";
            sshot_files.push_back("gui/main_help.png");
        }

        if (unlock_manager->getCurrentSlot()->isLocked(gp->getId()))
        {
            gps_widget->addAnimatedItem(_("Locked!"),
                                        "locked", sshot_files, 1.5f, LOCKED_BADGE | TROPHY_BADGE,
                                        IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
        }
        else
        {
            gps_widget->addAnimatedItem(translations->fribidize(gp->getName()), gp->getId(),
                                        sshot_files, 1.5f, TROPHY_BADGE,
                                        IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE );
        }
    }
    gps_widget->updateItemDisplay();


    RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
    assert( tabs != NULL );
    tabs->select(UserConfigParams::m_last_used_track_group, PLAYER_ID_GAME_MASTER);


    buildTrackList();

    // select something for the game master
    // FIXME: 'setSelection' will not scroll up to the passed track, so if given track
    //         is not visible with current scrolling this fails
    if (!tracks_widget->setSelection(UserConfigParams::m_last_track,
                                     PLAYER_ID_GAME_MASTER, true))
    {
        tracks_widget->setSelection(0, PLAYER_ID_GAME_MASTER, true);
    }
}

// -----------------------------------------------------------------------------

void TracksScreen::buildTrackList()
{
    DynamicRibbonWidget* tracks_widget = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( tracks_widget != NULL );

    RibbonWidget* tabs = this->getWidget<RibbonWidget>("trackgroups");
    assert( tabs != NULL );

    // Reset track list everytime (accounts for locking changes, etc.)
    tracks_widget->clearItems();
    m_random_track_list.clear();

    const std::string curr_group_name = tabs->getSelectionIDString(0);

    // Build track list
    if (curr_group_name == ALL_TRACK_GROUPS_ID)
    {
        const int trackAmount = track_manager->getNumberOfTracks();

        for (int n=0; n<trackAmount; n++)
        {
            Track* curr = track_manager->getTrack( n );
            if(race_manager->getMinorMode()==RaceManager::MINOR_MODE_EASTER_EGG
                && !curr->hasEasterEggs())
                continue;
            if (curr->isArena() || curr->isSoccer()) continue;
            if (curr->isInternal()) continue;

            if (unlock_manager->getCurrentSlot()->isLocked(curr->getIdent()))
            {
                tracks_widget->addItem( _("Locked : solve active challenges to gain access to more!"),
                                       "locked", curr->getScreenshotFile(), LOCKED_BADGE,
                                       IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
            }
            else
            {
                tracks_widget->addItem(translations->fribidize(curr->getName()), curr->getIdent(),
                                       curr->getScreenshotFile(), 0,
                                       IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE );
                m_random_track_list.push_back(curr->getIdent());
            }
        }

    }
    else
    {
        const std::vector<int>& curr_group = track_manager->getTracksInGroup( curr_group_name );
        const int trackAmount = curr_group.size();

        for (int n=0; n<trackAmount; n++)
        {
            Track* curr = track_manager->getTrack( curr_group[n] );
            if(race_manager->getMinorMode()==RaceManager::MINOR_MODE_EASTER_EGG
                && !curr->hasEasterEggs())
                continue;
            if (curr->isArena()) continue;
            if (curr->isSoccer()) continue;
            if (curr->isInternal()) continue;

            if (unlock_manager->getCurrentSlot()->isLocked(curr->getIdent()))
            {
                tracks_widget->addItem( _("Locked : solve active challenges to gain access to more!"),
                                       "locked", curr->getScreenshotFile(), LOCKED_BADGE,
                                       IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
            }
            else
            {
                tracks_widget->addItem(translations->fribidize(curr->getName()), curr->getIdent(),
                                       curr->getScreenshotFile(), 0 /* no badge */,
                                       IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE );
                m_random_track_list.push_back(curr->getIdent());
            }
        }
    }

    tracks_widget->addItem(_("Random Track"), "random_track", "/gui/track_random.png",
                           0 /* no badge */, IconButtonWidget::ICON_PATH_TYPE_RELATIVE);

    tracks_widget->updateItemDisplay();
    std::random_shuffle( m_random_track_list.begin(), m_random_track_list.end() );
}

// -----------------------------------------------------------------------------

void TracksScreen::setFocusOnTrack(const std::string& trackName)
{
    DynamicRibbonWidget* tracks_widget = this->getWidget<DynamicRibbonWidget>("tracks");
    assert( tracks_widget != NULL );

    // only the game master can select tracks, so it's safe to use 'PLAYER_ID_GAME_MASTER'
    tracks_widget->setSelection(trackName, PLAYER_ID_GAME_MASTER, true);
}

// -----------------------------------------------------------------------------

void TracksScreen::setFocusOnGP(const std::string& gpName)
{
    DynamicRibbonWidget* gps_widget = this->getWidget<DynamicRibbonWidget>("gps");
    assert( gps_widget != NULL );

    // only the game master can select tracks/GPs, so it's safe to use 'PLAYER_ID_GAME_MASTER'
    gps_widget->setSelection(gpName, PLAYER_ID_GAME_MASTER, true);
}

// -----------------------------------------------------------------------------

