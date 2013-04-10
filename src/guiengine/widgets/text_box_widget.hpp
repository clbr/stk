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



#ifndef HEADER_TEXTBOX_HPP
#define HEADER_TEXTBOX_HPP

#include <irrString.h>

#include "guiengine/widget.hpp"
#include "utils/leak_check.hpp"
#include "utils/ptr_vector.hpp"

namespace GUIEngine
{
    class ITextBoxWidgetListener
    {
    public:
        virtual ~ITextBoxWidgetListener() {}
        virtual void onTextUpdated() = 0;
    };
    
    /** \brief A text field widget. 
      * \ingroup widgetsgroup
      */
    class TextBoxWidget : public Widget
    {
        /** When inferring widget size from its label length, this method will be called to
         * if/how much space must be added to the raw label's size for the widget to be large enough */
        virtual int getWidthNeededAroundLabel()  const { return 10; }
        
        /** When inferring widget size from its label length, this method will be called to
         * if/how much space must be added to the raw label's size for the widget to be large enough */
        virtual int getHeightNeededAroundLabel() const { return 10; }
        
    public:
        
        LEAK_CHECK()
        
        TextBoxWidget();
        ~TextBoxWidget()
        {
            setWithinATextBox(false);
        }
        
        void add();
        void addItem(const char* item);
        
        virtual EventPropagation focused(const int playerID);
        virtual void unfocused(const int playerID, Widget* new_focus);

        void addListener(ITextBoxWidgetListener* listener);
        void clearListeners();
        
        irr::core::stringw getText() const;
        
        virtual void elementRemoved();
    };
}

#endif
