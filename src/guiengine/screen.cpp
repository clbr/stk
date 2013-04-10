//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010 Marianne Gagnon
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


#include "guiengine/screen.hpp"

#include "io/file_manager.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/layout_manager.hpp"
#include "guiengine/modaldialog.hpp"
#include "guiengine/widget.hpp"
#include "modes/world.hpp"
#include "states_screens/state_manager.hpp"

#include <irrlicht.h>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;
using namespace GUIEngine;

#include <assert.h>

// -----------------------------------------------------------------------------
/** 
 * \brief          Creates a screen populated by the widgets described
 *                 in a STK GUI file.
 * \param filename Name of the XML file describing the screen.
 *                 This is NOT a path.
 *                 The passed file name will be searched for in the
 *                 STK data/gui directory.
 */
Screen::Screen(const char* file, bool pause_race)
{
    m_magic_number   = 0xCAFEC001;

    m_filename       = file;
    m_throttle_FPS   = true;
    m_render_3d      = false;
    m_loaded         = false;
    m_pause_race     = pause_race;
}   // Screen

// -----------------------------------------------------------------------------
/** \brief Creates a dummy incomplete object; only use to override behaviour
 *         in sub-class.
 */
Screen::Screen(bool pause_race)
{
    m_magic_number = 0xCAFEC001;

    m_loaded       = false;
    m_render_3d    = false;
    m_pause_race   = pause_race;
}   // Screen

// -----------------------------------------------------------------------------

Screen::~Screen()
{
    assert(m_magic_number == 0xCAFEC001);
    m_magic_number = 0xDEADBEEF;
}

// -----------------------------------------------------------------------------
/** Initialisation before the object is displayed. If necessary this function
 *  will pause the race if it is running (i.e. world exists). While only some
 *  of the screen can be shown during the race (via the in-game menu you
 *  can get the options screen and the help screens only). This is used by
 *  the RaceResultGUI to leave the race running (for the end animation) while
 *  the results are being shown.
 */
void Screen::init()
{
    if(m_pause_race && World::getWorld())
        World::getWorld()->schedulePause(World::IN_GAME_MENU_PHASE);
}   // init

// -----------------------------------------------------------------------------
/** Prepares removal of this screen. If necessary this will unpause the
 *  race (so this means that if you have several consecutive screens while
 *  the race is running the race will be unpaused and paused when switching
 *  from one screen to the next.
 */
void Screen::tearDown()
{
    if(m_pause_race && World::getWorld())
        World::getWorld()->scheduleUnpause();
}   // tearDown

// -----------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Load/Init
#endif

/** \brief loads this Screen from the file passed to the constructor */
void Screen::loadFromFile()
{
    assert(m_magic_number == 0xCAFEC001);
    
    IXMLReader* xml = file_manager->createXMLReader( (file_manager->getGUIDir() + m_filename).c_str() );
    if (xml == NULL)
    {
        fprintf(stderr, "Cannot open file %s\n", m_filename.c_str());
        assert(false);
        return;
    }
    
    parseScreenFileDiv(xml, m_widgets);
    m_loaded = true;
    calculateLayout();
    
    // invoke callback so that the class deriving from Screen is aware of this event
    loadedFromFile();
    
    delete xml;
}   // loadFromFile

// -----------------------------------------------------------------------------
/** Next time this menu needs to be shown, don't use cached values,
 *  re-calculate everything. (useful e.g. on reschange, when sizes have changed
 *  and must be re-calculated)
 */
void Screen::unload()
{
    assert(m_magic_number == 0xCAFEC001);
    Widget* w;
    for_in (w, m_widgets)
    {
        assert(w->m_magic_number == 0xCAFEC001);
    }
    
    m_loaded = false;
    m_widgets.clearAndDeleteAll();
    
    // invoke callback so that the class deriving from Screen is aware of this event
    unloaded();
}   // unload

// -----------------------------------------------------------------------------
/** \brief Called after all widgets have been added. namely expands layouts
 *         into absolute positions.
 */
void Screen::calculateLayout()
{
    assert(m_magic_number == 0xCAFEC001);
    // build layout
    LayoutManager::calculateLayout( m_widgets, this );
}   // calculateLayout

// -----------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Adding/Removing widgets
#endif

/** \brief Adds the IrrLicht widgets corresponding to this screen to the
 *         IGUIEnvironment.
 */
void Screen::addWidgets()
{
    assert(m_magic_number == 0xCAFEC001);
    if (!m_loaded) loadFromFile();
    
    addWidgetsRecursively( m_widgets );

    //std::cout << "*****ScreenAddWidgets " << m_filename.c_str() << " : focusing the first widget*****\n";
    
    // select the first widget (for first players only; if other players need some focus the Screen must provide it).
    Widget* w = getFirstWidget();
    //std::cout << "First widget is " << (w == NULL ? "null" : w->m_properties[PROP_ID].c_str()) << std::endl;
    if (w != NULL) w->setFocusForPlayer( PLAYER_ID_GAME_MASTER );
    else           fprintf(stderr, "Couldn't select first widget, NULL was returned\n");
}   // addWidgets

// -----------------------------------------------------------------------------
/** \brief Can be used for custom purposes for which the load-screen-from-XML
 *         code won't make it.
 */
void Screen::manualAddWidget(Widget* w)
{
    assert(m_magic_number == 0xCAFEC001);
    m_widgets.push_back(w);
}   // manualAddWidget

// -----------------------------------------------------------------------------
/** \brief Can be used for custom purposes for which the load-screen-from-XML
 *         code won't make it. */
void Screen::manualRemoveWidget(Widget* w)
{
    assert(m_magic_number == 0xCAFEC001);
#ifdef DEBUG
    if(!m_widgets.contains(w))
    {
        fprintf(stderr, "Widget '%d' not found in screen when removing.\n",
                w->m_id);
        fprintf(stderr, "This can be ignored, but is probably wrong.\n");
    }
#endif
    m_widgets.remove(w);
}   // manualRemoveWidget

// -----------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Getters
#endif

// -----------------------------------------------------------------------------
/** \brief Implementing method from AbstractTopLevelContainer */
int Screen::getWidth()
{
    core::dimension2d<u32> frame_size = GUIEngine::getDriver()->getCurrentRenderTargetSize();
    return frame_size.Width;
}

// -----------------------------------------------------------------------------
/** \brief Implementing method from AbstractTopLevelContainer */
int Screen::getHeight()
{
    core::dimension2d<u32> frame_size = GUIEngine::getDriver()->getCurrentRenderTargetSize();
    return frame_size.Height;
}

// -----------------------------------------------------------------------------

