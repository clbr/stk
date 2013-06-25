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

#include "physics/physics.hpp"

#include "animations/three_d_animation.hpp"
#include "karts/kart_properties.hpp"
#include "karts/rescue_animation.hpp"
#include "network/race_state.hpp"
#include "graphics/stars.hpp"
#include "karts/explosion_animation.hpp"
#include "physics/btKart.hpp"
#include "physics/btUprightConstraint.hpp"
#include "physics/irr_debug_drawer.hpp"
#include "physics/physical_object.hpp"
#include "physics/stk_dynamics_world.hpp"
#include "physics/triangle_mesh.hpp"
#include "tracks/track.hpp"

// ----------------------------------------------------------------------------
/** Initialise physics.
 *  Create the bullet dynamics world.
 */
Physics::Physics() : btSequentialImpulseConstraintSolver()
{
    m_collision_conf      = new btDefaultCollisionConfiguration();
    m_dispatcher          = new btCollisionDispatcher(m_collision_conf);
}   // Physics

//-----------------------------------------------------------------------------
/** The actual initialisation of the physics, which is called after the track
 *  model is loaded. This allows the physics to use the actual track dimension
 *  for the axis sweep.
 */
void Physics::init(const Vec3 &world_min, const Vec3 &world_max)
{
    m_physics_loop_active = false;
    m_axis_sweep          = new btAxisSweep3(world_min, world_max);
    m_dynamics_world      = new STKDynamicsWorld(m_dispatcher,
                                                 m_axis_sweep,
                                                 this,
                                                 m_collision_conf);
    m_karts_to_delete.clear();
    m_dynamics_world->setGravity(
        btVector3(0.0f,
                  -World::getWorld()->getTrack()->getGravity(),
                  0.0f));
    m_debug_drawer = new IrrDebugDrawer();
    m_dynamics_world->setDebugDrawer(m_debug_drawer);
}   // init

//-----------------------------------------------------------------------------
Physics::~Physics()
{
    delete m_debug_drawer;
    delete m_dynamics_world;
    delete m_axis_sweep;
    delete m_dispatcher;
    delete m_collision_conf;
}   // ~Physics

// ----------------------------------------------------------------------------
/** Adds a kart to the physics engine.
 *  This adds the rigid body, the vehicle, and the upright constraint, but only
 *  if the kart is not already in the physics world.
 *  \param kart The kart to add.
 *  \param vehicle The raycast vehicle object.
 */
void Physics::addKart(const AbstractKart *kart)
{
	const btCollisionObjectArray &all_objs =
		m_dynamics_world->getCollisionObjectArray();
	for(unsigned int i=0; i<(unsigned int)all_objs.size(); i++)
	{
		if(btRigidBody::upcast(all_objs[i])== kart->getBody())
			return;
	}
    m_dynamics_world->addRigidBody(kart->getBody());
    m_dynamics_world->addVehicle(kart->getVehicle());
    m_dynamics_world->addConstraint(kart->getUprightConstraint());
}   // addKart

//-----------------------------------------------------------------------------
/** Removes a kart from the physics engine. This is used when rescuing a kart
 *  (and during cleanup).
 *  \param kart The kart to remove.
 */
void Physics::removeKart(const AbstractKart *kart)
{
    // We can't simply remove a kart from the physics world when currently
    // loops over all kart objects are active. This can happen in collision
    // handling, where a collision of a kart with a cake etc. removes
    // a kart from the physics. In this case save pointers to the kart
    // to be removed, and remove them once the physics processing is done.
    if(m_physics_loop_active)
    {
        // Make sure to remove each kart only once.
        if(std::find(m_karts_to_delete.begin(), m_karts_to_delete.end(), kart)
                     == m_karts_to_delete.end())
        {
            m_karts_to_delete.push_back(kart);
        }
    }
    else
    {
        m_dynamics_world->removeRigidBody(kart->getBody());
        m_dynamics_world->removeVehicle(kart->getVehicle());
        m_dynamics_world->removeConstraint(kart->getUprightConstraint());
    }
}   // removeKart

//-----------------------------------------------------------------------------
/** Updates the physics simulation and handles all collisions.
 *  \param dt Time step.
 */
void Physics::update(float dt)
{
    m_physics_loop_active = true;
    // Bullet can report the same collision more than once (up to 4
    // contact points per collision). Additionally, more than one internal
    // substep might be taken, resulting in potentially even more
    // duplicates. To handle this, all collisions (i.e. pair of objects)
    // are stored in a vector, but only one entry per collision pair
    // of objects.
    m_all_collisions.clear();

    // Maximum of three substeps. This will work for framerate down to
    // 20 FPS (bullet default frequency is 60 HZ).
    m_dynamics_world->stepSimulation(dt, 3);

    // Now handle the actual collision. Note: flyables can not be removed
    // inside of this loop, since the same flyables might hit more than one
    // other object. So only a flag is set in the flyables, the actual
    // clean up is then done later in the projectile manager.
    std::vector<CollisionPair>::iterator p;
    for(p=m_all_collisions.begin(); p!=m_all_collisions.end(); ++p)
    {
        // Kart-kart collision
        // --------------------
        if(p->getUserPointer(0)->is(UserPointer::UP_KART))
        {
            AbstractKart *a=p->getUserPointer(0)->getPointerKart();
            AbstractKart *b=p->getUserPointer(1)->getPointerKart();
            race_state->addCollision(a->getWorldKartId(),
                                     b->getWorldKartId());
            KartKartCollision(p->getUserPointer(0)->getPointerKart(),
                              p->getContactPointCS(0),
                              p->getUserPointer(1)->getPointerKart(),
                              p->getContactPointCS(1)                );
            continue;
        }  // if kart-kart collision

        if(p->getUserPointer(0)->is(UserPointer::UP_PHYSICAL_OBJECT))
        {
            // Kart hits physical object
            // -------------------------
            PhysicalObject *obj = p->getUserPointer(0)
                                   ->getPointerPhysicalObject();
            if(obj->isCrashReset())
            {
                AbstractKart *kart = p->getUserPointer(1)->getPointerKart();
                new RescueAnimation(kart);
            }
            else if (obj->isExplodeKartObject())
            {
                AbstractKart *kart = p->getUserPointer(1)->getPointerKart();
                ExplosionAnimation::create(kart);
            }
            continue;
        }

        if(p->getUserPointer(0)->is(UserPointer::UP_ANIMATION))
        {
            // Kart hits animation
            ThreeDAnimation *anim=p->getUserPointer(0)->getPointerAnimation();
            if(anim->isCrashReset())
            {
                AbstractKart *kart = p->getUserPointer(1)->getPointerKart();
                new RescueAnimation(kart);
            }
            else if (anim->isExplodeKartObject())
            {
                AbstractKart *kart = p->getUserPointer(1)->getPointerKart();
                ExplosionAnimation::create(kart);
            }
            continue;

        }
        // now the first object must be a projectile
        // =========================================
        if(p->getUserPointer(1)->is(UserPointer::UP_TRACK))
        {
            // Projectile hits track
            // ---------------------
            p->getUserPointer(0)->getPointerFlyable()->hitTrack();
        }
        else if(p->getUserPointer(1)->is(UserPointer::UP_PHYSICAL_OBJECT))
        {
            // Projectile hits physical object
            // -------------------------------
            p->getUserPointer(0)->getPointerFlyable()
                ->hit(NULL, p->getUserPointer(1)->getPointerPhysicalObject());

        }
        else if(p->getUserPointer(1)->is(UserPointer::UP_KART))
        {
            // Projectile hits kart
            // --------------------
            // Only explode a bowling ball if the target is
            // not invulnerable
            if(p->getUserPointer(0)->getPointerFlyable()->getType()
                !=PowerupManager::POWERUP_BOWLING                         ||
                !p->getUserPointer(1)->getPointerKart()->isInvulnerable()   )
                    p->getUserPointer(0)->getPointerFlyable()
                     ->hit(p->getUserPointer(1)->getPointerKart());
        }
        else
        {
            // Projectile hits projectile
            // --------------------------
            p->getUserPointer(0)->getPointerFlyable()->hit(NULL);
            p->getUserPointer(1)->getPointerFlyable()->hit(NULL);
        }
    }  // for all p in m_all_collisions

    m_physics_loop_active = false;
    // Now remove the karts that were removed while the above loop
    // was active. Now we can safely call removeKart, since the loop
    // is finished and m_physics_world_active is not set anymore.
    for(unsigned int i=0; i<m_karts_to_delete.size(); i++)
        removeKart(m_karts_to_delete[i]);
    m_karts_to_delete.clear();
}   // update

//-----------------------------------------------------------------------------
/** Handles the special case of two karts colliding with each other, which
 *  means that bombs must be passed on. If both karts have a bomb, they'll
 *  explode immediately. This function is called from physics::update() on the
 *  server and if no networking is used, and from race_state on the client to
 *  replay what happened on the server.
 *  \param kart_a First kart involved in the collision.
 *  \param contact_point_a Location of collision at first kart (in kart
 *         coordinates).
 *  \param kart_b Second kart involved in the collision.
 *  \param contact_point_b Location of collision at second kart (in kart
 *         coordinates).
 */
void Physics::KartKartCollision(AbstractKart *kart_a,
                                const Vec3 &contact_point_a,
                                AbstractKart *kart_b,
                                const Vec3 &contact_point_b)
{
    // Only one kart needs to handle the attachments, it will
    // fix the attachments for the other kart.
    kart_a->crashed(kart_b, /*handle_attachments*/true);
    kart_b->crashed(kart_a, /*handle_attachments*/false);

    AbstractKart *left_kart, *right_kart;

    // Determine which kart is pushed to the left, and which one to the
    // right. Ideally the sign of the X coordinate of the local conact point
    // could decide the direction (negative X --> was hit on left side, gets
    // push to right), but that can lead to both karts being pushed in the
    // same direction (front left of kart hits rear left).
    // So we just use a simple test (which does the right thing in ideal
    // crashes, but avoids pushing both karts in corner cases
    // - pun intended ;) ).
    if(contact_point_a.getX() < contact_point_b.getX())
    {
        left_kart  = kart_b;
        right_kart = kart_a;
    }
    else
    {
        left_kart  = kart_a;
        right_kart = kart_b;
    }

    // Add a scaling factor depending on the mass (avoid div by zero).
    // The value of f_right is applied to the right kart, and f_left
    // to the left kart. f_left = 1 / f_right
    float f_right =  right_kart->getKartProperties()->getMass() > 0
                     ? left_kart->getKartProperties()->getMass()
                       / right_kart->getKartProperties()->getMass()
                     : 1.5f;
    // Add a scaling factor depending on speed (avoid div by 0)
    f_right *= right_kart->getSpeed() > 0
               ? left_kart->getSpeed()
                  / right_kart->getSpeed()
               : 1.5f;
    // Cap f_right to [0.8,1.25], which results in f_left being
    // capped in the same interval
    if(f_right > 1.25f)
        f_right = 1.25f;
    else if(f_right< 0.8f)
        f_right = 0.8f;
    float f_left = 1/f_right;

    // Check if a kart is more 'actively' trying to push another kart
    // by checking its local sidewards velocity
    float vel_left  = left_kart->getVelocityLC().getX();
    float vel_right = right_kart->getVelocityLC().getX();

    // Use the difference in speed to determine which kart gets a
    // ramming bonus. Normally vel_right and vel_left will have
    // a different sign: right kart will be driving to the left,
    // and left kart to the right (both pushing at each other).
    // By using the sum we get the intended effect: if both karts
    // are pushing with the same speed, vel_diff is 0, if the right
    // kart is driving faster vel_diff will be < 0. If both velocities
    // have the same sign, one kart is trying to steer away from the
    // other, in which case it gets an even bigger push.
    float vel_diff = vel_right + vel_left;

    // More driving towards left --> left kart gets bigger impulse
    if(vel_diff<0)
    {
        // Avoid too large impulse for karts that are driving
        // slow (and division by zero)
        if(fabsf(vel_left)>2.0f)
            f_left *= 1.0f - vel_diff/fabsf(vel_left);
        if(f_left > 2.0f)
            f_left = 2.0f;
    }
    else
    {
        // Avoid too large impulse for karts that are driving
        // slow (and division by zero)
        if(fabsf(vel_right)>2.0f)
            f_right *= 1.0f + vel_diff/fabsf(vel_right);
        if(f_right > 2.0f)
            f_right = 2.0f;
    }

    // Increase the effect somewhat by squaring the factors
    f_left  = f_left  * f_left;
    f_right = f_right * f_right;

    // First push one kart to the left (if there is not already
    // an impulse happening - one collision might cause more
    // than one impulse otherwise)
    if(right_kart->getVehicle()->getCentralImpulseTime()<=0)
    {
        const KartProperties *kp = left_kart->getKartProperties();
        Vec3 impulse(kp->getCollisionImpulse()*f_right, 0, 0);
        impulse = right_kart->getTrans().getBasis() * impulse;
        right_kart->getVehicle()
                 ->setTimedCentralImpulse(kp->getCollisionImpulseTime(),
                                          impulse);
        right_kart ->getBody()->setAngularVelocity(btVector3(0,0,0));
    }

    // Then push the other kart to the right (if there is no
    // impulse happening atm).
    if(left_kart->getVehicle()->getCentralImpulseTime()<=0)
    {
        const KartProperties *kp = right_kart->getKartProperties();
        Vec3 impulse = Vec3(-kp->getCollisionImpulse()*f_left, 0, 0);
        impulse = left_kart->getTrans().getBasis() * impulse;
        left_kart->getVehicle()
                  ->setTimedCentralImpulse(kp->getCollisionImpulseTime(),
                                           impulse);
        left_kart->getBody()->setAngularVelocity(btVector3(0,0,0));
    }

}   // KartKartCollision

//-----------------------------------------------------------------------------
/** This function is called at each internal bullet timestep. It is used
 *  here to do the collision handling: using the contact manifolds after a
 *  physics time step might miss some collisions (when more than one internal
 *  time step was done, and the collision is added and removed). So this
 *  function stores all collisions in a list, which is then handled after the
 *  actual physics timestep. This list only stores a collision if it's not
 *  already in the list, so a collisions which is reported more than once is
 *  nevertheless only handled once.
 *  The list of collision
 *  Parameters: see bullet documentation for details.
 */
btScalar Physics::solveGroup(btCollisionObject** bodies, int numBodies,
                             btPersistentManifold** manifold,int numManifolds,
                             btTypedConstraint** constraints,
                             int numConstraints,
                             const btContactSolverInfo& info,
                             btIDebugDraw* debugDrawer,
                             btStackAlloc* stackAlloc,
                             btDispatcher* dispatcher)
{
    btScalar returnValue=
        btSequentialImpulseConstraintSolver::solveGroup(bodies, numBodies,
                                                        manifold, numManifolds,
                                                        constraints,
                                                        numConstraints, info,
                                                        debugDrawer,
                                                        stackAlloc,
                                                        dispatcher);
    int currentNumManifolds = m_dispatcher->getNumManifolds();
    // We can't explode a rocket in a loop, since a rocket might collide with
    // more than one object, and/or more than once with each object (if there
    // is more than one collision point). So keep a list of rockets that will
    // be exploded after the collisions
    for(int i=0; i<currentNumManifolds; i++)
    {
        btPersistentManifold* contact_manifold =
            m_dynamics_world->getDispatcher()->getManifoldByIndexInternal(i);

        btCollisionObject* objA =
            static_cast<btCollisionObject*>(contact_manifold->getBody0());
        btCollisionObject* objB =
            static_cast<btCollisionObject*>(contact_manifold->getBody1());

        unsigned int num_contacts = contact_manifold->getNumContacts();
        if(!num_contacts) continue;   // no real collision

        UserPointer *upA        = (UserPointer*)(objA->getUserPointer());
        UserPointer *upB        = (UserPointer*)(objB->getUserPointer());

        if(!upA || !upB) continue;

        // 1) object A is a track
        // =======================
        if(upA->is(UserPointer::UP_TRACK))
        {
            if(upB->is(UserPointer::UP_FLYABLE))   // 1.1 projectile hits track
                m_all_collisions.push_back(
                    upB, contact_manifold->getContactPoint(0).m_localPointB,
                    upA, contact_manifold->getContactPoint(0).m_localPointA);
            else if(upB->is(UserPointer::UP_KART))
            {
                AbstractKart *kart=upB->getPointerKart();
                race_state->addCollision(kart->getWorldKartId());
                int n = contact_manifold->getContactPoint(0).m_index0;
                const Material *m
                    = n>=0 ? upA->getPointerTriangleMesh()->getMaterial(n)
                           : NULL;
                // I assume that the normal needs to be flipped in this case,
                // but  I can't verify this since it appears that bullet
                // always has the kart as object A, not B.
                const btVector3 &normal = -contact_manifold->getContactPoint(0)
                                                            .m_normalWorldOnB;
                kart->crashed(m, normal);
            }
        }
        // 2) object a is a kart
        // =====================
        else if(upA->is(UserPointer::UP_KART))
        {
            if(upB->is(UserPointer::UP_TRACK))
            {
                AbstractKart *kart = upA->getPointerKart();
                race_state->addCollision(kart->getWorldKartId());
                int n = contact_manifold->getContactPoint(0).m_index1;
                const Material *m
                    = n>=0 ? upB->getPointerTriangleMesh()->getMaterial(n)
                           : NULL;
                const btVector3 &normal = contact_manifold->getContactPoint(0)
                                                           .m_normalWorldOnB;
                kart->crashed(m, normal);   // Kart hit track
            }
            else if(upB->is(UserPointer::UP_FLYABLE))
                // 2.1 projectile hits kart
                m_all_collisions.push_back(
                    upB, contact_manifold->getContactPoint(0).m_localPointB,
                    upA, contact_manifold->getContactPoint(0).m_localPointA);
            else if(upB->is(UserPointer::UP_KART))
                // 2.2 kart hits kart
                m_all_collisions.push_back(
                    upA, contact_manifold->getContactPoint(0).m_localPointA,
                    upB, contact_manifold->getContactPoint(0).m_localPointB);
            else if(upB->is(UserPointer::UP_PHYSICAL_OBJECT))
                // 2.3 kart hits physical object
                m_all_collisions.push_back(
                    upB, contact_manifold->getContactPoint(0).m_localPointB,
                    upA, contact_manifold->getContactPoint(0).m_localPointA);
            else if(upB->is(UserPointer::UP_ANIMATION))
                m_all_collisions.push_back(
                    upB, contact_manifold->getContactPoint(0).m_localPointB,
                    upA, contact_manifold->getContactPoint(0).m_localPointA);
        }
        // 3) object is a projectile
        // =========================
        else if(upA->is(UserPointer::UP_FLYABLE))
        {
            // 3.1) projectile hits track
            // 3.2) projectile hits projectile
            // 3.3) projectile hits physical object
            // 3.4) projectile hits kart
            if(upB->is(UserPointer::UP_TRACK          ) ||
               upB->is(UserPointer::UP_FLYABLE        ) ||
               upB->is(UserPointer::UP_PHYSICAL_OBJECT) ||
               upB->is(UserPointer::UP_KART           )   )
            {
                m_all_collisions.push_back(
                    upA, contact_manifold->getContactPoint(0).m_localPointA,
                    upB, contact_manifold->getContactPoint(0).m_localPointB);
            }
        }
        // Object is a physical object
        // ===========================
        else if(upA->is(UserPointer::UP_PHYSICAL_OBJECT))
        {
            if(upB->is(UserPointer::UP_FLYABLE))
                m_all_collisions.push_back(
                    upB, contact_manifold->getContactPoint(0).m_localPointB,
                    upA, contact_manifold->getContactPoint(0).m_localPointA);
            else if(upB->is(UserPointer::UP_KART))
                m_all_collisions.push_back(
                    upA, contact_manifold->getContactPoint(0).m_localPointA,
                    upB, contact_manifold->getContactPoint(0).m_localPointB);
        }
        else if (upA->is(UserPointer::UP_ANIMATION))
        {
            if(upB->is(UserPointer::UP_KART))
                m_all_collisions.push_back(
                    upA, contact_manifold->getContactPoint(0).m_localPointA,
                    upB, contact_manifold->getContactPoint(0).m_localPointB);
        }
        else
            assert("Unknown user pointer");           // 4) Should never happen
    }   // for i<numManifolds

    return returnValue;
}   // solveGroup

// ----------------------------------------------------------------------------
/** A debug draw function to show the track and all karts.
 */
void Physics::draw()
{
    if(!m_debug_drawer->debugEnabled() ||
        !World::getWorld()->isRacePhase()) return;

    video::SColor color(77,179,0,0);
    video::SMaterial material;
    material.Thickness = 2;
    material.AmbientColor = color;
    material.DiffuseColor = color;
    material.EmissiveColor= color;
    material.BackfaceCulling = false;
    material.setFlag(video::EMF_LIGHTING, false);
    irr_driver->getVideoDriver()->setMaterial(material);
    irr_driver->getVideoDriver()->setTransform(video::ETS_WORLD,
                                               core::IdentityMatrix);
    m_dynamics_world->debugDrawWorld();
    return;
}   // draw

// ----------------------------------------------------------------------------

/* EOF */

