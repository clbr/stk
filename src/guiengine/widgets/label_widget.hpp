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



#ifndef HEADER_LABEL_HPP
#define HEADER_LABEL_HPP

#include <irrString.h>
#include <SColor.h>

#include "guiengine/widget.hpp"
#include "utils/leak_check.hpp"
#include "utils/ptr_vector.hpp"

namespace GUIEngine
{
    /** \brief A simple label widget.
      * \ingroup widgetsgroup
      */
    class LabelWidget : public Widget
    {
        bool               m_has_color;
        irr::video::SColor m_color;

        /** Scroll speed in characters/seconds (0 if no scrolling). */
        float              m_scroll_speed;
        
        /** Current scroll offset. */
        float              m_scroll_offset;
        
    public:
        
        LEAK_CHECK()
        
        /** Constructs the label widget. Parameter:
          * \param title  True if the special title font should be used.
          * \param bright True if a bright color should be used
          * \note \c title and \c bright are mutually exclusive
          */
        LabelWidget(bool title=false, bool bright=false);
        
        virtual ~LabelWidget() {}
        
        /** \brief Callback from base class Widget */
        virtual void add();
                
        /** Sets the color of the widget. 
         *  \param color The color to use for this widget. */
        void     setColor(const irr::video::SColor& color)
        {
            m_color     = color;
            m_has_color = true;
        }   // setColor
        
        /** \brief Callback from base class Widget */
        virtual void update(float dt);
                
        /**
          * \brief Sets the text in the label.
          *
          * \note The change is permanent (if you visit the screen later this value will
          *       be remembered, and the old value is lost).
          *
          * \note If you wish this label to scroll, set the scroll speed before
          *       calling this function
          *
          * \param text           The string to use as text for this widget.
          * \param expandAsNeeded If true, the label will resize itself in case that it's
          *                       too small to contain \c text. Note that this option may
          *                       only be passed after the widget has been add()ed.
          */
        virtual void setText(const wchar_t *text, bool expandAsNeeded);
        
        /** Overloaded function which takes a stringw. */
        virtual void setText(const irr::core::stringw &s, bool expandAsNeeded) 
        {
            setText(s.c_str(), expandAsNeeded); 
        }
        
        // --------------------------------------------------------------------
        
        /** Sets horizontal scroll speed. */
        void setScrollSpeed(float speed);
        
        // --------------------------------------------------------------------
        
        /** 
          * \brief Check if the current has been fully scrolled
          * \return true if the text has completely scrolled off
          * \pre May only be called after this widget has been add()ed
          */
        bool scrolledOff() const;

    };
}

#endif
