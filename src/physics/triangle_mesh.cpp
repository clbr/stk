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
//  but WITHOUT ANY WARRANTY; without even the implied warranty ofati
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "physics/triangle_mesh.hpp"

#include "btBulletDynamicsCommon.h"

#include "modes/world.hpp"
#include "physics/physics.hpp"
#include "utils/constants.hpp"
#include "utils/time.hpp"

#include <fstream>

// -----------------------------------------------------------------------------
/** Constructor: Initialises all data structures with zero.
 */
TriangleMesh::TriangleMesh() : m_mesh()
{
    m_body             = NULL;
    m_motion_state     = NULL;
    // FIXME: on VS in release mode this statement actually overwrites
    // part of the data of m_mesh, causing a crash later. Debugging 
    // shows that apparently m_collision_shape is at the same address
    // as m_mesh->m_use32bitIndices and m_use4componentVertices
    // (and m_mesh->m_weldingThreshold at m_normals
    m_collision_shape  = NULL;
    m_collision_object = NULL;
    m_user_pointer.set(this);
}   // TriangleMesh

// -----------------------------------------------------------------------------
/** Destructor: delete all allocated data structures.
 */
TriangleMesh::~TriangleMesh()
{
    removeAll();
}   // ~TriangleMesh

// -----------------------------------------------------------------------------
/** Adds a triangle to the bullet mesh. It also stores the material used for
 *  this triangle, and the three normals.
 *  \param t1,t2,t3 Points of the triangle.
 *  \param n1,n2,n3 Normals at the corresponding points.
 *  \param m Material used for this triangle
 */
void TriangleMesh::addTriangle(const btVector3 &t1, const btVector3 &t2, 
                               const btVector3 &t3, 
                               const btVector3 &n1, const btVector3 &n2,
                               const btVector3 &n3, 
                               const Material* m)
{
    m_triangleIndex2Material.push_back(m);

    btVector3 normal = (t2-t1).cross(t3-t1);
    normal.normalize();
    m_normals.push_back( normal.angle(n1)>stk_config->m_smooth_angle_limit
                         ? normal : n1                                     );
    m_normals.push_back( normal.angle(n2)>stk_config->m_smooth_angle_limit
                         ? normal : n2                                     );
    m_normals.push_back( normal.angle(n3)>stk_config->m_smooth_angle_limit
                         ? normal : n3                                     );
    m_mesh.addTriangle(t1, t2, t3);
}   // addTriangle

// -----------------------------------------------------------------------------
/** Creates a collision body only, which can be used for raycasting, but
 *  has no physical properties.
 *  @param serialized_bhv if non-null, load the serialized bhv from file instead
 *                        of builing it on the fly
 */
void TriangleMesh::createCollisionShape(bool create_collision_object, const char* serialized_bhv)
{
    if(m_triangleIndex2Material.size()==0)
    {
        m_collision_shape  = NULL;
        m_motion_state     = NULL;
        m_body             = NULL;
        m_collision_object = NULL;
        return;
    }
    // Now convert the triangle mesh into a static rigid body
    btBvhTriangleMeshShape* bhv_triangle_mesh;

    if (serialized_bhv != NULL)
    {
        FILE *f = fopen(serialized_bhv, "rb");
        fseek(f, 0, SEEK_END);
        long pos = ftell(f);
        fseek(f, 0, SEEK_SET);
                
        void* bytes = btAlignedAlloc(pos, 16);
        fread(bytes, pos, 1, f);
        fclose(f);
        
        btOptimizedBvh* bhv = btOptimizedBvh::deSerializeInPlace(bytes, pos, !IS_LITTLE_ENDIAN);
        if (bhv == NULL)
        {
            fprintf(stderr, "[TriangleMesh] WARNING, failed to load serialized BHV\n");
            bhv_triangle_mesh = new btBvhTriangleMeshShape(&m_mesh, false /* useQuantizedAabbCompression */);
        }
        else
        {
            bhv_triangle_mesh = new btBvhTriangleMeshShape(&m_mesh, false /* useQuantizedAabbCompression */,
                                                           false /* buildBvh */);
            bhv_triangle_mesh->setOptimizedBvh( bhv );
        }
        // Do *NOT* free the bytes, 'deSerializeInPlace' makes the btOptimizedBvh object
        // directly at this memory location
        //free(bytes);
        
    }
    else
    {
        bhv_triangle_mesh = new btBvhTriangleMeshShape(&m_mesh, false /* useQuantizedAabbCompression */);

        /*
         // code to serialize triangle mesh
        btOptimizedBvh* bvh = bhv_triangle_mesh->getOptimizedBvh();
        unsigned int ssize = bvh->calculateSerializeBufferSize();
        char* buffer = (char*)btAlignedAlloc(ssize, 16);
        bool success = bvh->serialize(buffer, ssize, !IS_LITTLE_ENDIAN);
        printf("serialization success = %i\n", success);
        
        std::ofstream fileout("/tmp/btOptimizedBvh");
        fileout.write(buffer, ssize);
        fileout.close();
        
        btAlignedFree(buffer);
         */
    }
    
    m_collision_shape = bhv_triangle_mesh;
    m_collision_shape->setUserPointer(&m_user_pointer);
    if(create_collision_object)
    {
        m_collision_object = new btCollisionObject();
        btTransform bt;
        bt.setIdentity();
        m_collision_object->setWorldTransform(bt);
    }
    
    
}   // createCollisionShape

// -----------------------------------------------------------------------------
/** Creates the physics body for this triangle mesh. If the body already
 *  exists (because it was created by a previous call to createBody)
 *  it is first removed from the world. This is used by loading the track
 *  where a physics body is used to determine the height of terrain. To have
 *  an optimised rigid body including all static objects, the track is then
 *  removed and all objects together with the track is converted again into
 *  a single rigid body. This avoids using irrlicht (or the graphics engine)
 *  for height of terrain detection).
 *  @param serializedBhv if non-NULL, the bhv is deserialized instead of
 *                       being calculated on the fly
 */
void TriangleMesh::createPhysicalBody(btCollisionObject::CollisionFlags flags,
                                      const char* serializedBhv)
{
    // We need the collision shape, but not the collision object (since
    // this will be created when the dynamics body is anyway).
    createCollisionShape(/*create_collision_object*/false, serializedBhv);
    btTransform startTransform;
    startTransform.setIdentity();
    m_motion_state = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo info(0.0f, m_motion_state, 
                                                  m_collision_shape);
    m_body=new btRigidBody(info);
    World::getWorld()->getPhysics()->addBody(m_body);

    m_body->setUserPointer(&m_user_pointer);
    m_body->setCollisionFlags(m_body->getCollisionFlags()  | 
                              flags                        |
                              btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
}   // createPhysicalBody

// ----------------------------------------------------------------------------
/** Removes the created body and/or collision object from the physics world. 
 *  This is used when creating a temporary rigid body of the main track to get 
 *  bullet raycasts. Then the main track is removed, and the track (main track 
 *  including all additional objects which were loaded later) is converted 
 *  again.
 */
void TriangleMesh::removeAll()
{
    if(m_body)
    {
        World::getWorld()->getPhysics()->removeBody(m_body);
        delete m_body;
        delete m_motion_state;
        m_body         = NULL;
        m_motion_state = NULL;
    }
    if(m_collision_object)
    {
        delete m_collision_object;
        m_collision_object = NULL;
    }
    delete m_collision_shape;
    m_collision_shape = NULL;
}   // removeAll

// -----------------------------------------------------------------------------
/** Interpolates the normal at the given position for the triangle with
 *  a given index. The position must be inside of the given triangle.
 *  \param index Index of the triangle to use.
 *  \param position The position for which to interpolate the normal.
 */
btVector3 TriangleMesh::getInterpolatedNormal(unsigned int index,
                                              const btVector3 &position) const
{
    btVector3 p1, p2, p3;
    getTriangle(index, &p1, &p2, &p3);
    btVector3 n1, n2, n3;
    getNormals(index, &n1, &n2, &n3);

    // Compute the Barycentric coordinates of position inside  triangle 
    // p1, p2, p3.
    btVector3 edge1 = p2 - p1;
    btVector3 edge2 = p3 - p1;

    // Area of triangle ABC
    btScalar p1p2p3 = edge1.cross(edge2).length2();

    // Area of BCP
    btScalar p2p3p = (p3 - p2).cross(position - p2).length2();

    // Area of CAP
    btScalar p3p1p = edge2.cross(position - p3).length2();
    btScalar s = btSqrt(p2p3p / p1p2p3);
    btScalar t = btSqrt(p3p1p / p1p2p3);
    btScalar w = 1.0f - s - t;

#ifdef NORMAL_DEBUGGING
    btVector3 regen_position = s * p1 + t * p2 + w * p3;

    if((regen_position - position).length2() >= 0.0001f)
    {
        printf("bary:\n");
        printf("new: %f %f %f\n", regen_position.getX(),regen_position.getY(),regen_position.getZ());
        printf("old: %f %f %f\n", position.getX(), position.getY(),position.getZ());
        printf("stw: %f %f %f\n", s, t, w);
        printf("p1:  %f %f %f\n", p1.getX(),p1.getY(),p1.getZ());
        printf("p2:  %f %f %f\n", p2.getX(),p2.getY(),p2.getZ());
        printf("p3:  %f %f %f\n", p3.getX(),p3.getY(),p3.getZ());
        printf("pos: %f %f %f\n", position.getX(),position.getY(),position.getZ());
    }
#endif

    return s*n1 + t*n2 + w*n3;
}   // getInterpolatedNormal

// ----------------------------------------------------------------------------
/** Casts a ray from 'from' to 'to'. If a triangle of this mesh was hit,
 *  xyz and material will be set.
 *  \param from/to The from and to position for the raycast/
 *  \param xyz The position in world where the ray hit.
 *  \param material The material of the mesh that was hit.
 *  \param normal The intrapolated normal at that position.
 *  \return True if a triangle was hit, false otherwise (and no output
 *          variable will be set.
 */
bool TriangleMesh::castRay(const btVector3 &from, const btVector3 &to,
                           btVector3 *xyz, const Material **material, 
                           btVector3 *normal) const
{
    if(!m_collision_shape)
    {
        *material=NULL;
        return false;
    }

    btTransform trans_from;
    trans_from.setIdentity();
    trans_from.setOrigin(from);

    btTransform trans_to;
    trans_to.setIdentity();
    trans_to.setOrigin(to);

    btTransform world_trans;
    world_trans.setIdentity();

    btCollisionWorld::ClosestRayResultCallback result(from, to);


    class MaterialRayResult : public btCollisionWorld::ClosestRayResultCallback
    {
    public:
        const Material* m_material;
        const TriangleMesh *m_this;
        // --------------------------------------------------------------------
        MaterialRayResult(const btVector3 &p1, const btVector3 &p2, 
                          const TriangleMesh *me)
                        : btCollisionWorld::ClosestRayResultCallback(p1,p2)
        {
            m_material = NULL;
            m_this     = me;
        }   // MaterialRayResult
        // --------------------------------------------------------------------
        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
                                         bool normalInWorldSpace) 
        {
            m_material = 
                m_this->getMaterial(rayResult.m_localShapeInfo->m_triangleIndex);
            return btCollisionWorld::ClosestRayResultCallback
                    ::addSingleResult(rayResult, normalInWorldSpace);
        }   // AddSingleResult
        // --------------------------------------------------------------------
    };   // myCollision

    MaterialRayResult ray_callback(from, to, this);

    // If this is a rigid body, m_collision_object is NULL, and the
    // rigid body is the actual collision object.
    btCollisionWorld::rayTestSingle(trans_from, trans_to, 
                                    m_collision_object ? m_collision_object 
                                                       : m_body,
                                    m_collision_shape, world_trans, 
                                    ray_callback);
    if(ray_callback.hasHit())
    {
        *xyz      = ray_callback.m_hitPointWorld;
        *material = ray_callback.m_material;
        if(normal)
        {
            *normal   = ray_callback.m_hitNormalWorld;
            normal->normalize();
        }
    }
    else
    {
        *material = NULL;
        if(normal)
            normal->setValue(0, 1, 0);
    }
    return ray_callback.hasHit();

}   // castRay
