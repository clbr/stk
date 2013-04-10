//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009 Joerg Henrichs
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
//  Foundation, Inc., 59 Temple Place - Suite 330, B

#include "tracks/quad_graph.hpp"

#include "config/user_config.hpp"
#include "graphics/irr_driver.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "tracks/quad_graph.hpp"
#include "tracks/quad_set.hpp"

// ----------------------------------------------------------------------------
/** Constructor. Saves the quad index which belongs to this graph node.
 *  \param index Index of the quad to use for this node (in QuadSet).
 */
GraphNode::GraphNode(unsigned int quad_index, unsigned int node_index) 
{
    if (quad_index >= QuadSet::get()->getNumberOfQuads())
    {
        fprintf(stderr, "[GraphNode] ERROR: No driveline found, or empty driveline");
        abort();
    }
    m_quad_index          = quad_index;
    m_node_index          = node_index;
    m_distance_from_start = -1.0f;

    const Quad &quad      = QuadSet::get()->getQuad(m_quad_index);
    // The following values should depend on the actual orientation 
    // of the quad. ATM we always assume that indices 0,1 are the lower end,
    // and 2,3 are the upper end (or the reverse if reverse mode is selected).
    // The width is the average width at the beginning and at the end.
    m_right_unit_vector = ( quad[0]-quad[1]
                           +quad[3]-quad[2]) * 0.5f;
    m_right_unit_vector.normalize();

    m_width = (  (quad[1]-quad[0]).length() 
               + (quad[3]-quad[2]).length() ) * 0.5f;

    if(QuadGraph::get()->isReverse())
    {
        m_lower_center = (quad[2]+quad[3]) * 0.5f;
        m_upper_center = (quad[0]+quad[1]) * 0.5f;
        m_right_unit_vector *= -1.0f; 
    }
    else
    {
        m_lower_center = (quad[0]+quad[1]) * 0.5f;
        m_upper_center = (quad[2]+quad[3]) * 0.5f;
    }
    m_line     = core::line2df(m_upper_center.getX(), m_upper_center.getZ(),
                               m_lower_center.getX(), m_lower_center.getZ() );
    // Only this 2d point is needed later
    m_lower_center_2d = core::vector2df(m_lower_center.getX(), 
                                        m_lower_center.getZ() );

}   // GraphNode

// ----------------------------------------------------------------------------
/** Adds a successor to a node. This function will also pre-compute certain
 *  values (like distance from this node to the successor, angle (in world)
 *  between this node and the successor.
 *  \param to The index of the graph node of the successor.
 */
void GraphNode::addSuccessor(unsigned int to)
{
    m_successor_nodes.push_back(to);
    // m_quad_index is the quad index
    const Quad &this_quad = QuadSet::get()->getQuad(m_quad_index);
    // to is the graph node
    GraphNode &gn = QuadGraph::get()->getNode(to);
    const Quad &next_quad = QuadGraph::get()->getQuadOfNode(to);

    // Note that the first predecessor is (because of the way the quad graph
    // is exported) the most 'natural' one, i.e. the one on the main 
    // driveline. 
    gn.m_predecessor_nodes.push_back(m_node_index);

    Vec3 d = m_lower_center - QuadGraph::get()->getNode(to).m_lower_center;
    m_distance_to_next.push_back(d.length());

    Vec3 diff = next_quad.getCenter() - this_quad.getCenter();
    m_angle_to_next.push_back(atan2(diff.getX(), diff.getZ()));

}   // addSuccessor

// ----------------------------------------------------------------------------
/** If this node has more than one successor, it will set up a vector that
 *  contains the direction to use when a certain graph node X should be 
 *  reached.
 */
void GraphNode::setupPathsToNode()
{
    if(m_successor_nodes.size()<2) return;

    const unsigned int num_nodes = QuadGraph::get()->getNumNodes();
    m_path_to_node.resize(num_nodes);

    // Initialise each graph node with -1, indicating that 
    // it hasn't been reached yet.
    for(unsigned int i=0; i<num_nodes; i++)
        m_path_to_node[i] = -1;

    // Indicate that this node can be reached from this node by following
    // successor 0 - just a dummy value that might only be used during the
    // recursion below.
    m_path_to_node[m_node_index] = 0;

    // A simple depth first search is used to determine which successor to 
    // use to reach a certain graph node. Using Dijkstra's algorithm  would
    // give the shortest way to reach a certain node, but the shortest way
    // might involve some shortcuts which are hidden, and should therefore 
    // not be used.
    for(unsigned int i=0; i<getNumberOfSuccessors(); i++)
    {
        GraphNode &gn = QuadGraph::get()->getNode(getSuccessor(i));
        gn.markAllSuccessorsToUse(i, &m_path_to_node);
    }
#ifdef DEBUG
    for(unsigned int i=0; i<m_path_to_node.size(); i++)
    {
        if(m_path_to_node[i]==-1)
            printf("[WARNING] No path to node %d found on graph node %d.\n",
                   i, m_node_index);
    }
#endif
}   // setupPathsToNode

// ----------------------------------------------------------------------------
/** This function marks that the successor n should be used to reach this
 *  node. It then recursively (depth first) does the same for all its 
 *  successors. Depth-first 
 *  \param n The successor which should be used in m_path_node to reach 
 *         this node.
 *  \param path_to_node The path-to-node data structure of the node for
 *         which the paths are currently determined.
 */
void GraphNode::markAllSuccessorsToUse(unsigned int n, 
                                       PathToNodeVector *path_to_node)
{
    // End recursion if the path to this node has already been found.
    if( (*path_to_node)[m_node_index] >-1) return;

    (*path_to_node)[m_node_index] = n;
    for(unsigned int i=0; i<getNumberOfSuccessors(); i++)
    {
        GraphNode &gn = QuadGraph::get()->getNode(getSuccessor(i));
        gn.markAllSuccessorsToUse(n, path_to_node);
    }
}   // markAllSuccesorsToUse

// ----------------------------------------------------------------------------
void GraphNode::setDirectionData(unsigned int successor, DirectionType dir, 
                                 unsigned int last_node_index)
{
    if(m_direction.size()<successor+1)
    {
        m_direction.resize(successor+1);
        m_last_index_same_direction.resize(successor+1);
    }
    m_direction[successor]                 = dir;
    m_last_index_same_direction[successor] = last_node_index;
}   // setDirectionData

// ----------------------------------------------------------------------------
/** Returns the distance a point has from this quad in forward and sidewards
 *  direction, i.e. how far forwards the point is from the beginning of the 
 *  quad, and how far to the side from the line connecting the center points
 *  is it. All these computations are done in 2D only.
 *  \param xyz The coordinates of the point.
 *  \param result The X coordinate contains the sidewards distance, the
 *                Z coordinate the forward distance.
 */
void GraphNode::getDistances(const Vec3 &xyz, Vec3 *result)
{
    core::vector2df xyz2d(xyz.getX(), xyz.getZ());
    core::vector2df closest = m_line.getClosestPoint(xyz2d);
    if(m_line.getPointOrientation(xyz2d)>0)
        result->setX( (closest-xyz2d).getLength());   // to the right
    else
        result->setX(-(closest-xyz2d).getLength());   // to the left
    result->setZ( m_distance_from_start + 
                  (closest-m_lower_center_2d).getLength());
}   // getDistances

// ----------------------------------------------------------------------------
/** Returns the square of the distance between the given point and any point
 *  on the 'centre' line, i.e. the finite line from the middle point of the
 *  lower end of the quad node to the middle point of the upper end of the
 *  quad which belongs to this graph node. The value is computed in 2d only!
 *  \param xyz The point for which the distance to the line is computed.
 */
float GraphNode::getDistance2FromPoint(const Vec3 &xyz)
{
    core::vector2df xyz2d(xyz.getX(), xyz.getZ());
    core::vector2df closest = m_line.getClosestPoint(xyz2d);
    return (closest-xyz2d).getLengthSQ();
}   // getDistance2FromPoint

// ----------------------------------------------------------------------------

void GraphNode::setChecklineRequirements(int latest_checkline)
{
    m_checkline_requirements.push_back(latest_checkline);
}
