//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009 Joerg Henrichs
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

#include "graphics/irr_driver.hpp"

#include "config/user_config.hpp"
#include "graphics/callbacks.hpp"
#include "graphics/camera.hpp"
#include "graphics/glow.hpp"
#include "graphics/glwrap.hpp"
#include "graphics/lod_node.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/particle_kind_manager.hpp"
#include "graphics/per_camera_node.hpp"
#include "graphics/post_processing.hpp"
#include "graphics/referee.hpp"
#include "graphics/rtts.hpp"
#include "graphics/shaders.hpp"
#include "graphics/wind.hpp"
#include "items/item.hpp"
#include "items/item_manager.hpp"
#include "modes/world.hpp"
#include "physics/physics.hpp"
#include "utils/constants.hpp"
#include "utils/log.hpp"
#include "utils/profiler.hpp"

#include <irrlicht.h>

void IrrDriver::renderGLSL(float dt)
{
    World *world = World::getWorld(); // Never NULL.

    // Overrides
    video::SOverrideMaterial &overridemat = m_video_driver->getOverrideMaterial();
    overridemat.EnablePasses = scene::ESNRP_SOLID | scene::ESNRP_TRANSPARENT;
    overridemat.EnableFlags = 0;

    if (m_wireframe)
    {
        overridemat.Material.Wireframe = 1;
        overridemat.EnableFlags |= video::EMF_WIREFRAME;
    }
    if (m_mipviz)
    {
        overridemat.Material.MaterialType = m_shaders->getShader(ES_MIPVIZ);
        overridemat.EnableFlags |= video::EMF_MATERIAL_TYPE;
        overridemat.EnablePasses = scene::ESNRP_SOLID;
    }

    // Get a list of all glowing things. The driver's list contains the static ones,
    // here we add items, as they may disappear each frame.
    std::vector<glowdata_t> glows = m_glowing;
    std::vector<GlowNode *> transparent_glow_nodes;

    ItemManager * const items = ItemManager::get();
    const u32 itemcount = items->getNumberOfItems();
    u32 i;

    for (i = 0; i < itemcount; i++)
    {
        Item * const item = items->getItem(i);
        if (!item) continue;
        const Item::ItemType type = item->getType();

        if (type != Item::ITEM_NITRO_BIG && type != Item::ITEM_NITRO_SMALL &&
            type != Item::ITEM_BONUS_BOX)
            continue;

        LODNode * const lod = (LODNode *) item->getSceneNode();
        if (!lod->isVisible()) continue;

        const int level = lod->getLevel();
        if (level < 0) continue;

        scene::ISceneNode * const node = lod->getAllNodes()[level];

        glowdata_t dat;
        dat.node = node;

        dat.r = 1.0f;
        dat.g = 1.0f;
        dat.b = 1.0f;

        // Item colors
        switch (type)
        {
            case Item::ITEM_NITRO_BIG:
            case Item::ITEM_NITRO_SMALL:
                dat.r = stk_config->m_nitro_glow_color[0];
                dat.g = stk_config->m_nitro_glow_color[1];
                dat.b = stk_config->m_nitro_glow_color[2];
            break;
            case Item::ITEM_BONUS_BOX:
                dat.r = stk_config->m_box_glow_color[0];
                dat.g = stk_config->m_box_glow_color[1];
                dat.b = stk_config->m_box_glow_color[2];
            break;
            default:
                Log::fatal("render", "Unknown item type got through");
            break;
        }

        glows.push_back(dat);

        // Push back its representation too
        const float radius = (node->getBoundingBox().getExtent().getLength() / 2) * 2.0f;
        GlowNode * const repnode = new GlowNode(irr_driver->getSceneManager(), radius);
        repnode->setPosition(node->getTransformedBoundingBox().getCenter());
        transparent_glow_nodes.push_back(repnode);
    }

    // Start the RTT for post-processing.
    // We do this before beginScene() because we want to capture the glClear()
    // because of tracks that do not have skyboxes (generally add-on tracks)
    m_post_processing->begin();
    m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_COLOR), false, false);

    m_video_driver->beginScene(/*backBuffer clear*/ true, /*zBuffer*/ true,
                               world->getClearColor());

    irr_driver->getVideoDriver()->enableMaterial2D();
    RaceGUIBase *rg = world->getRaceGUI();
    if (rg) rg->update(dt);


    for(unsigned int cam = 0; cam < Camera::getNumCameras(); cam++)
    {
        Camera *camera = Camera::getCamera(cam);

#ifdef ENABLE_PROFILER
        std::ostringstream oss;
        oss << "drawAll() for kart " << cam << std::flush;
        PROFILER_PUSH_CPU_MARKER(oss.str().c_str(), (cam+1)*60,
                                 0x00, 0x00);
#endif
        camera->activate();
        rg->preRenderCallback(camera);   // adjusts start referee

        // Lights to be removed with light prepass later
        m_renderpass = scene::ESNRP_CAMERA | scene::ESNRP_SOLID | scene::ESNRP_LIGHT;
        m_scene_manager->drawAll(m_renderpass);

        m_renderpass = scene::ESNRP_SKY_BOX;
        m_scene_manager->drawAll(m_renderpass);

        // Render anything glowing.
        if (!m_mipviz && !m_wireframe)
        {
            m_scene_manager->setCurrentRendertime(scene::ESNRP_SOLID);

            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_TMP1), false, false);
            glClearColor(0, 0, 0, 0);
            glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            const u32 glowcount = glows.size();
            ColorizeProvider * const cb = (ColorizeProvider *) m_shaders->m_callbacks[ES_COLORIZE];

            GlowProvider * const glowcb = (GlowProvider *) m_shaders->m_callbacks[ES_GLOW];
            glowcb->setResolution(UserConfigParams::m_width,
                                  UserConfigParams::m_height);

            overridemat.Material.MaterialType = m_shaders->getShader(ES_COLORIZE);
            overridemat.EnableFlags = video::EMF_MATERIAL_TYPE;
            overridemat.EnablePasses = scene::ESNRP_SOLID;
            overridemat.Enabled = true;

            const core::aabbox3df cambox = camera->getCameraSceneNode()->
                                                 getViewFrustum()->getBoundingBox();

            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, ~0);
            glEnable(GL_STENCIL_TEST);

            for (u32 i = 0; i < glowcount; i++)
            {
                const glowdata_t &dat = glows[i];
                scene::ISceneNode * const cur = dat.node;

                // Quick box-based culling
                const core::aabbox3df nodebox = cur->getTransformedBoundingBox();
                if (!nodebox.intersectsWithBox(cambox))
                    continue;

                cb->setColor(dat.r, dat.g, dat.b);
                cur->render();
            }
            overridemat.Enabled = false;
            overridemat.EnablePasses = 0;

            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glDisable(GL_STENCIL_TEST);

            // Cool, now we have the colors set up. Progressively minify.
            video::SMaterial minimat;
            minimat.Lighting = false;
            minimat.ZWriteEnable = false;
            minimat.ZBuffer = video::ECFN_ALWAYS;
            minimat.setFlag(video::EMF_TEXTURE_WRAP, video::ETC_CLAMP_TO_EDGE);
            minimat.setFlag(video::EMF_TRILINEAR_FILTER, true);

            // To half
            minimat.setTexture(0, m_rtts->getRTT(RTT_TMP1));
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_HALF1), false, false);
            m_post_processing->drawQuad(cam, minimat);

            // To quarter
            minimat.setTexture(0, m_rtts->getRTT(RTT_HALF1));
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_QUARTER1), false, false);
            m_post_processing->drawQuad(cam, minimat);

            // Blur it
            ((GaussianBlurProvider *) m_shaders->m_callbacks[ES_GAUSSIAN3H])->setResolution(
                       UserConfigParams::m_width / 4,
                       UserConfigParams::m_height / 4);

            minimat.MaterialType = m_shaders->getShader(ES_GAUSSIAN6H);
            minimat.setTexture(0, m_rtts->getRTT(RTT_QUARTER1));
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_QUARTER2), false, false);
            m_post_processing->drawQuad(cam, minimat);

            minimat.MaterialType = m_shaders->getShader(ES_GAUSSIAN6V);
            minimat.setTexture(0, m_rtts->getRTT(RTT_QUARTER2));
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_QUARTER1), false, false);
            m_post_processing->drawQuad(cam, minimat);

            // The glows will be rendered in the transparent phase
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_COLOR), false, false);

            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glDisable(GL_STENCIL_TEST);
        } // end glow

        // We need to re-render camera due to the per-cam-node hack.
        m_renderpass = scene::ESNRP_CAMERA | scene::ESNRP_TRANSPARENT |
                                 scene::ESNRP_TRANSPARENT_EFFECT | scene::ESNRP_LIGHT;
        m_scene_manager->drawAll(m_renderpass);

        const u32 glowrepcount = transparent_glow_nodes.size();
        for (i = 0; i < glowrepcount; i++)
        {
            transparent_glow_nodes[i]->remove();
        }

        PROFILER_POP_CPU_MARKER();

        // Note that drawAll must be called before rendering
        // the bullet debug view, since otherwise the camera
        // is not set up properly. This is only used for
        // the bullet debug view.
        if (UserConfigParams::m_artist_debug_mode)
            World::getWorld()->getPhysics()->draw();
    }   // for i<world->getNumKarts()

    // Render the post-processed scene
    m_post_processing->render();

    // Set the viewport back to the full screen for race gui
    m_video_driver->setViewPort(core::recti(0, 0,
                                            UserConfigParams::m_width,
                                            UserConfigParams::m_height));

    for(unsigned int i=0; i<Camera::getNumCameras(); i++)
    {
        Camera *camera = Camera::getCamera(i);
        char marker_name[100];
        sprintf(marker_name, "renderPlayerView() for kart %d", i);

        PROFILER_PUSH_CPU_MARKER(marker_name, 0x00, 0x00, (i+1)*60);
        rg->renderPlayerView(camera, dt);

        PROFILER_POP_CPU_MARKER();
    }  // for i<getNumKarts

    // Either render the gui, or the global elements of the race gui.
    GUIEngine::render(dt);

    // Render the profiler
    if(UserConfigParams::m_profiler_enabled)
    {
        PROFILER_DRAW();
    }


#ifdef DEBUG
    drawDebugMeshes();
#endif

    m_video_driver->endScene();

    getPostProcessing()->update(dt);
}

// --------------------------------------------

void IrrDriver::renderFixed(float dt)
{
    World *world = World::getWorld(); // Never NULL.

    m_video_driver->beginScene(/*backBuffer clear*/ true, /*zBuffer*/ true,
                               world->getClearColor());

    irr_driver->getVideoDriver()->enableMaterial2D();

    RaceGUIBase *rg = world->getRaceGUI();
    if (rg) rg->update(dt);


    for(unsigned int i=0; i<Camera::getNumCameras(); i++)
    {
        Camera *camera = Camera::getCamera(i);

#ifdef ENABLE_PROFILER
        std::ostringstream oss;
        oss << "drawAll() for kart " << i << std::flush;
        PROFILER_PUSH_CPU_MARKER(oss.str().c_str(), (i+1)*60,
                                 0x00, 0x00);
#endif
        camera->activate();
        rg->preRenderCallback(camera);   // adjusts start referee

        m_renderpass = ~0;
        m_scene_manager->drawAll();

        PROFILER_POP_CPU_MARKER();

        // Note that drawAll must be called before rendering
        // the bullet debug view, since otherwise the camera
        // is not set up properly. This is only used for
        // the bullet debug view.
        if (UserConfigParams::m_artist_debug_mode)
            World::getWorld()->getPhysics()->draw();
    }   // for i<world->getNumKarts()

    // Set the viewport back to the full screen for race gui
    m_video_driver->setViewPort(core::recti(0, 0,
                                            UserConfigParams::m_width,
                                            UserConfigParams::m_height));

    for(unsigned int i=0; i<Camera::getNumCameras(); i++)
    {
        Camera *camera = Camera::getCamera(i);
        char marker_name[100];
        sprintf(marker_name, "renderPlayerView() for kart %d", i);

        PROFILER_PUSH_CPU_MARKER(marker_name, 0x00, 0x00, (i+1)*60);
        rg->renderPlayerView(camera, dt);
        PROFILER_POP_CPU_MARKER();

    }  // for i<getNumKarts

    // Either render the gui, or the global elements of the race gui.
    GUIEngine::render(dt);

    // Render the profiler
    if(UserConfigParams::m_profiler_enabled)
    {
        PROFILER_DRAW();
    }

#ifdef DEBUG
    drawDebugMeshes();
#endif

    m_video_driver->endScene();
}
