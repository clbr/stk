//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2008 Joerg Henrichs
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

#include "karts/kart_model.hpp"

#include <IMeshSceneNode.h>
#include <ISceneManager.h>

#include "config/stk_config.hpp"
#include "config/user_config.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/lod_node.hpp"
#include "graphics/mesh_tools.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/kart_properties.hpp"
#include "physics/btKart.hpp"
#include "utils/constants.hpp"
#include "utils/log.hpp"

#define SKELETON_DEBUG 0

float KartModel::UNDEFINED = -99.9f;

/** Default constructor which initialises all variables with defaults.
 *  Note that the KartModel is copied, so make sure to update makeCopy
 *  if any more variables are added to this object.
 *  ATM there are two pointers:
 *  - to the scene node (which is otherwise handled by kart/movable and set
 *    later anyway)
 *  - to the mesh. Sharing mesh is supported in irrlicht, so that's
 *    no problem.
 *  There are two different type of instances of this class:
 *  One is the 'master instances' which is part of the kart_properties.
 *  These instances have m_is_master = true, will cause an assertion
 *  crash if attachModel is called or the destructor notices any
 *  wheels being defined.
 *  The other types are copies of one of the master objects: these are
 *  used when actually displaying the karts (e.g. in race). They must
 *  be copied since otherwise (if the same kart is used more than once)
 *  shared variables in KartModel (esp. animation status) will cause
 *  incorrect animations. The mesh is shared (between the master instance
 *  and all of its copies).
 *  Technically the scene node and mesh should be grab'ed on copy,
 *  and dropped when the copy is deleted. But since the master copy
 *  in the kart_properties_manager is always kept, there is no risk of
 *  a mesh being deleted to early.
 */
KartModel::KartModel(bool is_master)
{
    m_is_master  = is_master;
    m_kart       = NULL;
    m_mesh       = NULL;
    m_hat_name   = "";
    m_hat_node   = NULL;
    m_hat_offset = core::vector3df(0,0,0);

    for(unsigned int i=0; i<4; i++)
    {
        m_wheel_graphics_position[i] = Vec3(UNDEFINED);
        m_wheel_physics_position[i]  = Vec3(UNDEFINED);
        m_wheel_graphics_radius[i]   = 0.0f;   // for kart without separate wheels
        m_wheel_model[i]             = NULL;
        m_wheel_node[i]              = NULL;

        // default value for kart suspensions. move to config file later
        // if we find each kart needs custom values
        m_min_suspension[i] = -0.59f;
        m_max_suspension[i] = 0.59f;
        m_dampen_suspension_amplitude[i] = 2.5f;
    }
    m_wheel_filename[0] = "";
    m_wheel_filename[1] = "";
    m_wheel_filename[2] = "";
    m_wheel_filename[3] = "";
    m_animated_node     = NULL;
    m_mesh              = NULL;
    for(unsigned int i=AF_BEGIN; i<=AF_END; i++)
        m_animation_frame[i]=-1;
    m_animation_speed   = 25;
    m_current_animation = AF_DEFAULT;
}   // KartModel

// ----------------------------------------------------------------------------
/** This function loads the information about the kart from a xml file. It
 *  does not actually load the models (see load()).
 *  \param node  XML object of configuration file.
 */
void KartModel::loadInfo(const XMLNode &node)
{
    node.get("model-file", &m_model_filename);
    if(const XMLNode *animation_node=node.getNode("animations"))
    {
        animation_node->get("left",           &m_animation_frame[AF_LEFT]      );
        animation_node->get("straight",       &m_animation_frame[AF_STRAIGHT]  );
        animation_node->get("right",          &m_animation_frame[AF_RIGHT]     );
        animation_node->get("start-winning",  &m_animation_frame[AF_WIN_START] );
        animation_node->get("start-winning-loop",
                                              &m_animation_frame[AF_WIN_LOOP_START] );
        animation_node->get("end-winning",    &m_animation_frame[AF_WIN_END]   );
        animation_node->get("start-losing",   &m_animation_frame[AF_LOSE_START]);
        animation_node->get("start-losing-loop",
                                             &m_animation_frame[AF_LOSE_LOOP_START]);
        animation_node->get("end-losing",     &m_animation_frame[AF_LOSE_END]  );
        animation_node->get("start-explosion",&m_animation_frame[AF_LOSE_START]);
        animation_node->get("end-explosion",  &m_animation_frame[AF_LOSE_END]  );
        animation_node->get("speed",          &m_animation_speed               );
    }

    if(const XMLNode *wheels_node=node.getNode("wheels"))
    {
        loadWheelInfo(*wheels_node, "front-right", 0);
        loadWheelInfo(*wheels_node, "front-left",  1);
        loadWheelInfo(*wheels_node, "rear-right",  2);
        loadWheelInfo(*wheels_node, "rear-left",   3);
    }
    if(const XMLNode *hat_node=node.getNode("hat"))
    {
        if(hat_node->get("offset", &m_hat_offset))
        {
            // Xmas mode handling :)
            if(UserConfigParams::m_xmas_enabled)
                setHatMeshName("christmas_hat.b3d");
        }
    }
    else
        m_hat_offset = core::vector3df(0,0,0);
}   // loadInfo

// ----------------------------------------------------------------------------
/** Destructor.
 */
KartModel::~KartModel()
{
    if (m_animated_node)
    {
        m_animated_node->setAnimationEndCallback(NULL);
        m_animated_node->drop();
    }

    for(unsigned int i=0; i<4; i++)
    {
        if(m_wheel_node[i])
        {
            // Master KartModels should never have a wheel attached.
            assert(!m_is_master);

            m_wheel_node[i]->drop();
        }
        if(m_is_master && m_wheel_model[i])
        {
            irr_driver->dropAllTextures(m_wheel_model[i]);
            irr_driver->removeMeshFromCache(m_wheel_model[i]);
        }
    }

    if(m_is_master && m_mesh)
    {
        m_mesh->drop();
        // If there is only one copy left, it's the copy in irrlicht's
        // mesh cache, so it can be remove.
        if(m_mesh && m_mesh->getReferenceCount()==1)
        {
            irr_driver->dropAllTextures(m_mesh);
            irr_driver->removeMeshFromCache(m_mesh);
        }
    }

#ifdef DEBUG
#if SKELETON_DEBUG
    irr_driver->clearDebugMeshes();
#endif
#endif

}  // ~KartModel

// ----------------------------------------------------------------------------
/** This function returns a copy of this object. The memory is allocated
 *  here, but needs to be managed (esp. freed) by the calling function.
 *  It is also marked not to be a master copy, so attachModel can be called
 *  for this instance.
 */
KartModel* KartModel::makeCopy()
{
    // Make sure that we are copying from a master objects, and
    // that there is indeed no animated node defined here ...
    // just in case.
    assert(m_is_master);
    assert(!m_animated_node);
    KartModel *km           = new KartModel(/*is master*/ false);
    km->m_kart_width        = m_kart_width;
    km->m_kart_length       = m_kart_length;
    km->m_kart_height       = m_kart_height;
    km->m_mesh              = m_mesh;
    km->m_model_filename    = m_model_filename;
    km->m_animation_speed   = m_animation_speed;
    km->m_current_animation = AF_DEFAULT;
    km->m_animated_node     = NULL;
    km->m_hat_offset        = m_hat_offset;
    km->m_hat_name          = m_hat_name;
    for(unsigned int i=0; i<4; i++)
    {
        km->m_wheel_model[i]             = m_wheel_model[i];
        // Master should not have any wheel nodes.
        assert(!m_wheel_node[i]);
        km->m_wheel_filename[i]             = m_wheel_filename[i];
        km->m_wheel_graphics_position[i]    = m_wheel_graphics_position[i];
        km->m_wheel_physics_position[i]     = m_wheel_physics_position[i];
        km->m_wheel_graphics_radius[i]      = m_wheel_graphics_radius[i];
        km->m_min_suspension[i]             = m_min_suspension[i];
        km->m_max_suspension[i]             = m_max_suspension[i];
        km->m_dampen_suspension_amplitude[i]= m_dampen_suspension_amplitude[i];
    }
    for(unsigned int i=AF_BEGIN; i<=AF_END; i++)
        km->m_animation_frame[i] = m_animation_frame[i];

    return km;
}   // makeCopy

// ----------------------------------------------------------------------------

/** Attach the kart model and wheels to the scene node.
 *  \return the node with the model attached
 */
scene::ISceneNode* KartModel::attachModel(bool animated_models)
{
    assert(!m_is_master);

    scene::ISceneNode* node;

    if (animated_models)
    {
        LODNode* lod_node = new LODNode("kart",
                                        irr_driver->getSceneManager()->getRootSceneNode(),
                                        irr_driver->getSceneManager()                    );


        node = irr_driver->addAnimatedMesh(m_mesh);
        // as animated mesh are not cheap to render use frustum box culling
        node->setAutomaticCulling(scene::EAC_FRUSTUM_BOX);

        lod_node->add(50, node, true);
        scene::ISceneNode* static_model = attachModel(false);
        lod_node->add(500, static_model, true);
        m_animated_node = static_cast<scene::IAnimatedMeshSceneNode*>(node);

        m_hat_node = NULL;
        if(m_hat_name.size()>0)
        {
            scene::IBoneSceneNode *bone = m_animated_node->getJointNode("Head");
            if(!bone)
                bone = m_animated_node->getJointNode("head");
            if(bone)
            {

                // Till we have all models fixed, accept Head and head as bone naartme
                scene::IMesh *hat_mesh =
                    irr_driver->getAnimatedMesh(
                         file_manager->getModelFile(m_hat_name));
                m_hat_node = irr_driver->addMesh(hat_mesh);
                bone->addChild(m_hat_node);
                m_animated_node->setCurrentFrame((float)m_animation_frame[AF_STRAIGHT]);
                m_animated_node->OnAnimate(0);
                bone->updateAbsolutePosition();

                // With the hat node attached to the head bone, we have to
                // reverse the transformation of the bone, so that the hat
                // is still properly placed. Esp. the hat offset needs
                // to be rotated.
                const core::matrix4 mat = bone->getAbsoluteTransformation();
                core::matrix4 inv;
                mat.getInverse(inv);
                core::vector3df rotated_offset;
                inv.rotateVect(rotated_offset, m_hat_offset);
                m_hat_node->setPosition(rotated_offset);
                m_hat_node->setScale(inv.getScale());
                m_hat_node->setRotation(inv.getRotationDegrees());
            }   // if bone
        }   // if(m_hat_name)

#ifdef DEBUG
        std::string debug_name = m_model_filename+" (animated-kart-model)";
        node->setName(debug_name.c_str());
#if SKELETON_DEBUG
        irr_driber->addDebugMesh(m_animated_node);
#endif
#endif
        m_animated_node->setLoopMode(false);
        m_animated_node->grab();
        node = lod_node;

        // Become the owner of the wheels
        for(unsigned int i=0; i<4; i++)
        {
            if(!m_wheel_model[i]) continue;
            m_wheel_node[i]->setParent(lod_node);
        }
    }
    else
    {
        // If no animations are shown, make sure to pick the frame
        // with a straight ahead animation (if exist).
        int straight_frame = m_animation_frame[AF_STRAIGHT]>=0
                           ? m_animation_frame[AF_STRAIGHT]
                           : 0;

        scene::IMesh* main_frame = m_mesh->getMesh(straight_frame);
        main_frame->setHardwareMappingHint(scene::EHM_STATIC);

        node = irr_driver->addMesh(main_frame);
#ifdef DEBUG
        std::string debug_name = m_model_filename+" (kart-model)";
        node->setName(debug_name.c_str());
#endif
        for(unsigned int i=0; i<4; i++)
        {
            if(!m_wheel_model[i]) continue;
            m_wheel_node[i] = irr_driver->addMesh(m_wheel_model[i], node);
            m_wheel_node[i]->grab();
    #ifdef DEBUG
            std::string debug_name = m_wheel_filename[i]+" (wheel)";
            m_wheel_node[i]->setName(debug_name.c_str());
    #endif
            m_wheel_node[i]->setPosition(m_wheel_graphics_position[i].toIrrVector());
        }
    }
    return node;
}   // attachModel


// ----------------------------------------------------------------------------
/** Loads the 3d model and all wheels.
 */
bool KartModel::loadModels(const KartProperties &kart_properties)
{
    assert(m_is_master);
    std::string  full_path = kart_properties.getKartDir()+m_model_filename;
    m_mesh                 = irr_driver->getAnimatedMesh(full_path);
    if(!m_mesh)
    {
        Log::error("Kart_Model", "Problems loading mesh '%s' - kart '%s' will"
                   "not be available.",
                   full_path.c_str(), kart_properties.getIdent().c_str());
        return false;
    }
    m_mesh->grab();
    irr_driver->grabAllTextures(m_mesh);

    Vec3 min, max;
    MeshTools::minMax3D(m_mesh->getMesh(m_animation_frame[AF_STRAIGHT]), &min, &max);

    Vec3 size     = max-min;
    m_kart_width  = size.getX();
    m_kart_height = size.getY();
    m_kart_length = size.getZ();

    // Now set default some default parameters (if not defined) that
    // depend on the size of the kart model (wheel position, center
    // of gravity shift)
    for(unsigned int i=0; i<4; i++)
    {
        if(m_wheel_graphics_position[i].getX()==UNDEFINED)
        {
            m_wheel_graphics_position[i].setX( ( i==1||i==3)
                                               ? -0.5f*m_kart_width
                                               :  0.5f*m_kart_width  );
            m_wheel_graphics_position[i].setY(0);
            m_wheel_graphics_position[i].setZ( (i<2) ?  0.5f*m_kart_length
                                                     : -0.5f*m_kart_length);
        }
    }

    // Load the wheel models. This can't be done early, since the default
    // values for the graphical position must be defined, which in turn
    // depend on the size of the model.
    for(unsigned int i=0; i<4; i++)
    {
        // For kart models without wheels.
        if(m_wheel_filename[i]=="") continue;
        std::string full_wheel =
            kart_properties.getKartDir()+m_wheel_filename[i];
        m_wheel_model[i] = irr_driver->getMesh(full_wheel);
        // Grab all textures. This is done for the master only, so
        // the destructor will only free the textures if a master
        // copy is freed.
        irr_driver->grabAllTextures(m_wheel_model[i]);
    }   // for i<4

    return true;
}   // loadModels

// ----------------------------------------------------------------------------
/** Loads a single wheel node. Currently this is the name of the wheel model
 *  and the position of the wheel relative to the kart.
 *  \param wheel_name Name of the wheel, e.g. wheel-rear-left.
 *  \param index Index of this wheel in the global m_wheel* fields.
 */
void KartModel::loadWheelInfo(const XMLNode &node,
                              const std::string &wheel_name, int index)
{
    const XMLNode *wheel_node = node.getNode(wheel_name);
    if(!wheel_node)
    {
        // Only print the warning if a model filename is given. Otherwise the
        // stk_config file is read (which has no model information).
        if(m_model_filename!="")
        {
            Log::error("Kart_Model", "Missing wheel information '%s' for model"
                       "'%s'.", wheel_name.c_str(), m_model_filename.c_str());
            Log::error("Kart_Model", "This can be ignored, but the wheels will"
                       "not rotate.");
        }
        return;
    }
    wheel_node->get("model",            &m_wheel_filename[index]         );
    wheel_node->get("position",         &m_wheel_graphics_position[index]);
    wheel_node->get("physics-position", &m_wheel_physics_position[index] );
    wheel_node->get("min-suspension",   &m_min_suspension[index]         );
    wheel_node->get("max-suspension",   &m_max_suspension[index]         );
}   // loadWheelInfo

// ----------------------------------------------------------------------------
/** Sets the default position for the physical wheels if they are not defined
 *  in the data file. The default position is to have the wheels at the corner
 *  of the chassis. But since the position is relative to the center of mass,
 *  this must be specified.
 *  \param center_shift Amount the kart chassis is moved relative to the center
 *                      of mass.
 *  \param wheel_radius Radius of the physics wheels.
 */
void  KartModel::setDefaultPhysicsPosition(const Vec3 &center_shift,
                                           float wheel_radius)
{
    for(unsigned int i=0; i<4; i++)
    {
        if(m_wheel_physics_position[i].getX()==UNDEFINED)
        {
            m_wheel_physics_position[i].setX( ( i==1||i==3)
                                               ? -0.5f*m_kart_width
                                               :  0.5f*m_kart_width
                                               +center_shift.getX(  ));
            // Set the connection point so that a maximum compressed wheel
            // (susp. length=0) will still poke a little bit out under the
            // kart
            m_wheel_physics_position[i].setY(wheel_radius-0.05f);
            m_wheel_physics_position[i].setZ( (0.5f*m_kart_length-wheel_radius)
                                              * ( (i<2) ? 1 : -1)
                                               +center_shift.getZ());
        }   // if physics position is not defined
    }

}   // setDefaultPhysicsPosition

// ----------------------------------------------------------------------------
/** Resets the kart model. It stops animation from being played and resets
 *  the wheels to the correct position (i.e. no suspension).
 */
void KartModel::reset()
{
    // Reset the wheels
    const float suspension[4]={0,0,0,0};
    update(0, 0.0f, suspension);

    // Stop any animations currently being played.
    setAnimation(KartModel::AF_DEFAULT);
    // Don't force any LOD
    ((LODNode*)m_kart->getNode())->forceLevelOfDetail(-1);
}   // reset

// ----------------------------------------------------------------------------
/** Called when the kart finished the race. It will force the highest LOD
 *  for the kart, since otherwise the end camera can be far away (due to
 *  zooming) and show non-animated karts.
 */
void KartModel::finishedRace()
{
    // Force the animated model, independent of actual camera distance.
    ((LODNode*)m_kart->getNode())->forceLevelOfDetail(0);
}   // finishedRace

// ----------------------------------------------------------------------------
/** Enables- or disables the end animation.
 *  \param type The type of animation to play.
 */
void KartModel::setAnimation(AnimationFrameType type)
{
    // if animations disabled, give up
    if (m_animated_node == NULL) return;

    m_current_animation = type;
    if(m_current_animation==AF_DEFAULT)
    {
        m_animated_node->setLoopMode(false);
        if(m_animation_frame[AF_LEFT] <= m_animation_frame[AF_RIGHT])
            m_animated_node->setFrameLoop(m_animation_frame[AF_LEFT],
                                          m_animation_frame[AF_RIGHT] );
        else
            m_animated_node->setFrameLoop(m_animation_frame[AF_RIGHT],
                                          m_animation_frame[AF_LEFT] );
        m_animated_node->setAnimationEndCallback(NULL);
        m_animated_node->setAnimationSpeed(0);
    }
    else if(m_animation_frame[type]>-1)
    {
        // 'type' is the start frame of the animation, type + 1 the frame
        // to begin the loop with, type + 2 to end the frame with
        AnimationFrameType end = (AnimationFrameType)(type+2);
        m_animated_node->setAnimationSpeed(m_animation_speed);
        m_animated_node->setFrameLoop(m_animation_frame[type],
                                      m_animation_frame[end]    );
        // Loop mode must be set to false so that we get a callback when
        // the first iteration is finished.
        m_animated_node->setLoopMode(false);
        m_animated_node->setAnimationEndCallback(this);
    }
}   // setAnimation

// ----------------------------------------------------------------------------
/** Called from irrlicht when a non-looped animation ends. This is used to
 *  implement an introductory frame sequence before the actual loop can
 *  start: first a non-looped version from the first frame to the last
 *  frame is being played. When this is finished, this function is called,
 *  which then enables the actual loop.
 *  \param node The node for which the animation ended. Should always be
 *         m_animated_node
 */
void KartModel::OnAnimationEnd(scene::IAnimatedMeshSceneNode *node)
{
    // It should only be called for the animated node of this
    // kart_model
    assert(node==m_animated_node);

    // It should be a non-default type of animation, and should have
    // a non negative frame (i.e. the animation is indeed defined).
    if(m_current_animation==AF_DEFAULT ||
        m_animation_frame[m_current_animation]<=-1)
    {
        Log::debug("Kart_Model", "OnAnimationEnd for '%s': current %d frame %d",
               m_model_filename.c_str(),
               m_current_animation, m_animation_frame[m_current_animation]);
        assert(false);
    }

    // 'type' is the start frame of the animation, type + 1 the frame
    // to begin the loop with, type + 2 to end the frame with
    AnimationFrameType start = (AnimationFrameType)(m_current_animation+1);
    // If there is no loop-start defined (i.e. no 'introductory' sequence)
    // use the normal start frame.
    if(m_animation_frame[start]==-1)
        start = m_current_animation;
    AnimationFrameType end   = (AnimationFrameType)(m_current_animation+2);
    m_animated_node->setAnimationSpeed(m_animation_speed);
    m_animated_node->setFrameLoop(m_animation_frame[start],
                                  m_animation_frame[end]   );
    m_animated_node->setLoopMode(true);
    m_animated_node->setAnimationEndCallback(NULL);
}   // OnAnimationEnd

// ----------------------------------------------------------------------------
/** Rotates and turns the wheels appropriately, and adjust for suspension.
 *  \param rotation_dt How far the wheels have rotated since last time.
 *  \param steer The actual steer settings.
 *  \param suspension Suspension height for all four wheels.
 */
void KartModel::update(float rotation_dt, float steer, const float suspension[4])
{
    float clamped_suspension[4];
    // Clamp suspension to minimum and maximum suspension length, so that
    // the graphical wheel models don't look too wrong.
    for(unsigned int i=0; i<4; i++)
    {
        const float suspension_length = (m_max_suspension[i]-m_min_suspension[i])/2;

        // limit amplitude between set limits, first dividing it by a
        // somewhat arbitrary constant to reduce visible wheel movement
        clamped_suspension[i] = suspension[i]/m_dampen_suspension_amplitude[i];
        float ratio = clamped_suspension[i] / suspension_length;
        const int sign = ratio < 0 ? -1 : 1;
        // expanded form of 1 - (1 - x)^2, i.e. making suspension display
        // quadratic and not linear
        ratio = sign * fabsf(ratio*(2-ratio));
//        clamped_suspension[i] = ratio*suspension_length;
        clamped_suspension[i] = std::min(std::max(ratio*suspension_length,
                                                  m_min_suspension[i]),
                                                  m_max_suspension[i]);
    }   // for i<4

    core::vector3df wheel_steer(0, steer*30.0f, 0);

    for(unsigned int i=0; i<4; i++)
    {
        if(!m_wheel_node[i]) continue;
#ifdef DEBUG
        if(UserConfigParams::m_physics_debug && m_kart)
        {
            // Make wheels that are not touching the ground invisible
            bool wheel_has_contact =
                m_kart->getVehicle()->getWheelInfo(i).m_raycastInfo
                                                     .m_isInContact;
            m_wheel_node[i]->setVisible(wheel_has_contact);
        }
#endif
        core::vector3df pos =  m_wheel_graphics_position[i].toIrrVector();
        pos.Y += clamped_suspension[i];
        m_wheel_node[i]->setPosition(pos);

        // Now calculate the new rotation: (old + change) mod 360
        float new_rotation = m_wheel_node[i]->getRotation().X
                             + rotation_dt * RAD_TO_DEGREE;
        new_rotation = fmodf(new_rotation, 360);
        core::vector3df wheel_rotation(new_rotation, 0, 0);
        // Only apply steer to first 2 wheels.
        if (i < 2)
            wheel_rotation += wheel_steer;
        m_wheel_node[i]->setRotation(wheel_rotation);
    } // for (i < 4)

    // If animations are disabled, stop here
    if (m_animated_node == NULL) return;

    // Check if the end animation is being played, if so, don't
    // play steering animation.
    if(m_current_animation!=AF_DEFAULT) return;

    if(m_animation_frame[AF_LEFT]<0) return;   // no animations defined

    // Update animation if necessary
    // -----------------------------
    float frame;
    if(steer>0.0f)      frame = m_animation_frame[AF_STRAIGHT]
                              - ( ( m_animation_frame[AF_STRAIGHT]
                                        -m_animation_frame[AF_RIGHT]  )*steer);
    else if(steer<0.0f) frame = m_animation_frame[AF_STRAIGHT]
                              + ( (m_animation_frame[AF_STRAIGHT]
                                        -m_animation_frame[AF_LEFT]   )*steer);
    else                frame = (float)m_animation_frame[AF_STRAIGHT];

    m_animated_node->setCurrentFrame(frame);
}   // update

