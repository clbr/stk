//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009  Joerg Henrichs
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

#ifndef HEADER_TRACK_OBJECT_MANAGER_HPP
#define HEADER_TRACK_OBJECT_MANAGER_HPP

#include "physics/physical_object.hpp"
#include "tracks/track_object.hpp"
#include "utils/ptr_vector.hpp"

class Track;
class Vec3;
class XMLNode;
class LODNode;

#include <map>
#include <vector>
#include <string>

/**
  * \ingroup tracks
  */
class TrackObjectManager
{
protected:
    /**
      * The different type of track objects: physical objects, graphical 
      * objects (without a physical representation) - the latter might be
      * eye candy (to reduce work for physics), ...
      */
    enum TrackObjectType {TO_PHYSICAL, TO_GRAPHICAL};
    PtrVector<TrackObject> m_all_objects;
    
    /** Temporary storage for LOD objects whose XML node was read but whose
      * scene node is not yet ready
      */
    std::map<std::string, std::vector<const XMLNode*> > m_lod_objects;
    
public:
         TrackObjectManager();
        ~TrackObjectManager();
    void add(const XMLNode &xml_node);
    void update(float dt);
    void handleExplosion(const Vec3 &pos, const PhysicalObject *mp,
                         bool secondary_hits=true);
    void reset();
    void init();
    
    /** Enable or disable fog on objects */
    void enableFog(bool enable);

    void insertObject(TrackObject* object);
    
    void removeObject(TrackObject* who);
    
    void assingLodNodes(const std::vector<LODNode*>& lod);
    
          PtrVector<TrackObject>& getObjects()       { return m_all_objects; }
    const PtrVector<TrackObject>& getObjects() const { return m_all_objects; }
    
};   // class TrackObjectManager

#endif
