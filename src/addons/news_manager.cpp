//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2011 Joerg Henrichs
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

#include "addons/news_manager.hpp"

#include "config/user_config.hpp"
#include "io/file_manager.hpp"
#include "states_screens/addons_screen.hpp"
#include "states_screens/main_menu_screen.hpp"
#include "utils/string_utils.hpp"
#include "utils/time.hpp"
#include "utils/translation.hpp"

#include <iostream>

NewsManager *news_manager=NULL;

// ----------------------------------------------------------------------------
NewsManager::NewsManager() : m_news(std::vector<NewsMessage>())
{
    m_current_news_message = -1;
    m_error_message        = "";
}   // NewsManage

// ---------------------------------------------------------------------------
NewsManager::~NewsManager()
{
}   // ~NewsManager

// ---------------------------------------------------------------------------
/** Initialises the online part of the network manager. It downloads the
 *  news.xml file from the server (if the frequency of downloads makes this
 *  necessary), and (again if necessary) the addons.xml file.
 *  \return 0 if an error happened and no online connection will be available,
 *          1 otherwise.
 */
void NewsManager::init()
{
    UserConfigParams::m_news_last_updated = Time::getTimeSinceEpoch();

    std::string xml_file = file_manager->getAddonsFile("news.xml");
    const XMLNode *xml   = new XMLNode(xml_file);
    checkRedirect(xml);
    updateNews(xml, xml_file);
    delete xml;
}   // init

// ---------------------------------------------------------------------------
/** Checks if a redirect is received, causing a new server to be used for
 *  downloading addons.
 *  \param xml XML data structure containing the redirect information.
 */
void NewsManager::checkRedirect(const XMLNode *xml)
{
    std::string new_server;
    int result = xml->get("redirect", &new_server);
    if(result==1 && new_server!="")
    {
        if(UserConfigParams::logAddons())
        {
            std::cout << "[Addons] Current server: " 
                      << (std::string)UserConfigParams::m_server_addons 
                      << std::endl
                      << "[Addons] New server: " << new_server << std::endl;
        }
        UserConfigParams::m_server_addons = new_server;
    }
}   // checkRedirect

// ----------------------------------------------------------------------------
/** Updates the 'news' string to be displayed in the main menu.
 *  \param xml The XML data from the news file.
 *  \param filename The filename of the news xml file. Only needed
 *         in case of an error (e.g. the file might be corrupted) 
 *         - the file will be deleted so that on next start of stk it 
 *         will be updated again.
 */
void NewsManager::updateNews(const XMLNode *xml, const std::string &filename)
{

    m_all_news_messages = "";
    const core::stringw message_divider="  +++  ";
    // This function is also called in case of a reinit, so
    // we have to delete existing news messages here first.
    m_news.lock();
    m_news.getData().clear();
    m_news.unlock();
    bool error = true;
    int frequency=0;
    if(xml->get("frequency", &frequency))
        UserConfigParams::m_news_frequency = frequency;

    for(unsigned int i=0; i<xml->getNumNodes(); i++)
    {
        const XMLNode *node = xml->getNode(i);
        if(node->getName()!="message") continue;
        core::stringw news;
        node->get("content", &news);
        int id=-1;
        node->get("id", &id);
        bool important=false;
        node->get("important", &important);

        std::string cond;
        node->get("condition", &cond);
        if(!conditionFulfilled(cond))
            continue;
        m_news.lock();
        {

            if(!important)
                m_all_news_messages += m_all_news_messages.size()>0 
                                    ?  message_divider + news
                                    : news;
            else
            // Define this if news messages should be removed
            // after being shown a certain number of times.
            {
                NewsMessage n(news, id, important);
                m_news.getData().push_back(n);
            }
        }   // m_news.lock()
        m_news.unlock();

        error = false;
    }
    if(error)
    {
        // In case of an error (e.g. the file only contains
        // an error message from the server), delete the file
        // so that it is not read again (and this will force
        // a new read on the next start, instead of waiting
        // for some time).
        file_manager->removeFile(filename);
        NewsMessage n(_("Can't access stkaddons server..."), -1);
        m_news.lock();
        m_news.getData().push_back(n);

        m_all_news_messages="";
        m_news.unlock();
    }
}   // updateNews

// ----------------------------------------------------------------------------
/** Add a news message. This is used to add error messages, e.g. for problems
 *  when downloading addons.
 *  \param s The news message to add.
 */
void NewsManager::addNewsMessage(const core::stringw &s)
{
    NewsMessage n(s, -1);
    m_news.lock();
    m_news.getData().push_back(n);
    m_news.unlock();
}   // addNewsMessage
// ----------------------------------------------------------------------------
/** Returns the  important message with the smallest id that has not been 
 *  shown, or NULL if no important (not shown before) message exists atm. The
 *  user config is updated to store the last important message id shown. 
 */
const core::stringw NewsManager::getImportantMessage()
{
    int index = -1;
    m_news.lock();
    for(unsigned int i=0; i<m_news.getData().size(); i++)
    {
        const NewsMessage &m = m_news.getData()[i];
        // 
        if(m.isImportant() && 
           m.getMessageId()>UserConfigParams::m_last_important_message_id  &&
            (index == -1 || 
            m.getMessageId() < m_news.getData()[index].getMessageId() )     )
        {
            index = i;
        }   // if new unshown important message with smaller message id
    }
    core::stringw message("");
    if(index>=0)
    {
        const NewsMessage &m = m_news.getData()[index];
        message = m.getNews();
        UserConfigParams::m_last_important_message_id = m.getMessageId();

    }
    m_news.unlock();

    return message;
}   // getImportantMessage

// ----------------------------------------------------------------------------
/** Returns the next loaded news message. It will 'wrap around', i.e.
 *  if there is only one message it will be returned over and over again.
 *  To be used by the the main menu to get the next news message after
 *  one message was scrolled off screen.
 */
const core::stringw NewsManager::getNextNewsMessage()
{
    // Only display error message in case of a problem.
    if(m_error_message.size()>0)
        return _(m_error_message.c_str());

    m_news.lock();
    if(m_all_news_messages.size()>0)
    {
        // Copy the news message while it is locked.
        core::stringw anm = m_all_news_messages;
        m_news.unlock();
        return anm;
    }

    if(m_news.getData().size()==0)
    {
        // Lock 
        m_news.unlock();
        return "";
    }

    core::stringw m("");
    {
        m_current_news_message++;
        if(m_current_news_message >= (int)m_news.getData().size())
            m_current_news_message = 0;            

        m = m_news.getData()[m_current_news_message].getNews();
    }
    m_news.unlock();
    return _(m.c_str());
}   // getNextNewsMessage

// ----------------------------------------------------------------------------
/** Checks if the given condition list are all fulfilled.
 *  The conditions must be separated by ";", and each condition
 *  must be of the form "type comp version".
 *  Type must be 'stkversion'
 *  comp must be one of "<", "=", ">"
 *  version must be a valid STK version string
 *  \param cond The list of conditions
 *  \return True if all conditions are true.
 */
bool NewsManager::conditionFulfilled(const std::string &cond)
{
    std::vector<std::string> cond_list;
    cond_list = StringUtils::split(cond, ';');
    for(unsigned int i=0; i<cond_list.size(); i++)
    {
        std::vector<std::string> cond = StringUtils::split(cond_list[i],' ');
        if(cond.size()!=3)
        {
            Log::warn("NewsManager", "Invalid condition '%s' - assumed to "
                                     "be true.", cond_list[i].c_str());
            continue;
        }
        // Check for stkversion comparisons
        // ================================
        if(cond[0]=="stkversion")
        {
            int news_version = StringUtils::versionToInt(cond[2]);
            int stk_version  = StringUtils::versionToInt(STK_VERSION);
            if(cond[1]=="=")
            {
                if(stk_version!=news_version) return false;
                continue;
            }
            if(cond[1]=="<")
            {
                if(stk_version>=news_version) return false;
                continue;
            }
            if(cond[1]==">")
            {
                if(stk_version<=news_version) return false;
                continue;
            }
            Log::warn("NewsManager", "Invalid comparison in condition '%s' - "
                                     "assumed true.", cond_list[i].c_str());
        }
        // Check for addons not installed
        // ==============================
        else if(cond[1]=="not" && cond[2]=="installed")
        {
            // The addons_manager can not be access, since it's
            // being initialised after the news manager. So a simple
            // test is made to see if the directory exists. It is
            // necessary to check for karts and tracks separately,
            // since it's not possible to know if the addons is
            // a kart or a track.
            const std::string dir=file_manager->getAddonsDir();
            if(file_manager->fileExists(dir+"/karts/"+cond[0]))
                return false;
            if(file_manager->fileExists(dir+"/tracks/"+cond[0]))
                return false;
            continue;
        }
        else
        {
            Log::warn("NewsManager", "Invalid condition '%s' - assumed to "
                                     "be true.", cond_list[i].c_str());
            continue;
        }

    }   // for i < cond_list
    return true;
}   // conditionFulfilled

// ----------------------------------------------------------------------------
