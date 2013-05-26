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

#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include "input/input_device.hpp"
#include "config/device_config.hpp"
#include "utils/no_copy.hpp"
#include "utils/ptr_vector.hpp"

#include <irrArray.h>

enum PlayerAssignMode
{
    NO_ASSIGN,  //!< react to all devices
    DETECT_NEW, //!< notify the manager when an inactive device is being asked to activate with fire
    ASSIGN      //!< only react to assigned devices
};

/**
 * \brief This class holds the list of all known devices (ands their configurations), as well as the
 * list of currently plugged (used) devices.
 * It thus takes care of finding to which device any given
 * input belongs, and what action each keypress is bound to, if any (and, since each device is
 * associated to a player, it also finds which player triggered this action)
 * These input mapping capabilities should *only* be used through the InputManager, not directly.
 *
 * The device manager starts in "no-assign" mode, which means no input configuration is associated
 * to any player. So all devices will react. This is used in menus before player set-up is done.
 * Switching back to no-assign mode will also clear anything in devices that was associated with
 * players in assign mode.
 *
 * \ingroup input
 */
class DeviceManager: public NoCopy
{
private:

    PtrVector<KeyboardDevice, HOLD>    m_keyboards;
    PtrVector<GamePadDevice, HOLD>     m_gamepads;
    PtrVector<KeyboardConfig, HOLD>    m_keyboard_configs;
    PtrVector<GamepadConfig, HOLD>     m_gamepad_configs;

    /** The list of all joysticks that were found and activated. */
    core::array<SJoystickInfo>          m_irrlicht_gamepads;
    InputDevice*                        m_latest_used_device;
    PlayerAssignMode                    m_assign_mode;

    /**
      * Helper method, only used internally. Takes care of analyzing gamepad input.
      *
      * \param[out]  player     Which player this input belongs to (only set in ASSIGN mode)
      * \param[out]  action     Which action is related to this input trigger
      * \param       mode       Used to determine whether to determine menu actions or game actions
      * \return                 The device to which this input belongs
      */
    InputDevice *mapGamepadInput      ( Input::InputType type,
                                        int deviceID,
                                        int btnID,
                                        int axisDir,
                                        int value,
                                        InputManager::InputDriverMode mode,
                                        StateManager::ActivePlayer **player /* out */,
                                        PlayerAction *action /* out */);

    /** Will be non-null in single-player mode */
    StateManager::ActivePlayer* m_single_player;
    
    /**
     * Helper method, only used internally. Takes care of analyzing keyboard input.
     *
     * \param[out]  player     Which player this input belongs to (only set in ASSIGN mode)
     * \param[out]  action     Which action is related to this input trigger
     * \param       mode       Used to determine whether to determine menu actions or game actions
     * \return                 The device to which this input belongs
     */
    InputDevice *mapKeyboardInput     ( int btnID, InputManager::InputDriverMode mode,
                                        StateManager::ActivePlayer **player /* out */,
                                        PlayerAction *action /* out */);
    /** If this is flag is set the next fire event (if the fire key is not 
     *  mapped to anything else) will be mapped to 'select'. This is used
     *  in the kart select GUI to support the old way of adding players by
     *  pressing fire. */
    bool m_map_fire_to_select;

    bool deserialize();
    void shutdown();

public:


    DeviceManager();
        
    // ---- Assign mode ----
    PlayerAssignMode    getAssignMode() const               { return m_assign_mode; }
    void                setAssignMode(const PlayerAssignMode assignMode);

    // ---- Gamepads ----
    void addGamepad(GamePadDevice* d);
    int getGamePadAmount() const                            { return m_gamepads.size(); }
    int getGamePadConfigAmount() const                      { return m_gamepad_configs.size(); }
    GamePadDevice*      getGamePad(const int i)             { return m_gamepads.get(i); }
    GamepadConfig*      getGamepadConfig(const int i)       { return m_gamepad_configs.get(i); }
    GamePadDevice*      getGamePadFromIrrID(const int i);
    void                clearGamepads()                     { m_gamepads.clearAndDeleteAll();  }
    /** Returns the keyboard that has a binding for this button, or NULL if none */
    bool                getConfigForGamepad(const int sdl_id, const core::stringc& pname, GamepadConfig **config);    
    
    // ---- Keyboard(s) ----
    void addEmptyKeyboard();
    void addKeyboard(KeyboardDevice* d);
    void                clearKeyboard()                     { m_keyboards.clearAndDeleteAll(); }
    int                 getKeyboardAmount()                 { return m_keyboards.size(); }
    int                 getKeyboardConfigAmount() const     { return m_keyboard_configs.size(); }
    KeyboardDevice*     getKeyboard(const int i)            { return m_keyboards.get(i); }
    KeyboardConfig*     getKeyboardConfig(const int i)      { return m_keyboard_configs.get(i); }
    KeyboardDevice*     getKeyboardFromBtnID(const int btnID);


    /**
      * \brief Delete the given config and removes DeviceManager references to it.
      */
    bool deleteConfig(DeviceConfig* config);
       
    /** Given some input, finds to which device it belongs and, using the corresponding device object,
      * maps this input to the corresponding player and game action. 
      *
      * \return false if player/action could not be set.
      * \note   Special case : can return 'true' but set action to PA_BEFORE_FIRST if the input was used but
      *         is not associated to an action and a player
      *
      * \param mode used to determine whether to map game actions or menu actions
      */
    bool translateInput( Input::InputType type,
                         int deviceID,
                         int btnID,
                         int axisDir,
                         int value,
                         InputManager::InputDriverMode mode,
                         StateManager::ActivePlayer** player /* out */,
                         PlayerAction* action /* out */ );

	void                clearLatestUsedDevice();
    InputDevice*        getLatestUsedDevice();
    bool initialize();
    void serialize();
    
    StateManager::ActivePlayer* getSinglePlayer()       { return m_single_player; }
    void setSinglePlayer(StateManager::ActivePlayer* p) { m_single_player = p;    }
    // ------------------------------------------------------------------------
    /** Sets or reset the 'map fire to select' option.
     */
    void mapFireToSelect(bool v) {m_map_fire_to_select = v; }
    
};   // DeviceManager


#endif

