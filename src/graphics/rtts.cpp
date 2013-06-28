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

#include "graphics/rtts.hpp"

#include "config/user_config.hpp"
#include "graphics/irr_driver.hpp"
#include "utils/log.hpp"

rtt_t::rtt_t()
{
    using namespace video;

    IVideoDriver * const drv = irr_driver->getVideoDriver();
    const core::dimension2du res(UserConfigParams::m_width, UserConfigParams::m_height);
    const core::dimension2du half = res/2;
    const core::dimension2du quarter = res/4;
    const core::dimension2du eighth = res/8;
    const core::dimension2du sixteenth = res/16;

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

    u32 i;
    for (i = 0; i < RTT_COUNT; i++)
    {
        if (!rtts[i])
            Log::fatal("RTT", "Failed to create a RTT");
    }
}

rtt_t::~rtt_t()
{
    u32 i;
    for (i = 0; i < RTT_COUNT; i++)
    {
        irr_driver->removeTexture(rtts[i]);
    }
}

ITexture *rtt_t::getRTT(E_RTT which)
{
    assert(which < RTT_COUNT);
    return rtts[which];
}
