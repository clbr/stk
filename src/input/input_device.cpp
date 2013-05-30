
#include "config/device_config.hpp"
#include "guiengine/abstract_state_manager.hpp"
#include "input/input.hpp"
#include "input/input_device.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/controller/player_controller.hpp"
#include "modes/world.hpp"
#include "race/race_manager.hpp"
#include "states_screens/state_manager.hpp"


InputDevice::InputDevice()
{
    m_player = NULL;
    m_configuration = NULL;
}
// ----------------------------------------------------------------------------
/**
  * Sets which players uses this device; or pass NULL to say no player uses it.
  */
void InputDevice::setPlayer(StateManager::ActivePlayer* owner)
{
    m_player = owner;
}

#if 0
#pragma mark -
#pragma mark Keyboard
#endif

// ----------------------------------------------------------------------------
KeyboardDevice::KeyboardDevice(KeyboardConfig *configuration)
{
    m_configuration = configuration;
    m_type = DT_KEYBOARD;
    m_name = "Keyboard";
    m_player = NULL;
}   // KeyboardDevice

// ----------------------------------------------------------------------------
KeyboardDevice::KeyboardDevice()
{
    m_configuration = new KeyboardConfig();
    m_type = DT_KEYBOARD;
    m_player = NULL;
}   // KeyboardDevice

// ----------------------------------------------------------------------------

bool KeyboardDevice::processAndMapInput(const int id,
                                        InputManager::InputDriverMode mode,
                                        PlayerAction* action /* out */)
{
    if (mode == InputManager::INGAME)
    {
        return m_configuration->getGameAction(Input::IT_KEYBOARD, id, 0,
                                              action);
    }
    else
    {
        // bindings can only be accessed in game and menu modes
        assert(mode == InputManager::MENU);
        return m_configuration->getMenuAction(Input::IT_KEYBOARD, id, 0,
                                              action);
    }
}   // processAndMapInput

// ============================================================================

#if 0
#pragma mark -
#pragma mark Gamepad
#endif


GamePadDevice::GamePadDevice(const int irrIndex, const std::string name,
                             const int axis_count, const int button_count,
                             GamepadConfig *configuration)
{
    m_type                  = DT_GAMEPAD;
    m_deadzone              = DEADZONE_JOYSTICK;
    m_prevAxisDirections    = NULL;
    m_configuration         = configuration;
    m_axis_count            = axis_count;
    m_prevAxisDirections    = new Input::AxisDirection[axis_count];
    m_prevAxisValue         = new int[axis_count];
    m_axis_ok               = new bool[axis_count];
    m_button_count          = button_count;
    m_index                 = irrIndex;
    m_name                  = name;

    for (int i = 0; i < axis_count; i++)
    {
        m_prevAxisDirections[i] = Input::AD_NEUTRAL;
        m_prevAxisValue[i] = -1;
        m_axis_ok[i] = false;
    }

    for(int n=0; n<SEvent::SJoystickEvent::NUMBER_OF_BUTTONS; n++)
        m_buttonPressed[n] = false;
}   // GamePadDevice

// ----------------------------------------------------------------------------
/** Destructor for GamePadDevice.
 */
GamePadDevice::~GamePadDevice()
{
    delete[] m_prevAxisDirections;
    delete[] m_prevAxisValue;
    delete[] m_axis_ok;

    // FIXME - any need to close devices?
}   // ~GamePadDevice

// ----------------------------------------------------------------------------

bool GamePadDevice::isButtonPressed(const int i)
{
    return m_buttonPressed[i];
}   // isButtonPressed

// ----------------------------------------------------------------------------

void GamePadDevice::setButtonPressed(const int i, bool isButtonPressed)
{
    m_buttonPressed[i] = isButtonPressed;
}   // setButtonPressed

// ----------------------------------------------------------------------------

void GamePadDevice::resetAxisDirection(const int axis,
                                       Input::AxisDirection direction,
                                       StateManager::ActivePlayer* player)
{
    // ignore this while in menus
    if (StateManager::get()->getGameState() != GUIEngine::GAME) return;

    AbstractKart* pk = player->getKart();
    if (pk == NULL)
    {
        fprintf(stderr, "Error, trying to reset axis for an unknown player\n");
        return;
    }

    for(int n=0; n<PA_COUNT; n++)
    {
        Binding& bind = m_configuration->getBinding(n);
        if(bind.getType() == Input::IT_STICKMOTION &&
           bind.getId() == axis &&
           bind.getDirection()== direction &&
           pk->getController() != NULL)
        {
            ((PlayerController*)(pk->getController()))
                                                  ->action((PlayerAction)n, 0);
            return;
        }
    }

}   // resetAxisDirection

// ----------------------------------------------------------------------------

bool GamePadDevice::processAndMapInput(Input::InputType type, const int id,
                                       const int value,
                                       InputManager::InputDriverMode mode,
                                       StateManager::ActivePlayer* player,
                                       PlayerAction* action /* out */)
{
    if (!m_configuration->isEnabled()) return false;

    bool success = false;
    if(m_prevAxisDirections == NULL) return false; // device not open

    if (type == Input::IT_STICKMOTION)
    {
        // this gamepad doesn't even have that many axes
        if (id >= m_axis_count) return false;

        if (player != NULL)
        {
            // going to negative from positive
            if (value < 0 && m_prevAxisDirections[id] == Input::AD_POSITIVE)
            {
                //  set positive id to 0
                resetAxisDirection(id, Input::AD_POSITIVE, player);
            }
            // going to positive from negative
            else if (value > 0 &&
                     m_prevAxisDirections[id] == Input::AD_NEGATIVE)
            {
                //  set negative id to 0
                resetAxisDirection(id, Input::AD_NEGATIVE, player);
            }
        }

        if     (value > 0) m_prevAxisDirections[id] = Input::AD_POSITIVE;
        else if(value < 0) m_prevAxisDirections[id] = Input::AD_NEGATIVE;

        if (!m_axis_ok[id])
        {
            if (m_prevAxisValue[id] == -1)
            {
                // first value we get from this axis
                m_prevAxisValue[id] = value;
            }
            else if (m_prevAxisValue[id] != value)
            {
                // second different value we get from this axis, consider it OK
                m_axis_ok[id] = true;
            }
        }

        // check if within deadzone
        if(value > -m_deadzone && value < m_deadzone && player != NULL)
        {
            // Axis stands still: This is reported once for digital axes and
            // can be called multipled times for analog ones. Uses the
            // previous direction in which the id was triggered to
            // determine which one has to be brought into the released
            // state. This allows us to regard two directions of an id
            // as completely independent input variants (as if they where
            // two buttons).

            if(m_prevAxisDirections[id] == Input::AD_NEGATIVE)
            {
                // set negative id to 0
                resetAxisDirection(id, Input::AD_NEGATIVE, player);
            }
            else if(m_prevAxisDirections[id] == Input::AD_POSITIVE)
            {
                // set positive id to 0
                resetAxisDirection(id, Input::AD_POSITIVE, player);
            }
            m_prevAxisDirections[id] = Input::AD_NEUTRAL;

            return false;
        }

        // If axis did not send proper values yet, ignore it.
        if (!m_axis_ok[id]) return false;
    }

    if (m_configuration != NULL)
    {
        if (mode == InputManager::INGAME)
        {
            success = m_configuration->getGameAction(type, id, value, action);
        }
        else if (abs(value) > Input::MAX_VALUE/2)
        {
            // bindings can only be accessed in game and menu modes
            assert(mode == InputManager::MENU);
            success = m_configuration->getMenuAction(type, id, value, action);
        }
    }
    else
    {
        fprintf(stderr, "processAndMapInput() called on improperly "
                        "initialized GamePadDevice\n");
        abort();
    }

    return success;
}   // processAndMapInput

// ----------------------------------------------------------------------------
