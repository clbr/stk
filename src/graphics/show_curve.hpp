//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2012 Joerg Henrichs
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

#ifndef HEADER_SHOW_CURVE_HPP
#define HEADER_SHOW_CURVE_HPP

#include "utils/leak_check.hpp"
#include "utils/no_copy.hpp"

#include <SColor.h>
class Vec3;

namespace irr
{
    namespace scene { class IMesh; class IMeshSceneNode; class IMeshBuffer; }
}
using namespace irr;

/** This class is used for debugging. It allows to show an arbitrary curve
 *  in the race. The curve is shown as a 'tunnel', i.e. in the constructor
 *  you can specify the width and height of this tunnel. Each point then
 *  adds 4 vertices: point + (+- width/2, +- height2, 0). That's not
 *  exact (if the curve is not parallel to the z axis), but good enough for
 *  debugging.
 * \ingroup graphics
 */
class ShowCurve : public NoCopy
{
    LEAK_CHECK();
private:

    /** The actual scene node. */
    scene::IMeshSceneNode *m_scene_node;

    /** The mesh of the curve. */
    scene::IMesh       *m_mesh;

    /** The mesh buffer containing the actual vertices of the curve. */
    scene::IMeshBuffer *m_buffer;

    /** The width of the graph when it is displayed. */
    float m_width;

    /** The height of the graph when it is displayed. */
    float m_height;

    /** The color to use for the curve. */
    irr::video::SColor m_color;

    void addEmptyMesh();
public:
         ShowCurve(float width, float height,
                   const irr::video::SColor &color = video::SColor(77, 0, 179, 0));
        ~ShowCurve();
    void addPoint(const Vec3 &pnt);
    void makeCircle(const Vec3 &center, float radius);
    void update(float dt);
    void setVisible(bool v);
    bool isVisible() const;
    void setPosition(const Vec3 &xyz);
    void setHeading(float heading);
    void clear();

};   // ShowCurve
#endif
