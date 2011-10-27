// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <set>

#include <boost/algorithm/string/erase.hpp>
#include <boost/tuple/tuple.hpp>

#include "common/Foreach.hpp"
#include "common/Log.hpp"
#include "common/Builder.hpp"
#include "common/FindComponents.hpp"
#include "common/OptionT.hpp"
#include "common/OptionArray.hpp"
#include "common/OptionComponent.hpp"
#include "common/Link.hpp"

#include "common/PE/Comm.hpp"
#include "common/PE/debug.hpp"

#include "math/Consts.hpp"
#include "mesh/Octtree.hpp"
#include "mesh/Mesh.hpp"
#include "common/Table.hpp"
#include "mesh/Region.hpp"
#include "mesh/Elements.hpp"
#include "mesh/Field.hpp"
#include "mesh/ElementType.hpp"
#include "mesh/ElementData.hpp"
#include "mesh/SpaceFields.hpp"
#include "mesh/Space.hpp"

//////////////////////////////////////////////////////////////////////////////

namespace cf3 {
namespace mesh {

  using namespace common;
  using namespace common::PE;
  using namespace math::Consts;

////////////////////////////////////////////////////////////////////////////////

cf3::common::ComponentBuilder < Octtree, Component, LibMesh > Octtree_Builder;

//////////////////////////////////////////////////////////////////////////////

Octtree::Octtree( const std::string& name )
  : Component(name), m_dim(0), m_bounding(2), m_N(3), m_D(3), m_octtree_idx(3)
{

  m_options.add_option(OptionComponent<Mesh>::create("mesh", &m_mesh))
      ->description("Mesh to create octtree from")
      ->pretty_name("Mesh")
      ->mark_basic();

  m_options.add_option< OptionT<Uint> >( "nb_elems_per_cell", 1 )
      ->description("The approximate amount of elements that are stored in a structured cell of the octtree")
      ->pretty_name("Number of Elements per Octtree Cell");

  std::vector<Uint> dummy;
  m_options.add_option< OptionArrayT<Uint> >( "nb_cells", dummy)
      ->description("The number of cells in each direction of the comb. "
                        "Takes precedence over \"Number of Elements per Octtree Cell\". ")
      ->pretty_name("Number of Cells");

  m_elements = create_component_ptr<UnifiedData>("elements");

}

//////////////////////////////////////////////////////////////////////

void Octtree::create_bounding_box()
{
  m_dim=0;

  if (m_mesh.expired())
    throw SetupError(FromHere(), "Option \"mesh\" has not been configured");

  m_dim = m_mesh.lock()->geometry_fields().coordinates().row_size();

  // find bounding box coordinates for region 1 and region 2
  m_bounding[MIN].setConstant(real_max());
  m_bounding[MAX].setConstant(real_min());

  boost_foreach(common::Table<Real>::ConstRow coords, m_mesh.lock()->geometry_fields().coordinates().array())
  {
    for (Uint d=0; d<m_dim; ++d)
    {
      m_bounding[MIN][d] = std::min(m_bounding[MIN][d],  coords[d]);
      m_bounding[MAX][d] = std::max(m_bounding[MAX][d],  coords[d]);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void Octtree::create_octtree()
{
  create_bounding_box();
  if (m_mesh.expired())
    throw SetupError(FromHere(), "Option \"mesh\" has not been configured");

  std::vector<Real> L(3);

  Real V=1;
  for (Uint d=0; d<m_dim; ++d)
  {
    L[d] = m_bounding[MAX][d] - m_bounding[MIN][d];
    V*=L[d];
  }

  const Uint nb_elems = m_mesh.lock()->topology().recursive_filtered_elements_count(IsElementsVolume());

  if (option("nb_cells").value<std::vector<Uint> >().size() > 0)
  {
    m_N = option("nb_cells").value<std::vector<Uint> >();
    for (Uint d=0; d<m_dim; ++d)
      m_D[d] = (L[d])/static_cast<Real>(m_N[d]);
  }
  else
  {
    Real V1 = V/nb_elems;
    Real D1 = std::pow(V1,1./m_dim)*option("nb_elems_per_cell").value<Uint>();

    for (Uint d=0; d<m_dim; ++d)
    {
      m_N[d] = (Uint) std::ceil(L[d]/D1);
      m_D[d] = (L[d])/static_cast<Real>(m_N[d]);
    }
  }

  CFinfo << "Octtree:" << CFendl;
  CFinfo << "--------" << CFendl;
  for (Uint d=0; d<m_dim; ++d)
  {
    std::cout<< PERank << "range["<<d<<"] :   L = " << L[d] << "    N = " << m_N[d] << "    D = " << m_D[d] << "    min = " << m_bounding[MIN][d] << "    max = " << m_bounding[MAX][d] << std::endl;
  }
  CFinfo << "V = " << V << CFendl;

  // initialize the honeycomb
  m_octtree.resize(boost::extents[std::max(Uint(1),m_N[XX])][std::max(Uint(1),m_N[YY])][std::max(Uint(1),m_N[ZZ])]);

  boost_foreach (const Elements& elements, find_components_recursively_with_filter<Elements>(*m_mesh.lock(),IsElementsVolume()))
    m_elements->add(elements);

  Uint unif_elem_idx=0;
  RealVector centroid(m_dim);
  std::vector<Uint> octtree_idx(3);
  boost_foreach (const Elements& elements, find_components_recursively_with_filter<Elements>(*m_mesh.lock(),IsElementsVolume()))
  {
    Uint nb_nodes_per_element = elements.node_connectivity().row_size();
    RealMatrix coordinates(nb_nodes_per_element,m_dim);

    for (Uint elem_idx=0; elem_idx<elements.size(); ++elem_idx)
    {
      elements.put_coordinates(coordinates,elem_idx);
      elements.element_type().compute_centroid(coordinates,centroid);
      for (Uint d=0; d<m_dim; ++d)
        octtree_idx[d]=std::min((Uint) std::floor( (centroid[d] - m_bounding[MIN][d])/m_D[d]), m_N[d]-1 );
      m_octtree[octtree_idx[XX]][octtree_idx[YY]][octtree_idx[ZZ]].push_back(unif_elem_idx);
      ++unif_elem_idx;
    }
  }


  // Uint total=0;
  //
  // switch (m_dim)
  // {
  //   case DIM_2D:
  //     for (Uint i=0; i<m_N[0]; ++i)
  //       for (Uint j=0; j<m_N[1]; ++j)
  //       {
  //         Uint k=0;
  //         // CFinfo << "("<<i<<","<<j<<") has elems ";
  //         // if (m_octtree[i][j][k].size())
  //         //   CFinfo << mesh::to_vector(m_octtree[i][j][k]).transpose() << CFendl;
  //         // else
  //         //   CFinfo << CFendl;
  //         total += m_octtree[i][j][k].size();
  //       }
  //     break;
  //   case DIM_3D:
  //     for (Uint i=0; i<m_N[0]; ++i)
  //       for (Uint j=0; j<m_N[1]; ++j)
  //         for (Uint k=0; k<m_N[2]; ++k)
  //         {
  //           // CFinfo << "("<<i<<","<<j<<","<<k<<") has elems ";
  //           // if (m_octtree[i][j][k].size())
  //           //   CFinfo << mesh::to_vector(m_octtree[i][j][k]).transpose() << CFendl;
  //           // else
  //           //   CFinfo << CFendl;
  //           total += m_octtree[i][j][k].size();
  //         }
  //     break;
  //   default:
  //     break;
  // }
  //
  // CFinfo << "total = " << total << " of " << m_nb_elems << CFendl;
}


//////////////////////////////////////////////////////////////////////////////

void Octtree::find_cell_ranks( const boost::multi_array<Real,2>& coordinates, std::vector<Uint>& ranks )
{
  ranks.resize(coordinates.size());

  Elements::ConstPtr element_component;
  Uint element_idx;
  std::deque<Uint> missing_cells;

  RealVector dummy(m_dim);

  for(Uint i=0; i<coordinates.size(); ++i)
  {
    for (Uint d=0; d<m_dim; ++d)
      dummy[d] = coordinates[i][d];
    if( find_element(dummy,element_component,element_idx) )
    {
      ranks[i] = Comm::instance().rank();
    }
    else
    {
      ranks[i] = math::Consts::uint_max();
      missing_cells.push_back(i);
    }
  }

  std::vector<Real> send_coords(m_dim*missing_cells.size());
  std::vector<Real> recv_coords;

  Uint c(0);
  boost_foreach(const Uint i, missing_cells)
  {
    for(Uint d=0; d<m_dim; ++d)
      send_coords[c++]=coordinates[i][d];
  }

  for (Uint root=0; root<PE::Comm::instance().size(); ++root)
  {

    recv_coords.resize(0);
    PE::Comm::instance().broadcast(send_coords,recv_coords,root,m_dim);

    // size is only because it doesn't get resized for this rank
    std::vector<Uint> send_found(missing_cells.size(),math::Consts::uint_max());

    if (root!=Comm::instance().rank())
    {
      std::vector<RealVector> recv_coordinates(recv_coords.size()/m_dim) ;
      boost_foreach(RealVector& realvec, recv_coordinates)
          realvec.resize(m_dim);

      c=0;
      for (Uint i=0; i<recv_coordinates.size(); ++i)
      {
        for(Uint d=0; d<m_dim; ++d)
          recv_coordinates[i][d]=recv_coords[c++];
      }

      send_found.resize(recv_coordinates.size());
      for (Uint i=0; i<recv_coordinates.size(); ++i)
      {
        if( find_element(recv_coordinates[i],element_component,element_idx) )
        {
          send_found[i] = Comm::instance().rank();
        }
        else
          send_found[i] = math::Consts::uint_max();
      }
    }

    std::vector<Uint> recv_found(missing_cells.size()*Comm::instance().size());
    PE::Comm::instance().gather(send_found,recv_found,root);

    if( root==Comm::instance().rank())
    {
      const Uint stride = missing_cells.size();
      for (Uint i=0; i<missing_cells.size(); ++i)
      {
        for(Uint p=0; p<Comm::instance().size(); ++p)
        {
          ranks[missing_cells[i]] = std::min(recv_found[i+p*stride] , ranks[missing_cells[i]]);
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

bool Octtree::find_octtree_cell(const RealVector& coordinate, std::vector<Uint>& octtree_idx)
{
  //CFinfo << "point " << coordinate << CFflush;
  cf3_assert(coordinate.size() == static_cast<int>(m_dim));

  for (Uint d=0; d<m_dim; ++d)
  {
    if ( (coordinate[d] > m_bounding[MAX][d]) ||
         (coordinate[d] < m_bounding[MIN][d]) )
    {
      return false; // no index found
    }
    octtree_idx[d] = std::min((Uint) std::floor( (coordinate[d] - m_bounding[MIN][d])/m_D[d]), m_N[d]-1 );
  }

  //CFinfo << " should be in box ("<<m_point_idx[0]<<","<<m_point_idx[1]<<","<<m_point_idx[2]<<")" << CFendl;
  return true;
}

//////////////////////////////////////////////////////////////////////////////

void Octtree::gather_elements_around_idx(const std::vector<Uint>& octtree_idx, const Uint ring, std::vector<Uint>& unified_elems)
{
  int i(0), j(0), k(0);
  int imin, jmin, kmin;
  int imax, jmax, kmax;

  int irmin, jrmin, krmin;
  int irmax, jrmax, krmax;

  if (ring == 0)
  {
    boost_foreach(const Uint unif_elem_idx, m_octtree[octtree_idx[XX]][octtree_idx[YY]][octtree_idx[ZZ]])
      unified_elems.push_back(unif_elem_idx);
    return;
  }
  else
  {
    switch (m_dim)
    {
      case DIM_3D:
      irmin = int(octtree_idx[XX])-int(ring);  irmax = int(octtree_idx[XX])+int(ring);
      jrmin = int(octtree_idx[YY])-int(ring);  jrmax = int(octtree_idx[YY])+int(ring);
      krmin = int(octtree_idx[ZZ])-int(ring);  krmax = int(octtree_idx[ZZ])+int(ring);

      imin = std::max(irmin, 0);  imax = std::min(irmax,int(m_N[XX])-1);
      jmin = std::max(jrmin, 0);  jmax = std::min(jrmax,int(m_N[YY])-1);
      kmin = std::max(krmin, 0);  kmax = std::min(krmax,int(m_N[ZZ])-1);

        // imin:
        i = imin;
        for (i = imin; i <= imax; ++i)
        {
          for (j = jmin; j <= jmax; ++j)
          {
            for (k = kmin; k <= kmax; ++k)
            {
              if ( i == irmin || i == irmax || j == jrmin || j == jrmax || k == krmin || k == krmax)
              {
                boost_foreach(const Uint unif_elem_idx, m_octtree[i][j][k])
                  unified_elems.push_back(unif_elem_idx);
              }
            }
          }
        }

        break;
      case DIM_2D:

        irmin = int(octtree_idx[XX])-int(ring);  irmax = int(octtree_idx[XX])+int(ring);
        jrmin = int(octtree_idx[YY])-int(ring);  jrmax = int(octtree_idx[YY])+int(ring);

        imin = std::max(irmin, 0);  imax = std::min(irmax,int(m_N[XX])-1);
        jmin = std::max(jrmin, 0);  jmax = std::min(jrmax,int(m_N[YY])-1);

        for (i = imin; i <= imax; ++i)
        {
          for (j = jmin; j <= jmax; ++j)
          {
            if ( i == irmin || i == irmax || j == jrmin || j == jrmax )
            {
              boost_foreach(const Uint unif_elem_idx, m_octtree[i][j][k])
                unified_elems.push_back(unif_elem_idx);
            }
          }
        }

        break;

      case DIM_1D:
        irmin = int(octtree_idx[XX])-int(ring);  irmax = int(octtree_idx[XX])+int(ring);
        imin = std::max(int(octtree_idx[XX])-int(ring), 0);  imax = std::min(int(octtree_idx[XX])+int(ring),int(m_N[XX])-1);

        for (i = imin; i <= imax; ++i)
        {
          if ( i == irmin || i == irmax)
          {
            boost_foreach(const Uint unif_elem_idx, m_octtree[i][j][k])
              unified_elems.push_back(unif_elem_idx);
          }
        }


        break;
    }
  }


  //CFinfo << m_pointcloud.size() << " points in the pointcloud " << CFendl;

}

//////////////////////////////////////////////////////////////////////////////

boost::tuple<Elements::ConstPtr,Uint> Octtree::find_element(const RealVector& target_coord)
{
  Elements::ConstPtr element_component;
  Uint element_idx;
  find_element(target_coord,element_component,element_idx);
  return boost::make_tuple(element_component, element_idx);
}


////////////////////////////////////////////////////////////////////////////////

bool Octtree::find_element(const RealVector& target_coord, Elements::ConstPtr& element_component, Uint& element_idx)
{
  if (m_octtree.num_elements() == 0)
    create_octtree();
  if (find_octtree_cell(target_coord,m_octtree_idx))
  {
    std::vector<Uint> unified_elements(0); unified_elements.reserve(16);

    Component::ConstPtr component;
    Uint elem_idx;

    gather_elements_around_idx(m_octtree_idx,0,unified_elements);

    boost_foreach(const Uint unif_elem_idx, unified_elements)
    {
      boost::tie(component,elem_idx)=m_elements->location(unif_elem_idx);
      const Elements& elements = component->as_type<Elements>();
      const RealMatrix elem_coordinates = elements.get_coordinates(elem_idx);
      if (elements.element_type().is_coord_in_element(target_coord,elem_coordinates))
      {
        element_component = elements.as_ptr<Elements>();
        element_idx = elem_idx;
        cf3_assert(is_not_null(element_component));
        return true;
      }
    }

    // if arrived here, it means no element has been found. Enlarge the search with one more ring, for possible misses.
    unified_elements.resize(0); unified_elements.reserve(16);
    gather_elements_around_idx(m_octtree_idx,1,unified_elements);

    boost_foreach(const Uint unif_elem_idx, unified_elements)
    {
      boost::tie(component,elem_idx)=m_elements->location(unif_elem_idx);
      const Elements& elements = component->as_type<Elements>();
      const RealMatrix elem_coordinates = elements.get_coordinates(elem_idx);
      if (elements.element_type().is_coord_in_element(target_coord,elem_coordinates))
      {
        element_component = elements.as_ptr<Elements>();
        element_idx = elem_idx;
        cf3_assert(is_not_null(element_component));
        return true;
      }
    }

  }
  // if arrived here, it means no element has been found. Give up.
  element_component.reset();
  cf3_assert(is_null(element_component));
  return false;
}

////////////////////////////////////////////////////////////////////////////////

} // mesh
} // cf3
