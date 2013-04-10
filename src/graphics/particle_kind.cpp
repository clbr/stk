//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2011  Joerg Henrichs, Marianne Gagnon
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

#include "graphics/particle_kind.hpp"

#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/irr_driver.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "utils/constants.hpp"

#include <stdexcept>

ParticleKind::ParticleKind(const std::string file) : m_min_start_color(255,255,255,255),
    m_max_start_color(255,255,255,255), m_name(file)
{
    // ---- Initial values to prevent readin uninitialized values
    m_max_size       = 0.5f;
    m_min_size       = 0.5f;
    m_shape          = EMITTER_POINT;
    m_min_rate       = 10;
    m_max_rate       = 10;
    m_lifetime_min   = 400;
    m_lifetime_max   = 400;
    m_fadeout_time   = 400;
    m_box_x          = 0.5f;
    m_box_y          = 0.5f;
    m_box_z          = 0.5f;
    m_angle_spread   = 45;
    m_velocity_x     = 0.001f;
    m_velocity_y     = 0.001f;
    m_velocity_z     = 0.001f;
    m_gravity_strength = 0.0f;
    m_fade_away_start  = -1.0f;
    m_fade_away_end    = -1.0f;
    m_force_lost_to_gravity_time = 1000;
    m_emission_decay_rate = 0;
    
    
    // ----- Read XML file
    
    //std::cout << "==== READING " << file << " ====\n";
    
    XMLNode* xml = file_manager->createXMLTree(file);
    
    if (xml == NULL)
    {
        throw std::runtime_error("[ParticleKind] Cannot find file " + file);
    }
    
    if (xml->getName() != "particles")
    {
        delete xml;
        throw std::runtime_error("[ParticleKind] No <particles> main node in " + file);
    }
    
    // ------------------------------------------------------------------------
    
    std::string emitterShape = "point";
    xml->get("emitter", &emitterShape);
    
    if (emitterShape == "point")
    {
        m_shape = EMITTER_POINT;
    }
    else if (emitterShape == "box")
    {
        m_shape = EMITTER_BOX;
        
        xml->get("box_x", &m_box_x);
        xml->get("box_y", &m_box_y);
        xml->get("box_z", &m_box_z);
    }
    else
    {
        fprintf(stderr, "[ParticleKind] <particles> main node has unknown value for attribute 'emitter'\n");
        m_shape = EMITTER_POINT;
    }
    
    // ------------------------------------------------------------------------
    
    const XMLNode* spreading = xml->getNode("spreading");
    spreading->get("angle", &m_angle_spread);
    
    //std::cout << "m_spread_factor = " << m_spread_factor << "\n";
    
    // ------------------------------------------------------------------------

    const XMLNode* velocity = xml->getNode("velocity");
    velocity->get("x", &m_velocity_x);
    velocity->get("y", &m_velocity_y);
    velocity->get("z", &m_velocity_z);
    
    // ------------------------------------------------------------------------
    // old deperecated way
    const XMLNode* material = xml->getNode("material");
    if (material != NULL)
    {
        material->get("file", &m_material_file);
        
        if (m_material_file.size() == 0)
        {
            delete xml;
            throw std::runtime_error("[ParticleKind] <material> tag has invalid 'file' attribute");
        }
    }
    
    // ------------------------------------------------------------------------
    
    const XMLNode* rate = xml->getNode("rate");
    if (rate != NULL)
    {
        rate->get("min", &m_min_rate);
        rate->get("max", &m_max_rate);
        rate->get("decay_rate", &m_emission_decay_rate);
    }
    
    //std::cout << "m_min_rate = " << m_min_rate << "\n";
    //std::cout << "m_max_rate = " << m_max_rate << "\n";

    // ------------------------------------------------------------------------
    
    const XMLNode* lifetime = xml->getNode("lifetime");
    if (lifetime != NULL)
    {
        lifetime->get("min", &m_lifetime_min);
        lifetime->get("max", &m_lifetime_max);
    }
    
    //std::cout << "m_lifetime_min = " << m_lifetime_min << "\n";
    //std::cout << "m_lifetime_max = " << m_lifetime_max << "\n";
    
    // ------------------------------------------------------------------------
    
    const XMLNode* size = xml->getNode("size");
    if (size != NULL)
    {
        size->get("min", &m_min_size);
        size->get("max", &m_max_size);
    }
    
    //std::cout << "m_particle_size = " << m_particle_size << "\n";
    //std::cout << "m_min_size = " << m_min_size << "\n";
    //std::cout << "m_max_size = " << m_max_size << "\n";
    
    // ------------------------------------------------------------------------
    
    const XMLNode* color = xml->getNode("color");
    if (color != NULL)
    {
        color->get("min", &m_min_start_color);
        color->get("max", &m_max_start_color);
    }
    
    // ------------------------------------------------------------------------
    
    const XMLNode* fadeout = xml->getNode("fadeout");
    if (fadeout != NULL)
    {
        fadeout->get("time", &m_fadeout_time);
    }
        
    //std::cout << "m_fadeout_time = " << m_fadeout_time << "\n";
    
    // ------------------------------------------------------------------------
    
    const XMLNode* gravity = xml->getNode("gravity");
    if (gravity != NULL)
    {
        gravity->get("strength",        &m_gravity_strength);
        gravity->get("only-force-time", &m_force_lost_to_gravity_time);
    }
        
    // ------------------------------------------------------------------------
    
    const XMLNode* fadeaway = xml->getNode("fade-away");
    if (fadeaway != NULL)
    {
        fadeaway->get("start", &m_fade_away_start);
        fadeaway->get("end",   &m_fade_away_end);
    }
    
    
    // ------------------------------------------------------------------------
    
    const XMLNode* materials = xml->getNode("materials");
    if (materials != NULL)
    {
        material_manager->pushTempMaterial(materials, file);
        m_material_file = material_manager->getLatestMaterial()->getTexFname();
    }
    
    
    // ------------------------------------------------------------------------
    
    delete xml;
}

// ----------------------------------------------------------------------------

Material* ParticleKind::getMaterial() const
{
    if (material_manager->hasMaterial(m_material_file))
    {
        Material* material = material_manager->getMaterial(m_material_file);
        if (material->getTexture() == NULL)
        {
            throw std::runtime_error("[ParticleKind] Cannot locate file " + m_material_file);
        }
        return material;
    }
    else
    {
        fprintf(stderr, "[ParticleKind] WARNING: particle image '%s' does not appear in the list of "
                "currently known materials\n", m_material_file.c_str());
        return NULL;
    }
}
