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

#include "states_screens/dialogs/player_info_dialog.hpp"

#include <IGUIStaticText.h>
#include <IGUIEnvironment.h>

#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/player.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/scalable_font.hpp"
#include "guiengine/widgets/text_box_widget.hpp"
#include "states_screens/options_screen_players.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

using namespace GUIEngine;
using namespace irr::gui;
using namespace irr::core;

// -----------------------------------------------------------------------------

PlayerInfoDialog::PlayerInfoDialog(PlayerProfile* player, const float w, const float h) : ModalDialog(w, h)
{
    m_player = player;

    showRegularDialog();
}

// -----------------------------------------------------------------------------

PlayerInfoDialog::~PlayerInfoDialog()
{
    if (m_player != NULL)
    {
        OptionsScreenPlayers::getInstance()->selectPlayer( translations->fribidize(m_player->getName()) );
    }
}

// -----------------------------------------------------------------------------
void PlayerInfoDialog::showRegularDialog()
{
    clearWindow();

    const int y1 = m_area.getHeight()/6;
    const int y2 = m_area.getHeight()*2/6;
    const int y3 = m_area.getHeight()*3/6;
    const int y4 = m_area.getHeight()*5/6;

    ScalableFont* font = GUIEngine::getFont();
    const int textHeight = GUIEngine::getFontHeight();
    const int buttonHeight = textHeight + 10;

    {
        textCtrl = new TextBoxWidget();
        textCtrl->m_properties[PROP_ID] = "newname";
        textCtrl->setText(m_player->getName());
        textCtrl->m_x = 50;
        textCtrl->m_y = y1 - textHeight/2;
        textCtrl->m_w = m_area.getWidth()-100;
        textCtrl->m_h = textHeight + 5;
        textCtrl->setParent(m_irrlicht_window);
        m_widgets.push_back(textCtrl);
        textCtrl->add();
    }

    {
        ButtonWidget* widget = new ButtonWidget();
        widget->m_properties[PROP_ID] = "renameplayer";

        //I18N: In the player info dialog
        widget->setText( _("Rename") );

        const int textWidth = font->getDimension( widget->getText().c_str() ).Width + 40;

        widget->m_x = m_area.getWidth()/2 - textWidth/2;
        widget->m_y = y2;
        widget->m_w = textWidth;
        widget->m_h = buttonHeight;
        widget->setParent(m_irrlicht_window);
        m_widgets.push_back(widget);
        widget->add();
    }
    {
        ButtonWidget* widget = new ButtonWidget();
        widget->m_properties[PROP_ID] = "cancel";
        widget->setText( _("Cancel") );

        const int textWidth = 
            font->getDimension(widget->getText().c_str()).Width + 40;

        widget->m_x = m_area.getWidth()/2 - textWidth/2;
        widget->m_y = y3;
        widget->m_w = textWidth;
        widget->m_h = buttonHeight;
        widget->setParent(m_irrlicht_window);
        m_widgets.push_back(widget);
        widget->add();
    }

    {
        ButtonWidget* widget = new ButtonWidget();
        widget->m_properties[PROP_ID] = "removeplayer";

        //I18N: In the player info dialog
        widget->setText( _("Remove"));

        const int textWidth = 
            font->getDimension(widget->getText().c_str()).Width + 40;

        widget->m_x = m_area.getWidth()/2 - textWidth/2;
        widget->m_y = y4;
        widget->m_w = textWidth;
        widget->m_h = buttonHeight;
        widget->setParent(m_irrlicht_window);
        m_widgets.push_back(widget);
        widget->add();
    }

    textCtrl->setFocusForPlayer( PLAYER_ID_GAME_MASTER );
}

// -----------------------------------------------------------------------------

void PlayerInfoDialog::showConfirmDialog()
{
    clearWindow();


    IGUIFont* font = GUIEngine::getFont();
    const int textHeight = GUIEngine::getFontHeight();
    const int buttonHeight = textHeight + 10;

    irr::core::stringw message = 
        //I18N: In the player info dialog (when deleting)
        _("Do you really want to delete player '%s' ?", m_player->getName());


    if (unlock_manager->getCurrentSlotID() == m_player->getUniqueID())
    {
        message = _("You cannot delete this player because it is currently in use.");
    }

    core::rect< s32 > area_left(5, 0, m_area.getWidth()-5, m_area.getHeight()/2);

    // When there is no need to tab through / click on images/labels,
    // we can add irrlicht labels directly
    // (more complicated uses require the use of our widget set)
    IGUIStaticText* a = GUIEngine::getGUIEnv()->addStaticText( message.c_str(),
                                              area_left, false /* border */, true /* word wrap */,
                                              m_irrlicht_window);
    a->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);

    if (unlock_manager->getCurrentSlotID() != m_player->getUniqueID())
    {
        ButtonWidget* widget = new ButtonWidget();
        widget->m_properties[PROP_ID] = "confirmremove";

        //I18N: In the player info dialog (when deleting)
        widget->setText( _("Confirm Remove") );

        const int textWidth = 
            font->getDimension(widget->getText().c_str()).Width + 40;

        widget->m_x = m_area.getWidth()/2 - textWidth/2;
        widget->m_y = m_area.getHeight()/2;
        widget->m_w = textWidth;
        widget->m_h = buttonHeight;
        widget->setParent(m_irrlicht_window);
        m_widgets.push_back(widget);
        widget->add();
    }

    {
        ButtonWidget* widget = new ButtonWidget();
        widget->m_properties[PROP_ID] = "cancelremove";

        //I18N: In the player info dialog (when deleting)
        widget->setText( _("Cancel Remove") );

        const int textWidth = 
            font->getDimension( widget->getText().c_str() ).Width + 40;

        widget->m_x = m_area.getWidth()/2 - textWidth/2;
        widget->m_y = m_area.getHeight()*3/4;
        widget->m_w = textWidth;
        widget->m_h = buttonHeight;
        widget->setParent(m_irrlicht_window);
        m_widgets.push_back(widget);
        widget->add();

        widget->setFocusForPlayer( PLAYER_ID_GAME_MASTER );
    }

}

// -----------------------------------------------------------------------------

void PlayerInfoDialog::onEnterPressedInternal()
{
}

// -----------------------------------------------------------------------------

GUIEngine::EventPropagation PlayerInfoDialog::processEvent(const std::string& eventSource)
{
    if (eventSource == "renameplayer")
    {
        // accept entered name
        stringw playerName = textCtrl->getText().trim();

        const int playerAmount =  UserConfigParams::m_all_players.size();
        for(int n=0; n<playerAmount; n++)
        {
            if (UserConfigParams::m_all_players.get(n) == m_player) continue;

            if (UserConfigParams::m_all_players[n].getName() == playerName)
            {
                ButtonWidget* label = getWidget<ButtonWidget>("renameplayer");
                label->setBadge(BAD_BADGE);
                sfx_manager->quickSound( "anvil" );
                return GUIEngine::EVENT_BLOCK;
            }
        }

        if (playerName.size() <= 0) return GUIEngine::EVENT_BLOCK;

        OptionsScreenPlayers::getInstance()->renamePlayer( playerName, m_player );

        // irrLicht is too stupid to remove focus from deleted widgets
        // so do it by hand
        GUIEngine::getGUIEnv()->removeFocus( textCtrl->getIrrlichtElement() );
        GUIEngine::getGUIEnv()->removeFocus( m_irrlicht_window );

        ModalDialog::dismiss();

        dismiss();
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "removeplayer")
    {
        showConfirmDialog();
        return GUIEngine::EVENT_BLOCK;
    }
    else if (eventSource == "confirmremove")
    {
        OptionsScreenPlayers::getInstance()->deletePlayer( m_player );
        m_player = NULL;

        // irrLicht is too stupid to remove focus from deleted widgets
        // so do it by hand
        GUIEngine::getGUIEnv()->removeFocus( textCtrl->getIrrlichtElement() );
        GUIEngine::getGUIEnv()->removeFocus( m_irrlicht_window );

        ModalDialog::dismiss();
        return GUIEngine::EVENT_BLOCK;
    }
    else if(eventSource == "cancelremove")
    {
        showRegularDialog();
        return GUIEngine::EVENT_BLOCK;
    }
    else if(eventSource == "cancel")
    {
        // irrLicht is too stupid to remove focus from deleted widgets
        // so do it by hand
        GUIEngine::getGUIEnv()->removeFocus( textCtrl->getIrrlichtElement() );
        GUIEngine::getGUIEnv()->removeFocus( m_irrlicht_window );

        ModalDialog::dismiss();
        return GUIEngine::EVENT_BLOCK;
    }
    return GUIEngine::EVENT_LET;
}

// -----------------------------------------------------------------------------

