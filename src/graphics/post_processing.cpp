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
#include "graphics/mlaa_areamap.hpp"
#include "graphics/rtts.hpp"
#include "graphics/shaders.hpp"
#include "modes/world.hpp"
#include "race/race_manager.hpp"
#include "tracks/track.hpp"
#include "utils/log.hpp"

using namespace video;
using namespace scene;

PostProcessing::PostProcessing(IVideoDriver* video_driver)
{
    // Initialization
    m_material.Wireframe = false;
    m_material.Lighting = false;
    m_material.ZWriteEnable = false;
    m_material.ZBuffer = ECFN_ALWAYS;
    m_material.setFlag(EMF_TEXTURE_WRAP, ETC_CLAMP_TO_EDGE);
    m_material.setFlag(EMF_TRILINEAR_FILTER, true);

    // Load the MLAA area map
    io::IReadFile *areamap = irr_driver->getDevice()->getFileSystem()->
                         createMemoryReadFile((void *) AreaMap33, sizeof(AreaMap33),
                         "AreaMap33", false);
    if (!areamap) Log::fatal("postprocessing", "Failed to load the areamap");
    m_areamap = irr_driver->getVideoDriver()->getTexture(areamap);
    areamap->drop();

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
        SColor white(0xFF, 0xFF, 0xFF, 0xFF);
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
    m_any_boost = false;
    for(unsigned int i=0; i<m_boost_time.size(); i++)
        m_any_boost |= m_boost_time[i]>0.0f;
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
    IVideoDriver * const drv = irr_driver->getVideoDriver();
    drv->setTransform(ETS_WORLD, core::IdentityMatrix);
    drv->setTransform(ETS_VIEW, core::IdentityMatrix);
    drv->setTransform(ETS_PROJECTION, core::IdentityMatrix);

    MotionBlurProvider * const mocb = (MotionBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_MOTIONBLUR];
    GaussianBlurProvider * const gacb = (GaussianBlurProvider *) irr_driver->getShaders()->
                                                           m_callbacks[ES_GAUSSIAN3H];

    RTT * const rtts = irr_driver->getRTTs();
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

            drawQuad(cam, m_material);

            const bool globalbloom = World::getWorld()->getTrack()->getBloom();

            if (globalbloom)
            {
                const float threshold = World::getWorld()->getTrack()->getBloomThreshold();
                ((BloomProvider *) shaders->m_callbacks[ES_BLOOM])->setThreshold(threshold);

                // Catch bright areas, and progressively minify
                m_material.MaterialType = shaders->getShader(ES_BLOOM);
                m_material.setTexture(0, in);
                drv->setRenderTarget(rtts->getRTT(RTT_TMP3), true, false);

                drawQuad(cam, m_material);
            }

            // Do we have any forced bloom nodes? If so, draw them now
            const std::vector<scene::ISceneNode *> &blooms = irr_driver->getForcedBloom();
            const u32 bloomsize = blooms.size();

            if (!globalbloom && bloomsize)
                drv->setRenderTarget(rtts->getRTT(RTT_TMP3), true, false);


            if (globalbloom || bloomsize)
            {
                // The forced-bloom objects are drawn again, to know which pixels to pick.
                // While it's more drawcalls, there's a cost to using four MRTs over three,
                // and there shouldn't be many such objects in a track.
                // The stencil is already in use for the glow. The alpha channel is best
                // reserved for other use (specular, etc).
                //
                // They are drawn with depth and color writes off, giving 4x-8x drawing speed.
                if (bloomsize)
                {
                    const core::aabbox3df &cambox = Camera::getCamera(cam)->
                                                    getCameraSceneNode()->
                                                    getViewFrustum()->
                                                    getBoundingBox();

                    irr_driver->getSceneManager()->setCurrentRendertime(ESNRP_SOLID);
                    SOverrideMaterial &overridemat = drv->getOverrideMaterial();
                    overridemat.EnablePasses = ESNRP_SOLID;
                    overridemat.EnableFlags = EMF_MATERIAL_TYPE | EMF_ZWRITE_ENABLE | EMF_COLOR_MASK;
                    overridemat.Enabled = true;

                    overridemat.Material.MaterialType = EMT_SOLID;
                    overridemat.Material.ZWriteEnable = false;
                    overridemat.Material.ColorMask = ECP_NONE;
                    glClear(GL_STENCIL_BUFFER_BIT);

                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                    glStencilFunc(GL_ALWAYS, 1, ~0);
                    glEnable(GL_STENCIL_TEST);

                    Camera::getCamera(cam)->getCameraSceneNode()->render();

                    for (u32 i = 0; i < bloomsize; i++)
                    {
                        scene::ISceneNode * const cur = blooms[i];

                        // Quick box-based culling
                        const core::aabbox3df nodebox = cur->getTransformedBoundingBox();
                        if (!nodebox.intersectsWithBox(cambox))
                            continue;

                        cur->render();
                    }

                    overridemat.Enabled = 0;
                    overridemat.EnablePasses = 0;

                    // Ok, we have the stencil; now use it to blit from color to bloom tex
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                    glStencilFunc(GL_EQUAL, 1, ~0);
                    m_material.MaterialType = EMT_SOLID;
                    m_material.setTexture(0, rtts->getRTT(RTT_COLOR));

                    // Just in case.
                    glColorMask(1, 1, 1, 1);
                    drv->setRenderTarget(rtts->getRTT(RTT_TMP3), false, false);

                    drawQuad(cam, m_material);

                    glDisable(GL_STENCIL_TEST);
                } // end forced bloom

                // To half
                m_material.MaterialType = EMT_SOLID;
                m_material.setTexture(0, rtts->getRTT(RTT_TMP3));
                drv->setRenderTarget(rtts->getRTT(RTT_HALF1), true, false);

                drawQuad(cam, m_material);

                // To quarter
                m_material.MaterialType = EMT_SOLID;
                m_material.setTexture(0, rtts->getRTT(RTT_HALF1));
                drv->setRenderTarget(rtts->getRTT(RTT_QUARTER1), true, false);

                drawQuad(cam, m_material);

                // To eighth
                m_material.MaterialType = EMT_SOLID;
                m_material.setTexture(0, rtts->getRTT(RTT_QUARTER1));
                drv->setRenderTarget(rtts->getRTT(RTT_EIGHTH1), true, false);

                drawQuad(cam, m_material);

                // Blur it for distribution.
                {
                    gacb->setResolution(UserConfigParams::m_width / 8,
                                        UserConfigParams::m_height / 8);
                    m_material.MaterialType = shaders->getShader(ES_GAUSSIAN6V);
                    m_material.setTexture(0, rtts->getRTT(RTT_EIGHTH1));
                    drv->setRenderTarget(rtts->getRTT(RTT_EIGHTH2), true, false);

                    drawQuad(cam, m_material);

                    m_material.MaterialType = shaders->getShader(ES_GAUSSIAN6H);
                    m_material.setTexture(0, rtts->getRTT(RTT_EIGHTH2));
                    drv->setRenderTarget(rtts->getRTT(RTT_EIGHTH1), false, false);

                    drawQuad(cam, m_material);
                }

                // Additively blend on top of tmp1
                m_material.MaterialType = EMT_TRANSPARENT_ADD_COLOR;
                m_material.setTexture(0, rtts->getRTT(RTT_EIGHTH1));
                drv->setRenderTarget(out, false, false);

                drawQuad(cam, m_material);
            } // end if bloom

            in = rtts->getRTT(RTT_TMP1);
            out = rtts->getRTT(RTT_TMP2);
        }

        if (UserConfigParams::m_motionblur && m_any_boost) // motion blur
        {
            m_material.MaterialType = shaders->getShader(ES_MOTIONBLUR);
            m_material.setTexture(0, in);
            drv->setRenderTarget(out, true, false);

            drawQuad(cam, m_material);

            ITexture *tmp = in;
            in = out;
            out = tmp;
        }

        if (UserConfigParams::m_ssao == 1) // SSAO low
        {
/*            m_material.MaterialType = shaders->getShader(ES_SSAO);
            m_material.setTexture(0, in);
            drv->setRenderTarget(out, true, false);

            drawQuad(cam, m_material);

            ITexture *tmp = in;
            in = out;
            out = tmp;*/
        } else if (UserConfigParams::m_ssao == 2) // SSAO high
        {
/*            m_material.MaterialType = shaders->getShader(ES_SSAO);
            m_material.setTexture(0, in);
            drv->setRenderTarget(out, true, false);

            drawQuad(cam, m_material);

            ITexture *tmp = in;
            in = out;
            out = tmp;*/
        }

        if (UserConfigParams::m_mlaa) // MLAA. Must be the last pp filter.
        {
            drv->setRenderTarget(out, false, false);

            glEnable(GL_STENCIL_TEST);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glStencilFunc(GL_ALWAYS, 1, ~0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            // Pass 1: color edge detection
            m_material.setFlag(EMF_BILINEAR_FILTER, false);
            m_material.setFlag(EMF_TRILINEAR_FILTER, false);
            m_material.MaterialType = shaders->getShader(ES_MLAA_COLOR1);
            m_material.setTexture(0, in);

            drawQuad(cam, m_material);
            m_material.setFlag(EMF_BILINEAR_FILTER, true);
            m_material.setFlag(EMF_TRILINEAR_FILTER, true);

            glStencilFunc(GL_EQUAL, 1, ~0);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            // Pass 2: blend weights
            drv->setRenderTarget(rtts->getRTT(RTT_TMP3), true, false);

            m_material.MaterialType = shaders->getShader(ES_MLAA_BLEND2);
            m_material.setTexture(0, out);
            m_material.setTexture(1, m_areamap);
            m_material.TextureLayer[1].BilinearFilter = false;
            m_material.TextureLayer[1].TrilinearFilter = false;

            drawQuad(cam, m_material);

            m_material.TextureLayer[1].BilinearFilter = true;
            m_material.TextureLayer[1].TrilinearFilter = true;
            m_material.setTexture(1, 0);

            // Pass 3: gather
            drv->setRenderTarget(in, false, false);

            m_material.setFlag(EMF_BILINEAR_FILTER, false);
            m_material.setFlag(EMF_TRILINEAR_FILTER, false);
            m_material.MaterialType = shaders->getShader(ES_MLAA_NEIGH3);
            m_material.setTexture(0, rtts->getRTT(RTT_TMP3));
            m_material.setTexture(1, rtts->getRTT(RTT_COLOR));

            drawQuad(cam, m_material);

            m_material.setFlag(EMF_BILINEAR_FILTER, true);
            m_material.setFlag(EMF_TRILINEAR_FILTER, true);
            m_material.setTexture(1, 0);

            // Done.
            glDisable(GL_STENCIL_TEST);
        }

        // Final blit

        if (irr_driver->getNormals())
        {
            m_material.MaterialType = shaders->getShader(ES_FLIP);
            m_material.setTexture(0, rtts->getRTT(RTT_NORMAL));
        } else
        {
            m_material.MaterialType = shaders->getShader(ES_FLIP);
            m_material.setTexture(0, in);
        }

        drv->setRenderTarget(ERT_FRAME_BUFFER, false, false);

        drawQuad(cam, m_material);
    }

}   // render

void PostProcessing::drawQuad(u32 cam, const SMaterial &mat)
{
    const u16 indices[6] = {0, 1, 2, 3, 0, 2};
    IVideoDriver * const drv = irr_driver->getVideoDriver();

    drv->setTransform(ETS_WORLD, core::IdentityMatrix);
    drv->setTransform(ETS_VIEW, core::IdentityMatrix);
    drv->setTransform(ETS_PROJECTION, core::IdentityMatrix);

    drv->setMaterial(mat);
    drv->drawIndexedTriangleList(&(m_vertices[cam].v0),
                                      4, indices, 2);
}
