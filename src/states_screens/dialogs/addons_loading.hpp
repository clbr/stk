//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010 Lucas Baudin
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


#ifndef HEADER_ADDONS_LOADING_HPP
#define HEADER_ADDONS_LOADING_HPP

#include "addons/addon.hpp"
#include "addons/addons_manager.hpp"
#include "guiengine/widgets.hpp"
#include "guiengine/modaldialog.hpp"
#include "utils/synchronised.hpp"

class Request;

/**
  * \ingroup states_screens
  */
class AddonsLoading : public GUIEngine::ModalDialog
{
  virtual void escapePressed();
private:
    GUIEngine::LabelWidget       *m_state;
    GUIEngine::ProgressBarWidget *m_progress;
    GUIEngine::IconButtonWidget  *m_back_button;
    GUIEngine::IconButtonWidget  *m_install_button;

    GUIEngine::IconButtonWidget  *m_icon;
    
    /** The addon to load. */
    Addon                         m_addon;
    void startDownload();
    void doInstall();
    void doUninstall();

    /** True if the icon is being displayed. */
    bool m_icon_shown;

    /** A pointer to the download request, which gives access
     *  to the progress of a download. */
    Request *m_download_request;

public:
            AddonsLoading(const float percent_width, 
                          const float percent_height,
                          const std::string &addon_name);
    
           ~AddonsLoading();
    virtual GUIEngine::EventPropagation processEvent(const std::string& event_source);
    
    virtual void beforeAddingWidgets();
    
    virtual void init();
    
    /** This function is called by the GUI, all the frame (or somthing like
     * that). It checks the flags (m_can_load_icon and
     *  and do the necessary.
     * */
    void onUpdate(float delta);
    
};   // AddonsLoading

#endif
