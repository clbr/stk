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

    m_callbacks[ES_NORMAL_MAP_LIGHTMAP] = new NormalMapProvider(true);
    m_callbacks[ES_NORMAL_MAP] = new NormalMapProvider(false);
    m_callbacks[ES_SPLATTING] = new SplattingProvider();
    m_callbacks[ES_WATER] = new WaterShaderProvider();
    m_callbacks[ES_GRASS] = new GrassShaderProvider();
    m_callbacks[ES_BUBBLES] = new BubbleEffectProvider();
    m_callbacks[ES_RAIN] = new RainEffectProvider();
    m_callbacks[ES_SNOW] = new SnowEffectProvider();
    m_callbacks[ES_MOTIONBLUR] = new MotionBlurProvider();
    m_callbacks[ES_GAUSSIAN3V] = m_callbacks[ES_GAUSSIAN3H] = new GaussianBlurProvider();
    m_callbacks[ES_MIPVIZ] = new MipVizProvider();
    m_callbacks[ES_COLORIZE] = new ColorizeProvider();
    m_callbacks[ES_GLOW] = new GlowProvider();
    m_callbacks[ES_OBJECTPASS] = new ObjectPassProvider();
    m_callbacks[ES_LIGHTBLEND] = new LightBlendProvider();
    m_callbacks[ES_POINTLIGHT] = new PointLightProvider();
    m_callbacks[ES_SUNLIGHT] = new SunLightProvider();

    // Ok, go
    m_shaders[ES_NORMAL_MAP] = glslmat(dir + "normalmap.vert", dir + "normalmap.frag",
                                       m_callbacks[ES_NORMAL_MAP], EMT_SOLID_2_LAYER);

    m_shaders[ES_NORMAL_MAP_LIGHTMAP] = glslmat(dir + "normalmap.vert", dir + "normalmap.frag",
                                             m_callbacks[ES_NORMAL_MAP_LIGHTMAP], EMT_SOLID_2_LAYER);

    m_shaders[ES_SPLATTING] = glsl(dir + "objectpass.vert", dir + "splatting.frag",
                                   m_callbacks[ES_SPLATTING]);

    m_shaders[ES_WATER] = glslmat(dir + "water.vert", dir + "water.frag",
                                  m_callbacks[ES_WATER], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_SPHERE_MAP] = glslmat(dir + "objectpass_rimlit.vert", dir + "objectpass_spheremap.frag",
                                       m_callbacks[ES_OBJECTPASS], EMT_SOLID);

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

    m_shaders[ES_GAUSSIAN3H] = glslmat(std::string(""), dir + "gaussian3h.frag",
                                    m_callbacks[ES_GAUSSIAN3H], EMT_SOLID);
    m_shaders[ES_GAUSSIAN3V] = glslmat(std::string(""), dir + "gaussian3v.frag",
                                    m_callbacks[ES_GAUSSIAN3V], EMT_SOLID);

    m_shaders[ES_GAUSSIAN6H] = glslmat(std::string(""), dir + "gaussian6h.frag",
                                    m_callbacks[ES_GAUSSIAN3H], EMT_SOLID);
    m_shaders[ES_GAUSSIAN6V] = glslmat(std::string(""), dir + "gaussian6v.frag",
                                    m_callbacks[ES_GAUSSIAN3V], EMT_SOLID);

    m_shaders[ES_MIPVIZ] = glslmat(std::string(""), dir + "mipviz.frag",
                                    m_callbacks[ES_MIPVIZ], EMT_SOLID);

    m_shaders[ES_FLIP] = glslmat(std::string(""), dir + "flip.frag",
                                    0, EMT_SOLID);
    m_shaders[ES_FLIP_ADDITIVE] = glslmat(std::string(""), dir + "flip.frag",
                                    0, EMT_TRANSPARENT_ADD_COLOR);

    m_shaders[ES_BLOOM] = glslmat(std::string(""), dir + "bloom.frag",
                                    0, EMT_SOLID);

    m_shaders[ES_COLORIZE] = glslmat(std::string(""), dir + "colorize.frag",
                                    m_callbacks[ES_COLORIZE], EMT_SOLID);

    m_shaders[ES_PASS] = glslmat(std::string(""), dir + "pass.frag",
                                    0, EMT_SOLID);
    m_shaders[ES_PASS_ADDITIVE] = glslmat(std::string(""), dir + "pass.frag",
                                    0, EMT_TRANSPARENT_ADD_COLOR);

    m_shaders[ES_GLOW] = glslmat(std::string(""), dir + "glow.frag",
                                    m_callbacks[ES_GLOW], EMT_TRANSPARENT_ALPHA_CHANNEL);

    m_shaders[ES_OBJECTPASS] = glslmat(dir + "objectpass.vert", dir + "objectpass.frag",
                                    m_callbacks[ES_OBJECTPASS], EMT_SOLID);
    m_shaders[ES_OBJECTPASS_REF] = glslmat(dir + "objectpass.vert", dir + "objectpass_ref.frag",
                                    m_callbacks[ES_OBJECTPASS], EMT_SOLID);
    m_shaders[ES_OBJECTPASS_RIMLIT] = glslmat(dir + "objectpass_rimlit.vert", dir + "objectpass_rimlit.frag",
                                    m_callbacks[ES_OBJECTPASS], EMT_SOLID);

    m_shaders[ES_LIGHTBLEND] = glslmat(std::string(""), dir + "lightblend.frag",
                                    m_callbacks[ES_LIGHTBLEND], EMT_ONETEXTURE_BLEND);

    m_shaders[ES_POINTLIGHT] = glslmat(std::string(""), dir + "pointlight.frag",
                                    m_callbacks[ES_POINTLIGHT], EMT_ONETEXTURE_BLEND);

    m_shaders[ES_SUNLIGHT] = glslmat(std::string(""), dir + "sunlight.frag",
                                    m_callbacks[ES_SUNLIGHT], EMT_ONETEXTURE_BLEND);


    // Check that all successfully loaded
    u32 i;
    for (i = 0; i < ES_COUNT; i++) {

        // Old Intel Windows drivers fail here.
        // It's an artist option, so not necessary to play.
        if (i == ES_MIPVIZ)
            continue;

        check(m_shaders[i]);
    }

    #undef glsl
    #undef glslmat
    #undef glsl_noinput
}

Shaders::~Shaders()
{
    u32 i;
    for (i = 0; i < ES_COUNT; i++)
    {
        if (i == ES_GAUSSIAN3V || !m_callbacks[i]) continue;
        delete m_callbacks[i];
    }
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
