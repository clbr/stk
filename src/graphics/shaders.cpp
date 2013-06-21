//  SuperTuxKart - a fun racing game with go-kart
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

#include "graphics/callbacks.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/shaders.hpp"
#include "io/file_manager.hpp"
#include "utils/log.hpp"

#include <assert.h>
#include <IGPUProgrammingServices.h>

using namespace video;

Shaders::Shaders()
{
    const std::string &dir = file_manager->getShaderDir();

    IGPUProgrammingServices * const gpu = irr_driver->getVideoDriver()->getGPUProgrammingServices();

    #define glsl(a, b, c) gpu->addHighLevelShaderMaterialFromFiles((a).c_str(), (b).c_str(), (IShaderConstantSetCallBack*) c)
    #define glslmat(a, b, c, d) gpu->addHighLevelShaderMaterialFromFiles((a).c_str(), (b).c_str(), (IShaderConstantSetCallBack*) c, d)
    #define glsl_noinput(a, b) gpu->addHighLevelShaderMaterialFromFiles((a).c_str(), (b).c_str(), (IShaderConstantSetCallBack*) 0)

    // Callbacks
    memset(m_callbacks, 0, sizeof(m_callbacks));

    m_callbacks[ES_SPHERE_MAP] = new SphereMapProvider();
    m_callbacks[ES_NORMAL_MAP_LIGHTMAP] = new NormalMapProvider(true);
    m_callbacks[ES_NORMAL_MAP] = new NormalMapProvider(false);
    m_callbacks[ES_SPLATTING] = new SplattingProvider(false);
    m_callbacks[ES_SPLATTING_LIGHTMAP] = new SplattingProvider(true);
    m_callbacks[ES_WATER] = new WaterShaderProvider();
    m_callbacks[ES_GRASS] = new GrassShaderProvider();
    m_callbacks[ES_BUBBLES] = new BubbleEffectProvider();
    m_callbacks[ES_RAIN] = new RainEffectProvider();
    m_callbacks[ES_SNOW] = new SnowEffectProvider();
    m_callbacks[ES_MOTIONBLUR] = new MotionBlurProvider();

    // Ok, go
    m_shaders[ES_NORMAL_MAP] = glslmat(dir + "normalmap.vert", dir + "normalmap.frag",
                                       m_callbacks[ES_NORMAL_MAP], EMT_SOLID_2_LAYER);

    m_shaders[ES_NORMAL_MAP_LIGHTMAP] = glslmat(dir + "normalmap.vert", dir + "normalmap.frag",
                                             m_callbacks[ES_NORMAL_MAP_LIGHTMAP], EMT_SOLID_2_LAYER);

    m_shaders[ES_SPLATTING] = glsl(dir + "splatting.vert", dir + "splatting.frag",
                                   m_callbacks[ES_SPLATTING]);

//    m_shaders[ES_SPLATTING_LIGHTMAP] = glsl(dir + "splatting_lightmap.vert", dir + "splatting_lightmap.frag",
//                                            m_callbacks[ES_SPLATTING_LIGHTMAP]);
// This doesn't exist ATM, default to solid
    m_shaders[ES_SPLATTING_LIGHTMAP] = EMT_SOLID;

    m_shaders[ES_WATER] = glslmat(dir + "water.vert", dir + "water.frag",
                                  m_callbacks[ES_WATER], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_SPHERE_MAP] = glslmat(dir + "spheremap.vert", dir + "spheremap.frag",
                                       m_callbacks[ES_SPHERE_MAP], EMT_SOLID_2_LAYER);

    m_shaders[ES_GRASS] = glslmat(dir + "grass.vert", dir + "grass.frag",
                                  m_callbacks[ES_GRASS], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_BUBBLES] = glslmat(dir + "bubble.vert", dir + "bubble.frag",
                                    m_callbacks[ES_BUBBLES], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_RAIN] = glslmat(dir + "rain.vert", dir + "rain.frag",
                                    m_callbacks[ES_RAIN], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_SNOW] = glslmat(dir + "snow.vert", dir + "snow.frag",
                                    m_callbacks[ES_SNOW], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_MOTIONBLUR] = glslmat(dir + "motion_blur.vert", dir + "motion_blur.frag",
                                    m_callbacks[ES_MOTIONBLUR], EMT_SOLID);

    // Check that all successfully loaded
    u32 i;
    for (i = 0; i < ES_COUNT; i++) {
        check(m_shaders[i]);
    }

    #undef glsl
    #undef glslmat
    #undef glsl_noinput
}

Shaders::~Shaders()
{

}

E_MATERIAL_TYPE Shaders::getShader(const E_SHADER num) const
{
    assert(num < ES_COUNT);

    return (E_MATERIAL_TYPE) m_shaders[num];
}

void Shaders::check(const int num) const
{
    if (num == -1)
    {
        Log::fatal("shaders", "Shader failed to load. Update your drivers, if the issue "
                              "persists, report a bug to us.");
    }
}
