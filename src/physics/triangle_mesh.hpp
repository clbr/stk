//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 Joerg Henrichs
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

#ifndef HEADER_TRIANGLE_MESH_HPP
#define HEADER_TRIANGLE_MESH_HPP

#include <vector>
#include "btBulletDynamicsCommon.h"

#include "physics/user_pointer.hpp"
#include "utils/aligned_array.hpp"

class Material;

/** 
 * \brief A special class to store a triangle mesh with a separate material per triangle.
 * \ingroup physics
 */
class TriangleMesh
{
private:
    UserPointer                  m_user_pointer;
    std::vector<const Material*> m_triangleIndex2Material;
    btRigidBody                 *m_body;
    btCollisionObject           *m_collision_object;
    btTriangleMesh               m_mesh;
    btVector3 dummy1, dummy2;
    btDefaultMotionState        *m_motion_state;
    btCollisionShape            *m_collision_shape;
    /** The three normals for each triangle. */
    AlignedArray<btVector3>      m_normals;
public:
         TriangleMesh();
        ~TriangleMesh();
    void addTriangle(const btVector3 &t1, const btVector3 &t2, 
                     const btVector3 &t3, const btVector3 &n1,
                     const btVector3 &n2, const btVector3 &n3,
                     const Material* m);
    void createCollisionShape(bool create_collision_object=true, const char* serialized_bhv=NULL);
    void createPhysicalBody(btCollisionObject::CollisionFlags flags=
                               (btCollisionObject::CollisionFlags)0,
                            const char* serializedBhv = NULL);
    void removeAll();
    void removeCollisionObject();
    btVector3 getInterpolatedNormal(unsigned int index,
                                    const btVector3 &position) const;
    // ------------------------------------------------------------------------
    const Material* getMaterial(int n) const 
                                          {return m_triangleIndex2Material[n];}
    // ------------------------------------------------------------------------
    const btCollisionShape &getCollisionShape() const 
                                          { return *m_collision_shape; }
    // ------------------------------------------------------------------------
    btCollisionShape &getCollisionShape() { return *m_collision_shape; }
    // ------------------------------------------------------------------------
    bool castRay(const btVector3 &from, const btVector3 &to,
                 btVector3 *xyz, const Material **material, 
                 btVector3 *normal=NULL) const;
    // ------------------------------------------------------------------------
    /** Returns the points of the 'indx' triangle.
     *  \param indx Index of the triangle to get. 
     *  \param p1,p2,p3 On return the three points of the triangle. */
    void getTriangle(unsigned int indx, btVector3 *p1, btVector3 *p2,
                     btVector3 *p3) const
    {
        const IndexedMeshArray &m = m_mesh.getIndexedMeshArray();
        btVector3 *p = &(((btVector3*)(m[0].m_vertexBase))[3*indx]);
        *p1 = p[0];
        *p2 = p[1];
        *p3 = p[2];
    }   // getTriangle
    // ------------------------------------------------------------------------
    /** Returns the normals of the triangle with the given index.
     *  \param indx Index of the triangle to get the three normals of.
     *  \result n1,n2,n3 The three normals. */
    void getNormals(unsigned int indx, btVector3 *n1, btVector3 *n2,
                    btVector3 *n3) const
    {
        assert(indx < m_triangleIndex2Material.size());
        unsigned int n = indx*3;
        *n1 = m_normals[n  ];
        *n2 = m_normals[n+1];
        *n3 = m_normals[n+2];
    }   // getNormals
};
#endif
/* EOF */

