// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "Test module for CF::RDM::ScalarAdvection"

#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

#include "Common/BoostFilesystem.hpp"

#include "Common/CreateComponent.hpp"
#include "Common/FindComponents.hpp"
#include "Common/Log.hpp"
#include "Common/CLink.hpp"
#include "Common/Foreach.hpp"
#include "Common/LibLoader.hpp"
#include "Common/OSystem.hpp"

#include "Solver/CSolver.hpp"
#include "Solver/CModel.hpp"
#include "Solver/CPhysicalModel.hpp"
#include "Solver/Actions/CLoop.hpp"

#include "Mesh/LoadMesh.hpp"
#include "Mesh/CCells.hpp"
#include "Mesh/CMeshReader.hpp"
#include "Mesh/CMeshWriter.hpp"
#include "Mesh/CDomain.hpp"
#include "Mesh/CRegion.hpp"
#include "Mesh/Actions/CBubbleEnrich.hpp"
#include "Mesh/Actions/CBubbleRemove.hpp"

#include "RDM/RKRD.hpp"
#include "RDM/DomainTerm.hpp"
#include "RDM/ScalarAdvection.hpp"

using namespace CF;
using namespace CF::Common;
using namespace CF::Common::XML;
using namespace CF::Mesh;
using namespace CF::Solver;
using namespace CF::Solver::Actions;
using namespace CF::RDM;

//#define BUBBLE

struct euler2d_global_fixture
{
  euler2d_global_fixture()
  {
    // Load the required libraries (we assume the working dir is the binary path)
    LibLoader& loader = *OSystem::instance().lib_loader();

    const std::vector< boost::filesystem::path > lib_paths = boost::assign::list_of
                                                             ("../../../dso");
    loader.set_search_paths(lib_paths);

    loader.load_library("coolfluid_mesh_neutral");
    loader.load_library("coolfluid_mesh_gmsh");
    loader.load_library("coolfluid_mesh_tecplot");

    euler2d_wizard = allocate_component<ScalarAdvection>("mymodel");

    SignalFrame frame("", "", "");
    SignalFrame& options = frame.map( Protocol::Tags::key_options() );

    options.set_option<std::string>("ModelName","mymodel");
    options.set_option<std::string>("PhysicalModel","Euler2D");

    euler2d_wizard->signal_create_model(frame);
  }

  ScalarAdvection::Ptr euler2d_wizard;

};

struct euler2d_local_fixture
{
  euler2d_local_fixture() :
    model  ( * Core::instance().root()->get_child_ptr("mymodel")->as_ptr<CModel>() ),
    domain ( find_component_recursively<CDomain>(model)  ),
    solver ( find_component_recursively<CSolver>(model) )
  {}

  CModel& model;
  CDomain& domain;
  CSolver& solver;
};


//////////////////////////////////////////////////////////////////////////////

BOOST_GLOBAL_FIXTURE( euler2d_global_fixture )

BOOST_AUTO_TEST_SUITE( euler2d_test_suite )

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( test_check_tree , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  SignalFrame frame("", "", "");
  SignalFrame& options = frame.map( Protocol::Tags::key_options() );

  Core::instance().root()->signal_list_tree(frame);

//  CFinfo << model.tree() << CFendl;
}

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( test_read_mesh , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  // create the xml parameters for the read mesh signal

  SignalFrame frame("", "", "");
  SignalFrame& options = frame.map( Protocol::Tags::key_options() );

  BOOST_CHECK(true);

  std::vector<URI> files;

  URI file( "file:square1x1-tg-p1.msh" );
//  URI file( "file:square1x1-tg-p2.msh" );
//  URI file( "file:square1x1-qd-p1.msh" );
//  URI file( "file:square1x1-qd-p2.msh" );
//  URI file( "file:square1x1-tgqd-p1.msh" );

  options.set_option<URI>("File", file );

  domain.signal_load_mesh( frame );

  BOOST_CHECK_NE( domain.count_children(), (Uint) 0);

#ifdef BUBBLE // enrich the mesh with bubble functions
  CMeshTransformer::Ptr enricher =
      create_component_abstract_type<CMeshTransformer>("CF.Mesh.Actions.CBubbleEnrich","enricher");

  domain.add_component( enricher );

  CMesh::Ptr mesh = find_component_ptr<CMesh>(domain);

  enricher->transform( mesh );
#endif
}

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( test_setup_iterative_solver , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  solver.configure_property("Domain",URI("cpath:../Domain"));
  solver.get_child("time_stepping").configure_property("CFL", 0.1);;
  solver.get_child("time_stepping").configure_property("MaxIter", 1500u);;
}

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( test_create_boundary_term , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  SignalFrame frame("", "", "");
  SignalFrame& options = frame.map( Protocol::Tags::key_options() );

  std::vector<URI> regions;
  boost_foreach( const CRegion& region, find_components_recursively_with_name<CRegion>(domain,"bottom"))
    regions.push_back( region.full_path() );

  BOOST_CHECK_EQUAL( regions.size() , 1u);

  std::string name ("INLET");

  options.set_option<std::string>("Name",name);
  options.set_option<std::string>("Type","CF.RDM.BcDirichlet");
  options.set_array("Regions", regions, " ; ");

  solver.as_ptr<RKRD>()->signal_create_boundary_term(frame);

  Component::Ptr inletbc = find_component_ptr_recursively_with_name( solver, name );
  cf_assert( is_not_null(inletbc) );

  std::vector<std::string> fns(4);

  fns[0] = "if(x>0.5,0.5,1.)";
  fns[1] = "0.0";
  fns[2] = "if(x>0.5,1.67332,2.83972)";
  fns[3] = "if(x>0.5,3.425,6.532)";

//  fns[0] = "1.0";
//  fns[1] = "0.0";
//  fns[2] = "1+cos(x)";
//  fns[4] = "1.0";


  inletbc->configure_property("Functions", fns);

  BOOST_CHECK(true);
}

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( signal_initialize_solution , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  SignalFrame frame("", "", "");
  SignalFrame& options = frame.map( Protocol::Tags::key_options() );

  std::vector<std::string> fns(4);

  fns[0] = "0.5";
  fns[1] = "0.0";
  fns[2] = "1.67332";
  fns[3] = "3.425";


//  fns[0] = "1.0";
//  fns[1] = "0.0";
//  fns[2] = "1+cos(x)";
//  fns[3] = "1.0";

  options.set_array("Functions", fns, " ; ");

  solver.as_type<RKRD>().signal_initialize_solution( frame );
}

////////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( test_init_output , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  CMesh::Ptr mesh = find_component_ptr<CMesh>(domain);

  CMeshWriter::Ptr gmsh_writer = create_component_abstract_type<CMeshWriter> ( "CF.Mesh.Gmsh.CWriter", "GmshWriter" );
  model.add_component(gmsh_writer);

  std::vector<URI> fields;
  boost_foreach(const CField& field, find_components_recursively<CField>(*mesh))
    fields.push_back(field.full_path());

  gmsh_writer->configure_property("Fields",fields);
  gmsh_writer->configure_property("File",model.name()+"_init.msh");
  gmsh_writer->configure_property("Mesh",mesh->full_path());

  gmsh_writer->write();

  model.remove_component("GmshWriter");
}

//////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( solve_lda , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  CFinfo << "solving with LDA scheme" << CFendl;

  // delete previous domain terms
  Component& domain_terms = solver.get_child("compute_domain_terms");
  boost_foreach( RDM::DomainTerm& term, find_components_recursively<RDM::DomainTerm>( domain_terms ))
  {
    const std::string name = term.name();
    domain_terms.remove_component( name );
  }

  BOOST_CHECK( domain_terms.count_children() == 0 );

  CMesh::Ptr mesh = find_component_ptr<CMesh>(domain);

  SignalFrame frame("", "", "");
  SignalFrame& options = frame.map( Protocol::Tags::key_options() );

  std::vector<URI> regions;
  boost_foreach( const CRegion& region, find_components_recursively_with_name<CRegion>(*mesh,"topology"))
    regions.push_back( region.full_path() );

  BOOST_CHECK_EQUAL( regions.size() , 1u);

  options.set_option<std::string>("Name","INTERNAL");
  options.set_option<std::string>("Type","CF.RDM.CSysLDA");
  options.set_array("Regions", regions, " ; ");

  solver.as_ptr<RKRD>()->signal_create_domain_term(frame);

  BOOST_CHECK(true);

  solver.solve();

  BOOST_CHECK(true);

}

////////////////////////////////////////////////////////////////////////////////

BOOST_FIXTURE_TEST_CASE( test_output , euler2d_local_fixture )
{
  BOOST_CHECK(true);

  CMesh::Ptr mesh = find_component_ptr<CMesh>(domain);

#ifdef BUBBLE // remove the bubble functions from the mesh
  CMeshTransformer::Ptr remover =
      create_component_abstract_type<CMeshTransformer>("CF.Mesh.Actions.CBubbleRemove","remover");

  domain.add_component( remover );
  remover->transform( mesh );
#endif

  BOOST_CHECK(true);

  std::vector<URI> fields;
  boost_foreach(const CField& field, find_components_recursively<CField>(*mesh))
    fields.push_back(field.full_path());

  // gmsh writer

  CMeshWriter::Ptr gmsh_writer = create_component_abstract_type<CMeshWriter> ( "CF.Mesh.Gmsh.CWriter", "GmshWriter" );
  model.add_component(gmsh_writer);

  gmsh_writer->configure_property("Fields",fields);
  gmsh_writer->configure_property("File",model.name()+".msh");
  gmsh_writer->configure_property("Mesh",mesh->full_path());

  gmsh_writer->write();

  // tecplot writer

  CMeshWriter::Ptr tec_writer = create_component_abstract_type<CMeshWriter>("CF.Mesh.Tecplot.CWriter","TecWriter");
  model.add_component(tec_writer);

  tec_writer->configure_property("Fields",fields);
  tec_writer->configure_property("File",model.name()+".plt");
  tec_writer->configure_property("Mesh",mesh->full_path());

  tec_writer->write();

}

//////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE_END()

////////////////////////////////////////////////////////////////////////////////

