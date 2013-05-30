//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2010 Marianne Gagnon, Joerg Henrichs
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


#include "states_screens/dialogs/addons_loading.hpp"

#include <pthread.h>

#include "addons/addons_manager.hpp"
#include "addons/inetwork_http.hpp"
#include "addons/request.hpp"
#include "config/user_config.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/scalable_font.hpp"
#include "guiengine/widgets.hpp"
#include "input/input_manager.hpp"
#include "io/file_manager.hpp"
#include "states_screens/addons_screen.hpp"
#include "states_screens/dialogs/message_dialog.hpp"
#include "states_screens/state_manager.hpp"
#include "tracks/track_manager.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

using namespace GUIEngine;
using namespace irr::gui;

// ----------------------------------------------------------------------------
/** Creates a modal dialog with given percentage of screen width and height
*/

AddonsLoading::AddonsLoading(const float w, const float h,
                             const std::string &id)
             : ModalDialog(w, h)
{
    m_addon            = *(addons_manager->getAddon(id));
    m_icon_shown       = false;
    m_download_request = NULL;

    loadFromFile("addons_loading.stkgui");

    m_icon             = getWidget<IconButtonWidget> ("icon"    );
    m_progress         = getWidget<ProgressBarWidget>("progress");
    m_install_button   = getWidget<IconButtonWidget> ("install" );
    m_back_button      = getWidget<IconButtonWidget> ("back"  );
    m_back_button->setFocusForPlayer( PLAYER_ID_GAME_MASTER );

    if(m_progress)
        m_progress->setVisible(false);

}   // AddonsLoading

// ----------------------------------------------------------------------------
/** Destructor.
 */
AddonsLoading::~AddonsLoading()
{
    // Select the last selected item in the addons_screen, so that
    // users can keep on installing from the last selected item.
    AddonsScreen::getInstance()->setLastSelected();
}   // AddonsLoading

// ----------------------------------------------------------------------------

void AddonsLoading::beforeAddingWidgets()
{
    /* Init the icon here to be able to load a single image*/
    m_icon             = getWidget<IconButtonWidget> ("icon"    );
    m_progress         = getWidget<ProgressBarWidget>("progress");
    m_back_button      = getWidget<IconButtonWidget> ("back"    );


    RibbonWidget* r = getWidget<RibbonWidget>("actions");

    if (m_addon.isInstalled())
    {
        /* only keep the button as "update" if allowed to access the net
         * and  not in error state
         */
        if (m_addon.needsUpdate() && !addons_manager->wasError()
            && UserConfigParams::m_internet_status==INetworkHttp::IPERM_ALLOWED)
            getWidget<IconButtonWidget> ("install")->setLabel( _("Update") );
        else
            r->removeChildNamed("install");
    }
    else
    {
        r->removeChildNamed("uninstall");
    }

    getWidget<LabelWidget>("name")->setText(m_addon.getName().c_str(), false);
    getWidget<BubbleWidget>("description")
        ->setText(m_addon.getDescription().c_str());
    core::stringw revision = _("Version: %d", m_addon.getRevision());
    getWidget<LabelWidget>("revision")->setText(revision, false);
    char rating_val[4];
    std::sprintf(rating_val, "%.1f", m_addon.getRating());
    //I18N: for add-on rating, "Rating: 1.5/3.0"
    core::stringw rating = _("Rating: %s/%s", rating_val, "3.0");
    getWidget<LabelWidget>("rating")->setText(rating, false);

    // Display flags for this addon
    // ============================
    std::vector<core::stringw> l;
    if(UserConfigParams::m_artist_debug_mode)
    {
        // In non artist-debug-mode only approved items will be shown anyway,
        // but give even tester an idea about the status:
        if (!m_addon.testStatus(Addon::AS_APPROVED))
            l.push_back("NOT APPROVED");

        // Note that an approved addon should never have alpha, beta, or
        // RC status - and only one of those should be used
        if(m_addon.testStatus(Addon::AS_ALPHA))
            l.push_back("alpha");
        else if(m_addon.testStatus(Addon::AS_BETA))
            l.push_back("beta");
        else if(m_addon.testStatus(Addon::AS_RC))
            l.push_back("RC");

        if(m_addon.testStatus(Addon::AS_BAD_DIM))
            l.push_back("bad-texture");
        if(!m_addon.testStatus(Addon::AS_DFSG))
            l.push_back("non-DFSG");
    }
    if(m_addon.testStatus(Addon::AS_FEATURED))
        l.push_back(_("featured"));

    GUIEngine::LabelWidget *flags = getWidget<LabelWidget>("flags");
    if(flags)
    {
        core::stringw s1("");
        for(unsigned int i=0; i<l.size(); i++)
        {
            s1+=l[i];
            // if it's not the last item, add a ",".
            // Don't test for l.size()-1 - since this is unsigned!
            if(i+1<l.size())
                s1+=", ";
        }
        flags->setText(s1, false);
    }

    // Display the size
    // ================
    int n = m_addon.getSize();
    core::stringw unit="";
    if(n>1024*1024)
    {
        float f = ((int)(n/1024.0f/1024.0f*10.0f+0.5f))/10.0f;
        char s[32];
        sprintf(s, "%.1f", f);
        unit=_("%s MB", s);
    }
    else if(n>1024)
    {
        float f = ((int)(n/1024.0f*10.0f+0.5f))/10.0f;
        char s[32];
        sprintf(s, "%.1f", f);
        unit=_("%s KB", s);
    }
    else
        // Anything smaller just let it be 1 KB
        unit=_("%s KB", 1);
    core::stringw size = _("Size: %s", unit.c_str());
    getWidget<LabelWidget>("size")->setText(size, false);
}   // AddonsLoading

// ----------------------------------------------------------------------------

void AddonsLoading::init()
{
    GUIEngine::LabelWidget* flags = getWidget<LabelWidget>("flags");
    if (flags)
    {
        flags->getIrrlichtElement<IGUIStaticText>()->setOverrideFont(GUIEngine::getSmallFont());
    }
}

// ----------------------------------------------------------------------------
void AddonsLoading::escapePressed()
{
    processEvent("back");
    ModalDialog::dismiss();
}   // escapePressed

// ----------------------------------------------------------------------------

GUIEngine::EventPropagation
                    AddonsLoading::processEvent(const std::string& event_source)
{
    if(event_source == "back")
    {
        // Cancel a download only if we are installing/upgrading one
        // (and not uninstalling an installed one):
        if(m_download_request)
        {
            assert(m_download_request);
            // In case of a cancel we can't free the memory, since
            // network_http will potentially update the request. So in
            // order to avoid a memory leak, we let network_http free
            // the request.
            m_download_request->setManageMemory(true);
            m_download_request->cancel();
        }
        dismiss();
        return GUIEngine::EVENT_BLOCK;
    }
    else if(event_source == "install")
    {
        // Only display the progress bar etc. if we are
        // not uninstalling an addon.
        if(!m_addon.isInstalled() || m_addon.needsUpdate())
        {
            m_progress->setValue(0);
            m_progress->setVisible(true);
            // Change the 'back' button into a 'cancel' button.
            m_back_button->setLabel(_("Cancel"));

            RibbonWidget* r = getWidget<RibbonWidget>("actions");
            r->setVisible(false);

            startDownload();
        }
        return GUIEngine::EVENT_BLOCK;
    }
    else if (event_source == "uninstall")
    {
        doUninstall();
        return GUIEngine::EVENT_BLOCK;
    }
    return GUIEngine::EVENT_LET;
}   // processEvent

// ----------------------------------------------------------------------------
void AddonsLoading::onUpdate(float delta)
{
    if(m_progress->isVisible())
    {
        float progress = m_download_request->getProgress();
        m_progress->setValue((int)(progress*100.0f));
        if(progress<0)
        {
            // Avoid displaying '-100%' in case of an error.
            m_progress->setVisible(false);
            dismiss();
            new MessageDialog( _("Sorry, downloading the add-on failed"));
            return;
        }
        else if(progress>=1.0f)
        {
            m_back_button->setLabel(_("Back"));
            // No sense to update state text, since it all
            // happens before the GUI is refrehsed.
            doInstall();
            return;
        }
    }   // if(m_progress->isVisible())

    // See if the icon is loaded (but not yet displayed)
    if(!m_icon_shown && m_addon.iconReady())
    {
        const std::string icon = "icons/"+m_addon.getIconBasename();
        m_icon->setImage( file_manager->getAddonsFile(icon).c_str(),
                          IconButtonWidget::ICON_PATH_TYPE_ABSOLUTE  );
        m_icon_shown = true;
    }
}   // onUpdate

// ----------------------------------------------------------------------------
/** This function is called when the user click on 'Install', 'Uninstall', or
 *  'Update'.
 **/
void AddonsLoading::startDownload()
{
    std::string file   = m_addon.getZipFileName();
    std::string save   = "tmp/"
                       + StringUtils::getBasename(m_addon.getZipFileName());
    m_download_request = INetworkHttp::get()->downloadFileAsynchron(file, save,
                                                       /*priority*/5,
                                                       /*manage memory*/false);
}   // startDownload

// ----------------------------------------------------------------------------
/** Called when the asynchronous download of the addon finished.
 */
void AddonsLoading::doInstall()
{
    delete m_download_request;
    m_download_request = NULL;
    bool error=false;

    assert(!m_addon.isInstalled() || m_addon.needsUpdate());
    error = !addons_manager->install(m_addon);
    if(error)
    {
        core::stringw msg = StringUtils::insertValues(
            _("Problems installing the addon '%s'."),
            core::stringw(m_addon.getName().c_str()));
        getWidget<BubbleWidget>("description")->setText(msg.c_str());
    }

    if(error)
    {
        m_progress->setVisible(false);

        RibbonWidget* r = getWidget<RibbonWidget>("actions");
        r->setVisible(true);

        m_install_button->setLabel(_("Try again"));
    }
    else
    {
        // The list of the addon screen needs to be updated to correctly
        // display the newly (un)installed addon.
        AddonsScreen::getInstance()->loadList();
        dismiss();
    }

    track_manager->loadTrackList();
}   // doInstall

// ----------------------------------------------------------------------------

void AddonsLoading::doUninstall()
{
    delete m_download_request;
    m_download_request = NULL;
    bool error=false;

    error = !addons_manager->uninstall(m_addon);
    if(error)
    {
        printf("[addons]Directory '%s' can not be removed.\n",
               m_addon.getDataDir().c_str());
        printf("[addons]Please remove this directory manually.\n");
        core::stringw msg = StringUtils::insertValues(
                                                      _("Problems removing the addon '%s'."),
                                                      core::stringw(m_addon.getName().c_str()));
        getWidget<BubbleWidget>("description")->setText(msg.c_str());
    }

    if(error)
    {
        m_progress->setVisible(false);

        RibbonWidget* r = getWidget<RibbonWidget>("actions");
        r->setVisible(true);

        m_install_button->setLabel(_("Try again"));
    }
    else
    {
        // The list of the addon screen needs to be updated to correctly
        // display the newly (un)installed addon.
        AddonsScreen::getInstance()->loadList();
        dismiss();
    }
}   // doUninstall
