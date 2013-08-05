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

#ifndef HEADER_CALLBACKS_HPP
#define HEADER_CALLBACKS_HPP

#include "config/user_config.hpp"
#include "graphics/irr_driver.hpp"

#include <irrlicht.h>
#include <set>

using namespace irr;

class callbase: public video::IShaderConstantSetCallBack
{
public:
    callbase()
    {
        firstdone = 0;
    }

    virtual void OnSetMaterial(const video::SMaterial &material)
    {
        mat = material;
    }

protected:
    bool firstdone;
    video::SMaterial mat;
};

//

class NormalMapProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    NormalMapProvider(bool withLightmap)
    {
        m_with_lightmap = withLightmap;
    }

private:
    bool m_with_lightmap;
};

//

class WaterShaderProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setSpeed(const float s1, const float s2)
    {
        m_water_shader_speed_1 = s1;
        m_water_shader_speed_2 = s2;
    }

    WaterShaderProvider()
    {
        m_dx_1 = 0.0f;
        m_dx_2 = 0.0f;
        m_dy_1 = 0.0f;
        m_dy_2 = 0.0f;

        m_water_shader_speed_1 =
        m_water_shader_speed_2 = 0.0f;
    }

private:
    float m_dx_1, m_dy_1, m_dx_2, m_dy_2;
    float m_water_shader_speed_1;
    float m_water_shader_speed_2;
};

//

class GrassShaderProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    GrassShaderProvider()
    {
        m_amplitude =
        m_speed = 0.0f;
    }

    void setSpeed(float speed)
    {
        m_speed = speed;
    }

    void setAmplitude(float amp)
    {
        m_amplitude = amp;
    }

private:
    float m_amplitude, m_speed;
};

//

class SplattingProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class BubbleEffectProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    BubbleEffectProvider()
    {
    }

    // We hijack the material type param 2 of bubbles.
    // It's time to start the fade, negative if fade out, positive if in.
    // It'd be unused otherwise.

    void onMadeVisible(scene::IMeshBuffer * const mb)
    {
        if (!contains(mb))
            return;

        video::SMaterial &mat = mb->getMaterial();
        mat.MaterialTypeParam2 = irr_driver->getDevice()->getTimer()->getTime() / 1000.0f;
    }

    void onHidden(scene::IMeshBuffer * const mb)
    {
        if (!contains(mb))
            return;

        video::SMaterial &mat = mb->getMaterial();
        mat.MaterialTypeParam2 = irr_driver->getDevice()->getTimer()->getTime() / -1000.0f;
    }

    void isInitiallyHidden(scene::IMeshBuffer * const mb)
    {
        if (!contains(mb))
            return;

        video::SMaterial &mat = mb->getMaterial();
        mat.MaterialTypeParam2 = irr_driver->getDevice()->getTimer()->getTime() / -1000.0f;
    }

    void removeBubble(const scene::IMeshBuffer * const mb)
    {
        m_bubbles.erase(mb);
    }

    void addBubble(scene::IMeshBuffer * const mb)
    {
        m_bubbles.insert(mb);

        video::SMaterial &mat = mb->getMaterial();
        mat.MaterialTypeParam2 = 1;
    }

    bool contains(const scene::IMeshBuffer * const mb) const
    {
        return m_bubbles.count(mb);
    }

private:
    std::set<const scene::IMeshBuffer *> m_bubbles;
};

//

class RainEffectProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class SnowEffectProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class MotionBlurProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setMaxHeight(u32 who, float height)
    {
        assert(who < MAX_PLAYER_COUNT);
        m_maxheight[who] = height;
    }

    void setBoostTime(u32 who, float time)
    {
        assert(who < MAX_PLAYER_COUNT);
        m_boost_time[who] = time;
    }

    void setCenter(u32 who, float X, float Y)
    {
        assert(who < MAX_PLAYER_COUNT);
        m_center[who].X = X;
        m_center[who].Y = Y;
    }

    void setDirection(u32 who, float X, float Y)
    {
        assert(who < MAX_PLAYER_COUNT);
        m_direction[who].X = X;
        m_direction[who].Y = Y;
    }

    void setCurrentCamera(u32 who)
    {
        m_current_camera = who;
    }

private:
    float m_maxheight[MAX_PLAYER_COUNT];
    u32 m_current_camera;
    float m_boost_time[MAX_PLAYER_COUNT];
    core::vector2df m_center[MAX_PLAYER_COUNT];
    core::vector2df m_direction[MAX_PLAYER_COUNT];
};

//

class GaussianBlurProvider: public callbase
{
public:
    GaussianBlurProvider()
    {
        m_pixel[0] = 1.0f / UserConfigParams::m_width;
        m_pixel[1] = 1.0f / UserConfigParams::m_height;
    }

    void setResolution(float x, float y)
    {
        m_pixel[0] = 1.0f / x;
        m_pixel[1] = 1.0f / y;
    }

    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

private:
    float m_pixel[2];
};

//

class MipVizProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class ColorizeProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setColor(float r, float g, float b)
    {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
    }

private:
    float m_color[3];
};

//

class GlowProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setResolution(float x, float y)
    {
        m_res[0] = x;
        m_res[1] = y;
    }

private:
    float m_res[2];
};

//

class ObjectPassProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class LightBlendProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class PointLightProvider: public callbase
{
public:
    PointLightProvider()
    {
        m_screen[0] = UserConfigParams::m_width;
        m_screen[1] = UserConfigParams::m_height;

        m_specular = 200;
    }

    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setColor(float r, float g, float b)
    {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
    }

    void setPosition(float x, float y, float z)
    {
        m_pos[0] = x;
        m_pos[1] = y;
        m_pos[2] = z;
    }

    void setRadius(float r)
    {
        m_radius = r;
    }

    void setSpecular(float s)
    {
        m_specular = s;
    }

    void updateIPVMatrix()
    {
        // Update the campos and IPV matrix, only once per frame since it's costly
        const core::vector3df &campos =
                     irr_driver->getSceneManager()->getActiveCamera()->getAbsolutePosition();
        m_campos[0] = campos.X;
        m_campos[1] = campos.Y;
        m_campos[2] = campos.Z;

        const video::IVideoDriver * const drv = irr_driver->getVideoDriver();

        m_invprojview = drv->getTransform(video::ETS_PROJECTION);
        m_invprojview *= drv->getTransform(video::ETS_VIEW);
        m_invprojview.makeInverse();
    }

private:
    core::matrix4 m_invprojview;

    float m_campos[3];
    float m_color[3];
    float m_pos[3];
    float m_screen[2];
    float m_radius;
    float m_specular;
};

//

class SunLightProvider: public callbase
{
public:
    SunLightProvider()
    {
        m_screen[0] = UserConfigParams::m_width;
        m_screen[1] = UserConfigParams::m_height;

        m_wind[0] = m_wind[1] = 0;
    }

    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setColor(float r, float g, float b)
    {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
    }

    void setPosition(float x, float y, float z)
    {
        m_pos[0] = x;
        m_pos[1] = y;
        m_pos[2] = z;
    }

    void updateIPVMatrix()
    {
        // Update the IPV matrix, only once per frame since it's costly
        const video::IVideoDriver * const drv = irr_driver->getVideoDriver();

        m_invprojview = drv->getTransform(video::ETS_PROJECTION);
        m_invprojview *= drv->getTransform(video::ETS_VIEW);
        m_invprojview.makeInverse();
    }

    void setShadowMatrix(const core::matrix4 &mat)
    {
        m_shadowmat = mat;
    }

private:
    core::matrix4 m_invprojview, m_shadowmat;
    float m_color[3];
    float m_pos[3];
    float m_screen[2];
    float m_wind[2];
};

//

class BloomProvider: public callbase
{
public:
    BloomProvider() { m_threshold = 0.75f; }

    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    void setThreshold(const float f) { m_threshold = f; }

private:
    float m_threshold;
};

//

class MLAAColor1Provider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class MLAABlend2Provider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class MLAANeigh3Provider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class SSAOProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class GodRayProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);

    // In texcoords
    void setSunPosition(float x, float y) { m_sunpos[0] = x; m_sunpos[1] = y; }

private:
    float m_sunpos[2];
};

//

class ShadowPassProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

//

class ShadowImportanceProvider: public callbase
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *srv, int);
};

#endif
