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

#include "graphics/irr_driver.hpp"
#include "guiengine/abstract_top_level_container.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/scalable_font.hpp"
#include "guiengine/widget.hpp"
#include "io/file_manager.hpp"
#include "utils/ptr_vector.hpp"
#include <iostream>

#include <IGUIElement.h>

using namespace GUIEngine;

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

AbstractTopLevelContainer::AbstractTopLevelContainer()
{
    m_first_widget = NULL;
    m_last_widget = NULL;
}

void AbstractTopLevelContainer::addWidgetsRecursively(
                                                    PtrVector<Widget>& widgets,
                                                    Widget* parent)
{
    const unsigned short widgets_amount = widgets.size();
    
    // ------- add widgets
    for (int n=0; n<widgets_amount; n++)
    {
        if (widgets[n].getType() == WTYPE_DIV)
        {
            widgets[n].add(); // Will do nothing, but will maybe reserve an ID
            addWidgetsRecursively(widgets[n].m_children, &widgets[n]);
        }
        else
        {
            // warn if widget has no dimensions (except for ribbons and icons,
            // where it is normal since it adjusts to its contents)
            if ((widgets[n].m_w < 1 || widgets[n].m_h < 1) &&
                widgets[n].getType() != WTYPE_RIBBON &&
                widgets[n].getType() != WTYPE_ICON_BUTTON &&
                widgets[n].getType() != WTYPE_SPACER)
            {
                std::cerr << "/!\\ Warning /!\\ : widget " 
                          << widgets[n].m_properties[PROP_ID].c_str()
                          << " of type " << widgets[n].getType() 
                          << " has no dimensions" << std::endl;
            }
            
            if (widgets[n].m_x == -1 || widgets[n].m_y == -1)
            {
                std::cerr << "/!\\ Warning /!\\ : widget " 
                          << widgets[n].m_properties[PROP_ID].c_str()
                          << " of type " << widgets[n].getType()
                          << " has no position" << std::endl;
            }
            
            widgets[n].add();
        }
        
    } // next widget
    
}   // addWidgetsRecursively

// ----------------------------------------------------------------------------

bool isMyChildHelperFunc(const PtrVector<Widget>* within, const Widget* widget)
{
    if (within->size() == 0) return false;
    
    if (within->contains(widget))
    {
        return true;
    }
    
    const int count = within->size();
    for (int n=0; n<count; n++)
    {
        if (isMyChildHelperFunc(&within->get(n)->getChildren(), widget))
        {
            return true;
        }
    }
    
    return false;
}

bool AbstractTopLevelContainer::isMyChild(Widget* widget) const
{
    return isMyChildHelperFunc(&m_widgets, widget);
}



Widget* AbstractTopLevelContainer::getWidget(const char* name)
{
    return getWidget(name, &m_widgets);
}   // getWidget

// -----------------------------------------------------------------------------

Widget* AbstractTopLevelContainer::getWidget(const int id)
{
    return getWidget(id, &m_widgets);
}   // getWidget

// -----------------------------------------------------------------------------

Widget* AbstractTopLevelContainer::getWidget(const char* name, 
                                             PtrVector<Widget>* within_vector)
{
    const unsigned short widgets_amount = within_vector->size();
    
    for(int n=0; n<widgets_amount; n++)
    {
        Widget& widget = (*within_vector)[n];
        
        if (widget.m_properties[PROP_ID] == name) return &widget;
        
        if (widget.searchInsideMe() && widget.m_children.size() > 0)
        {
            Widget* el = getWidget(name, &(widget.m_children));
            if(el != NULL) return el;
        }
    } // next
    
    return NULL;
}   // getWidget

// -----------------------------------------------------------------------------

Widget* AbstractTopLevelContainer::getWidget(const int id, 
                                             PtrVector<Widget>* within_vector)
{
    const unsigned short widgets_amount = within_vector->size();
    
    for (int n=0; n<widgets_amount; n++)
    {
        Widget& widget = (*within_vector)[n];
        
        if (widget.m_element != NULL && 
            widget.getIrrlichtElement()->getID() == id) return &widget;
        
        if (widget.searchInsideMe() && widget.getChildren().size() > 0)
        {
            // std::cout << "widget = <" 
            //           << widget.m_properties[PROP_ID].c_str()
            //           << ">  widget.m_children.size()=" 
            // << widget.m_children.size() << std::endl;
            Widget* el = getWidget(id, &(widget.m_children));
            if(el != NULL) return el;
        }
    } // next
    
    return NULL;
}   // getWidget

// -----------------------------------------------------------------------------

Widget* AbstractTopLevelContainer::getFirstWidget(
                                              PtrVector<Widget>* within_vector)
{
    if (m_first_widget != NULL) return m_first_widget;
    if (within_vector == NULL) within_vector = &m_widgets;
    
    for (int i = 0; i < within_vector->size(); i++)
    {
        if (!within_vector->get(i)->m_focusable) continue;
        
        // if container, also checks children 
        // (FIXME: don't hardcode which types to avoid descending into)
        if (within_vector->get(i)->m_children.size() > 0 &&
            within_vector->get(i)->getType() != WTYPE_RIBBON &&
            within_vector->get(i)->getType() != WTYPE_SPINNER)
        {
            Widget* w = getFirstWidget(&within_vector->get(i)->m_children);
            if (w != NULL) return w;
        }
        
        Widget* item = within_vector->get(i);
        if (item->getIrrlichtElement() == NULL ||
            item->getIrrlichtElement()->getTabOrder() == -1 ||
             /* non-tabbing items are given such IDs */ 
            item->getIrrlichtElement()->getTabOrder() >= 1000 ||
            !item->m_focusable)
        {
            continue;
        }
        
        return item;
    }
    return NULL;
}   // getFirstWidget

// -----------------------------------------------------------------------------

Widget* AbstractTopLevelContainer::getLastWidget(
                                              PtrVector<Widget>* within_vector)
{
    if (m_last_widget != NULL) return m_last_widget;
    if (within_vector == NULL) within_vector = &m_widgets;
    
    for (int i = within_vector->size()-1; i >= 0; i--)
    {
        if (!within_vector->get(i)->m_focusable) continue;
        
        // if container, also checks children
        if (within_vector->get(i)->getChildren().size() > 0 &&
            within_vector->get(i)->getType() != WTYPE_RIBBON &&
            within_vector->get(i)->getType() != WTYPE_SPINNER)
        {
            Widget* w = getLastWidget(&within_vector->get(i)->m_children);
            if (w != NULL) return w;
        }
        
        Widget* item = within_vector->get(i);
        IGUIElement* elem = item->getIrrlichtElement();
        
        if (elem == NULL ||
            elem->getTabOrder() == -1 ||
            !Widget::isFocusableId(elem->getTabOrder()) ||
            !item->m_focusable)
        {
            continue;
        }
        
        return item;
    }
    return NULL;
}   // getLastWidget

// ----------------------------------------------------------------------------

/**
 * Called when screen is removed. This means all irrlicht widgets this object has pointers
 * to are now gone. Set all references to NULL to avoid problems.
 */
void AbstractTopLevelContainer::elementsWereDeleted(PtrVector<Widget>* within_vector)
{
    if (within_vector == NULL) within_vector = &m_widgets;
    const unsigned short widgets_amount = within_vector->size();
    
    for (int n=0; n<widgets_amount; n++)
    {
        Widget& widget = (*within_vector)[n];
        
        widget.elementRemoved();
        
        if (widget.m_children.size() > 0)
        {
            elementsWereDeleted( &(widget.m_children) );
        }
    }
}   // elementsWereDeleted
