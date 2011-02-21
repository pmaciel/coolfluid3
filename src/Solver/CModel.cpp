// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "boost/filesystem.hpp"

#include "Common/Core.hpp"
#include "Common/OptionT.hpp"
#include "Common/OptionURI.hpp"
#include "Solver/CModel.hpp"

#include "Mesh/CDomain.hpp"

namespace CF {
namespace Solver {

using namespace Common;
using namespace Mesh;

////////////////////////////////////////////////////////////////////////////////

CModel::CModel( const std::string& name  ) :
  Component ( name )
{
   mark_basic();

   // options

   std::string cwd = boost::filesystem::current_path().string();

   m_properties.add_option< OptionURI >("WorkingDir", "Your working directory", URI( cwd ) )
       ->mark_basic();
   m_properties.add_option< OptionURI >("ResultsDir", "Directory to store the output files", URI( cwd ) )
       ->mark_basic();
   m_properties.add_option< OptionT<Uint> >("CPUs", "Number of cpus to use in simulation", 1u )
       ->mark_basic();

   // properties

   properties()["steady"] = bool(true);

   // signals

   regist_signal ( "simulate" , "Simulates this model", "Simulate" )
       ->connect ( boost::bind ( &CModel::signal_simulate, this, _1 ) );
}

CModel::~CModel() {}

////////////////////////////////////////////////////////////////////////////////

CDomain::Ptr CModel::create_domain( const std::string& name )
{
  return this->create_component<CDomain>( name );
}

////////////////////////////////////////////////////////////////////////////////

void CModel::signature_create_domain ( Common::XmlNode& node )
{
  XmlParams p(node);
  p.add_option<URI>("File", URI(), "Location of the file holding the mesh" );
}

////////////////////////////////////////////////////////////////////////////////

void CModel::signal_create_domain ( Common::XmlNode& node )
{
  XmlParams p ( node );

  CDomain::Ptr domain = this->create_domain("Domain"); // dispatch to virtual function

  URI file = p.get_option<URI>("File");
  if (!file.empty())
    domain->signal_load_mesh( node );
}

////////////////////////////////////////////////////////////////////////////////

void CModel::signal_simulate ( Common::XmlNode& node )
{
  // XmlParams p ( node );

  this->simulate(); // dispatch tos virtual function
}

////////////////////////////////////////////////////////////////////////////////

} // Solver
} // CF
