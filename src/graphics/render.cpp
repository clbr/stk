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
#include "graphics/lens_flare.hpp"
#include "graphics/light.hpp"
#include "graphics/lod_node.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/particle_kind_manager.hpp"
#include "graphics/per_camera_node.hpp"
#include "graphics/post_processing.hpp"
#include "graphics/referee.hpp"
#include "graphics/rtts.hpp"
#include "graphics/screenquad.hpp"
#include "graphics/shaders.hpp"
#include "graphics/shadow_importance.hpp"
#include "graphics/wind.hpp"
#include "items/item.hpp"
#include "items/item_manager.hpp"
#include "modes/world.hpp"
#include "physics/physics.hpp"
#include "tracks/track.hpp"
#include "utils/constants.hpp"
#include "utils/helpers.hpp"
#include "utils/log.hpp"
#include "utils/profiler.hpp"

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
    std::vector<GlowData> glows = m_glowing;
    std::vector<GlowNode *> transparent_glow_nodes;

    ItemManager * const items = ItemManager::get();
    const u32 itemcount = items->getNumberOfItems();
    u32 i;

    // For each static node, give it a glow representation
    const u32 staticglows = glows.size();
    for (i = 0; i < staticglows; i++)
    {
        scene::ISceneNode * const node = glows[i].node;

        const float radius = (node->getBoundingBox().getExtent().getLength() / 2) * 2.0f;
        GlowNode * const repnode = new GlowNode(irr_driver->getSceneManager(), radius);
        repnode->setPosition(node->getTransformedBoundingBox().getCenter());
        transparent_glow_nodes.push_back(repnode);
    }

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
        node->updateAbsolutePosition();

        GlowData dat;
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

    // Clear normal and depth to zero
    m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_NORMAL), true, false, video::SColor(0,0,0,0));
    m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_DEPTH), true, false, video::SColor(0,0,0,0));

    irr_driver->getVideoDriver()->enableMaterial2D();
    RaceGUIBase *rg = world->getRaceGUI();
    if (rg) rg->update(dt);


    for(unsigned int cam = 0; cam < Camera::getNumCameras(); cam++)
    {
        // Fire up the MRT
        m_video_driver->setRenderTarget(m_mrt, false, false);

        Camera *camera = Camera::getCamera(cam);

#ifdef ENABLE_PROFILER
        std::ostringstream oss;
        oss << "drawAll() for kart " << cam << std::flush;
        PROFILER_PUSH_CPU_MARKER(oss.str().c_str(), (cam+1)*60,
                                 0x00, 0x00);
#endif
        camera->activate();
        rg->preRenderCallback(camera);   // adjusts start referee

        m_renderpass = scene::ESNRP_CAMERA | scene::ESNRP_SOLID;
        m_scene_manager->drawAll(m_renderpass);

        ShadowImportanceProvider * const sicb = (ShadowImportanceProvider *)
                                                 irr_driver->getCallback(ES_SHADOW_IMPORTANCE);
        sicb->updateIPVMatrix();

        // Used to cull glowing items & lights
        const core::aabbox3df cambox = camera->getCameraSceneNode()->
                                             getViewFrustum()->getBoundingBox();
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

            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, ~0);
            glEnable(GL_STENCIL_TEST);

            for (u32 i = 0; i < glowcount; i++)
            {
                const GlowData &dat = glows[i];
                scene::ISceneNode * const cur = dat.node;

                // Quick box-based culling
                const core::aabbox3df nodebox = cur->getTransformedBoundingBox();
                if (!nodebox.intersectsWithBox(cambox))
                    continue;

                cb->setColor(dat.r, dat.g, dat.b);
                cur->render();
            }

            // Second round for transparents; it's a no-op for solids
            m_scene_manager->setCurrentRendertime(scene::ESNRP_TRANSPARENT);
            overridemat.Material.MaterialType = m_shaders->getShader(ES_COLORIZE_REF);
            for (u32 i = 0; i < glowcount; i++)
            {
                const GlowData &dat = glows[i];
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
            minimat.setFlag(video::EMF_TRILINEAR_FILTER, true);

            minimat.TextureLayer[0].TextureWrapU =
            minimat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;

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

        // Shadows
        if (!m_mipviz && UserConfigParams::m_shadows &&
            World::getWorld()->getTrack()->hasShadows())
        {
            m_scene_manager->setCurrentRendertime(scene::ESNRP_SOLID);
            static u8 tick = 0;

            const Vec3 *vmin, *vmax;
            World::getWorld()->getTrack()->getAABB(&vmin, &vmax);
            core::aabbox3df trackbox(vmin->toIrrVector(), vmax->toIrrVector() -
                                                          core::vector3df(0, 30, 0));

            scene::ICameraSceneNode * const camnode = camera->getCameraSceneNode();
            const float oldfar = camnode->getFarValue();
            camnode->setFarValue(std::min(100.0f, oldfar));
            camnode->render();
            const core::aabbox3df smallcambox = camnode->
                                             getViewFrustum()->getBoundingBox();
            camnode->setFarValue(oldfar);
            camnode->render();

            // Set up a nice ortho projection that contains our camera frustum
            core::matrix4 ortho;
            core::aabbox3df box = smallcambox;
            box = box.intersect(trackbox);

            m_suncam->getViewMatrix().transformBoxEx(box);
            m_suncam->getViewMatrix().transformBoxEx(trackbox);

            core::vector3df extent = trackbox.getExtent();
            const float w = fabsf(extent.X);
            const float h = fabsf(extent.Y);
            float z = box.MaxEdge.Z;

            // Snap to texels
            const float units_per_w = w / m_rtts->getRTT(RTT_SHADOW)->getSize().Width;
            const float units_per_h = h / m_rtts->getRTT(RTT_SHADOW)->getSize().Height;

            float left = box.MinEdge.X;
            float right = box.MaxEdge.X;
            float up = box.MaxEdge.Y;
            float down = box.MinEdge.Y;

            left -= fmodf(left, units_per_w);
            right -= fmodf(right, units_per_w);
            up -= fmodf(up, units_per_h);
            down -= fmodf(down, units_per_h);
            z -= fmodf(z, 0.5f);

            ortho.buildProjectionMatrixOrthoLH(left, right,
                                               up, down,
                                               30, z);

            m_suncam->setProjectionMatrix(ortho, true);
            m_scene_manager->setActiveCamera(m_suncam);
            m_suncam->render();

            ortho *= m_suncam->getViewMatrix();
            ((SunLightProvider *) m_shaders->m_callbacks[ES_SUNLIGHT])->setShadowMatrix(ortho);
            sicb->setShadowMatrix(ortho);

            // Render the importance map
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_SHADOW), true, true);

            m_shadow_importance->render();

            CollapseProvider * const colcb = (CollapseProvider *)
                                                 m_shaders->
                                                 m_callbacks[ES_COLLAPSE];
            ScreenQuad sq(m_video_driver);
            sq.setMaterialType(m_shaders->getShader(ES_COLLAPSE));
            sq.setTexture(m_rtts->getRTT(RTT_SHADOW));
            sq.getMaterial().setFlag(EMF_BILINEAR_FILTER, false);

            colcb->setResolution(1, m_rtts->getRTT(RTT_WARPV)->getSize().Height);
            sq.setTexture(m_rtts->getRTT(RTT_COLLAPSEH_OLD), 1);
            sq.render(m_rtts->getRTT(RTT_WARPH));

            colcb->setResolution(m_rtts->getRTT(RTT_WARPV)->getSize().Height, 1);
            sq.setTexture(m_rtts->getRTT(RTT_COLLAPSEV_OLD), 1);
            sq.render(m_rtts->getRTT(RTT_WARPV));

            sq.setTexture(0, 1);
            ((GaussianBlurProvider *) m_shaders->m_callbacks[ES_GAUSSIAN3H])->setResolution(
                       m_rtts->getRTT(RTT_WARPV)->getSize().Height,
                       m_rtts->getRTT(RTT_WARPV)->getSize().Height);

            sq.setMaterialType(m_shaders->getShader(ES_GAUSSIAN6H));
            sq.setTexture(m_rtts->getRTT(RTT_WARPH));
            sq.render(m_rtts->getRTT(RTT_COLLAPSEH));

            sq.setMaterialType(m_shaders->getShader(ES_GAUSSIAN6V));
            sq.setTexture(m_rtts->getRTT(RTT_WARPV));
            sq.render(m_rtts->getRTT(RTT_COLLAPSEV));

            // Blit them to the _old ones for next frame

            // Convert importance maps to warp maps
            //
            // It should be noted that while they do repeated work
            // calculating the min, max, and total, it's several hundred us
            // faster to do that than to do it once in a separate shader
            // (shader switch overhead, measured).
            colcb->setResolution(m_rtts->getRTT(RTT_WARPV)->getSize().Height,
                                 m_rtts->getRTT(RTT_WARPV)->getSize().Height);

            sq.setMaterialType(m_shaders->getShader(ES_SHADOW_WARPH));
            sq.setTexture(m_rtts->getRTT(RTT_COLLAPSEH));
            sq.render(m_rtts->getRTT(RTT_WARPH));

            sq.setMaterialType(m_shaders->getShader(ES_SHADOW_WARPV));
            sq.setTexture(m_rtts->getRTT(RTT_COLLAPSEV));
            sq.render(m_rtts->getRTT(RTT_WARPV));

            // Actual shadow map
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_SHADOW), true, true);
            overridemat.Material.MaterialType = m_shaders->getShader(ES_SHADOWPASS);
            overridemat.EnableFlags = video::EMF_MATERIAL_TYPE | video::EMF_TEXTURE1 |
                                      video::EMF_TEXTURE2 | video::EMF_WIREFRAME;
            overridemat.EnablePasses = scene::ESNRP_SOLID;
            overridemat.Material.setTexture(1, m_rtts->getRTT(RTT_WARPH));
            overridemat.Material.setTexture(2, m_rtts->getRTT(RTT_WARPV));
            overridemat.Material.TextureLayer[1].TextureWrapU =
            overridemat.Material.TextureLayer[1].TextureWrapV =
            overridemat.Material.TextureLayer[2].TextureWrapU =
            overridemat.Material.TextureLayer[2].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
            overridemat.Material.TextureLayer[1].BilinearFilter =
            overridemat.Material.TextureLayer[2].BilinearFilter = true;
            overridemat.Material.TextureLayer[1].TrilinearFilter =
            overridemat.Material.TextureLayer[2].TrilinearFilter = false;
            overridemat.Material.TextureLayer[1].AnisotropicFilter =
            overridemat.Material.TextureLayer[2].AnisotropicFilter = 0;
            overridemat.Material.Wireframe = m_wireframe;
            overridemat.Enabled = true;

            m_scene_manager->drawAll(scene::ESNRP_SOLID);

            overridemat.EnablePasses = 0;
            overridemat.Enabled = false;
            camera->activate();

            tick++;
            tick %= 2;
        }

        // Lights
        if (!m_lightviz)
        {
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_TMP1), true, false,
                                            video::SColor(255, 0, 0, 0));
        } else
        {
            m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_COLOR), false, false);
        }

        const vector3df camcenter = cambox.getCenter();
        const float camradius = cambox.getExtent().getLength() / 2;
        const vector3df campos = camera->getCameraSceneNode()->getPosition();
        const float camnear = camera->getCameraSceneNode()->getNearValue();

        m_scene_manager->drawAll(scene::ESNRP_CAMERA);
        PointLightProvider * const pcb = (PointLightProvider *) irr_driver->
                                            getCallback(ES_POINTLIGHT);
        pcb->updateIPVMatrix();
        SunLightProvider * const scb = (SunLightProvider *) irr_driver->
                                            getCallback(ES_SUNLIGHT);
        scb->updateIPVMatrix();

        const u32 lightcount = m_lights.size();
        for (i = 0; i < lightcount; i++)
        {
            // Sphere culling
            const float distance_sq = (m_lights[i]->getPosition() - camcenter).getLengthSQ();
            float radius_sum = camradius + m_lights[i]->getRadius();
            radius_sum *= radius_sum;
            if (radius_sum < distance_sq)
                continue;

            bool inside = false;

            const float camdistance_sq = (m_lights[i]->getPosition() - campos).getLengthSQ();
            float adjusted_radius = m_lights[i]->getRadius() + camnear;
            adjusted_radius *= adjusted_radius;

            // Camera inside the light's radius? Needs adjustment for the near plane.
            if (camdistance_sq < adjusted_radius)
            {
                inside = true;

                video::SMaterial &m = m_lights[i]->getMaterial(0);
                m.FrontfaceCulling = true;
                m.BackfaceCulling = false;
                m.ZBuffer = video::ECFN_GREATER;
            }

            if (m_lightviz)
            {
                overridemat.Enabled = true;
                overridemat.EnableFlags = video::EMF_MATERIAL_TYPE | video::EMF_WIREFRAME |
                                          video::EMF_FRONT_FACE_CULLING |
                                          video::EMF_BACK_FACE_CULLING |
                                          video::EMF_ZBUFFER;
                overridemat.Material.MaterialType = m_shaders->getShader(ES_COLORIZE);
                overridemat.Material.Wireframe = true;
                overridemat.Material.BackfaceCulling = false;
                overridemat.Material.FrontfaceCulling = false;
                overridemat.Material.ZBuffer = video::ECFN_LESSEQUAL;


                ColorizeProvider * const cb = (ColorizeProvider *) m_shaders->m_callbacks[ES_COLORIZE];
                float col[3];
                m_lights[i]->getColor(col);
                cb->setColor(col[0], col[1], col[2]);
            }

            // Action
            m_lights[i]->render();

            // Reset the inside change
            if (inside)
            {
                video::SMaterial &m = m_lights[i]->getMaterial(0);
                m.FrontfaceCulling = false;
                m.BackfaceCulling = true;
                m.ZBuffer = video::ECFN_LESSEQUAL;
            }

            if (m_lightviz)
            {
                overridemat.Enabled = false;
            }

        } // for i in lights

        // Blend lights to the image
        video::SMaterial lightmat;
        lightmat.Lighting = false;
        lightmat.ZWriteEnable = false;
        lightmat.ZBuffer = video::ECFN_ALWAYS;
        lightmat.setFlag(video::EMF_BILINEAR_FILTER, false);
        lightmat.setTexture(0, m_rtts->getRTT(RTT_TMP1));
        lightmat.MaterialType = m_shaders->getShader(ES_LIGHTBLEND);
        lightmat.MaterialTypeParam = video::pack_textureBlendFunc(video::EBF_DST_COLOR, video::EBF_ZERO);
        lightmat.BlendOperation = video::EBO_ADD;

        lightmat.TextureLayer[0].TextureWrapU =
        lightmat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;

        m_video_driver->setRenderTarget(m_rtts->getRTT(RTT_COLOR), false, false);
        if (!m_mipviz)
            m_post_processing->drawQuad(cam, lightmat);

        m_renderpass = scene::ESNRP_SKY_BOX;
        m_scene_manager->drawAll(m_renderpass);

        // Is the lens flare enabled & visible? Check last frame's query.
        const bool hasflare = World::getWorld()->getTrack()->hasLensFlare();
        const bool hasgodrays = World::getWorld()->getTrack()->hasGodRays();
        if (hasflare | hasgodrays)
        {
            irr::video::COpenGLDriver*	gl_driver = (irr::video::COpenGLDriver*)m_device->getVideoDriver();

            GLuint res;
            gl_driver->extGlGetQueryObjectuiv(m_lensflare_query, GL_QUERY_RESULT, &res);
            m_post_processing->setSunPixels(res);

            // Prepare the query for the next frame.
            gl_driver->extGlBeginQuery(GL_SAMPLES_PASSED_ARB, m_lensflare_query);
            m_scene_manager->setCurrentRendertime(scene::ESNRP_SOLID);
            m_scene_manager->drawAll(scene::ESNRP_CAMERA);
            m_sun_interposer->render();
            gl_driver->extGlEndQuery(GL_SAMPLES_PASSED_ARB);

            m_lensflare->setStrength(res / 4000.0f);

            if (hasflare)
                m_lensflare->OnRegisterSceneNode();

            // Make sure the color mask is reset
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }

        // We need to re-render camera due to the per-cam-node hack.
        m_renderpass = scene::ESNRP_CAMERA | scene::ESNRP_TRANSPARENT |
                                 scene::ESNRP_TRANSPARENT_EFFECT;
        m_scene_manager->drawAll(m_renderpass);

        // Drawing for this cam done, cleanup
        const u32 glowrepcount = transparent_glow_nodes.size();
        for (i = 0; i < glowrepcount; i++)
        {
            transparent_glow_nodes[i]->remove();
            transparent_glow_nodes[i]->drop();
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
