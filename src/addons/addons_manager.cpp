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
/**
  \page addons Addons
  */

#include "addons/addons_manager.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string.h>
#include <vector>

#include "addons/inetwork_http.hpp"
#include "addons/request.hpp"
#include "addons/zip.hpp"
#include "graphics/irr_driver.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "states_screens/kart_selection.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/string_utils.hpp"

AddonsManager* addons_manager = 0;

// ----------------------------------------------------------------------------
/** Initialises the non-online component of the addons manager (i.e. handling
 *  the list of already installed addons). The online component is initialised
 *  later from a separate thread in network_http (once network_http is setup).
 */
AddonsManager::AddonsManager() : m_addons_list(std::vector<Addon>() ),
                                 m_state(STATE_INIT)
{
    m_file_installed = file_manager->getAddonsFile("addons_installed.xml");

    // Load the addons list (even if internet is disabled)
    m_addons_list.lock();
    // Clear the list in case that a reinit is being done.
    m_addons_list.getData().clear();
    loadInstalledAddons();
    m_addons_list.unlock();
}   // AddonsManager

// ----------------------------------------------------------------------------
/** The destructor saves the installed addons file again. This is necessary
 *  so that information about downloaded icons is saved for the next run.
 */
AddonsManager::~AddonsManager()
{
    saveInstalled();
}   // ~AddonsManager

// ----------------------------------------------------------------------------
/** This initialises the online portion of the addons manager. It uses the
 *  downloaded list of available addons. This is called by network_http before
 *  it goes into command-receiving mode, so we can't use any asynchronous calls
 *  here (though this is being called from a separate thread , so the
 *  main GUI is not blocked anyway). This function will update the state 
 *  variable
 */
void AddonsManager::initOnline(const XMLNode *xml)
{
    m_addons_list.lock();
    // Clear the list in case that a reinit is being done.
    m_addons_list.getData().clear();
    loadInstalledAddons();
    m_addons_list.unlock();

    for(unsigned int i=0; i<xml->getNumNodes(); i++)
    {
        const XMLNode *node = xml->getNode(i);
        const std::string &name = node->getName();
        // Ignore news/redirect, which is handled by network_http
        if(name=="include" || name=="message") continue;
        if(node->getName()=="track" || node->getName()=="kart" ||
            node->getName()=="arena"                                 )
        {
            Addon addon(*node);
            int index = getAddonIndex(addon.getId());

            int stk_version=0;
            node->get("format", &stk_version);
            int   testing=-1;
            node->get("testing", &testing);

            bool wrong_version=false;

            if(addon.getType()=="kart")
                wrong_version = stk_version <stk_config->m_min_kart_version ||
                                stk_version >stk_config->m_max_kart_version   ;
            else
                wrong_version = stk_version <stk_config->m_min_track_version ||
                                stk_version >stk_config->m_max_track_version   ;
            // If the add-on is included, behave like it is a wrong version
            if (addon.testIncluded(addon.getMinIncludeVer(), addon.getMaxIncludeVer()))
                wrong_version = true;

            // Check which version to use: only for this stk version,
            // and not addons that are marked as hidden (testing=0)
            if(wrong_version|| testing==0)
            {
                // If the version is too old (e.g. after an update of stk)
                // remove a cached icon.
                std::string full_path = 
                    file_manager->getAddonsFile("icons/"
                                                +addon.getIconBasename());
                if(file_manager->fileExists(full_path))
                {
                    if(UserConfigParams::logAddons())
                        Log::warn("[addons] Removing cached icon '%s'.\n", 
                               addon.getIconBasename().c_str());
                    file_manager->removeFile(full_path);
                }
                continue;
            }

            m_addons_list.lock();
            if(index>=0)
            {
                Addon& tmplist_addon = m_addons_list.getData()[index];
                
                // Only copy the data if a newer revision is found (ignore unapproved
                // revisions unless player is in the mode to see them)
                if (tmplist_addon.getRevision() < addon.getRevision() &&
                    (addon.testStatus(Addon::AS_APPROVED) || UserConfigParams::m_artist_debug_mode))
                {
                    m_addons_list.getData()[index].copyInstallData(addon);
                }
            }
            else
            {
                m_addons_list.getData().push_back(addon);
                index = m_addons_list.getData().size()-1;
            }
            // Mark that this addon still exists on the server
            m_addons_list.getData()[index].setStillExists();
            m_addons_list.unlock();
        }
        else
        {
            fprintf(stderr, 
                    "[addons] Found invalid node '%s' while downloading addons.\n",
                    node->getName().c_str());
            fprintf(stderr, "[addons] Ignored.\n");
        }
    }   // for i<xml->getNumNodes
    delete xml;

    // Now remove all items from the addons-installed list, that are not
    // on the server anymore (i.e. not in the addons.xml file), and not
    // installed. If found, remove the icon cached for this addon.
    // Note that if (due to a bug) an icon is shared (i.e. same icon on
    // an addon that's still on the server and an invalid entry in the
    // addons installed file), it will be re-downloaded later.
    m_addons_list.lock();
    unsigned int count = m_addons_list.getData().size();

    for(unsigned int i=0; i<count;)
    {
        if(m_addons_list.getData()[i].getStillExists() ||
            m_addons_list.getData()[i].isInstalled())
        {
            i++;
            continue;
        }
        // This addon is not on the server anymore, and not installed. Remove
        // it from the list. 
        if(UserConfigParams::logAddons())
            Log::warn(
                "[addons] Removing '%s' which is not on the server anymore.\n",
                m_addons_list.getData()[i].getId().c_str() );
        std::string icon = m_addons_list.getData()[i].getIconBasename();
        std::string icon_file =file_manager->getAddonsFile("icons/"+icon);
        if(file_manager->fileExists(icon_file))
        {
            file_manager->removeFile(icon_file);
            // Ignore errors silently.
        }
        m_addons_list.getData()[i] = m_addons_list.getData()[count-1];
        m_addons_list.getData().pop_back();
        count--;
    }
    m_addons_list.unlock();

    m_state.setAtomic(STATE_READY);

    if (UserConfigParams::m_internet_status == INetworkHttp::IPERM_ALLOWED)
        downloadIcons();
}   // initOnline

// ----------------------------------------------------------------------------
/** Reinitialises the addon manager, which happens when the user selects
 *  'reload' in the addon manager.
 */
void AddonsManager::reInit()
{
    m_state.setAtomic(STATE_INIT);
}   // reInit

// ----------------------------------------------------------------------------
/** This function checks if the information in the installed addons file is
 *  consistent with what is actually available. This avoids e.g. that an
 *  addon is installed, but not marked here (and therefore shows up as
 *  not installed in the addons GUI), see bug #455.
 */
void AddonsManager::checkInstalledAddons()
{
    bool something_was_changed = false;

    // Lock the whole addons list to make sure a consistent view is
    // written back to disk. The network thread might still be 
    // downloading icons and modify content
    m_addons_list.lock();

    // First karts
    // -----------
    for(unsigned int i=0; i<kart_properties_manager->getNumberOfKarts(); i++)
    {
        const KartProperties *kp = kart_properties_manager->getKartById(i);
        const std::string &dir=kp->getKartDir();
        if(dir.find(file_manager->getAddonsDir())==std::string::npos)
            continue;
        int n = getAddonIndex(kp->getIdent());
        if(n<0) continue;
        if(!m_addons_list.getData()[n].isInstalled())
        {
            Log::info("[addons] Marking '%s' as being installed.\n", 
                   kp->getIdent().c_str());
            m_addons_list.getData()[n].setInstalled(true);
            something_was_changed = true;
        }
    }

    // Then tracks
    // -----------
    for(unsigned int i=0; i<track_manager->getNumberOfTracks(); i++)
    {
        const Track *track = track_manager->getTrack(i);
        const std::string &dir=track->getFilename();
        if(dir.find(file_manager->getAddonsDir())==std::string::npos)
            continue;
        int n = getAddonIndex(track->getIdent());
        if(n<0) continue;
        if(!m_addons_list.getData()[n].isInstalled())
        {
            Log::info("[addons] Marking '%s' as being installed.\n", 
                   track->getIdent().c_str());
            m_addons_list.getData()[n].setInstalled(true);
            something_was_changed = true;
        }
    }
    if(something_was_changed)
        saveInstalled();
    m_addons_list.unlock();
}   // checkInstalledAddons

// ----------------------------------------------------------------------------
/** Download all necessary icons (i.e. icons that are either missing or have 
 *  been updated since they were downloaded).
 */
void AddonsManager::downloadIcons()
{
    for(unsigned int i=0; i<m_addons_list.getData().size(); i++)
    {
        Addon &addon            = m_addons_list.getData()[i];
        const std::string &icon = addon.getIconBasename();
        const std::string &icon_full
                                = file_manager->getAddonsFile("icons/"+icon); 
        if(addon.iconNeedsUpdate() ||
            !file_manager->fileExists(icon_full))
        {
            const std::string &url  = addon.getIconURL();
            const std::string &icon = addon.getIconBasename();
            if(icon=="")
            {
                if(UserConfigParams::logAddons())
                    fprintf(stderr,
                            "[addons] No icon or image specified for '%s'.\n",
                            addon.getId().c_str());
                continue;
            }
            std::string save        = "icons/"+icon;
            Request *r = INetworkHttp::get()->downloadFileAsynchron(url, save, 
                                                            /*priority*/1,
                                                           /*manage_mem*/true);
            if (r != NULL)
                r->setAddonIconNotification(&addon);            
        }
        else
            m_addons_list.getData()[i].setIconReady();
    }   // for i<m_addons_list.size()

    return;
}   // downloadIcons

// ----------------------------------------------------------------------------
/** Loads the installed addons from .../addons/addons_installed.xml.
 */
void AddonsManager::loadInstalledAddons()
{
    /* checking for installed addons */
    if(UserConfigParams::logAddons())
    {
        std::cout << "[addons] Loading an xml file for installed addons: ";
        std::cout << m_file_installed << std::endl;
    }
    const XMLNode *xml = file_manager->createXMLTree(m_file_installed);
    if(!xml)
        return;

    for(unsigned int i=0; i<xml->getNumNodes(); i++)
    {
        const XMLNode *node=xml->getNode(i);
        if(node->getName()=="kart"   || node->getName()=="arena" ||
            node->getName()=="track"    )
        {
            Addon addon(*node);
            m_addons_list.getData().push_back(addon);
        }
    }   // for i <= xml->getNumNodes()

    delete xml;
}   // loadInstalledAddons

// ----------------------------------------------------------------------------
/** Returns an addon with a given id. Raises an assertion if the id is not 
 *  found!
 *  \param id The id to search for.
 */
const Addon* AddonsManager::getAddon(const std::string &id) const
{
    int i = getAddonIndex(id);
    return (i<0) ? NULL : &(m_addons_list.getData()[i]);
}   // getAddon

// ----------------------------------------------------------------------------
/** Returns the index of the addon with the given id, or -1 if no such
 *  addon exist.
 *  \param id The (unique) identifier of the addon.
 */
int AddonsManager::getAddonIndex(const std::string &id) const
{
    for(unsigned int i = 0; i < m_addons_list.getData().size(); i++)
    {
        if(m_addons_list.getData()[i].getId()== id)
        {
            return i;
        }
    }
    return -1;
}   // getAddonIndex

// ----------------------------------------------------------------------------
/** Installs or updates (i.e. = install on top of an existing installation) an 
 *  addon. It checks for the directories and then unzips the file (which must 
 *  already have been downloaded).
 *  \param addon Addon data for the addon to install.
 *  \return true if installation was successful.
 */
bool AddonsManager::install(const Addon &addon)
{
    bool success=true;
    file_manager->checkAndCreateDirForAddons(addon.getDataDir());

    //extract the zip in the addons folder called like the addons name    
    std::string base_name = StringUtils::getBasename(addon.getZipFileName());
    std::string from      = file_manager->getAddonsFile("tmp/"+base_name);
    std::string to        = addon.getDataDir();
    
    success = extract_zip(from, to);
    if (!success)
    {
        // TODO: show a message in the interface
        std::cerr << "[addons] Failed to unzip '" << from << "' to '" 
                  << to << "'\n";
        std::cerr << "[addons] Zip file will not be removed.\n";
        return false;
    }

    if(!file_manager->removeFile(from))
    {
        std::cerr << "[addons] Problems removing temporary file '"
                  << from << "'.\n";
    }

    int index = getAddonIndex(addon.getId());
    assert(index>=0 && index < (int)m_addons_list.getData().size());
    m_addons_list.getData()[index].setInstalled(true);
    
    if(addon.getType()=="kart")
    {
        // We have to remove the mesh of the kart since otherwise it remains
        // cashed (if a kart is updated), and will therefore be found again 
        // when reloading the karts. This is important on one hand since we 
        // reload all karts (this function is easily available) and existing
        // karts will not reload their meshes.
        const KartProperties *prop = 
            kart_properties_manager->getKart(addon.getId());
        // If the model already exist, first remove the old kart
        if(prop)
            kart_properties_manager->removeKart(addon.getId());
        kart_properties_manager->loadKart(addon.getDataDir());
    }
    else if (addon.getType()=="track" || addon.getType()=="arena")
    {
        Track *track = track_manager->getTrack(addon.getId());
        if(track)
            track_manager->removeTrack(addon.getId());
        
        try
        {
            track_manager->loadTrack(addon.getDataDir());
        }
        catch (std::exception& e)
        {
            fprintf(stderr, "[AddonsManager] ERROR: Cannot load track <%s> : %s\n",
                    addon.getDataDir().c_str(), e.what());
        }
    }
    saveInstalled();
    return true;
}   // install

// ----------------------------------------------------------------------------
/** Removes all files froma login.
 *  \param addon The addon to be removed.
 *  \return True if uninstallation was successful.
 */
bool AddonsManager::uninstall(const Addon &addon)
{
    std::cout << "[addons] Uninstalling <" 
              << core::stringc(addon.getName()).c_str() << ">\n";

    // addon is a const reference, and to avoid removing the const, we
    // find the proper index again to modify the installed state
    int index = getAddonIndex(addon.getId());
    assert(index>=0 && index < (int)m_addons_list.getData().size());
    m_addons_list.getData()[index].setInstalled(false);

    //remove the addons directory
	bool error = false;
	// if the user deleted the data directory for an add-on with
	// filesystem tools, removeTrack/removeKart will trigger an assert
	// because the kart/track was never added in the first place
	if (file_manager->fileExists(addon.getDataDir()))
	{
		error = !file_manager->removeDirectory(addon.getDataDir());
		if(addon.getType()=="kart")
		{
			kart_properties_manager->removeKart(addon.getId());
		}
		else if(addon.getType()=="track" || addon.getType()=="arena")
		{
			track_manager->removeTrack(addon.getId());
		}
	}
    saveInstalled();
    return !error;
}   // uninstall

// ----------------------------------------------------------------------------
/** Saves the information about installed addons and cached icons to 
 *  addons_installed.xml. If this is not called, information about downloaded
 *  icons is lost (and will trigger a complete redownload when STK is started
 *  next time).
 */
void AddonsManager::saveInstalled()
{
    //Put the addons in the xml file
    //Manually because the irrlicht xml writer doesn't seem finished, FIXME ?
    std::ofstream xml_installed(m_file_installed.c_str());

    //write the header of the xml file
    xml_installed << "<?xml version=\"1.0\"?>" << std::endl;
    xml_installed << "<addons  xmlns='http://stkaddons.net/'>"
                    << std::endl;

    for(unsigned int i = 0; i < m_addons_list.getData().size(); i++)
    {
        m_addons_list.getData()[i].writeXML(&xml_installed);
    }
    xml_installed << "</addons>" << std::endl;
    xml_installed.close();
}   // saveInstalled

