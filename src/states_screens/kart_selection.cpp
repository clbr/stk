//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006
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
#include "config/player.hpp"
#include "config/user_config.hpp"
#include "kart_selection.hpp"
#include "graphics/irr_driver.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/screen.hpp"
#include "guiengine/widgets/bubble_widget.hpp"
#include "guiengine/widgets/dynamic_ribbon_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/widgets/model_view_widget.hpp"
#include "guiengine/widgets/ribbon_widget.hpp"
#include "guiengine/widgets/spinner_widget.hpp"
#include "input/input.hpp"
#include "input/input_manager.hpp"
#include "input/device_manager.hpp"
#include "input/input_device.hpp"
#include "items/item_manager.hpp"
#include "io/file_manager.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "modes/overworld.hpp"
#include "states_screens/race_setup_screen.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/translation.hpp"
#include "utils/random_generator.hpp"
#include "utils/string_utils.hpp"

#include <iostream>
#include <string>
#include <IGUIEnvironment.h>
#include <IGUIImage.h>
#include <IGUIButton.h>

InputDevice* player_1_device = NULL;

using namespace GUIEngine;
using irr::core::stringw;

static const char RANDOM_KART_ID[] = "randomkart";
static const char ID_DONT_USE[] = "x";
// Use '/' as special character to avoid that someone creates
// a kart called 'locked'
static const char ID_LOCKED[] = "locked/";

DEFINE_SCREEN_SINGLETON( KartSelectionScreen );

class PlayerKartWidget;

/** Currently, navigation for multiple players at the same time is implemented
    in a somewhat clunky way. An invisible "dispatcher" widget is added above
    kart icons. When a player moves up, he focuses the dispatcher, which in
    turn moves the selection to the appropriate spinner. "tabbing roots" are
    used to make navigation back down possible. (FIXME: maybe find a cleaner
    way?) */
int g_root_id;

class FocusDispatcher : public Widget
{
    KartSelectionScreen* m_parent;
    int m_reserved_id;

    bool m_is_initialised;

public:

    LEAK_CHECK()

    // ------------------------------------------------------------------------
    FocusDispatcher(KartSelectionScreen* parent) : Widget(WTYPE_BUTTON)
    {
        m_parent = parent;
        m_supports_multiplayer = true;
        m_is_initialised = false;

        m_x = 0;
        m_y = 0;
        m_w = 1;
        m_h = 1;

        m_reserved_id = Widget::getNewNoFocusID();
    }   // FocusDispatcher
    // ------------------------------------------------------------------------
    void setRootID(const int reservedID)
    {
        assert(reservedID != -1);

        m_reserved_id = reservedID;

        if (m_element != NULL)
        {
            m_element->setID(m_reserved_id);
        }

        m_is_initialised = true;
    }   // setRootID

    // ------------------------------------------------------------------------
    virtual void add()
    {
        core::rect<s32> widget_size(m_x, m_y, m_x + m_w, m_y + m_h);

        m_element = GUIEngine::getGUIEnv()->addButton(widget_size, NULL,
                                                      m_reserved_id,
                                                      L"Dispatcher", L"");

        m_id = m_element->getID();
        m_element->setTabStop(true);
        m_element->setTabGroup(false);
        m_element->setTabOrder(m_id);
        m_element->setVisible(false);
    }   // add

    // ------------------------------------------------------------------------

    virtual EventPropagation focused(const int playerID);
};   // FocusDispatcher

FocusDispatcher* g_dispatcher = NULL;

// ============================================================================

/** A small extension to the spinner widget to add features like player ID
 *  management or badging */
class PlayerNameSpinner : public SpinnerWidget
{
    int m_playerID;
    bool m_incorrect;
    irr::gui::IGUIImage* m_red_mark_widget;
    KartSelectionScreen* m_parent;

    //virtual EventPropagation focused(const int m_playerID) ;

public:
    PlayerNameSpinner(KartSelectionScreen* parent, const int playerID)
    {
        m_playerID        = playerID;
        m_incorrect       = false;
        m_red_mark_widget = NULL;
        m_parent          = parent;
    }   // PlayerNameSpinner
    // ------------------------------------------------------------------------
    void setID(const int m_playerID)
    {
        PlayerNameSpinner::m_playerID = m_playerID;
    }   // setID
    // ------------------------------------------------------------------------
    /** Add a red mark on the spinner to mean "invalid choice" */
    void markAsIncorrect()
    {
        if (m_incorrect) return; // already flagged as incorrect

        m_incorrect = true;

        irr::video::ITexture* texture = irr_driver->getTexture(
                                file_manager->getTextureFile("red_mark.png") );
        const int mark_size = m_h;
        const int mark_x = m_w - mark_size*2;
        const int mark_y = 0;
        core::recti red_mark_area(mark_x, mark_y, mark_x + mark_size,
                                  mark_y + mark_size);
        m_red_mark_widget = GUIEngine::getGUIEnv()->addImage( red_mark_area,
                                                      /* parent */ m_element );
        m_red_mark_widget->setImage(texture);
        m_red_mark_widget->setScaleImage(true);
        m_red_mark_widget->setTabStop(false);
        m_red_mark_widget->setUseAlphaChannel(true);
    }   // markAsIncorrect

    // ------------------------------------------------------------------------
    /** Remove any red mark set with 'markAsIncorrect' */
    void markAsCorrect()
    {
        if (m_incorrect)
        {
            m_red_mark_widget->remove();
            m_red_mark_widget = NULL;
            m_incorrect = false;
        }
    }   // markAsCorrect
};

// ============================================================================

#if 0
#pragma mark -
#pragma mark PlayerKartWidget
#endif

/** A widget representing the kart selection for a player (i.e. the player's
 *  number, name, the kart view, the kart's name) */
class PlayerKartWidget : public Widget,
                         public SpinnerWidget::ISpinnerConfirmListener
{
    /** Whether this player confirmed their selection */
    bool m_ready;

    /** widget coordinates */
    int player_id_x, player_id_y, player_id_w, player_id_h;
    int player_name_x, player_name_y, player_name_w, player_name_h;
    int model_x, model_y, model_w, model_h;
    int kart_name_x, kart_name_y, kart_name_w, kart_name_h;

    /** A reserved ID for this widget if any, -1 otherwise.  (If no ID is
     *  reserved, widget will not be in the regular tabbing order */
    int m_irrlicht_widget_ID;

    /** For animation purposes (see method 'move') */
    int target_x, target_y, target_w, target_h;
    float x_speed, y_speed, w_speed, h_speed;

    /** Object representing this player */
    StateManager::ActivePlayer* m_associatedPlayer;
    int m_playerID;

    /** Internal name of the spinner; useful to interpret spinner events,
     *  which contain the name of the activated object */
    std::string spinnerID;

#ifdef DEBUG
    long m_magic_number;
#endif

public:

    LEAK_CHECK()

    /** Sub-widgets created by this widget */
    //LabelWidget* m_player_ID_label;
    PlayerNameSpinner* m_player_ident_spinner;
    ModelViewWidget* m_model_view;
    LabelWidget* m_kart_name;

    KartSelectionScreen* m_parent_screen;

    gui::IGUIStaticText* m_ready_text;

    //LabelWidget *getPlayerIDLabel() {return m_player_ID_label;}
    core::stringw deviceName;
    std::string m_kartInternalName;

    bool m_not_updated_yet;

    PlayerKartWidget(KartSelectionScreen* parent,
                     StateManager::ActivePlayer* associatedPlayer,
                     core::recti area, const int playerID,
                     std::string kartGroup,
                     const int irrlichtWidgetID=-1) : Widget(WTYPE_DIV)
    {
#ifdef DEBUG
        assert(associatedPlayer->ok());
        m_magic_number = 0x33445566;
#endif
        m_ready_text = NULL;
        m_parent_screen = parent;

        m_associatedPlayer = associatedPlayer;
        x_speed = 1.0f;
        y_speed = 1.0f;
        w_speed = 1.0f;
        h_speed = 1.0f;
        m_ready = false;
        m_not_updated_yet = true;

        m_irrlicht_widget_ID = irrlichtWidgetID;

        m_playerID = playerID;
        m_properties[PROP_ID] = StringUtils::insertValues("@p%i", m_playerID);

        setSize(area.UpperLeftCorner.X, area.UpperLeftCorner.Y,
                area.getWidth(), area.getHeight()               );
        target_x = m_x;
        target_y = m_y;
        target_w = m_w;
        target_h = m_h;

        // ---- Player identity spinner
        m_player_ident_spinner = new PlayerNameSpinner(parent, m_playerID);
        m_player_ident_spinner->m_x = player_name_x;
        m_player_ident_spinner->m_y = player_name_y;
        m_player_ident_spinner->m_w = player_name_w;
        m_player_ident_spinner->m_h = player_name_h;

        if (parent->m_multiplayer)
        {
            if (associatedPlayer->getDevice()->getType() == DT_KEYBOARD)
            {
                m_player_ident_spinner->setBadge(KEYBOARD_BADGE);
            }
            else if (associatedPlayer->getDevice()->getType() == DT_GAMEPAD)
            {
                m_player_ident_spinner->setBadge(GAMEPAD_BADGE);
            }
        }

        if (irrlichtWidgetID == -1)
        {
            m_player_ident_spinner->m_tab_down_root = g_root_id;
        }

        spinnerID = StringUtils::insertValues("@p%i_spinner", m_playerID);

        m_player_ident_spinner->m_properties[PROP_ID] = spinnerID;
        if (parent->m_multiplayer)
        {
            const int playerAmount = UserConfigParams::m_all_players.size();
            m_player_ident_spinner->m_properties[PROP_MIN_VALUE] = "0";
            m_player_ident_spinner->m_properties[PROP_MAX_VALUE] =
                                             StringUtils::toString(playerAmount-1);
            m_player_ident_spinner->m_properties[PROP_WRAP_AROUND] = "true";
        }
        else
        {
            m_player_ident_spinner->m_properties[PROP_MIN_VALUE] = "0";
            m_player_ident_spinner->m_properties[PROP_MAX_VALUE] = "0";
        }

        //m_player_ident_spinner->m_event_handler = this;
        m_children.push_back(m_player_ident_spinner);


        // ----- Kart model view
        m_model_view = new ModelViewWidget();

        m_model_view->m_x = model_x;
        m_model_view->m_y = model_y;
        m_model_view->m_w = model_w;
        m_model_view->m_h = model_h;
        m_model_view->m_properties[PROP_ID] =
            StringUtils::insertValues("@p%i_model", m_playerID);
        //m_model_view->setParent(this);
        m_children.push_back(m_model_view);

        // Init kart model
        const std::string default_kart = UserConfigParams::m_default_kart;
        const KartProperties* props =
            kart_properties_manager->getKart(default_kart);

        if(!props)
        {
            // If the default kart can't be found (e.g. previously a addon
            // kart was used, but the addon package was removed), use the
            // first kart as a default. This way we don't have to hardcode
            // any kart names.
            int id = kart_properties_manager->getKartByGroup(kartGroup, 0);
            if (id == -1)
            {
                props = kart_properties_manager->getKartById(0);
            }
            else
            {
                props = kart_properties_manager->getKartById(id);
            }

            if(!props)
            {
                fprintf(stderr,
                        "[KartSelectionScreen] WARNING: Can't find default "
                        "kart '%s' nor any other kart.\n",
                        default_kart.c_str());
                exit(-1);
            }
        }
        m_kartInternalName = props->getIdent();

        const KartModel &kart_model = props->getMasterKartModel();

        m_model_view->addModel( kart_model.getModel(), Vec3(0,0,0),
                                Vec3(35.0f, 35.0f, 35.0f),
                                kart_model.getBaseFrame() );
        m_model_view->addModel( kart_model.getWheelModel(0),
                                kart_model.getWheelGraphicsPosition(0) );
        m_model_view->addModel( kart_model.getWheelModel(1),
                                kart_model.getWheelGraphicsPosition(1) );
        m_model_view->addModel( kart_model.getWheelModel(2),
                                kart_model.getWheelGraphicsPosition(2) );
        m_model_view->addModel( kart_model.getWheelModel(3),
                                kart_model.getWheelGraphicsPosition(3) );
        m_model_view->setRotateContinuously( 35.0f );

        // ---- Kart name label
        m_kart_name = new LabelWidget();
        m_kart_name->setText(props->getName(), false);
        m_kart_name->m_properties[PROP_TEXT_ALIGN] = "center";
        m_kart_name->m_properties[PROP_ID] =
            StringUtils::insertValues("@p%i_kartname", m_playerID);
        m_kart_name->m_x = kart_name_x;
        m_kart_name->m_y = kart_name_y;
        m_kart_name->m_w = kart_name_w;
        m_kart_name->m_h = kart_name_h;
        //m_kart_name->setParent(this);
        m_children.push_back(m_kart_name);
    }   // PlayerKartWidget
    // ------------------------------------------------------------------------

    ~PlayerKartWidget()
    {
        if (GUIEngine::getFocusForPlayer(m_playerID) == this)
        {
            GUIEngine::focusNothingForPlayer(m_playerID);
        }

        //if (m_player_ID_label->getIrrlichtElement() != NULL)
        //    m_player_ID_label->getIrrlichtElement()->remove();

        if (m_player_ident_spinner != NULL)
        {
            m_player_ident_spinner->setListener(NULL);

            if (m_player_ident_spinner->getIrrlichtElement() != NULL)
            {
                m_player_ident_spinner->getIrrlichtElement()->remove();
            }
        }

        if (m_model_view->getIrrlichtElement() != NULL)
            m_model_view->getIrrlichtElement()->remove();

        if (m_kart_name->getIrrlichtElement() != NULL)
            m_kart_name->getIrrlichtElement()->remove();

        getCurrentScreen()->manualRemoveWidget(this);

#ifdef DEBUG
         m_magic_number = 0xDEADBEEF;
#endif
    }   // ~PlayerKartWidget

    // ------------------------------------------------------------------------
    /** Called when players are renumbered (changes the player ID) */
    void setPlayerID(const int newPlayerID)
    {
        assert(m_magic_number == 0x33445566);

        if (StateManager::get()->getActivePlayer(newPlayerID)
            != m_associatedPlayer)
        {
            std::cerr << "[KartSelectionScreen]  WARNING: Internal "
                         "inconsistency, PlayerKartWidget has IDs and "
                         "pointers that do not correspond to one player\n";
            fprintf(stderr,
                   "    Player: %p  -  Index: %d  -  m_associatedPlayer: %p\n",
                    StateManager::get()->getActivePlayer(newPlayerID),
                    newPlayerID, m_associatedPlayer);
            assert(false);
        }

        // Remove current focus, but rembmer it
        Widget* focus = GUIEngine::getFocusForPlayer(m_playerID);
        GUIEngine::focusNothingForPlayer(m_playerID);

        // Change the player ID
        m_playerID = newPlayerID;

        // restore previous focus, but with new player ID
        if (focus != NULL) focus->setFocusForPlayer(m_playerID);

        if (m_player_ident_spinner != NULL)
            m_player_ident_spinner->setID(m_playerID);
    }   // setPlayerID

    // ------------------------------------------------------------------------
    /** Returns the ID of this player */
    int getPlayerID() const
    {
        assert(m_magic_number == 0x33445566);
        return m_playerID;
    }   // getPlayerID

    // ------------------------------------------------------------------------
    /** Add the widgets to the current screen */
    virtual void add()
    {
        assert(m_magic_number == 0x33445566);

        assert(KartSelectionScreen::getInstance()
               ->m_kart_widgets.contains(this));
        bool mineInList = false;
        for (int p=0; p<StateManager::get()->activePlayerCount(); p++)
        {
#ifdef DEBUG
            assert(StateManager::get()->getActivePlayer(p)->ok());
#endif
            if (StateManager::get()->getActivePlayer(p) == m_associatedPlayer)
            {
                mineInList = true;
            }
        }
        assert(mineInList);

        //m_player_ID_label->add();

        // the first player will have an ID of its own to allow for keyboard
        // navigation despite this widget being added last
        if (m_irrlicht_widget_ID != -1)
            m_player_ident_spinner->m_reserved_id = m_irrlicht_widget_ID;
        else
            m_player_ident_spinner->m_reserved_id = Widget::getNewNoFocusID();

        m_player_ident_spinner->add();
        m_player_ident_spinner->getIrrlichtElement()->setTabStop(false);
        m_player_ident_spinner->setListener(this);

        m_model_view->add();
        m_kart_name->add();

        m_model_view->update(0);

        m_player_ident_spinner->clearLabels();
        if (m_parent_screen->m_multiplayer)
        {
            const int playerAmount = UserConfigParams::m_all_players.size();
            for (int n=0; n<playerAmount; n++)
            {
                core::stringw name = UserConfigParams::m_all_players[n].getName();
                m_player_ident_spinner->addLabel( translations->fribidize(name) );
            }

            // select the right player profile in the spinner
            m_player_ident_spinner->setValue(m_associatedPlayer->getProfile()
                                                               ->getName()   );
        }
        else
        {
            m_player_ident_spinner->addLabel( m_associatedPlayer->getProfile()->getName() );
            m_player_ident_spinner->setVisible(false);
        }

        assert(m_player_ident_spinner->getStringValue() ==
                 m_associatedPlayer->getProfile()->getName());
    }   // add

    // ------------------------------------------------------------------------
    /** Get the associated ActivePlayer object*/
    StateManager::ActivePlayer* getAssociatedPlayer()
    {
        assert(m_magic_number == 0x33445566);
        return m_associatedPlayer;
    }   // getAssociatedPlayer

    // ------------------------------------------------------------------------
    /** Starts a 'move/resize' animation, by simply passing destination coords.
     *  The animation will then occur on each call to 'onUpdate'. */
    void move(const int x, const int y, const int w, const int h)
    {
        assert(m_magic_number == 0x33445566);
        target_x = x;
        target_y = y;
        target_w = w;
        target_h = h;

        x_speed = abs( m_x - x ) / 300.0f;
        y_speed = abs( m_y - y ) / 300.0f;
        w_speed = abs( m_w - w ) / 300.0f;
        h_speed = abs( m_h - h ) / 300.0f;
    }   // move

    // ------------------------------------------------------------------------
    /** Call when player confirmed his identity and kart */
    void markAsReady()
    {
        assert(m_magic_number == 0x33445566);
        if (m_ready) return; // already ready

        m_ready = true;

        stringw playerNameString = m_player_ident_spinner->getStringValue();
        core::rect<s32> rect(core::position2di(m_player_ident_spinner->m_x,
                                               m_player_ident_spinner->m_y),
                            core::dimension2di(m_player_ident_spinner->m_w,
                                               m_player_ident_spinner->m_h));
        // 'playerNameString' is already fribidize, so we need to use
        // 'insertValues' and not _("...", a) so it's not flipped again
        m_ready_text =
            GUIEngine::getGUIEnv()->addStaticText(
                            StringUtils::insertValues(_("%s is ready"),
                                                     playerNameString).c_str(),
                                                     rect                    );
        m_ready_text->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER );

        m_children.remove(m_player_ident_spinner);
        m_player_ident_spinner->setListener(NULL);
        m_player_ident_spinner->getIrrlichtElement()->remove();
        m_player_ident_spinner->elementRemoved();
        delete m_player_ident_spinner;
        m_player_ident_spinner = NULL;

        sfx_manager->quickSound( "wee" );

        m_model_view->setRotateTo(30.0f, 1.0f);

        player_id_w *= 2;
        player_name_w = 0;

        m_model_view->setBadge(OK_BADGE);
    }   // markAsReady

    // ------------------------------------------------------------------------
    /** \return Whether this player confirmed his kart and indent selection */
    bool isReady()
    {
        assert(m_magic_number == 0x33445566);
        return m_ready;
    }   // isReady

    // -------------------------------------------------------------------------
    /** Updates the animation (moving/shrinking/etc.) */
    void onUpdate(float delta)
    {
        assert(m_magic_number == 0x33445566);
        if (target_x == m_x && target_y == m_y &&
            target_w == m_w && target_h == m_h) return;

        int move_step = (int)(delta*1000.0f);

        // move x towards target
        if (m_x < target_x)
        {
            m_x += (int)(move_step*x_speed);
            // don't move to the other side of the target
            if (m_x > target_x) m_x = target_x;
        }
        else if (m_x > target_x)
        {
            m_x -= (int)(move_step*x_speed);
            // don't move to the other side of the target
            if (m_x < target_x) m_x = target_x;
        }

        // move y towards target
        if (m_y < target_y)
        {
            m_y += (int)(move_step*y_speed);
            // don't move to the other side of the target
            if (m_y > target_y) m_y = target_y;
        }
        else if (m_y > target_y)
        {
            m_y -= (int)(move_step*y_speed);
            // don't move to the other side of the target
            if (m_y < target_y) m_y = target_y;
        }

        // move w towards target
        if (m_w < target_w)
        {
            m_w += (int)(move_step*w_speed);
            // don't move to the other side of the target
            if (m_w > target_w) m_w = target_w;
        }
        else if (m_w > target_w)
        {
            m_w -= (int)(move_step*w_speed);
            // don't move to the other side of the target
            if (m_w < target_w) m_w = target_w;
        }
        // move h towards target
        if (m_h < target_h)
        {
            m_h += (int)(move_step*h_speed);
            // don't move to the other side of the target
            if (m_h > target_h) m_h = target_h;
        }
        else if (m_h > target_h)
        {
            m_h -= (int)(move_step*h_speed);
            // don't move to the other side of the target
            if (m_h < target_h) m_h = target_h;
        }

        setSize(m_x, m_y, m_w, m_h);

        if (m_player_ident_spinner != NULL)
        {
            m_player_ident_spinner->move(player_name_x,
                             player_name_y,
                             player_name_w,
                             player_name_h );
        }
        if (m_ready_text != NULL)
        {
            m_ready_text->setRelativePosition(
                core::recti(core::position2di(player_name_x, player_name_y),
                core::dimension2di(player_name_w, player_name_h))           );
        }

        m_model_view->move(model_x,
                        model_y,
                        model_w,
                        model_h);
        m_kart_name->move(kart_name_x,
                       kart_name_y,
                       kart_name_w,
                       kart_name_h);

        // When coming from the overworld, we must rebuild the preview scene at
        // least once, since the scene is being cleared by leaving the overworld
        if (m_not_updated_yet)
        {
            m_model_view->clearRttProvider();
            m_not_updated_yet = false;
        }
    }   // onUpdate

    // -------------------------------------------------------------------------
    /** Event callback */
    virtual GUIEngine::EventPropagation transmitEvent(
                                        Widget* w,
                                        const std::string& originator,
                                        const int m_playerID)
    {
        assert(m_magic_number == 0x33445566);
        // if it's declared ready, there is really nothing to process
        if (m_ready) return EVENT_LET;

        //std::cout << "= kart selection :: transmitEvent "
        // << originator << " =\n";

        std::string name = w->m_properties[PROP_ID];

        //std::cout << "    (checking if that's me: I am "
        // << spinnerID << ")\n";

        // update player profile when spinner changed
        if (originator == spinnerID)
        {
            if(UserConfigParams::logGUI())
            {
                std::cout << "[KartSelectionScreen] Identity changed "
                             "for player " << m_playerID
                          << " : " << irr::core::stringc(
                                      m_player_ident_spinner->getStringValue()
                                      .c_str()).c_str()
                          << std::endl;
            }

            if (m_parent_screen->m_multiplayer)
            {
                m_associatedPlayer->setPlayerProfile(
                    UserConfigParams::m_all_players.get(m_player_ident_spinner
                                                        ->getValue()) );
            }
        }

        return EVENT_LET; // continue propagating the event
    }   // transmitEvent

    // -------------------------------------------------------------------------
    /** Sets the size of the widget as a whole, and placed children widgets
     * inside itself */
    void setSize(const int x, const int y, const int w, const int h)
    {
        assert(m_magic_number == 0x33445566);
        m_x = x;
        m_y = y;
        m_w = w;
        m_h = h;

        // -- sizes
        player_id_w = w;
        player_id_h = GUIEngine::getFontHeight();

        player_name_h = 40;
        player_name_w = std::min(400, w);

        kart_name_w = w;
        kart_name_h = 25;

        // for shrinking effect
        if (h < 175)
        {
            const float factor = h / 175.0f;
            kart_name_h   = (int)(kart_name_h*factor);
            player_name_h = (int)(player_name_h*factor);
            player_id_h   = (int)(player_id_h*factor);
        }

        // --- layout
        player_id_x = x;
        player_id_y = y;

        player_name_x = x + w/2 - player_name_w/2;
        player_name_y = y + player_id_h;

        const int modelMaxHeight =  h - kart_name_h - player_name_h
                                      - player_id_h;
        const int modelMaxWidth =  w;
        const int bestSize = std::min(modelMaxWidth, modelMaxHeight);
        const int modelY = y + player_name_h + player_id_h;
        model_x = x + w/2 - (int)(bestSize/2);
        model_y = modelY + modelMaxHeight/2 - bestSize/2;
        model_w = (int)(bestSize);
        model_h = bestSize;

        kart_name_x = x;
        kart_name_y = y + h - kart_name_h;
    }   // setSize

    // -------------------------------------------------------------------------

    /** Sets which kart was selected for this player */
    void setKartInternalName(const std::string& whichKart)
    {
        assert(m_magic_number == 0x33445566);
        m_kartInternalName = whichKart;
    }   // setKartInternalName

    // -------------------------------------------------------------------------

    const std::string& getKartInternalName() const
    {
        assert(m_magic_number == 0x33445566);
        return m_kartInternalName;
    }   // getKartInternalName

    // -------------------------------------------------------------------------

    /** \brief Event callback from ISpinnerConfirmListener */
    virtual EventPropagation onSpinnerConfirmed()
    {
        KartSelectionScreen::getInstance()->playerConfirm(m_playerID);
        return EVENT_BLOCK;
    }   // onSpinnerConfirmed
};   // PlayerKartWidget

/** Small utility function that returns whether the two given players chose
 *  the same kart. The advantage of this function is that it can handle
 *  "random kart" selection. */
bool sameKart(const PlayerKartWidget& player1, const PlayerKartWidget& player2)
{
    return player1.getKartInternalName() == player2.getKartInternalName() &&
    player1.getKartInternalName() != RANDOM_KART_ID;
}

#if 0
#pragma mark -
#pragma mark KartHoverListener
#endif
// ============================================================================

class KartHoverListener : public DynamicRibbonHoverListener
{
    KartSelectionScreen* m_parent;
public:
    unsigned int m_magic_number;

    KartHoverListener(KartSelectionScreen* parent)
    {
        m_magic_number = 0xCAFEC001;
        m_parent = parent;
    }   // KartHoverListener

    // ------------------------------------------------------------------------
    virtual ~KartHoverListener()
    {
        assert(m_magic_number == 0xCAFEC001);
        m_magic_number = 0xDEADBEEF;
    }   // ~KartHoverListener

    // ------------------------------------------------------------------------
    void onSelectionChanged(DynamicRibbonWidget* theWidget,
                            const std::string& selectionID,
                            const irr::core::stringw& selectionText,
                            const int playerID)
    {
        assert(m_magic_number == 0xCAFEC001);

        // Don't allow changing the selection after confirming it
        if (m_parent->m_kart_widgets[playerID].isReady())
        {
            // discard events sent when putting back to the right kart
            if (selectionID ==
                m_parent->m_kart_widgets[playerID].m_kartInternalName) return;

            DynamicRibbonWidget* w =
                m_parent->getWidget<DynamicRibbonWidget>("karts");
            assert(w != NULL);

            w->setSelection(m_parent->m_kart_widgets[playerID]
                                     .m_kartInternalName, playerID, true);
            return;
        }

        // Update the displayed model
        ModelViewWidget* w3 = m_parent->m_kart_widgets[playerID].m_model_view;
        assert( w3 != NULL );

        if (selectionID == RANDOM_KART_ID)
        {
            // Random kart
            scene::IMesh* model =
                ItemManager::getItemModel(Item::ITEM_BONUS_BOX);
            w3->clearModels();
            w3->addModel( model, Vec3(0.0f, -12.0f, 0.0f),
                                 Vec3(35.0f, 35.0f, 35.0f) );
            w3->update(0);
            m_parent->m_kart_widgets[playerID].m_kart_name
                    ->setText( _("Random Kart"), false );
        }
        // selectionID contains the name of the kart, so check only for substr
        else if (StringUtils::startsWith(selectionID, ID_LOCKED))
        {
            w3->clearModels();
            w3->addModel(irr_driver->getAnimatedMesh(
               file_manager->getDataDir() + "/models/chest.b3d" )->getMesh(20),
                         Vec3(0,0,0), Vec3(15.0f, 15.0f, 15.0f) );
            w3->update(0);

            if (m_parent->m_multiplayer)
            {
                m_parent->m_kart_widgets[playerID].m_kart_name
                       ->setText(_("Locked"), false );
            }
            else
            {
                m_parent->m_kart_widgets[playerID].m_kart_name
                        ->setText(_("Locked : solve active challenges to gain "
                                    "access to more!"), false );
            }
        }
        else
        {
            const KartProperties *kp =
                kart_properties_manager->getKart(selectionID);
            if (kp != NULL)
            {
                const KartModel &kart_model = kp->getMasterKartModel();

                w3->clearModels();
                w3->addModel( kart_model.getModel(), Vec3(0,0,0),
                              Vec3(35.0f, 35.0f, 35.0f),
                              kart_model.getBaseFrame() );
                w3->addModel( kart_model.getWheelModel(0),
                              kart_model.getWheelGraphicsPosition(0) );
                w3->addModel( kart_model.getWheelModel(1),
                              kart_model.getWheelGraphicsPosition(1) );
                w3->addModel( kart_model.getWheelModel(2),
                              kart_model.getWheelGraphicsPosition(2) );
                w3->addModel( kart_model.getWheelModel(3),
                              kart_model.getWheelGraphicsPosition(3) );
                w3->update(0);

                m_parent->m_kart_widgets[playerID].m_kart_name
                       ->setText( selectionText.c_str(), false );
            }
            else
            {
                fprintf(stderr, "[KartSelectionScreen] WARNING: could not "
                                "find a kart named '%s'\n",
                        selectionID.c_str());
            }
        }

        m_parent->m_kart_widgets[playerID].setKartInternalName(selectionID);
        m_parent->validateKartChoices();
    }   // onSelectionChanged
};   // KartHoverListener

#if 0
#pragma mark -
#pragma mark KartSelectionScreen
#endif

// ============================================================================

KartSelectionScreen::KartSelectionScreen() : Screen("karts.stkgui")
{
    m_removed_widget       = NULL;
    m_multiplayer_message  = NULL;
    m_from_overworld       = false;
    m_go_to_overworld_next = false;
}   // KartSelectionScreen

// ----------------------------------------------------------------------------

void KartSelectionScreen::loadedFromFile()
{
    g_dispatcher          = new FocusDispatcher(this);
    m_first_widget        = g_dispatcher;
    m_game_master_confirmed    = false;
    m_multiplayer_message = NULL;
    // Dynamically add tabs
    RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
    assert( tabs != NULL );

    m_last_widget = tabs;
}   // loadedFromFile

// ----------------------------------------------------------------------------

void KartSelectionScreen::beforeAddingWidget()
{
    // Dynamically add tabs
    RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
    assert( tabs != NULL );

    m_last_widget = tabs;
    tabs->clearAllChildren();

    const std::vector<std::string>& groups =
        kart_properties_manager->getAllGroups();
    const int group_amount = groups.size();

    // add all group first
    if (group_amount > 1)
    {
        //I18N: name of the tab that will show tracks from all groups
        tabs->addTextChild( _("All") , ALL_KART_GROUPS_ID);
    }

    // Make group names being picked up by gettext
#define FOR_GETTEXT_ONLY(x)
    //I18N: kart group name
    FOR_GETTEXT_ONLY( _("standard") )
    //I18N: kart group name
    FOR_GETTEXT_ONLY( _("Add-Ons") )


    // add others after
    for (int n=0; n<group_amount; n++)
    {
        // try to translate group names
        tabs->addTextChild( _(groups[n].c_str()) , groups[n]);
    }   // for n<group_amount


    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    assert( w != NULL );

    w->setItemCountHint( kart_properties_manager->getNumberOfKarts() );
}   // beforeAddingWidget

// ----------------------------------------------------------------------------

void KartSelectionScreen::init()
{
    Screen::init();

    RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
    assert( tabs != NULL );
    tabs->select(UserConfigParams::m_last_used_kart_group,
                 PLAYER_ID_GAME_MASTER);

    Widget* placeholder = getWidget("playerskarts");
    assert(placeholder != NULL);

    g_dispatcher->setRootID(placeholder->m_reserved_id);

    g_root_id = placeholder->m_reserved_id;
    if (!m_widgets.contains(g_dispatcher))
    {
        m_widgets.push_back(g_dispatcher);

        // this is only needed if the dispatcher wasn't already in
        // the list of widgets. If it already was, it was added along
        // other widgets.
        g_dispatcher->add();
    }

    m_game_master_confirmed = false;

    tabs->setActivated();

    m_kart_widgets.clearAndDeleteAll();
    StateManager::get()->resetActivePlayers();
    input_manager->getDeviceList()->setAssignMode(DETECT_NEW);

    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    assert( w != NULL );


    KartHoverListener* karthoverListener = new KartHoverListener(this);
    w->registerHoverListener(karthoverListener);


    // Build kart list (it is built everytime, to account for .g. locking)
    setKartsFromCurrentGroup();

    /*

     TODO: Ultimately, it'd be nice to *not* clear m_kart_widgets so that
     when players return to the kart selection screen, it will appear as
     it did when they left (at least when returning from the track menu).
     Rebuilding the screen is a little tricky.

     */

    /*
    if (m_kart_widgets.size() > 0)
    {
        // trying to rebuild the screen
        for (int n = 0; n < m_kart_widgets.size(); n++)
        {
            PlayerKartWidget *pkw;
            pkw = m_kart_widgets.get(n);
            manualAddWidget(pkw);
            pkw->add();
        }

    }
    else */
    // For now this is what will happen
    {
        playerJoin( input_manager->getDeviceList()->getLatestUsedDevice(),
                    true );
        w->updateItemDisplay();
    }

    // Player 0 select default kart
    if (!w->setSelection(UserConfigParams::m_default_kart, 0, true))
    {
        // if kart from config not found, select the first instead
        w->setSelection(0, 0, true);
    }
    // This flag will cause that a 'fire' event will be mapped to 'select' (if
    // 'fire' is not assigned to a GUI event). This is done to support the old
    // way of player joining by pressing 'fire' instead of 'select'.
    input_manager->getDeviceList()->mapFireToSelect(true);

}   // init

// ----------------------------------------------------------------------------

void KartSelectionScreen::tearDown()
{
    // Reset the 'map fire to select' option of the device manager
    input_manager->getDeviceList()->mapFireToSelect(false);

    // if a removed widget is currently shrinking down, remove it upon leaving
    // the screen
    if (m_removed_widget != NULL)
    {
        manualRemoveWidget(m_removed_widget);
        delete m_removed_widget;
        m_removed_widget = NULL;
    }

    if (m_multiplayer_message != NULL)
    {
        manualRemoveWidget(m_multiplayer_message);
        delete m_multiplayer_message;
        m_multiplayer_message = NULL;
    }

    Screen::tearDown();
    m_kart_widgets.clearAndDeleteAll();
}   // tearDown

// ----------------------------------------------------------------------------

void KartSelectionScreen::unloaded()
{
    // these pointers are no more valid (have been deleted along other widgets)
    g_dispatcher = NULL;
}

// ----------------------------------------------------------------------------
// Return true if event was handled successfully
bool KartSelectionScreen::playerJoin(InputDevice* device, bool firstPlayer)
{
    if (UserConfigParams::logGUI())
        std::cout << "[KartSelectionScreen]  playerJoin() invoked\n";
    if (!m_multiplayer && !firstPlayer) return false;

    assert (g_dispatcher != NULL);

    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    if (w == NULL)
    {
        std::cerr << "[KartSelectionScreen] playerJoin(): Called outside of "
                     "kart selection screen.\n";
        return false;
    }
    else if (device == NULL)
    {
        std::cerr << "[KartSelectionScreen] playerJoin(): Received null "
                     "device pointer\n";
        return false;
    }

    if (StateManager::get()->activePlayerCount() >= MAX_PLAYER_COUNT)
    {
        std::cerr << "[KartSelectionScreen] Maximum number of players "
                     "reached\n";
        sfx_manager->quickSound( "anvil" );
        return false;
    }

    // ---- Get available area for karts
    // make a copy of the area, ands move it to be outside the screen
    Widget* kartsAreaWidget = getWidget("playerskarts");
    // start at the rightmost of the screen
    const int shift = irr_driver->getFrameSize().Width;
    core::recti kartsArea(kartsAreaWidget->m_x + shift,
                          kartsAreaWidget->m_y,
                          kartsAreaWidget->m_x + shift + kartsAreaWidget->m_w,
                          kartsAreaWidget->m_y + kartsAreaWidget->m_h);

    // ---- Create new active player
    PlayerProfile* profileToUse = unlock_manager->getCurrentPlayer();

    if (!firstPlayer)
    {
        const int playerProfileCount = UserConfigParams::m_all_players.size();
        for (int n=0; n<playerProfileCount; n++)
        {
            if (UserConfigParams::m_all_players[n].isGuestAccount())
            {
                profileToUse = UserConfigParams::m_all_players.get(n);
                break;
            }
        }


        // Remove multiplayer message
        if (m_multiplayer_message != NULL)
        {
            manualRemoveWidget(m_multiplayer_message);
            m_multiplayer_message->getIrrlichtElement()->remove();
            m_multiplayer_message->elementRemoved();
            delete m_multiplayer_message;
            m_multiplayer_message = NULL;
        }
    }

    const int new_player_id =
        StateManager::get()->createActivePlayer( profileToUse, device );
    StateManager::ActivePlayer* aplayer =
        StateManager::get()->getActivePlayer(new_player_id);

    RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
    assert(tabs != NULL);

    std::string selected_kart_group =
        tabs->getSelectionIDString(PLAYER_ID_GAME_MASTER);

    // ---- Create player/kart widget
    PlayerKartWidget* newPlayerWidget =
        new PlayerKartWidget(this, aplayer, kartsArea, m_kart_widgets.size(),
                             selected_kart_group);

    manualAddWidget(newPlayerWidget);
    m_kart_widgets.push_back(newPlayerWidget);

    newPlayerWidget->add();

    // ---- Divide screen space among all karts
    const int amount = m_kart_widgets.size();
    Widget* fullarea = getWidget("playerskarts");

    // in this special case, leave room for a message on the right
    if (m_multiplayer && firstPlayer)
    {
        const int splitWidth = fullarea->m_w / 2;

        m_kart_widgets[0].move( fullarea->m_x, fullarea->m_y, splitWidth,
                                fullarea->m_h );

        m_multiplayer_message = new BubbleWidget();
        m_multiplayer_message->m_properties[PROP_TEXT_ALIGN] = "center";
        m_multiplayer_message->setText( _("Everyone:\nPress 'Select' now to "
                                          "join the game!") );
        m_multiplayer_message->m_x =
            (int)(fullarea->m_x + splitWidth + splitWidth*0.2f);
        m_multiplayer_message->m_y = (int)(fullarea->m_y + fullarea->m_h*0.3f);
        m_multiplayer_message->m_w = (int)(splitWidth*0.6f);
        m_multiplayer_message->m_h = (int)(fullarea->m_h*0.6f);
        m_multiplayer_message->setFocusable(false);
        m_multiplayer_message->add();
        manualAddWidget(m_multiplayer_message);
    }
    else
    {
        const int splitWidth = fullarea->m_w / amount;

        for (int n=0; n<amount; n++)
        {
            m_kart_widgets[n].move( fullarea->m_x + splitWidth*n,
                                    fullarea->m_y, splitWidth, fullarea->m_h);
        }
    }


    if (!firstPlayer)
    {
        // select something (anything) in the ribbon; by default, only the
        // game master has something selected. Thus, when a new player joins,
        // we need to select something for them
        w->setSelection(new_player_id, new_player_id, true);

        newPlayerWidget->m_player_ident_spinner
                       ->setFocusForPlayer(new_player_id);
    }

    if (!m_multiplayer)
    {
        input_manager->getDeviceList()->setSinglePlayer( StateManager::get()
                                                         ->getActivePlayer(0));
    }

    return true;
}   // playerJoin

// -----------------------------------------------------------------------------

bool KartSelectionScreen::playerQuit(StateManager::ActivePlayer* player)
{
    int playerID = -1;

    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    if (w == NULL)
    {
        std::cerr << "[KartSelectionScreen] ERROR: playerQuit() called "
                     "outside of kart selection screen, "
                  << "or the XML file for this screen was changed without "
                     "adapting the code accordingly\n";
        return false;
    }

    // If last player quits, return to main menu
    if (m_kart_widgets.size() <= 1) return false;

    std::map<PlayerKartWidget*, std::string> selections;

    // Find the player ID associated to this player
    for (int n=0; n<m_kart_widgets.size(); n++)
    {
        if (m_kart_widgets[n].getAssociatedPlayer() == player)
        {
            // Check that this player has not already confirmed,
            // then they can't back out
            if (m_kart_widgets[n].isReady())
            {
                sfx_manager->quickSound( "anvil" );
                return true;
            }

            playerID = n;
        }
        else
        {
            selections[m_kart_widgets.get(n)] =
                m_kart_widgets[n].getKartInternalName();
        }
    }
    if (playerID == -1)
    {
        std::cerr << "[KartSelectionScreen] WARNING: playerQuit cannot find "
                     "passed player\n";
        return false;
    }
    if(UserConfigParams::logGUI())
        std::cout << "playerQuit( " << playerID << " )\n";

    // Just a cheap way to check if there is any discrepancy
    // between m_kart_widgets and the active player array
    assert( m_kart_widgets.size() == StateManager::get()->activePlayerCount());

    // unset selection of this player
    GUIEngine::focusNothingForPlayer(playerID);

    // delete a previous removed widget that didn't have time to fully shrink
    // yet.
    // TODO: handle multiple shrinking widgets gracefully?
    if (m_removed_widget != NULL)
    {
        manualRemoveWidget(m_removed_widget);
        delete m_removed_widget;
        m_removed_widget = NULL;
    }

    // keep the removed kart a while, for the 'disappear' animation
    // to take place
    m_removed_widget = m_kart_widgets.remove(playerID);

    // Tell the StateManager to remove this player
    StateManager::get()->removeActivePlayer(playerID);

    // Karts count changed, maybe order too, so renumber them.
    renumberKarts();

    // Tell the removed widget to perform the shrinking animation (which will
    // be updated in onUpdate, and will stop when the widget has disappeared)
    Widget* fullarea = getWidget("playerskarts");
    m_removed_widget->move(m_removed_widget->m_x + m_removed_widget->m_w/2,
                           fullarea->m_y + fullarea->m_h, 0, 0);

    // update selections

    const int amount = m_kart_widgets.size();
    for (int n=0; n<amount; n++)
    {
        const std::string& selectedKart = selections[m_kart_widgets.get(n)];
        if (selectedKart.size() > 0)
        {
            //std::cout << m_kart_widgets[n].getAssociatedPlayer()
            //              ->getProfile()->getName() << " selected "
            //          << selectedKart.c_str() << "\n";
            const bool success = w->setSelection(selectedKart, n, true);
            if (!success)
            {
                std::cerr << "[KartSelectionScreen] Failed to select kart "
                          << selectedKart << " for player " << n
                          << ", what's going on??\n";
            }
        }
    }


    // check if all players are ready
    bool allPlayersReady = true;
    for (int n=0; n<amount; n++)
    {
        if (!m_kart_widgets[n].isReady())
        {
            allPlayersReady = false;
            break;
        }
    }
    if (allPlayersReady) allPlayersDone();

    return true;
}   // playerQuit

// ----------------------------------------------------------------------------

void KartSelectionScreen::onUpdate(float delta, irr::video::IVideoDriver*)
{
    // Dispatch the onUpdate event to each kart, so they can perform their
    // animation if any
    const int amount = m_kart_widgets.size();
    for (int n=0; n<amount; n++)
    {
        m_kart_widgets[n].onUpdate(delta);
    }

    // When a kart widget is removed, it's a kept a while, for the disappear
    // animation to take place
    if (m_removed_widget != NULL)
    {
        m_removed_widget->onUpdate(delta);

        if (m_removed_widget->m_w == 0 || m_removed_widget->m_h == 0)
        {
            // destruct when too small (for "disappear" effects)
            manualRemoveWidget(m_removed_widget);
            delete m_removed_widget;
            m_removed_widget = NULL;
        }
    }
}   // onUpdate

// ----------------------------------------------------------------------------

void KartSelectionScreen::playerConfirm(const int playerID)
{
    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    assert(w != NULL);
    const std::string selection = w->getSelectionIDString(playerID);
    if (StringUtils::startsWith(selection, ID_LOCKED))
    {
        unlock_manager->playLockSound();
        return;
    }

    if (playerID == PLAYER_ID_GAME_MASTER)
    {
        UserConfigParams::m_default_kart = selection;
    }

    if (m_kart_widgets[playerID].getKartInternalName().size() == 0)
    {
        sfx_manager->quickSound( "anvil" );
        return;
    }

    const int amount = m_kart_widgets.size();

    // Check if we have enough karts for everybody. If there are more players
    // than karts then just allow duplicates
    const int availableKartCount = w->getItems().size();
    const bool willNeedDuplicates = (amount > availableKartCount);

    // make sure no other player selected the same identity or kart
    for (int n=0; n<amount; n++)
    {
        if (n == playerID) continue; // don't check a kart against itself

        const bool player_ready   = m_kart_widgets[n].isReady();
        const bool ident_conflict =
            !m_kart_widgets[n].getAssociatedPlayer()->getProfile()
                                                    ->isGuestAccount() &&
            m_kart_widgets[n].getAssociatedPlayer()->getProfile() ==
            m_kart_widgets[playerID].getAssociatedPlayer()->getProfile();
        const bool kart_conflict  = sameKart(m_kart_widgets[n],
                                            m_kart_widgets[playerID]);

        if (player_ready && (ident_conflict || kart_conflict) &&
            !willNeedDuplicates)
        {
            if (UserConfigParams::logGUI())
                printf("[KartSelectionScreen] You can't select this identity "
                       "or kart, someone already took it!!\n");

            sfx_manager->quickSound( "anvil" );
            return;
        }

        // If two PlayerKart entries are associated to the same ActivePlayer,
        // something went wrong
        assert(m_kart_widgets[n].getAssociatedPlayer() !=
                 m_kart_widgets[playerID].getAssociatedPlayer());
    }

    // Mark this player as ready to start
    m_kart_widgets[playerID].markAsReady();

    if (playerID == PLAYER_ID_GAME_MASTER)
    {
        m_game_master_confirmed = true;
        RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
        assert( tabs != NULL );
        tabs->setDeactivated();
    }

    // validate choices to notify player of duplicates
    const bool names_ok = validateIdentChoices();
    const bool karts_ok = validateKartChoices();

    if (!names_ok || !karts_ok) return;

    // check if all players are ready
    bool allPlayersReady = true;
    for (int n=0; n<amount; n++)
    {
        if (!m_kart_widgets[n].isReady())
        {
            allPlayersReady = false;
            break;
        }
    }

    if (allPlayersReady && (!m_multiplayer || amount > 1)) allPlayersDone();
}   // playerConfirm

// ----------------------------------------------------------------------------
/**
 * Callback handling events from the kart selection menu
 */
void KartSelectionScreen::eventCallback(Widget* widget,
                                        const std::string& name,
                                        const int playerID)
{
    // don't allow changing group after someone confirmed
    if (name == "kartgroups" && !m_game_master_confirmed)
    {
        RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
        assert(tabs != NULL);
        DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
        assert(w != NULL);

        setKartsFromCurrentGroup();

        const std::string selected_kart_group =
            tabs->getSelectionIDString(PLAYER_ID_GAME_MASTER);

        UserConfigParams::m_last_used_kart_group = selected_kart_group;

        RandomGenerator random;

        const int num_players = m_kart_widgets.size();
        for (int n=0; n<num_players; n++)
        {
            // The game master is the one that can change the groups, leave
            // his focus on the tabs for others, remove focus from kart that
            // might no more exist in this tab.
            if (n != PLAYER_ID_GAME_MASTER)
                GUIEngine::focusNothingForPlayer(n);

            if (!m_kart_widgets[n].isReady())
            {
                // try to preserve the same kart for each player (except for
                // game master, since it's the one  that can change the
                // groups, so focus for this player must remain on the tabs)
                const std::string& selected_kart =
                    m_kart_widgets[n].getKartInternalName();
                if (!w->setSelection( selected_kart, n,
                                      n != PLAYER_ID_GAME_MASTER))
                {
                    // if we get here, it means one player "lost" his kart in
                    // the tab switch
                    if (UserConfigParams::logGUI())
                        std::cout << "[KartSelectionScreen] Player " << n
                           << " lost their selection when switching tabs!!!\n";

                    // Select a random kart in this case
                    const int count = w->getItems().size();
                    if (count > 0)
                    {
                        // FIXME: two players may be given the same kart by
                        // the use of random
                        const int randomID = random.get( count );

                        // select kart for players > 0 (player 0 is the one
                        // that can change the groups, so focus for player 0
                        // must remain on the tabs)
                        const bool success =
                            w->setSelection( randomID, n,
                                             n != PLAYER_ID_GAME_MASTER );
                        if (!success)
                            std::cerr << "[KartSelectionScreen] WARNING: "
                                         "setting kart of player " << n
                                      << " failed :(\n";
                    }
                    else
                    {
                        std::cerr << "[KartSelectionScreen] WARNING : 0 items "
                                     "in the ribbon\n";
                    }
                }
            }
        } // end for
    }
    else if (name == "karts")
    {
        playerConfirm(playerID);
    }
    else if (name == "back")
    {
        m_go_to_overworld_next = false; // valid once

        if (m_from_overworld)
        {
            m_from_overworld = false; // valid once
            OverWorld::enterOverWorld();
        }
        else
        {
            StateManager::get()->escapePressed();
        }
    }
    else
    {
        // Transmit to all subwidgets, maybe *they* care about this event
        const int amount = m_kart_widgets.size();
        for (int n=0; n<amount; n++)
        {
            m_kart_widgets[n].transmitEvent(widget, name, playerID);
        }

        // those events may mean that a player selection changed, so
        // validate again
        validateIdentChoices();
        validateKartChoices();
    }
}   // eventCallback

// ----------------------------------------------------------------------------

void KartSelectionScreen::setMultiplayer(bool multiplayer)
{
    m_multiplayer = multiplayer;
}   // setMultiplayer

// ----------------------------------------------------------------------------

bool KartSelectionScreen::onEscapePressed()
{
    m_go_to_overworld_next = false; // valid once

    if (m_from_overworld)
    {
        m_from_overworld = false; // valid once
        OverWorld::enterOverWorld();
        return false;
    }
    else
    {
        return true;
    }
}

// ----------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark KartSelectionScreen (private)
#endif

void KartSelectionScreen::allPlayersDone()
{
    input_manager->setMasterPlayerOnly(true);

    RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
    assert(tabs != NULL);

    std::string selected_kart_group =
        tabs->getSelectionIDString(PLAYER_ID_GAME_MASTER);

    UserConfigParams::m_last_used_kart_group = selected_kart_group;

    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    assert( w != NULL );

    const PtrVector< StateManager::ActivePlayer, HOLD >& players =
        StateManager::get()->getActivePlayers();

    // ---- Print selection (for debugging purposes)
    if(UserConfigParams::logGUI())
    {
        std::cout << "[KartSelectionScreen] " << players.size()
                  << " players :\n";
        for (int n=0; n<players.size(); n++)
        {
            std::cout << "     Player " << n << " is "
                << core::stringc(
                    players[n].getConstProfile()->getName().c_str()).c_str()
                << " on " << players[n].getDevice()->m_name << std::endl;
        }
    }

    for (int n=0; n<players.size(); n++)
    {
        StateManager::get()->getActivePlayer(n)->getProfile()
                                               ->incrementUseFrequency();
    }
    // ---- Give player info to race manager
    race_manager->setNumLocalPlayers( players.size() );

    // ---- Manage 'random kart' selection(s)
    RandomGenerator random;

    //m_kart_widgets.clearAndDeleteAll();
    //race_manager->setLocalKartInfo(0, w->getSelectionIDString());

    std::vector<ItemDescription> items = w->getItems();

    // remove the 'random' item itself
    const int item_count = items.size();
    for (int n=0; n<item_count; n++)
    {
        if (items[n].m_code_name == RANDOM_KART_ID)
        {
            items[n].m_code_name = ID_DONT_USE;
            break;
        }
    }

    // pick random karts
    const int kart_count = m_kart_widgets.size();
    for (int n = 0; n < kart_count; n++)
    {
        std::string selected_kart = m_kart_widgets[n].m_kartInternalName;

        if (selected_kart == RANDOM_KART_ID)
        {
            // don't select an already selected kart
            int randomID;
            // to prevent infinite loop in case they are all locked
            int count = 0;
            bool done = false;
            do
            {
                randomID = random.get(item_count);
                if (items[randomID].m_code_name != ID_DONT_USE &&
                    !StringUtils::startsWith(items[randomID].m_code_name,
                                             ID_LOCKED))
                {
                    selected_kart = items[randomID].m_code_name;
                    done = true;
                }
                items[randomID].m_code_name = ID_DONT_USE;
                count++;
                if (count > 100) return;
            } while (!done);
        }
        else
        {
            // mark the item as taken
            for (int i=0; i<item_count; i++)
            {
                if (items[i].m_code_name ==
                        m_kart_widgets[n].m_kartInternalName)
                {
                    items[i].m_code_name = ID_DONT_USE;
                    break;
                }
            }
        }
        // std::cout << "selection=" << selection.c_str() << std::endl;

        race_manager->setLocalKartInfo(n, selected_kart);
    }

    // ---- Switch to assign mode
    input_manager->getDeviceList()->setAssignMode(ASSIGN);

    if (!m_multiplayer)
    {
        input_manager->getDeviceList()
                 ->setSinglePlayer( StateManager::get()->getActivePlayer(0) );
    }
    else
    {
        input_manager->getDeviceList()->setSinglePlayer( NULL );
    }

    // ---- Go to next screen or return to overworld
    if (m_from_overworld || m_go_to_overworld_next)
    {
        m_from_overworld = false; // valid once
        m_go_to_overworld_next = false;
        OverWorld::enterOverWorld();
    }
    else
    {
        StateManager::get()->pushScreen( RaceSetupScreen::getInstance() );
    }
}   // allPlayersDone

// ----------------------------------------------------------------------------

bool KartSelectionScreen::validateIdentChoices()
{
    bool ok = true;

    const int amount = m_kart_widgets.size();

    // reset all marks, we'll re-add them next if errors are still there
    for (int n=0; n<amount; n++)
    {
        // first check if the player name widget is still there, it won't
        // be for those that confirmed
        if (m_kart_widgets[n].m_player_ident_spinner != NULL)
        {
            m_kart_widgets[n].m_player_ident_spinner->markAsCorrect();

            // verify internal consistency in debug mode
            if (m_multiplayer)
            {
                assert( m_kart_widgets[n].getAssociatedPlayer()->getProfile() ==
                       UserConfigParams::m_all_players.get(m_kart_widgets[n]
                                        .m_player_ident_spinner->getValue()) );
            }
        }
    }

    // perform actual checking
    for (int n=0; n<amount; n++)
    {
        // skip players that took a guest account, they can be many on the
        // same identity in this case
        if (m_kart_widgets[n].getAssociatedPlayer()->getProfile()
                                                   ->isGuestAccount())
        {
            continue;
        }

        // check if another kart took the same identity as the current one
        for (int m=n+1; m<amount; m++)
        {

            // check if 2 players took the same name
            if (m_kart_widgets[n].getAssociatedPlayer()->getProfile() ==
                m_kart_widgets[m].getAssociatedPlayer()->getProfile())
            {
                // two players took the same name. check if one is ready
                if (!m_kart_widgets[n].isReady() &&
                    m_kart_widgets[m].isReady())
                {
                    // player m is ready, so player n should not choose
                    // this name
                    m_kart_widgets[n].m_player_ident_spinner
                                      ->markAsIncorrect();
                }
                else if (m_kart_widgets[n].isReady() &&
                        !m_kart_widgets[m].isReady())
                {
                    // player n is ready, so player m should not
                    // choose this name
                    m_kart_widgets[m].m_player_ident_spinner
                                     ->markAsIncorrect();
                }
                else if (m_kart_widgets[n].isReady() &&
                         m_kart_widgets[m].isReady())
                {
                    // it should be impossible for two players to confirm
                    // they're ready with the same name
                    assert(false);
                }

                ok = false;
            }
        } // end for
    }

    return ok;
}   // validateIdentChoices

// -----------------------------------------------------------------------------

bool KartSelectionScreen::validateKartChoices()
{
    bool ok = true;

    const int amount = m_kart_widgets.size();

    // reset all marks, we'll re-add them next if errors are still there
    for (int n=0; n<amount; n++)
    {
        m_kart_widgets[n].m_model_view->unsetBadge(BAD_BADGE);
    }

    // Check if we have enough karts for everybody. If there are more
    // players than karts then just allow duplicates
    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    assert( w != NULL );
    const int availableKartCount = w->getItems().size();
    if (amount > availableKartCount) return true;

    // Check everyone for duplicates
    for (int n=0; n<amount; n++)
    {
        for (int m=n+1; m<amount; m++)
        {
            // check if 2 players took the same name
            if (sameKart(m_kart_widgets[n], m_kart_widgets[m]))
            {
                if (UserConfigParams::logGUI())
                {
                    printf("[KartSelectionScreen] Kart conflict!!\n");
                    std::cout << "    Player " << n << " chose "
                              << m_kart_widgets[n].getKartInternalName()
                              << std::endl;
                    std::cout << "    Player " << m << " chose "
                              << m_kart_widgets[m].getKartInternalName()
                              << std::endl;
                }

                // two players took the same kart. check if one is ready
                if (!m_kart_widgets[n].isReady() &&
                     m_kart_widgets[m].isReady())
                {
                    if (UserConfigParams::logGUI())
                        std::cout << "    --> Setting red badge on player "
                                  << n << std::endl;

                    // player m is ready, so player n should not choose
                    // this name
                    m_kart_widgets[n].m_model_view->setBadge(BAD_BADGE);
                }
                else if (m_kart_widgets[n].isReady() &&
                        !m_kart_widgets[m].isReady())
                {
                    if (UserConfigParams::logGUI())
                        std::cout << "    --> Setting red badge on player "
                                  << m << std::endl;

                    // player n is ready, so player m should not
                    // choose this name
                    m_kart_widgets[m].m_model_view->setBadge(BAD_BADGE);
                }
                else if (m_kart_widgets[n].isReady() &&
                         m_kart_widgets[m].isReady())
                {
                    // it should be impossible for two players to confirm
                    // they're ready with the same kart
                    assert(false);
                }

                // we know it's not ok (but don't stop right now, all bad
                // ones need red badges)
                ok = false;
            }
        } // end for
    }

    return ok;

}   // validateKartChoices

// ----------------------------------------------------------------------------

void KartSelectionScreen::renumberKarts()
{
    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    assert( w != NULL );
    Widget* fullarea = getWidget("playerskarts");
    const int splitWidth = fullarea->m_w / m_kart_widgets.size();

    for (int n=0; n < m_kart_widgets.size(); n++)
    {
        m_kart_widgets[n].setPlayerID(n);
        m_kart_widgets[n].move( fullarea->m_x + splitWidth*n, fullarea->m_y,
                                splitWidth, fullarea->m_h );
    }

    w->updateItemDisplay();
}   // renumberKarts

// ----------------------------------------------------------------------------

void KartSelectionScreen::setKartsFromCurrentGroup()
{
    RibbonWidget* tabs = getWidget<RibbonWidget>("kartgroups");
    assert(tabs != NULL);

    std::string selected_kart_group =
        tabs->getSelectionIDString(PLAYER_ID_GAME_MASTER);

    UserConfigParams::m_last_used_kart_group = selected_kart_group;

    // This can happen if addons are removed so that also the previously
    // selected kart group is removed. In this case, select the
    // 'standard' group
    if (selected_kart_group != ALL_KART_GROUPS_ID &&
        !kart_properties_manager->getKartsInGroup(selected_kart_group).size())
    {
        selected_kart_group = DEFAULT_GROUP_NAME;
    }

    DynamicRibbonWidget* w = getWidget<DynamicRibbonWidget>("karts");
    w->clearItems();

    int usableKartCount = 0;

    if (selected_kart_group == ALL_KART_GROUPS_ID)
    {
        const int kart_amount = kart_properties_manager->getNumberOfKarts();

        for (int n=0; n<kart_amount; n++)
        {
            const KartProperties* prop =
                kart_properties_manager->getKartById(n);
            if (unlock_manager->getCurrentSlot()->isLocked(prop->getIdent()))
            {
                w->addItem(
                    _("Locked : solve active challenges to gain access "
                      "to more!"),
                    ID_LOCKED+prop->getIdent(),
                    prop->getAbsoluteIconFile(), LOCKED_BADGE,
                           IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
            }
            else
            {
                w->addItem(translations->fribidize(prop->getName()),
                           prop->getIdent(),
                           prop->getAbsoluteIconFile(), 0,
                           IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
                usableKartCount++;
            }
        }
    }
    else if (selected_kart_group != RibbonWidget::NO_ITEM_ID)
    {
        std::vector<int> group =
            kart_properties_manager->getKartsInGroup(selected_kart_group);
        const int kart_amount = group.size();


        for (int n=0; n<kart_amount; n++)
        {
            const KartProperties* prop =
                kart_properties_manager->getKartById(group[n]);
            const std::string &icon_path = prop->getAbsoluteIconFile();

            if (unlock_manager->getCurrentSlot()->isLocked(prop->getIdent()))
            {
                w->addItem(
                    _("Locked : solve active challenges to gain access "
                      "to more!"),
                    ID_LOCKED+prop->getIdent(), icon_path, LOCKED_BADGE,
                    IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
            }
            else
            {
                w->addItem(translations->fribidize(prop->getName()),
                            prop->getIdent(),
                           icon_path, 0,
                           IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE);
                usableKartCount++;
            }
        }
    }

    // add random
    if (usableKartCount > 1)
    {
        w->addItem(_("Random Kart"), RANDOM_KART_ID, "/gui/random_kart.png");
    }

    w->updateItemDisplay();
}

// ----------------------------------------------------------------------------

#if 0
#pragma mark -
#endif

// FIXME : clean this mess, this file should not contain so many classes
// spread all over the place

EventPropagation FocusDispatcher::focused(const int playerID)
{
    if (!m_is_initialised) return EVENT_LET;

    if(UserConfigParams::logGUI())
        std::cout << "[KartSelectionScreen] FocusDispatcher focused by player "
                  << playerID << std::endl;

    // since this screen is multiplayer, redirect focus to the right widget
    const int amount = m_parent->m_kart_widgets.size();
    for (int n=0; n<amount; n++)
    {
        if (m_parent->m_kart_widgets[n].getPlayerID() == playerID)
        {
            // If player is done, don't do anything with focus
            if (m_parent->m_kart_widgets[n].isReady())
                return GUIEngine::EVENT_BLOCK;

            //std::cout << "--> Redirecting focus for player " << playerID
            //          << " from FocusDispatcher "  <<
            //             " (ID " << m_element->getID() <<
            //             ") to spinner " << n << " (ID " <<
            //             m_parent->m_kart_widgets[n].m_player_ident_spinner
            //             ->getIrrlichtElement()->getID() <<
            //             ")" << std::endl;

            m_parent->m_kart_widgets[n].m_player_ident_spinner
                                        ->setFocusForPlayer(playerID);


            return GUIEngine::EVENT_BLOCK;
        }
    }

    //std::cerr << "[KartSelectionScreen] WARNING: the focus dispatcher can't
    // find the widget for player " << playerID << "!\n";
    //assert(false);
    return GUIEngine::EVENT_LET;
}   // focused

