//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
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

#include "graphics/skid_marks.hpp"

#include "config/stk_config.hpp"
#include "graphics/irr_driver.hpp"
#include "karts/controller/controller.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/skidding.hpp"
#include "physics/btKart.hpp"

#include <IMeshSceneNode.h>
#include <SMesh.h>

float     SkidMarks::m_avoid_z_fighting  = 0.005f;
const int SkidMarks::m_start_alpha       = 128;
const int SkidMarks::m_start_grey        = 32;

/** Initialises empty skid marks. */
SkidMarks::SkidMarks(const AbstractKart& kart, float width) : m_kart(kart)
{
    m_width                   = width;
    m_material                = new video::SMaterial();
    m_material->MaterialType  = video::EMT_TRANSPARENT_VERTEX_ALPHA;
    m_material->AmbientColor  = video::SColor(128, 0, 0, 0);
    m_material->DiffuseColor  = video::SColor(128, 16, 16, 16);
    m_material->Shininess     = 0;
    m_skid_marking            = false;
    m_current                 = -1;
}   // SkidMark

//-----------------------------------------------------------------------------
/** Removes all skid marks from the scene graph and frees the state. */
SkidMarks::~SkidMarks()
{
    reset();  // remove all skid marks
    delete m_material;
}   // ~SkidMarks

//-----------------------------------------------------------------------------
/** Removes all skid marks, called when a race is restarted.
 */
void SkidMarks::reset()
{
    for(unsigned int i=0; i<m_nodes.size(); i++)
    {
        irr_driver->removeNode(m_nodes[i]);
        m_left[i]->drop();
        m_right[i]->drop();
    }
    m_left.clear();
    m_right.clear();
    m_nodes.clear();
    m_skid_marking = false;
    m_current      = -1;
}   // reset

//-----------------------------------------------------------------------------
/** Either adds to an existing skid mark quad, or (if the kart is skidding) 
 *  starts a new skid mark quad.
 *  \param dt Time step.
 */
void SkidMarks::update(float dt, bool force_skid_marks, 
                       video::SColor* custom_color)
{
    //if the kart is gnu, then don't skid because he floats!
    if(m_kart.isWheeless())
        return;

    float f = dt/stk_config->m_skid_fadeout_time*m_start_alpha;
    for(unsigned int i=0; i<m_left.size(); i++)
    {
        m_left[i]->fade(f);
        m_right[i]->fade(f);
    }

    // Get raycast information
    // -----------------------
    const btWheelInfo::RaycastInfo &raycast_right =
            m_kart.getVehicle()->getWheelInfo(2).m_raycastInfo;
    const btWheelInfo::RaycastInfo raycast_left =
            m_kart.getVehicle()->getWheelInfo(3).m_raycastInfo;
    Vec3 delta = raycast_right.m_contactPointWS 
               - raycast_left.m_contactPointWS;

    // The kart is making skid marks when it's:
    // - forced to leave skid marks, or all of:
    // - in accumulating skidding mode
    // - not doing the grphical jump
    // - wheels are in contact with floor, which includes a special case:
    //   the physics force both wheels on one axis to touch the ground or not.
    //   If only one wheel touches the ground, the 2nd one gets the same 
    //   raycast result --> delta is 0, which is considered to be not skidding.
    const Skidding *skid = m_kart.getSkidding();
    bool is_skidding = force_skid_marks ||
                   (  (skid->getSkidState()==Skidding::SKID_ACCUMULATE_LEFT ||
                       skid->getSkidState()==Skidding::SKID_ACCUMULATE_RIGHT  )
                     &&  m_kart.getSkidding()->getGraphicalJumpOffset()<=0
                     &&  raycast_right.m_isInContact
                     &&  delta.length2()>=0.0001f                              );
                     
    if(m_skid_marking)
    {
        if (!is_skidding)   // end skid marking
        {
            m_skid_marking = false;
            // The vertices and indices will not change anymore 
            // (till these skid mark quads are deleted)
            m_left[m_current]->setHardwareMappingHint(scene::EHM_STATIC);
            m_right[m_current]->setHardwareMappingHint(scene::EHM_STATIC);
            return;
        }

        // We are still skid marking, so add the latest quad
        // -------------------------------------------------

        delta.normalize();
        delta *= m_width;

        m_left [m_current]->add(raycast_left.m_contactPointWS,
                                raycast_left.m_contactPointWS + delta);
        m_right[m_current]->add(raycast_right.m_contactPointWS-delta,
                                raycast_right.m_contactPointWS);
        // Adjust the boundary box of the mesh to include the
        // adjusted aabb of its buffers.
        core::aabbox3df aabb=m_nodes[m_current]->getMesh()
                            ->getBoundingBox();
        aabb.addInternalBox(m_left[m_current]->getAABB());
        aabb.addInternalBox(m_right[m_current]->getAABB());
        m_nodes[m_current]->getMesh()->setBoundingBox(aabb);
        return;
    }

    // Currently no skid marking
    // -------------------------
    if (!is_skidding) return;

    // Start new skid marks
    // --------------------
    // No skidmarking if wheels don't have contact
    if(!raycast_right.m_isInContact) return;
    if(delta.length2()<0.0001) return;

    delta.normalize();
    delta *= m_width;

    SkidMarkQuads *smq_left = 
        new SkidMarkQuads(raycast_left.m_contactPointWS,
                          raycast_left.m_contactPointWS + delta,
                          m_material, m_avoid_z_fighting, custom_color);
    scene::SMesh *new_mesh = new scene::SMesh();
    new_mesh->addMeshBuffer(smq_left);

    SkidMarkQuads *smq_right = 
        new SkidMarkQuads(raycast_right.m_contactPointWS - delta,
                          raycast_right.m_contactPointWS,
                          m_material, m_avoid_z_fighting, custom_color);
    new_mesh->addMeshBuffer(smq_right);
    scene::IMeshSceneNode *new_node = irr_driver->addMesh(new_mesh);
#ifdef DEBUG
    std::string debug_name = m_kart.getIdent()+" (skid-mark)";
    new_node->setName(debug_name.c_str());
#endif

    // We don't keep a reference to the mesh here, so we have to decrement
    // the reference count (which is set to 1 when doing "new SMesh()".
    // The scene node will keep the mesh alive.
    new_mesh->drop();
    m_current++;
    if(m_current>=stk_config->m_max_skidmarks)
        m_current = 0;
    if(m_current>=(int)m_left.size())
    {
        m_left. push_back (smq_left );
        m_right.push_back (smq_right);
        m_nodes.push_back (new_node);
    }
    else
    {
        irr_driver->removeNode(m_nodes[m_current]);
        // Not necessary to delete m_nodes: removeNode
        // deletes the node since its refcount reaches zero.
        m_left[m_current]->drop();
        m_right[m_current]->drop();

        m_left  [m_current] = smq_left;
        m_right [m_current] = smq_right;
        m_nodes [m_current] = new_node;
    }

    m_skid_marking = true;
    // More triangles are added each frame, so for now leave it
    // to stream.
    m_left[m_current]->setHardwareMappingHint(scene::EHM_STREAM);
    m_right[m_current]->setHardwareMappingHint(scene::EHM_STREAM);
}   // update

//=============================================================================
SkidMarks::SkidMarkQuads::SkidMarkQuads(const Vec3 &left,
                                        const Vec3 &right,
                                        video::SMaterial *material, 
                                        float z_offset,
                                        video::SColor* custom_color)
                         : scene::SMeshBuffer()
{
    m_z_offset = z_offset;
    m_fade_out = 0.0f;

    m_start_color = (custom_color != NULL ? *custom_color :
                     video::SColor(255,
                                   SkidMarks::m_start_grey,
                                   SkidMarks::m_start_grey,
                                   SkidMarks::m_start_grey));
    
    Material   = *material;
    m_aabb     = core::aabbox3df(left.toIrrVector());
    add(left, right);
    

}   // SkidMarkQuads

//-----------------------------------------------------------------------------
/** Adds the two points to this SkidMarkQuads.
 *  \param left,right Left and right coordinates.
 */
void SkidMarks::SkidMarkQuads::add(const Vec3 &left,
                                   const Vec3 &right)
{
    // The skid marks must be raised slightly higher, otherwise it blends
    // too much with the track.
    int n = Vertices.size();

    video::S3DVertex v;
    v.Color = m_start_color;
    v.Color.setAlpha(m_start_alpha);
    v.Pos = left.toIrrVector();
    v.Pos.Y += m_z_offset;
    v.Normal = core::vector3df(0, 1, 0);
    Vertices.push_back(v);
    v.Pos = right.toIrrVector();
    v.Pos.Y += m_z_offset;
    Vertices.push_back(v);
    // Out of the box Irrlicht only supports triangle meshes and not
    // triangle strips. Since this is a strip it would be more efficient
    // to use a special triangle strip scene node.
    if(n>=2)
    {
        Indices.push_back(n-2);
        Indices.push_back(n  );
        Indices.push_back(n-1);
        Indices.push_back(n-1);
        Indices.push_back(n  );
        Indices.push_back(n+1);
    }
    // Adjust the axis-aligned boundary boxes.
    m_aabb.addInternalPoint(left.toIrrVector() );
    m_aabb.addInternalPoint(right.toIrrVector());
    setBoundingBox(m_aabb);
    
    setDirty();
}   // add

// ----------------------------------------------------------------------------
/** Fades the current skid marks. 
 *  \param f fade factor.
 */
void SkidMarks::SkidMarkQuads::fade(float f)
{
    m_fade_out += f;
    // Changing the alpha value is quite expensive, so it's only done
    // about 10 times till 0 is reached.
    if(m_fade_out*10>SkidMarks::m_start_alpha)
    {
        video::SColor &c = Material.DiffuseColor;
        int a = c.getAlpha();
        a -= (a < m_fade_out ? a : (int)m_fade_out);

        c.setAlpha(a);
        for(unsigned int i=0; i<Vertices.size(); i++)
        {
            Vertices[i].Color.setAlpha(a);
        }
        m_fade_out = 0.0f;
    }
}   // fade

// ----------------------------------------------------------------------------
/** Sets the fog handling for the skid marks.
 *  \param enabled True if fog should be enabled.
 */
void SkidMarks::adjustFog(bool enabled)
{
    m_material->FogEnable = enabled; 
}
