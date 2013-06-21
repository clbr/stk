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

#ifndef HEADER_RTTS_HPP
#define HEADER_RTTS_HPP

#include <ITexture.h>
#include <assert.h>
using namespace irr;
using video::ITexture;

enum e_rtt
{
    RTT_TMP1 = 0,
    RTT_TMP2,
    RTT_DEPTH,
    RTT_NORMAL,
    RTT_COLOR,

    RTT_HALF1,
    RTT_HALF2,

    RTT_COUNT
};

class rtt_t
{
public:
    rtt_t();
    ~rtt_t();

    ITexture *getRTT(e_rtt which);

private:
    ITexture *rtts[RTT_COUNT];
};

#endif

