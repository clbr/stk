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



#ifndef HEADER_MODELVIEW_HPP
#define HEADER_MODELVIEW_HPP

#include <IMesh.h>

#include "graphics/irr_driver.hpp"
#include "guiengine/widgets/icon_button_widget.hpp"
#include "utils/aligned_array.hpp"
#include "utils/leak_check.hpp"
#include "utils/ptr_vector.hpp"

namespace GUIEngine
{
    /** \brief A model view widget.
      * \ingroup widgetsgroup
      */
    class ModelViewWidget : public IconButtonWidget
    {
        enum RotationMode
        {
            ROTATE_OFF,
            ROTATE_CONTINUOUSLY,
            ROTATE_TO
        };
        RotationMode m_rotation_mode;
        float m_rotation_speed;
        float m_rotation_target;
        
        PtrVector<scene::IMesh, REF> m_models;
        AlignedArray<Vec3> m_model_location;
        AlignedArray<Vec3> m_model_scale;
        std::vector<int> m_model_frames;
        
        video::ITexture* m_texture;
        
        IrrDriver::RTTProvider* m_rtt_provider;
        
        float angle;
        
        bool m_rtt_unsupported;
        
    public:
        
        LEAK_CHECK()
        
        ModelViewWidget();
        virtual ~ModelViewWidget();
        
        void add();
        void clearModels();
        void addModel(irr::scene::IMesh* mesh,
                      const Vec3& location = Vec3(0,0,0),
                      const Vec3& scale = Vec3(1,1,1),
                      const int frame=-1);
        
        void update(float delta);
        
        virtual void elementRemoved();
        
        /** Disables any model rotation */
        void setRotateOff();
        
        /** Makes the model rotate at given speed (in degrees per second) */
        void setRotateContinuously(float speed);
        
        /** Rotate to 'targetAngle' in degrees at given speed (in degrees per second) */
        void setRotateTo(float targetAngle, float speed);
        
        void clearRttProvider();
    };
    
}

#endif
