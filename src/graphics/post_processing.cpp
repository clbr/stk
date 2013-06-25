//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2011-2013 the SuperTuxKart team
//  Copyright (C) 2013      Joerg Henrichs
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

#include "post_processing.hpp"

#include "config/user_config.hpp"
#include "io/file_manager.hpp"
#include "graphics/callbacks.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/rtts.hpp"
#include "graphics/shaders.hpp"
#include "race/race_manager.hpp"
#include "utils/log.hpp"

using namespace video;
using namespace scene;

PostProcessing::PostProcessing(video::IVideoDriver* video_driver)
{
    // Initialization
    m_material.Wireframe = false;
    m_material.Lighting = false;
    m_material.ZWriteEnable = false;
    m_material.ZBuffer = ECFN_ALWAYS;
    m_material.setFlag(EMF_TEXTURE_WRAP, ETC_CLAMP_TO_EDGE);
    m_material.setFlag(EMF_TRILINEAR_FILTER, true);
}   // PostProcessing

// ----------------------------------------------------------------------------
PostProcessing::~PostProcessing()
{
    // TODO: do we have to delete/drop anything?
}   // ~PostProcessing

// ----------------------------------------------------------------------------
/** Initialises post processing at the (re-)start of a race. This sets up
 *  the vertices, normals and texture coordinates for each
 */
void PostProcessing::reset()
{
    const u32 n = Camera::getNumCameras();
    m_boost_time.resize(n);
    m_vertices.resize(n);
    m_center.resize(n);
    m_direction.resize(n);

    MotionBlurProvider * const cb = (MotionBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_MOTIONBLUR];

    for(unsigned int i=0; i<n; i++)
    {
        m_boost_time[i] = 0.0f;

        const core::recti &vp = Camera::getCamera(i)->getViewport();
        // Map viewport to [-1,1] x [-1,1]. First define the coordinates
        // left, right, top, bottom:
        float right  = vp.LowerRightCorner.X < UserConfigParams::m_width
                     ? 0.0f : 1.0f;
        float left   = vp.UpperLeftCorner.X  > 0.0f ? 0.0f : -1.0f;
        float top    = vp.UpperLeftCorner.Y  > 0.0f ? 0.0f : 1.0f;
        float bottom = vp.LowerRightCorner.Y < UserConfigParams::m_height
                     ? 0.0f : -1.0f;

        // Use left etc to define 4 vertices on which the rendered screen
        // will be displayed:
        m_vertices[i].v0.Pos = core::vector3df(left,  bottom, 0);
        m_vertices[i].v1.Pos = core::vector3df(left,  top,    0);
        m_vertices[i].v2.Pos = core::vector3df(right, top,    0);
        m_vertices[i].v3.Pos = core::vector3df(right, bottom, 0);
        // Define the texture coordinates of each vertex, which must
        // be in [0,1]x[0,1]
        m_vertices[i].v0.TCoords  = core::vector2df(left  ==-1.0f ? 0.0f : 0.5f,
                                                    bottom==-1.0f ? 0.0f : 0.5f);
        m_vertices[i].v1.TCoords  = core::vector2df(left  ==-1.0f ? 0.0f : 0.5f,
                                                    top   == 1.0f ? 1.0f : 0.5f);
        m_vertices[i].v2.TCoords  = core::vector2df(right == 0.0f ? 0.5f : 1.0f,
                                                    top   == 1.0f ? 1.0f : 0.5f);
        m_vertices[i].v3.TCoords  = core::vector2df(right == 0.0f ? 0.5f : 1.0f,
                                                    bottom==-1.0f ? 0.0f : 0.5f);
        // Set normal and color:
        core::vector3df normal(0,0,1);
        m_vertices[i].v0.Normal = m_vertices[i].v1.Normal =
        m_vertices[i].v2.Normal = m_vertices[i].v3.Normal = normal;
        video::SColor white(0xFF, 0xFF, 0xFF, 0xFF);
        m_vertices[i].v0.Color  = m_vertices[i].v1.Color  =
        m_vertices[i].v2.Color  = m_vertices[i].v3.Color  = white;

        m_center[i].X=(m_vertices[i].v0.TCoords.X
                      +m_vertices[i].v2.TCoords.X) * 0.5f;

        // Center is around 20 percent from bottom of screen:
        float tex_height = m_vertices[i].v1.TCoords.Y
                         - m_vertices[i].v0.TCoords.Y;
        m_center[i].Y=m_vertices[i].v0.TCoords.Y + 0.2f*tex_height;
        m_direction[i].X = m_center[i].X;
        m_direction[i].Y = m_vertices[i].v0.TCoords.Y + 0.7f*tex_height;

        cb->setCenter(i, m_center[i].X, m_center[i].Y);
        cb->setDirection(i, m_direction[i].X, m_direction[i].Y);
        cb->setMaxHeight(i, m_vertices[i].v1.TCoords.Y);
    }  // for i <number of cameras
}   // reset

// ----------------------------------------------------------------------------
/** Setup some PP data.
 */
void PostProcessing::begin()
{
    bool any_boost = false;
    for(unsigned int i=0; i<m_boost_time.size(); i++)
        any_boost |= m_boost_time[i]>0.0f;
}   // beginCapture

// ----------------------------------------------------------------------------
/** Set the boost amount according to the speed of the camera */
void PostProcessing::giveBoost(unsigned int camera_index)
{
    m_boost_time[camera_index] = 0.75f;

    MotionBlurProvider * const cb = (MotionBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_MOTIONBLUR];
    cb->setBoostTime(camera_index, m_boost_time[camera_index]);
}   // giveBoost

// ----------------------------------------------------------------------------
/** Updates the boost times for all cameras, called once per frame.
 *  \param dt Time step size.
 */
void PostProcessing::update(float dt)
{
    MotionBlurProvider * const cb = (MotionBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_MOTIONBLUR];

    for(unsigned int i=0; i<m_boost_time.size(); i++)
    {
        if (m_boost_time[i] > 0.0f)
        {
            m_boost_time[i] -= dt;
            if (m_boost_time[i] < 0.0f) m_boost_time[i] = 0.0f;
        }

        cb->setBoostTime(i, m_boost_time[i]);
    }
}   // update

// ----------------------------------------------------------------------------
/** Render the post-processed scene */
void PostProcessing::render()
{
    const u16 indices[6] = {0, 1, 2, 3, 0, 2};

    video::IVideoDriver * const drv = irr_driver->getVideoDriver();
    drv->setTransform(video::ETS_WORLD, core::IdentityMatrix);
    drv->setTransform(video::ETS_VIEW, core::IdentityMatrix);
    drv->setTransform(video::ETS_PROJECTION, core::IdentityMatrix);

    MotionBlurProvider * const mocb = (MotionBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_MOTIONBLUR];
    GaussianBlurProvider * const gacb = (GaussianBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_GAUSSIAN3H];

    rtt_t * const rtts = irr_driver->getRTTs();
    Shaders * const shaders = irr_driver->getShaders();

    const u32 cams = Camera::getNumCameras();
    for(u32 cam = 0; cam < cams; cam++)
    {
        mocb->setCurrentCamera(cam);
        ITexture *in = rtts->getRTT(RTT_COLOR);
        ITexture *out = rtts->getRTT(RTT_TMP1);
	// Each effect uses these as named, and sets them up for the next effect.
	// This allows chaining effects where some may be disabled.

	// As the original color shouldn't be touched, the first effect can't be disabled.

        if (1) // bloom
        {
            // Blit the base to tmp1
            m_material.MaterialType = EMT_SOLID;
            m_material.setTexture(0, in);
            drv->setRenderTarget(out, true, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            // Catch bright areas, and progressively minify
            m_material.MaterialType = shaders->getShader(ES_BLOOM);
            m_material.setTexture(0, in);
            drv->setRenderTarget(rtts->getRTT(RTT_TMP3), true, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            // To half
            m_material.MaterialType = EMT_SOLID;
            m_material.setTexture(0, rtts->getRTT(RTT_TMP3));
            drv->setRenderTarget(rtts->getRTT(RTT_HALF1), true, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            // To quarter
            m_material.MaterialType = EMT_SOLID;
            m_material.setTexture(0, rtts->getRTT(RTT_HALF1));
            drv->setRenderTarget(rtts->getRTT(RTT_QUARTER1), true, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            // To eighth
            m_material.MaterialType = EMT_SOLID;
            m_material.setTexture(0, rtts->getRTT(RTT_QUARTER1));
            drv->setRenderTarget(rtts->getRTT(RTT_EIGHTH1), true, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            // Blur it for distribution, thrice gives good results while still fast
            // TODO: replace with one blur of radius 6 (sqrt(9*3))
            for (int i = 0; i < 3; i++)
            {
                gacb->setResolution(UserConfigParams::m_width / 8,
                                    UserConfigParams::m_height / 8);
                m_material.MaterialType = shaders->getShader(ES_GAUSSIAN3V);
                m_material.setTexture(0, rtts->getRTT(RTT_EIGHTH1));
                drv->setRenderTarget(rtts->getRTT(RTT_EIGHTH2), true, false);

                drv->setMaterial(m_material);
                drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

                m_material.MaterialType = shaders->getShader(ES_GAUSSIAN3H);
                m_material.setTexture(0, rtts->getRTT(RTT_EIGHTH2));
                drv->setRenderTarget(rtts->getRTT(RTT_EIGHTH1), false, false);

                drv->setMaterial(m_material);
                drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                                  4, indices, 2);
            }

            // Additively blend on top of tmp1
            m_material.MaterialType = EMT_TRANSPARENT_ADD_COLOR;
            m_material.setTexture(0, rtts->getRTT(RTT_EIGHTH1));
            drv->setRenderTarget(out, false, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            in = rtts->getRTT(RTT_TMP1);
            out = rtts->getRTT(RTT_TMP2);
        }

        if (1) // motion blur
        {
            m_material.MaterialType = shaders->getShader(ES_MOTIONBLUR);
            m_material.setTexture(0, in);
            drv->setRenderTarget(out, true, false);

            drv->setMaterial(m_material);
            drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                              4, indices, 2);

            ITexture *tmp = in;
            in = out;
            out = tmp;
        }

        // Final blit
// TODO, calculate if a flip is needed, apparently even passcount doesn't need it
//        m_material.MaterialType = shaders->getShader(ES_FLIP);
        m_material.MaterialType = EMT_SOLID;
        m_material.setTexture(0, in);
        drv->setRenderTarget(ERT_FRAME_BUFFER, false, false);

        drv->setMaterial(m_material);
        drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                          4, indices, 2);
    }

}   // render
