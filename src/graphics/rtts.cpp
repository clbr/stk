//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013 Lauri Kasanen
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

#include "graphics/rtts.hpp"

#include "config/user_config.hpp"
#include "graphics/irr_driver.hpp"
#include "utils/log.hpp"

RTT::RTT()
{
    using namespace video;
    using namespace core;

    IVideoDriver * const drv = irr_driver->getVideoDriver();
    const dimension2du res(UserConfigParams::m_width, UserConfigParams::m_height);
    const dimension2du half = res/2;
    const dimension2du quarter = res/4;
    const dimension2du eighth = res/8;
    const dimension2du sixteenth = res/16;

    const dimension2du ssaosize = UserConfigParams::m_ssao == 2 ? res : quarter;

    const u16 shadowside = UserConfigParams::m_shadows == 2 ? 2048 : 512;
    const dimension2du shadowsize(shadowside, shadowside);
    const dimension2du warpvsize(1, shadowside);
    const dimension2du warphsize(shadowside, 1);

    // The last parameter stands for "has stencil". The name is used in the texture
    // cache, and when saving textures to files as the default name.
    rtts[RTT_TMP1] = drv->addRenderTargetTexture(res, "rtt.tmp1", ECF_A8R8G8B8, true);
    rtts[RTT_TMP2] = drv->addRenderTargetTexture(res, "rtt.tmp2", ECF_A8R8G8B8, true);
    rtts[RTT_TMP3] = drv->addRenderTargetTexture(res, "rtt.tmp3", ECF_A8R8G8B8, true);
    rtts[RTT_TMP4] = drv->addRenderTargetTexture(res, "rtt.tmp4", ECF_A8R8G8B8, true);
    rtts[RTT_DEPTH] = drv->addRenderTargetTexture(res, "rtt.depth", ECF_A8R8G8B8, true);
    rtts[RTT_NORMAL] = drv->addRenderTargetTexture(res, "rtt.normal", ECF_A8R8G8B8, true);
    rtts[RTT_COLOR] = drv->addRenderTargetTexture(res, "rtt.color", ECF_A8R8G8B8, true);

    rtts[RTT_HALF1] = drv->addRenderTargetTexture(half, "rtt.half1", ECF_A8R8G8B8, true);
    rtts[RTT_HALF2] = drv->addRenderTargetTexture(half, "rtt.half2", ECF_A8R8G8B8, true);

    rtts[RTT_QUARTER1] = drv->addRenderTargetTexture(quarter, "rtt.q1", ECF_A8R8G8B8, true);
    rtts[RTT_QUARTER2] = drv->addRenderTargetTexture(quarter, "rtt.q2", ECF_A8R8G8B8, true);

    rtts[RTT_EIGHTH1] = drv->addRenderTargetTexture(eighth, "rtt.e1", ECF_A8R8G8B8, true);
    rtts[RTT_EIGHTH2] = drv->addRenderTargetTexture(eighth, "rtt.e2", ECF_A8R8G8B8, true);

    rtts[RTT_SIXTEENTH1] = drv->addRenderTargetTexture(sixteenth, "rtt.s1", ECF_A8R8G8B8, true);
    rtts[RTT_SIXTEENTH2] = drv->addRenderTargetTexture(sixteenth, "rtt.s2", ECF_A8R8G8B8, true);

    rtts[RTT_SSAO1] = drv->addRenderTargetTexture(ssaosize, "rtt.ssao1", ECF_A8R8G8B8, true);
    rtts[RTT_SSAO2] = drv->addRenderTargetTexture(ssaosize, "rtt.ssao2", ECF_A8R8G8B8, true);

    rtts[RTT_SHADOW] = drv->addRenderTargetTexture(shadowsize, "rtt.shadow", ECF_A8R8G8B8, true);
    rtts[RTT_COLLAPSEV] = drv->addRenderTargetTexture(warpvsize, "rtt.collapsev", ECF_A8R8G8B8, true);
    rtts[RTT_COLLAPSEH] = drv->addRenderTargetTexture(warphsize, "rtt.collapseh", ECF_A8R8G8B8, true);
    rtts[RTT_WARPV] = drv->addRenderTargetTexture(warpvsize, "rtt.warpv", ECF_A8R8G8B8, true);
    rtts[RTT_WARPH] = drv->addRenderTargetTexture(warphsize, "rtt.warph", ECF_A8R8G8B8, true);

    u32 i;
    for (i = 0; i < RTT_COUNT; i++)
    {
        if (!rtts[i])
            Log::fatal("RTT", "Failed to create a RTT");
    }
}

RTT::~RTT()
{
    u32 i;
    for (i = 0; i < RTT_COUNT; i++)
    {
        irr_driver->removeTexture(rtts[i]);
    }
}

ITexture *RTT::getRTT(TypeRTT which)
{
    assert(which < RTT_COUNT);
    return rtts[which];
}
