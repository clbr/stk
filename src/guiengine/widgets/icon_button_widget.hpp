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



#ifndef HEADER_IBTN_HPP
#define HEADER_IBTN_HPP

#include <irrString.h>
namespace irr
{
    namespace gui   { class IGUIStaticText; }
    namespace video { class ITexture;       }
}

#include "guiengine/widget.hpp"
#include "utils/leak_check.hpp"
#include "utils/ptr_vector.hpp"

namespace GUIEngine
{
    /** \brief A button widget with an icon and optionnaly a label beneath
      * \ingroup widgetsgroup
      */
    class IconButtonWidget : public Widget
    {
    public:
        enum ScaleMode
        {
            SCALE_MODE_STRETCH,
            SCALE_MODE_KEEP_TEXTURE_ASPECT_RATIO,
            SCALE_MODE_KEEP_CUSTOM_ASPECT_RATIO
        };
        enum IconPathType
        {
            ICON_PATH_TYPE_ABSOLUTE,
            /** relative to the data dir */
            ICON_PATH_TYPE_RELATIVE,
            /** not a valid value per se, but can be passed as argument to leave
              * the path type as it currently is */
            ICON_PATH_TYPE_NO_CHANGE
        };
        
    protected:
        
        IconPathType m_icon_path_type;
        
        friend class Skin;
        
        irr::gui::IGUIStaticText* m_label;
        irr::video::ITexture* m_texture;
        irr::video::ITexture* m_highlight_texture;

        int m_texture_w, m_texture_h;
        
        ScaleMode m_scale_mode;
        float m_custom_aspect_ratio;
        
    public:

        LEAK_CHECK()
        
        /** Whether to make the widget included in keyboard navigation order when adding */
        bool m_tab_stop;

        IconButtonWidget(ScaleMode scale_mode=SCALE_MODE_KEEP_TEXTURE_ASPECT_RATIO, const bool tab_stop=true,
                         const bool focusable=true, IconPathType pathType=ICON_PATH_TYPE_RELATIVE);
        virtual ~IconButtonWidget() {}
        
        /** \brief Implement callback from base class Widget */
        virtual void add();
        
        /**
          * \brief Call this if scale mode is SCALE_MODE_KEEP_CUSTOM_ASPECT_RATIO.
          * \param custom_aspect_ratio  The width/height aspect ratio
          */
        void setCustomAspectRatio(float custom_aspect_ratio) { m_custom_aspect_ratio = custom_aspect_ratio; }
        
        /**
          * \brief Temporarily change the text label if there is a label (next time this screen is
          *        visited, the previous label will be back. For a permanent change, edit the 'text'
          *        property in the base Widget class).
          *
          * \pre Must be called after this widget is add()ed to have any effect
          * \note         Calling this method on a button without label will have no effect
          */
        void setLabel(irr::core::stringw new_label);
        
        /**
         * Change the texture used for this icon.
         * \pre At the moment, the new texture must have the same aspct ratio
         *                as the previous one since the object will not
         *                be resized to fit a different aspect ratio.
         * \note May safely be called no matter if the widget is add()ed or not
         */
        void setImage(const char* path_to_texture, IconPathType pathType=ICON_PATH_TYPE_NO_CHANGE);
        
        /**
          * Change the texture used for this icon.
          * \pre At the moment, the new texture must have the same aspct ratio
          *                as the previous one since the object will not
          *                be resized to fit a different aspect ratio.
          * \note May safely be called no matter if the widget is add()ed or not
          */
        void setImage(irr::video::ITexture* texture);
        
        void setHighlightedImage(irr::video::ITexture* texture)
        {
            m_highlight_texture = texture;
        }
        
        /** \brief override from base class */
        virtual EventPropagation focused(const int playerID);
        
        /** \brief override from base class */
        virtual void unfocused(const int playerID, Widget* new_focus);
    };
}

#endif
