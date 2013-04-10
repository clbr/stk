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


#ifndef HEADER_CONFIRM_DIALOG_HPP
#define HEADER_CONFIRM_DIALOG_HPP

#include "config/player.hpp"
#include "guiengine/modaldialog.hpp"
#include "utils/leak_check.hpp"

/**
 * \brief Generic dialog to ask the user to confirm something, or to show a simple message box
 * \ingroup states_screens
 */
class MessageDialog : public GUIEngine::ModalDialog
{
public:
    
    /**
     * \brief Listener interface to get notified of whether the user chose to confirm or cancel
     * \ingroup states_screens
     */
    class IConfirmDialogListener
    {
    public:
        
        LEAK_CHECK()
        
        IConfirmDialogListener() {}
        virtual ~IConfirmDialogListener() {}
        
        /** \brief Implement to be notified of dialog confirmed.
          * \note  The dialog is not closed automatically, close it in the callback if this
          *        behavior is desired.
          */
        virtual void onConfirm() { ModalDialog::dismiss(); };
        
        /** \brief Implement to be notified of dialog cancelled.
          * \note  The default implementation is to close the modal dialog, but you may override
          *        this method to change the behavior.
          */
        virtual void onCancel() { ModalDialog::dismiss(); };
        
        /**
          * \brief Optional callback
          */
        virtual void onDialogUpdate(float dt) {}
    };

    enum MessageDialogType { MESSAGE_DIALOG_OK, MESSAGE_DIALOG_CONFIRM };
    
private:
    
    IConfirmDialogListener* m_listener;
    bool m_own_listener;
    void doInit(irr::core::stringw msg, MessageDialogType type, IConfirmDialogListener* listener, bool own_listener);

public:

    /**
      * \param msg Message to display in the dialog
      * \param listener A listener object to notify when the user made a choice.
      * \param If set to true, 'listener' will be owned by this dialog and deleted
      *        along with the dialog.
      */
    MessageDialog(irr::core::stringw msg, MessageDialogType type, IConfirmDialogListener* listener, bool delete_listener);
    
    /**
      * Variant of MessageDialog where cancelling is not possible (i.e. just shows a message box with OK)
      * \param msg Message to display in the dialog
      */
    MessageDialog(irr::core::stringw msg);

    
    ~MessageDialog();
    
    virtual void onEnterPressedInternal();
    virtual void onUpdate(float dt);

    GUIEngine::EventPropagation processEvent(const std::string& eventSource);
};


#endif
