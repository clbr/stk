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


#include "guiengine/screen.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/widgets.hpp"
#include "utils/translation.hpp"
#include <iostream>
#include <irrXML.h>
#include <sstream>

using namespace irr;

using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;
using namespace GUIEngine;

void Screen::parseScreenFileDiv(irr::io::IXMLReader* xml, PtrVector<Widget>& append_to,
                                irr::gui::IGUIElement* parent)
{
    // parse XML file
    while (xml && xml->read())
    {

        switch (xml->getNodeType())
        {
            case irr::io::EXN_TEXT:
            {
                break;
            }

            case irr::io::EXN_ELEMENT:
            {
                /* find which type of widget is specified by the current tag, and instanciate it */
                if (wcscmp(L"div", xml->getNodeName()) == 0)
                {
                    Widget* w = new Widget(WTYPE_DIV);
                    append_to.push_back(w);
                }
                else if (wcscmp(L"stkgui", xml->getNodeName()) == 0)
                {
                    // outer node that's there only to comply with XML standard (and expat)
                    continue;
                }
                else if (wcscmp(L"placeholder", xml->getNodeName()) == 0)
                {
                    Widget* w = new Widget(WTYPE_DIV, true);
                    append_to.push_back(w);
                }
                else if (wcscmp(L"box", xml->getNodeName()) == 0)
                {
                    Widget* w = new Widget(WTYPE_DIV);
                    w->m_show_bounding_box = true;
                    append_to.push_back(w);
                }
                else if (wcscmp(L"bottombar", xml->getNodeName()) == 0)
                {
                    Widget* w = new Widget(WTYPE_DIV);
                    w->m_bottom_bar = true;
                    append_to.push_back(w);
                }
                else if (wcscmp(L"topbar", xml->getNodeName()) == 0)
                {
                    Widget* w = new Widget(WTYPE_DIV);
                    w->m_top_bar = true;
                    append_to.push_back(w);
                }
                else if (wcscmp(L"roundedbox", xml->getNodeName()) == 0)
                {
                    Widget* w = new Widget(WTYPE_DIV);
                    w->m_show_bounding_box = true;
                    w->m_is_bounding_box_round = true;
                    append_to.push_back(w);
                }
                else if (wcscmp(L"ribbon", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new RibbonWidget());
                }
                else if (wcscmp(L"buttonbar", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new RibbonWidget(RIBBON_TOOLBAR));
                }
                else if (wcscmp(L"tabs", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new RibbonWidget(RIBBON_TABS));
                }
                else if (wcscmp(L"spinner", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new SpinnerWidget());
                }
                else if (wcscmp(L"button", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new ButtonWidget());
                }
                else if (wcscmp(L"gauge", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new SpinnerWidget(true));
                }
                else if (wcscmp(L"progressbar", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new ProgressBarWidget());
                }
                else if (wcscmp(L"icon-button", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new IconButtonWidget());
                }
                else if (wcscmp(L"icon", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new IconButtonWidget(IconButtonWidget::SCALE_MODE_KEEP_TEXTURE_ASPECT_RATIO,
                                                             false, false));
                }
                else if (wcscmp(L"checkbox", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new CheckBoxWidget());
                }
                else if (wcscmp(L"label", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new LabelWidget());
                }
                else if (wcscmp(L"bright", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new LabelWidget(false, true));
                }
                else if (wcscmp(L"bubble", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new BubbleWidget());
                }
                else if (wcscmp(L"header", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new LabelWidget(true));
                }
                else if (wcscmp(L"spacer", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new Widget(WTYPE_SPACER));
                }
                else if (wcscmp(L"ribbon_grid", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new DynamicRibbonWidget(false /* combo */, true /* multi-row */));
                }
                else if (wcscmp(L"scrollable_ribbon", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new DynamicRibbonWidget(true /* combo */, false /* multi-row */));
                }
                else if (wcscmp(L"scrollable_toolbar", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new DynamicRibbonWidget(false /* combo */, false /* multi-row */));
                }
                else if (wcscmp(L"model", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new ModelViewWidget());
                }
                else if (wcscmp(L"list", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new ListWidget());
                }
                else if (wcscmp(L"textbox", xml->getNodeName()) == 0)
                {
                    append_to.push_back(new TextBoxWidget());
                }
                else
                {
                    std::cerr << "/!\\ Warning /!\\ : unknown tag found in STK GUI file  : '"
                              << xml->getNodeName()  << "'" << std::endl;
                    continue;
                }

                /* retrieve the created widget */
                Widget& widget = append_to[append_to.size()-1];

                /* read widget properties using macro magic */

#define READ_PROPERTY( prop_name, prop_flag ) const wchar_t* prop_name = xml->getAttributeValue( L###prop_name ); \
if(prop_name != NULL) widget.m_properties[prop_flag] = core::stringc(prop_name).c_str(); else widget.m_properties[prop_flag] = ""

                READ_PROPERTY(id,             PROP_ID);
                READ_PROPERTY(proportion,     PROP_PROPORTION);
                READ_PROPERTY(width,          PROP_WIDTH);
                READ_PROPERTY(height,         PROP_HEIGHT);
                READ_PROPERTY(child_width,    PROP_CHILD_WIDTH);
                READ_PROPERTY(child_height,   PROP_CHILD_HEIGHT);
                READ_PROPERTY(word_wrap,      PROP_WORD_WRAP);
                //READ_PROPERTY(grow_with_text, PROP_GROW_WITH_TEXT);
                READ_PROPERTY(x,              PROP_X);
                READ_PROPERTY(y,              PROP_Y);
                READ_PROPERTY(layout,         PROP_LAYOUT);
                READ_PROPERTY(align,          PROP_ALIGN);

                READ_PROPERTY(icon,           PROP_ICON);
                READ_PROPERTY(focus_icon,     PROP_FOCUS_ICON);
                READ_PROPERTY(text_align,     PROP_TEXT_ALIGN);
                READ_PROPERTY(min_value,      PROP_MIN_VALUE);
                READ_PROPERTY(max_value,      PROP_MAX_VALUE);
                READ_PROPERTY(square_items,   PROP_SQUARE);

                READ_PROPERTY(max_width,      PROP_MAX_WIDTH);
                READ_PROPERTY(max_height,     PROP_MAX_HEIGHT);
                READ_PROPERTY(extend_label,   PROP_EXTEND_LABEL);
                READ_PROPERTY(label_location, PROP_LABELS_LOCATION);
                READ_PROPERTY(max_rows,       PROP_MAX_ROWS);
                READ_PROPERTY(wrap_around,    PROP_WRAP_AROUND);
#undef READ_PROPERTY

                const wchar_t* text = xml->getAttributeValue( L"text" );

                if (text != NULL)
                {
                    widget.m_text = _(text);
                    widget.m_is_text_rtl = (translations->isRTLLanguage() && widget.m_text != text);
                }

                if (parent != NULL)
                {
                    widget.setParent(parent);
                }

                /* a new div starts here, continue parsing with this new div as new parent */
                if (widget.getType() == WTYPE_DIV || widget.getType() == WTYPE_RIBBON)
                {
                    parseScreenFileDiv( xml, append_to[append_to.size()-1].m_children, parent );
                }
            }// end case EXN_ELEMENT

                break;
            case irr::io::EXN_ELEMENT_END:
            {
                // we're done parsing this 'div', return one step back in the recursive call
                if (wcscmp(L"div", xml->getNodeName()) == 0)
                    return;
                if (wcscmp(L"box", xml->getNodeName()) == 0)
                    return;
                if (wcscmp(L"placeholder", xml->getNodeName()) == 0)
                    return;
                if (wcscmp(L"roundedbox", xml->getNodeName()) == 0)
                    return;
                if (wcscmp(L"bottombar", xml->getNodeName()) == 0)
                    return;
                if (wcscmp(L"topbar", xml->getNodeName()) == 0)
                    return;

                // We're done parsing this 'ribbon', return one step back in
                // the recursive call.
                if (wcscmp(L"ribbon", xml->getNodeName()) == 0 ||
                    wcscmp(L"buttonbar", xml->getNodeName()) == 0 ||
                    wcscmp(L"tabs", xml->getNodeName()) == 0)
                    return;
            }
                break;

            default: break;
        }//end switch
    } // end while
} // end function

