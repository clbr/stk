//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004 Steve Baker <sjbaker1@airmail.net>
//                2010 Steve Baker, Joerg Henrichs
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

#include "graphics/material.hpp"

#include <stdexcept>
#include <iostream>

#include "audio/sfx_base.hpp"
#include "audio/sfx_buffer.hpp"
#include "config/user_config.hpp"
#include "config/stk_config.hpp"
#include "guiengine/engine.hpp"
#include "graphics/callbacks.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/particle_kind_manager.hpp"
#include "graphics/shaders.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "utils/string_utils.hpp"
#include "modes/world.hpp"
#include "tracks/track.hpp"
#include "utils/log.hpp"

#include <IMaterialRendererServices.h>
#include <ISceneNode.h>
#include <IShaderConstantSetCallBack.h>

using namespace irr::video;

const unsigned int UCLAMP = 1;
const unsigned int VCLAMP = 2;

//-----------------------------------------------------------------------------
/** Create a new material using the parameters specified in the xml file.
 *  \param node Node containing the parameters for this material.
 *  \param index Index in material_manager.
 */
Material::Material(const XMLNode *node, int index, bool deprecated)
{
    m_deprecated = deprecated;

    node->get("name", &m_texname);

    if (m_texname=="")
    {
        throw std::runtime_error("[Material] No texture name specified "
                                 "in file\n");
    }
    init(index);

    bool b = false;

    node->get("clampu", &b);  if (b) m_clamp_tex |= UCLAMP; //blender 2.4 style
    node->get("clampU", &b);  if (b) m_clamp_tex |= UCLAMP; //blender 2.5 style
    b = false;
    node->get("clampv", &b);  if (b) m_clamp_tex |= VCLAMP; //blender 2.4 style
    node->get("clampV", &b);  if (b) m_clamp_tex |= VCLAMP; //blender 2.5 style

    node->get("transparency",     &m_alpha_testing     );
    node->get("lightmap",         &m_lightmap          );
    node->get("additive-lightmap",&m_additive_lightmap );

    std::string s;
    node->get("adjust-image",     &s                   );
    if(s=="premultiply")
        m_adjust_image = ADJ_PREMUL;
    else if (s=="divide")
        m_adjust_image = ADJ_DIV;
    else if (s=="" || s=="none")
        m_adjust_image = ADJ_NONE;
    else
        printf("Incorrect adjust-image specification: '%s' - ignored.\n",
               s.c_str());
    node->get("alpha",            &m_alpha_blending    );
    node->get("light",            &m_lighting          );

    node->get("smooth-reflection",&m_smooth_reflection_shader);
    node->get("high-adhesion",    &m_high_tire_adhesion);
    node->get("reset",            &m_drive_reset       );

    // backwards compatibility
    bool crash_reset = false;
    node->get("crash-reset",      &crash_reset         );
    if (crash_reset)
    {
        m_collision_reaction = RESCUE;
        m_drive_reset = true; // if crash reset is enabled then drive reset should be too
    }

    std::string creaction;
    node->get("collision-reaction", &creaction);
    if (creaction == "reset")
    {
        m_collision_reaction = RESCUE;
    }
    else if (creaction == "push")
    {
        m_collision_reaction = PUSH_BACK;
    }
    else if (creaction.size() > 0)
    {
        fprintf(stderr, "[Material] WARNING: Unknown collision reaction '%s'\n", creaction.c_str());
    }

    node->get("below-surface",    &m_below_surface     );
    node->get("falling-effect",   &m_falling_effect    );
    // A terrain with falling effect has to force a reset
    // when the kart is on it. So to make it easier for artists,
    // force the reset flag in this case.
    if(m_falling_effect)
        m_drive_reset=true;
    node->get("surface",          &m_surface           );
    node->get("ignore",           &m_ignore            );

    node->get("additive",         &m_add               );
    node->get("max-speed",        &m_max_speed_fraction);
    node->get("slowdown-time",    &m_slowdown_time     );
    node->get("backface-culling", &m_backface_culling  );
    node->get("disable-z-write",  &m_disable_z_write   );
    node->get("fog",              &m_fog               );

    node->get("mask",             &m_mask              );

    node->get("water-splash",     &m_water_splash      );
    node->get("jump",             &m_is_jump_texture   );

    if (m_collision_reaction != NORMAL)
    {
        node->get("collision-particles", &m_collision_particles);

        if (m_collision_particles.size() == 0)
        {
            // backwards compatibility
            node->get("crash-reset-particles", &m_collision_particles);
        }
    }


    s="";
    node->get("graphical-effect", &s);

    if (s == "water")
    {
        // For backwards compatibility only, eventually remove
        m_water_splash = true;
    }
    else if (s == "bubble")
    {
        m_graphical_effect = GE_BUBBLE;
    }
    else if (s == "grass")
    {
        m_graphical_effect = GE_GRASS;
        m_grass_speed = 1.5f;
        m_grass_amplitude = 0.25f;
        node->get("grass-speed", &m_grass_speed);
        node->get("grass-amplitude", &m_grass_amplitude);
    }
    else if (s == "water_shader")
    {
        m_graphical_effect = GE_WATER_SHADER;
        node->get("water-shader-speed-1", &m_water_shader_speed_1);
        node->get("water-shader-speed-2", &m_water_shader_speed_2);
    }
    else if (s == "normal_map")
    {
        m_graphical_effect = GE_NORMAL_MAP;
        node->get("normal-map",  &m_normal_map_tex);
        node->get("normal-light-map", &m_normal_map_shader_lightmap);

        // TODO: add support for parallax and height maps?
        /*
        else if (node->get("normal-heightmap",  &m_normal_map_tex))
        {
            m_is_heightmap = true;
            m_normal_map = true;
        }
        else if (node->get("parallax-map",  &m_normal_map_tex))
        {
            m_parallax_map = true;
            m_parallax_height = 0.2f;
            node->get("parallax-height", &m_parallax_height);
        }
        else if (node->get("parallax-heightmap",  &m_normal_map_tex))
        {
            m_is_heightmap = true;
            m_parallax_map = true;
            m_parallax_height = 0.2f;
            node->get("parallax-height", &m_parallax_height);
        }
         */
    }
    else if (s == "spheremap")
    {
        m_graphical_effect = GE_SPHERE_MAP;
    }
    else if (s == "splatting")
    {
        m_graphical_effect = GE_SPLATTING;
        node->get("splatting-texture-1", &m_splatting_texture_1);
        node->get("splatting-texture-2", &m_splatting_texture_2);
        node->get("splatting-texture-3", &m_splatting_texture_3);
        node->get("splatting-texture-4", &m_splatting_texture_4);
    }
    else if (s == "none")
    {
    }
    else if (s != "")
    {
        fprintf(stderr,
                "Invalid graphical effect specification: '%s' - ignored.\n",
                s.c_str());
    }
    else
    {
        m_graphical_effect = GE_NONE;
    }


    // BACKWARDS COMPATIBILITY, remove eventually
    bool use_normal_map = false;
    node->get("use-normal-map",  &use_normal_map);

    if (use_normal_map)
    {
        if (node->get("normal-map",  &m_normal_map_tex))
        {
            m_graphical_effect = GE_NORMAL_MAP;
        }
        else
        {
            fprintf(stderr, "[Material] WARNING: could not find normal map image in materials.xml\n");
        }

        node->get("normal-light-map", &m_normal_map_shader_lightmap);
    }


    // BACKWARDS COMPATIBILITY, remove eventually
    bool sphere_map = false;
    node->get("sphere",           &sphere_map          );
    if (sphere_map)
    {
        m_graphical_effect = GE_SPHERE_MAP;
    }


    if (node->get("compositing", &s))
    {
        if      (s == "blend")    m_alpha_blending = true;
        else if (s == "test")     m_alpha_testing = true;
        else if (s == "additive") m_add = true;
        else if (s == "coverage") m_alpha_to_coverage = true;
        else if (s != "none")
            fprintf(stderr,
                    "[Material] WARNING: Unknown compositing mode '%s'\n",
                    s.c_str());
    }


    bool water_shader = false;
    node->get("water-shader", &water_shader);
    if (water_shader)
    {
        // BACKWARDS COMPATIBILITY, eventually remove
        m_graphical_effect = GE_WATER_SHADER;
        node->get("water-shader-speed-1", &m_water_shader_speed_1);
        node->get("water-shader-speed-2", &m_water_shader_speed_2);
    }

    // Terrain-specifc sound effect
    const unsigned int children_count = node->getNumNodes();
    for (unsigned int i=0; i<children_count; i++)
    {
        const XMLNode *child_node = node->getNode(i);

        if (child_node->getName() == "sfx")
        {
            initCustomSFX(child_node);
        }
        else if (child_node->getName() == "particles")
        {
            initParticlesEffect(child_node);
        }
        else if (child_node->getName() == "zipper")
        {
            // Track version 4 uses a separate node:
            m_zipper                    = true;
            m_zipper_duration           = 3.5f;
            m_zipper_max_speed_increase = 15.0f;
            m_zipper_fade_out_time      = 3.0f;
            m_zipper_speed_gain         = 4.5f;
            m_zipper_engine_force       = 250;
            child_node->get("duration",          &m_zipper_duration          );
            child_node->get("fade-out-time",     &m_zipper_fade_out_time     );
            child_node->get("max-speed-increase",&m_zipper_max_speed_increase);
            child_node->get("speed-gain",        &m_zipper_speed_gain        );
            child_node->get("sengine-force",     &m_zipper_engine_force      );
        }
        else
        {
            fprintf(stderr,
                   "[Material] WARNING: unknown node type '%s' for texture "
                   "'%s' - ignored.\n",
                    child_node->getName().c_str(), m_texname.c_str());
        }

    }   // for i <node->getNumNodes()
    install(/*is_full_path*/false);
}   // Material

//-----------------------------------------------------------------------------
/** Create a standard material using the default settings for materials.
 *  \param fname Name of the texture file.
 *  \param index Unique index in material_manager.
 *  \param is_full_path If the fname contains the full path.
 */
Material::Material(const std::string& fname, int index, bool is_full_path,
                   bool complain_if_not_found)
{
    m_deprecated = false;

    m_texname = fname;
    init(index);
    install(is_full_path, complain_if_not_found);
}   // Material

//-----------------------------------------------------------------------------
/** Inits all material data with the default settings.
 *  \param Index of this material in the material_manager index array.
 */
void Material::init(unsigned int index)
{
    m_index                     = index;
    m_clamp_tex                 = 0;
    m_alpha_testing             = false;
    m_lightmap                  = false;
    m_additive_lightmap         = false;
    m_adjust_image              = ADJ_NONE;
    m_alpha_blending            = false;
    m_lighting                  = true;
    m_backface_culling          = true;
    m_smooth_reflection_shader  = false;
    m_high_tire_adhesion        = false;
    m_below_surface             = false;
    m_falling_effect            = false;
    m_surface                   = false;
    m_ignore                    = false;
    m_drive_reset               = false;
    m_collision_reaction        = NORMAL;
    m_add                       = false;
    m_disable_z_write           = false;
    m_water_shader_speed_1      = 6.6667f;
    m_water_shader_speed_2      = 4.0f;
    m_fog                       = true;
    m_max_speed_fraction        = 1.0f;
    m_slowdown_time             = 1.0f;
    m_sfx_name                  = "";
    m_sfx_min_speed             = 0.0f;
    m_sfx_max_speed             = 30;
    m_sfx_min_pitch             = 1.0f;
    m_sfx_max_pitch             = 1.0f;
    m_graphical_effect          = GE_NONE;
    m_zipper                    = false;
    m_zipper_duration           = -1.0f;
    m_zipper_fade_out_time      = -1.0f;
    m_zipper_max_speed_increase = -1.0f;
    m_zipper_speed_gain         = -1.0f;
    m_zipper_engine_force       = -1.0f;
    m_parallax_map              = false;
    m_is_heightmap              = false;
    m_alpha_to_coverage         = false;
    m_water_splash              = false;
    m_is_jump_texture           = false;

    for (int n=0; n<EMIT_KINDS_COUNT; n++)
    {
        m_particles_effects[n] = NULL;
    }
}   // init

//-----------------------------------------------------------------------------
void Material::install(bool is_full_path, bool complain_if_not_found)
{
    const std::string &full_path = is_full_path
                                 ? m_texname
                                 : file_manager->getTextureFile(m_texname);

    if (complain_if_not_found && full_path.size() == 0)
    {
        fprintf(stderr, "[Material] WARNING, cannot find texture '%s'\n", m_texname.c_str());
    }


    m_texture = irr_driver->getTexture(full_path,
                                       isPreMul(),
                                       isPreDiv(),
                                       complain_if_not_found);

    if (m_texture == NULL) return;

    // now set the name to the basename, so that all tests work as expected
    m_texname  = StringUtils::getBasename(m_texname);

    if (m_mask.size() > 0)
    {
        video::ITexture* tex = irr_driver->applyMask(m_texture, m_mask);
        if (tex)
        {
            irr_driver->removeTexture(m_texture);
            m_texture = tex;
        }
        else
        {
            fprintf(stderr, "Applying mask failed for '%s'!\n",
                    m_texname.c_str());
            return;
        }
    }
    m_texture->grab();
}   // install

//-----------------------------------------------------------------------------
Material::~Material()
{
    if (m_texture != NULL)
    {
        m_texture->drop();
        if(m_texture->getReferenceCount()==1)
            irr_driver->removeTexture(m_texture);
    }

    // If a special sfx is installed (that isn't part of stk itself), the
    // entry needs to be removed from the sfx_manager's mapping, since other
    // tracks might use the same name.
    if(m_sfx_name!="" && m_sfx_name==m_texname)
    {
        sfx_manager->deleteSFXMapping(m_sfx_name);
    }
}   // ~Material

//-----------------------------------------------------------------------------
/** Initialise the data structures for a custom sfx to be played when a
 *  kart is driving on that particular material.
 *  \param sfx The xml node containing the information for this sfx.
 */
void Material::initCustomSFX(const XMLNode *sfx)
{

    std::string filename;
    sfx->get("filename", &filename);

    if (filename.empty())
    {
        fprintf(stderr, "[Material] WARNING: sfx node has no 'filename' "
                        "attribute, sound effect will be ignored\n");
        return;
    }

    m_sfx_name = StringUtils::removeExtension(filename);
    sfx->get("min-speed", &m_sfx_min_speed); // 2.4 style
    sfx->get("min_speed", &m_sfx_min_speed); // 2.5 style

    sfx->get("max-speed", &m_sfx_max_speed); // 2.4 style
    sfx->get("max_speed", &m_sfx_max_speed); // 2.5 style

    sfx->get("min-pitch", &m_sfx_min_pitch); // 2.4 style
    sfx->get("min_pitch", &m_sfx_min_pitch); // 2.5 style

    sfx->get("max-pitch", &m_sfx_max_pitch); // 2.4 style
    sfx->get("max_pitch", &m_sfx_max_pitch); // 2.5 style

    m_sfx_pitch_per_speed = (m_sfx_max_pitch - m_sfx_min_pitch)
                          / (m_sfx_max_speed - m_sfx_min_speed);

    if(!sfx_manager->soundExist(m_sfx_name))
    {

        // The directory for the track was added to the model search path
        // so just misuse the getModelFile function
        const std::string full_path = file_manager->getModelFile(filename);
        SFXBuffer* buffer = sfx_manager->loadSingleSfx(sfx, full_path);

        if (buffer != NULL)
        {
            buffer->setPositional(true);
        }
    }
}   // initCustomSFX

//-----------------------------------------------------------------------------

void Material::initParticlesEffect(const XMLNode *node)
{
    ParticleKindManager* pkm = ParticleKindManager::get();

    std::string base;
    node->get("base", &base);
    if (base.size() < 1)
    {
        Log::warn("Material::initParticlesEffect"
                  "Invalid particle settings for material '%s'\n",
                  m_texname.c_str());
        return;
    }

    ParticleKind* particles = NULL;
    try
    {
        particles = pkm->getParticles(base.c_str());
        
        if (particles == NULL)
        {
            Log::warn("Material::initParticlesEffect",
                      "Error loading particles '%s' for material '%s'\n",
                      base.c_str(), m_texname.c_str());
        }
    }
    catch (...)
    {
        Log::warn("Material::initParticlesEffect",
                  "Cannot find particles '%s' for material '%s'\n",
                  base.c_str(), m_texname.c_str());
        return;
    }

    std::vector<std::string> conditions;
    node->get("condition", &conditions);

    const int count = conditions.size();

    if (count == 0)
    {
        fprintf(stderr, "[Material::initParticlesEffect] WARNING: Particles "
                        "'%s' for material '%s' are declared but not used "
                        "(no emission condition set)\n",
                base.c_str(), m_texname.c_str());
    }

    for (int c=0; c<count; c++)
    {
        if (conditions[c] == "skid")
        {
            m_particles_effects[EMIT_ON_SKID] = particles;
        }
        else if (conditions[c] == "drive")
        {
            m_particles_effects[EMIT_ON_DRIVE] = particles;
        }
        else
        {
            fprintf(stderr, "[Material::initParticlesEffect] WARNING: Unknown "
                            "condition '%s' for material '%s'\n",
                    conditions[c].c_str(), m_texname.c_str());
        }
    }
} // initParticlesEffect

//-----------------------------------------------------------------------------
/** Adjusts the pitch of the given sfx depending on the given speed.
 *  \param sfx The sound effect to adjust.
 *  \param speed The speed of the kart.
 */
void Material::setSFXSpeed(SFXBase *sfx, float speed) const
{
    // Still make a sound when driving backwards on the material.
    if (speed < 0) speed = -speed;

    // If we paused it due to too low speed earlier, we can continue now.
    if (sfx->getStatus() == SFXManager::SFX_PAUSED)
    {
        if (speed<m_sfx_min_speed) return;
        // TODO: Do we first need to stop the sound completely so it
        // starts over?
        sfx->play();
    }
    else if (sfx->getStatus() == SFXManager::SFX_PLAYING)
    {
        if (speed<m_sfx_min_speed)
        {
            // Pausing it to differentiate with sounds that ended etc
            sfx->pause();
            return;
        }
    }
    if (speed > m_sfx_max_speed)
    {
        sfx->speed(m_sfx_max_pitch);
        return;
    }

    float f = m_sfx_pitch_per_speed*(speed-m_sfx_min_speed) + m_sfx_min_pitch;
    sfx->speed(f);
}   // setSFXSpeed

//-----------------------------------------------------------------------------
/** Sets the appropriate flags in an irrlicht SMaterial.
 *  \param material The irrlicht SMaterial which gets the flags set.
 */
void  Material::setMaterialProperties(video::SMaterial *m, scene::IMeshBuffer* mb)
{
    // !!======== This method is only called for materials that can be found in
    //            materials.xml, if you want to set flags for all surfaces, see
    //            'MaterialManager::setAllMaterialFlags'

    if (m_deprecated || (m->getTexture(0) != NULL && ((core::stringc)m->getTexture(0)->getName()).find("deprecated") != -1))
    {
        fprintf(stderr, "WARNING: track uses deprecated texture <%s>\n", m_texname.c_str());
    }


    int modes = 0;

    if (m_alpha_testing)
    {
        m->MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
        modes++;
    }
    if (m_alpha_to_coverage)
    {
        m->MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
        if (UserConfigParams::m_graphical_effects &&
            irr_driver->getVideoDriver()->queryFeature(video::EVDF_ALPHA_TO_COVERAGE))
        {
            m->AntiAliasing = video::EAAM_QUALITY | video::EAAM_ALPHA_TO_COVERAGE;
        }
        modes++;
    }
    if (m_alpha_blending)
    {
        //m->MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

        // EMT_TRANSPARENT_ALPHA_CHANNEL does include vertex color alpha into
        // account, which messes up fading in/out effects. So we use the more
        // customizable EMT_ONETEXTURE_BLEND instead.
        m->MaterialType = video::EMT_ONETEXTURE_BLEND ;
        m->MaterialTypeParam =
            pack_textureBlendFunc(video::EBF_SRC_ALPHA,
                                  video::EBF_ONE_MINUS_SRC_ALPHA,
                                  video::EMFN_MODULATE_1X,
                                  video::EAS_TEXTURE | video::EAS_VERTEX_COLOR);

        modes++;
    }
    if (m_smooth_reflection_shader)
    {
        if (irr_driver->isGLSL())
        {
            m->MaterialType = irr_driver->getShaders()->getShader(ES_SPHERE_MAP);
        }
        else
        {
            m->MaterialType = video::EMT_SPHERE_MAP;

            // sphere map + alpha blending is a supported combination so in
            // this case don't increase mode count
            if (m_alpha_blending)
            {
                m->BlendOperation = video::EBO_ADD;
            }
            else
            {
                modes++;
            }
        }
    }
    if (m_graphical_effect == GE_SPHERE_MAP)
    {
        m->MaterialType = video::EMT_SPHERE_MAP;

        // sphere map + alpha blending is a supported combination so in
        // this case don't increase mode count
        if (m_alpha_blending)
        {
            m->BlendOperation = video::EBO_ADD;
        }
        else
        {
            modes++;
        }
    }

    if (m_lightmap)
    {
        m->MaterialType = video::EMT_LIGHTMAP;
        modes++;
    }
    if (m_additive_lightmap)
    {
        m->MaterialType = video::EMT_LIGHTMAP_ADD;
        modes++;
    }

    if (m_add)
    {
        //m->MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;

        // EMT_TRANSPARENT_ADD_COLOR does include vertex color alpha into
        // account, which messes up fading in/out effects. So we use the
        // more customizable EMT_ONETEXTURE_BLEND instead
        m->MaterialType = video::EMT_ONETEXTURE_BLEND ;
        m->MaterialTypeParam = pack_textureBlendFunc(video::EBF_SRC_ALPHA,
                                                    video::EBF_ONE,
                                                    video::EMFN_MODULATE_1X,
                                                    video::EAS_TEXTURE |
                                                      video::EAS_VERTEX_COLOR);
        modes++;
    }
    if (m_graphical_effect == GE_NORMAL_MAP)
    {
        IVideoDriver* video_driver = irr_driver->getVideoDriver();
        if (irr_driver->isGLSL())
        {
            ITexture* tex = irr_driver->getTexture(m_normal_map_tex);
            if (m_is_heightmap)
            {
                video_driver->makeNormalMapTexture( tex );
            }
            m->setTexture(1, tex);

            bool with_lightmap = false;

            if (m_normal_map_shader_lightmap.size() > 0)
            {
                ITexture* lm_tex = irr_driver->getTexture(m_normal_map_shader_lightmap);
                m->setTexture(2, lm_tex);
                with_lightmap = true;
            }

            // Material and shaders
            m->MaterialType = irr_driver->getShaders()->getShader(
                                 with_lightmap ? ES_NORMAL_MAP_LIGHTMAP : ES_NORMAL_MAP );
            m->Lighting = false;
            m->ZWriteEnable = true;

            modes++;
        }
        else
        {
            // remove normal map texture so that it's not blended with the rest
            m->setTexture(1, NULL);
        }
    }
    if (m_parallax_map)
    {
        video::ITexture* tex = irr_driver->getTexture(m_normal_map_tex);
        if (m_is_heightmap)
        {
            irr_driver->getVideoDriver()->makeNormalMapTexture( tex );
        }
        m->setTexture(1, tex);
        m->MaterialType = video::EMT_PARALLAX_MAP_SOLID;
        m->MaterialTypeParam = m_parallax_height;
        m->SpecularColor.set(0,0,0,0);
        modes++;
    }
    if (m_graphical_effect == GE_SPLATTING)
    {
        if (irr_driver->supportsSplatting())
        {
            ITexture* tex = irr_driver->getTexture(m_splatting_texture_1);
            m->setTexture(2, tex);

            if (m_splatting_texture_2.size() > 0)
            {
                tex = irr_driver->getTexture(m_splatting_texture_2);
            }
            m->setTexture(3, tex);

            if (m_splatting_texture_3.size() > 0)
            {
                tex = irr_driver->getTexture(m_splatting_texture_3);
            }
            m->setTexture(4, tex);

            if (m_splatting_texture_4.size() > 0)
            {
                tex = irr_driver->getTexture(m_splatting_texture_4);
            }
            m->setTexture(5, tex);

            // Material and shaders
            m->MaterialType = irr_driver->getShaders()->getShader(ES_SPLATTING);
        }
        else
        {
            m->MaterialType = video::EMT_SOLID;
        }
    }


    // Modify lightmap materials so that vertex colors are taken into account.
    // But disable lighting because we assume all lighting is already part
    // of the lightmap
    if (m->MaterialType == video::EMT_LIGHTMAP)
    {
        m->MaterialType = video::EMT_LIGHTMAP_LIGHTING;
        m->AmbientColor  = video::SColor(255, 255, 255, 255);
        m->DiffuseColor  = video::SColor(255, 255, 255, 255);
        m->EmissiveColor = video::SColor(255, 255, 255, 255);
        m->SpecularColor = video::SColor(255, 255, 255, 255);
    }

    if (m_graphical_effect == GE_BUBBLE && mb != NULL)
    {
        if (irr_driver->isGLSL())
        {
            BubbleEffectProvider * bubble = (BubbleEffectProvider *)
                                            irr_driver->getShaders()->m_callbacks[ES_BUBBLES];
            bubble->addBubble(mb);

            m->MaterialType = irr_driver->getShaders()->getShader(ES_BUBBLES);

            // alpha blending and bubble shading can work together so when both are enabled
            // don't increment the 'modes' counter to not get the 'too many modes' warning
            if (!m_alpha_blending)
            {
                modes++;
            }
        }
    }


    if (m_graphical_effect == GE_WATER_SHADER)
    {
        if (irr_driver->isGLSL())
        {
            m->setTexture(1, irr_driver->getTexture(file_manager->getTextureFile("waternormals.jpg")));
            m->setTexture(2, irr_driver->getTexture(file_manager->getTextureFile("waternormals2.jpg")));

            ((WaterShaderProvider *) irr_driver->getShaders()->m_callbacks[ES_WATER])->
                setSpeed(m_water_shader_speed_1/100.0f, m_water_shader_speed_2/100.0f);

            m->MaterialType = irr_driver->getShaders()->getShader(ES_WATER);
        }
        modes++;
    }

    if (m_graphical_effect == GE_GRASS)
    {
        if (UserConfigParams::m_weather_effects &&
            irr_driver->isGLSL())
        {
            // Only one grass speed & amplitude per map for now
            ((GrassShaderProvider *) irr_driver->getShaders()->m_callbacks[ES_GRASS])->
                setSpeed(m_grass_speed);
            ((GrassShaderProvider *) irr_driver->getShaders()->m_callbacks[ES_GRASS])->
                setAmplitude(m_grass_amplitude);

            // Material and shaders
            m->MaterialType = irr_driver->getShaders()->getShader(ES_GRASS);

        }
    }

    if (modes > 1)
    {
        std::cerr << "[Material::setMaterialProperties] More than one main "
                     "mode set for " << m_texname.c_str() << "\n";
    }

    if (m_disable_z_write)
    {
        m->ZWriteEnable = false;
    }

    if (!m_lighting)
    {
        //m->setFlag( video::EMF_LIGHTING, false );
        m->AmbientColor  = video::SColor(255, 255, 255, 255);
        m->DiffuseColor  = video::SColor(255, 255, 255, 255);
        m->EmissiveColor = video::SColor(255, 255, 255, 255);
        m->SpecularColor = video::SColor(255, 255, 255, 255);
    }

#ifdef DEBUG
    if(UserConfigParams::m_rendering_debug)
    {
        m->Shininess = 100.0f;
        m->DiffuseColor  = video::SColor(200, 255, 0, 0);
        m->AmbientColor  = video::SColor(200, 0, 0, 255);
        m->SpecularColor = video::SColor(200, 0, 255, 0);
    }
#endif

    if (UserConfigParams::m_anisotropic > 0)
    {
        for (u32 i=0; i<video::MATERIAL_MAX_TEXTURES; ++i)
        {
            m->TextureLayer[i].AnisotropicFilter =
                    UserConfigParams::m_anisotropic;
        }
    }
    else if (UserConfigParams::m_trilinear)
    {
        m->setFlag(video::EMF_TRILINEAR_FILTER, true);
    }

    // UV clamping
    if ( (m_clamp_tex & UCLAMP) != 0)
    {
        /**
        //! Texture is clamped to the last pixel
		ETC_CLAMP,
		//! Texture is clamped to the edge pixel
		ETC_CLAMP_TO_EDGE,
		//! Texture is clamped to the border pixel (if exists)
		ETC_CLAMP_TO_BORDER,
        */
        for (unsigned int n=0; n<video::MATERIAL_MAX_TEXTURES; n++)
        {
            m->TextureLayer[n].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
        }
    }
    if ( (m_clamp_tex & VCLAMP) != 0)
    {
        for (unsigned int n=0; n<video::MATERIAL_MAX_TEXTURES; n++)
        {
            m->TextureLayer[n].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
        }
    }

    // Backface culling
    if(!m_backface_culling)
        m->setFlag(video::EMF_BACK_FACE_CULLING, false);

    // Material color
    m->ColorMaterial = video::ECM_DIFFUSE_AND_AMBIENT;

#ifdef DEBUG
    if (UserConfigParams::m_rendering_debug)
    {
        m->ColorMaterial = video::ECM_NONE; // Override one above
    }
#endif

    //if (UserConfigParams::m_fullscreen_antialiasing)
    //    m->AntiAliasing = video::EAAM_LINE_SMOOTH;

} // setMaterialProperties

//-----------------------------------------------------------------------------

void Material::adjustForFog(scene::ISceneNode* parent, video::SMaterial *m,
                            bool use_fog) const
{
    // The new pipeline does fog as a post-process effect.
    if (irr_driver->isGLSL())
        return;

    m->setFlag(video::EMF_FOG_ENABLE, m_fog && use_fog);

    if (parent != NULL)
    {
        parent->setMaterialFlag(video::EMF_FOG_ENABLE, m_fog && use_fog);
    }
}   // adjustForFog

//-----------------------------------------------------------------------------

/** Callback from LOD nodes to create some effects */
void Material::onMadeVisible(scene::IMeshBuffer* who)
{
    BubbleEffectProvider * bubble = (BubbleEffectProvider *)
                                     irr_driver->getShaders()->m_callbacks[ES_BUBBLES];
    bubble->onMadeVisible(who);
}

//-----------------------------------------------------------------------------

/** Callback from LOD nodes to create some effects */
void Material::onHidden(scene::IMeshBuffer* who)
{
    BubbleEffectProvider * bubble = (BubbleEffectProvider *)
                                     irr_driver->getShaders()->m_callbacks[ES_BUBBLES];
    bubble->onHidden(who);
}

//-----------------------------------------------------------------------------

void Material::isInitiallyHidden(scene::IMeshBuffer* who)
{
    BubbleEffectProvider * bubble = (BubbleEffectProvider *)
                                     irr_driver->getShaders()->m_callbacks[ES_BUBBLES];
    bubble->isInitiallyHidden(who);
}

//-----------------------------------------------------------------------------
