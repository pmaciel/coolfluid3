// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_Mesh_CNodeElementConnectivity_hpp
#define CF_Mesh_CNodeElementConnectivity_hpp

#include "Mesh/CElements.hpp"
#include "Mesh/CUnifiedData.hpp"
#include "Mesh/CDynTable.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace Common {
  class CLink;
}
namespace Mesh {
  
  class CRegion;
  class CNodes;

////////////////////////////////////////////////////////////////////////////////

/// Stores connectivity data between nodes and their adjacent elements
/// and provides a convenient API to access the data
class Mesh_API CNodeElementConnectivity : public Common::Component
{
public:

  typedef boost::shared_ptr<CNodeElementConnectivity> Ptr;
  typedef boost::shared_ptr<CNodeElementConnectivity const> ConstPtr;

  /// Contructor
  /// @param name of the component
  CNodeElementConnectivity ( const std::string& name );

  /// Virtual destructor
  virtual ~CNodeElementConnectivity() {}

  /// Get the class name
  static std::string type_name () { return "CNodeElementConnectivity"; }

  /// setup the node to element connectivity
  /// This function calls 
  /// - set_elements(elements_range)
  /// - build_connectivity
  /// They could be called seperately if wanted
  /// @post all access functions can be used after setup
  /// @param [in] regions in which the elements are connected to the nodes.
  void setup(CRegion& region);

  /// set the element for the node to element connectivity
  /// Elements have a continuous index spanning all element components
  /// stored in a CUnifiedData<CElements> component
  /// @param [in] elements_range the elements range to see if they are connected to the nodes.
  ///                            Can be made using "find_components_recursively<CElements>()" function
  template<typename ElementsRangeT>
      void add_elements( const ElementsRangeT& range)
  {
    m_elements->add_data(range);
    cf_assert(m_elements->size());
    set_nodes(m_elements->data_components()[0]->nodes()); 
  }
  
  /// Build the connectivity table
  /// Build the connectivity table as a CDynTable<Uint>
  /// @pre set_nodes() and set_elements() must have been called
  void build_connectivity();

  /// Find the elements connected to a given node by its index
  /// The return type is CDynTable<Uint>::ConstRow which (or "std::vector<Uint> const&")
  /// @return continuous indices of the elments
  CDynTable<Uint>::ConstRow elements(const Uint node_index) const;

  /// Find the element location given a unified element index
  /// @return boost::tuple<CElements::Ptr,Uint>(elem_component,elem_idx)
  CUnifiedData<CElements>::data_location_type element_location(const Uint unified_elem_idx);

  /// Find the element location given a unified element index
  /// @return boost::tuple<CElements::ConstPtr,Uint>(elem_component,elem_idx)  
  CUnifiedData<CElements>::const_data_location_type element_location(const Uint unified_elem_idx) const;
  
  /// const access to the node to element connectivity table in unified indices
  const CDynTable<Uint>& connectivity() const { return *m_connectivity; }

private: //functions

  /// set the nodes for the node to element connectivity
  /// @param [in] nodes the nodes component to find connected elements of
  void set_nodes(CNodes& nodes);

private: // data

  /// link to the nodes component
  boost::shared_ptr<Common::CLink> m_nodes;

  /// unified view of the elements
  CUnifiedData<CElements>::Ptr m_elements;

  /// Actual connectivity table
  CDynTable<Uint>::Ptr m_connectivity;
  
}; // CNodeElementConnectivity

////////////////////////////////////////////////////////////////////////////////

} // Mesh
} // CF

////////////////////////////////////////////////////////////////////////////////

#endif // CF_Mesh_ConnectivityData_hpp