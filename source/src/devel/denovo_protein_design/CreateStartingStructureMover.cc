// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file devel/DenovoProteinDesign/CreateStartingStructureMover.cc
/// @brief CreateStartingStructureMover methods implemented
/// @author


// Unit Headers
#include <devel/denovo_protein_design/CreateStartingStructureMover.hh>


// Package Headers

// Project Headers
#include <core/pose/Pose.hh>


#include <basic/options/option.hh>
#include <core/chemical/ChemicalManager.hh>
// core::pose::make_pose_from_sequence


#include <core/fragment/ConstantLengthFragSet.hh>

#include <core/import_pose/import_pose.hh>


#include <protocols/abinitio/ClassicAbinitio.hh>
#include <protocols/simple_moves/SmoothFragmentMover.hh>
#include <protocols/simple_moves/GunnCost.hh>


#include <core/scoring/dssp/Dssp.hh>// dssp info

#include <protocols/moves/Mover.fwd.hh> //MoverOP
#include <protocols/simple_moves/BackboneMover.hh> //Small/ShearMover
#include <protocols/moves/MoverContainer.hh> //Sequence Mover
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/moves/RepeatMover.hh>
#include <devel/denovo_protein_design/util.hh>
// Utility Headers
#include <basic/Tracer.hh>
#include <core/types.hh>

// C++ Headers
#include <vector>
#include <iostream>
#include <string>
#include <utility/vector1.hh>

// option key includes

#include <basic/options/keys/DenovoProteinDesign.OptionKeys.gen.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>

#include <core/kinematics/MoveMap.hh>
#include <core/pose/annotated_sequence.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <protocols/moves/TrialMover.hh>
#include <protocols/relax/FastRelax.hh>


using basic::Error;
using basic::Warning;

using namespace core;

namespace devel {
namespace denovo_protein_design {

/// @details
void CreateStartingStructureMover::apply( core::pose::Pose & pose ){

	std::string nucleated_sequence;
	core::pose::Pose template_pose;
	utility::vector1< char > nucleated_sequence_v;
	utility::vector1< char > hydrophobic_polar_sequence_v;
	core::pose::Pose nucleated_pose;
	utility::vector1< char > secstructs;
	utility::vector1< bool > KeepNativeTorsions;
	core::Size numhelix(0);

	do { // this is a huge loop that makes sure the starting structure has 4 helices

		// set up for create from template pdb
		if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::create_from_template_pdb ].user() ) {
			core::import_pose::pose_from_file( template_pose,  basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::create_from_template_pdb ].name() , core::import_pose::PDB_file);

			if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::use_template_topology ].value() == true ) {

				devel::denovo_protein_design::create_nucleated_sequence_from_template_pdb( nucleated_sequence, KeepNativeTorsions );

			}

		}


		// set up for create from secondary structure
		// read secondary structure info from a file or hydrophobic polar pattern from file
		// or read both

		if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::create_from_secondary_structure ].value() == true ) {

			if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::secondary_structure_file ].user() ) {
				std::string ss_filename( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::secondary_structure_file ]().name() );

				devel::denovo_protein_design::read_ss_file_into_vector( ss_filename, secstructs );

				if ( !basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::hydrophobic_polar_pattern ].user() ) {
					devel::denovo_protein_design::create_hydrophobic_polar_pattern_from_secstruct_vector( hydrophobic_polar_sequence_v, secstructs );
				}

			}

			if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::hydrophobic_polar_pattern ].user() ) {
				std::string hp_filename( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::hydrophobic_polar_pattern ]().name() );
				devel::denovo_protein_design::read_hp_file_into_vector( hp_filename,  hydrophobic_polar_sequence_v );
			}

			devel::denovo_protein_design::create_sequence_from_hydrophobic_polar_pattern( nucleated_sequence, hydrophobic_polar_sequence_v );

		}

		// everything above this could be two functions - one for with template pdb and one for without

		core::pose::make_pose_from_sequence(  nucleated_pose, nucleated_sequence,
			*( core::chemical::ChemicalManager::get_instance()->residue_type_set( core::chemical::CENTROID ))
		);


		// put secondary structure info into nucleated pose - need to do this for all methods of creating starting structures
		if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::create_from_template_pdb ].user() ) {
			core::scoring::dssp::Dssp dssp( template_pose );
			dssp.insert_ss_into_pose(template_pose);
			for ( core::Size ii = 1; ii<=template_pose.size(); ++ii ) {
				nucleated_pose.set_secstruct(ii, template_pose.secstruct(ii));
			}
		}

		if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::create_from_secondary_structure ].value() == true ) {
			for ( core::Size ii =1; ii<=secstructs.size(); ++ii ) {
				nucleated_pose.set_secstruct(ii, secstructs[ii]);
			}
		}


		KeepNativeTorsions.resize( nucleated_pose.size(), false);

		// set phi, psi, omega
		if (  basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::secondary_structure_file ].user() ||  basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::use_template_topology ].value() == true ) {
			for ( core::Size seqpos = 1; seqpos <= nucleated_pose.size(); ++seqpos ) {

				if ( nucleated_pose.secstruct(seqpos) == 'H' ) {
					Real helix_phi = rand() % (62 - 58 +1 ) + 58;
					Real helix_psi = rand() % (42 - 38 +1 ) + 38;
					if ( KeepNativeTorsions[seqpos] == true ) {
						nucleated_pose.set_phi   ( seqpos, template_pose.phi(seqpos) );
						nucleated_pose.set_psi   ( seqpos, template_pose.psi(seqpos) );
						nucleated_pose.set_omega ( seqpos, template_pose.omega(seqpos) );
					}
					if ( KeepNativeTorsions[seqpos] == false ) {
						nucleated_pose.set_phi   ( seqpos, -helix_phi );
						nucleated_pose.set_psi   ( seqpos, -helix_psi );
						nucleated_pose.set_omega ( seqpos, 180 );
					}
				}

				if ( nucleated_pose.secstruct(seqpos) == 'L' ) {
					Real loop_phi = 180;
					Real loop_psi = 180;
					if ( KeepNativeTorsions[seqpos] == true ) {
						nucleated_pose.set_phi   ( seqpos, template_pose.phi(seqpos) );
						nucleated_pose.set_psi   ( seqpos, template_pose.psi(seqpos) );
						nucleated_pose.set_omega ( seqpos, template_pose.omega(seqpos) );
					}
					if ( KeepNativeTorsions[seqpos] == false ) {
						nucleated_pose.set_phi   ( seqpos, loop_phi );
						nucleated_pose.set_psi   ( seqpos, loop_psi );
						nucleated_pose.set_omega ( seqpos, 180 );
					}
				}

				if ( nucleated_pose.secstruct(seqpos) == 'E' ) {
					Real sheet_phi = rand() % (130 - 115 + 1) + 115; // I just guessed at these values, maybe a smaller range is better
					Real sheet_psi = rand() % (130 - 115 + 1) + 115;
					if ( KeepNativeTorsions[seqpos] == true ) {
						nucleated_pose.set_phi   ( seqpos, template_pose.phi(seqpos) );
						nucleated_pose.set_psi   ( seqpos, template_pose.psi(seqpos) );
						nucleated_pose.set_omega ( seqpos, template_pose.omega(seqpos) );
					}
					if ( KeepNativeTorsions[seqpos] == false ) {
						nucleated_pose.set_phi( seqpos, sheet_phi );
						nucleated_pose.set_psi( seqpos, -sheet_psi );
						nucleated_pose.set_omega( seqpos, 180 );
					}
				}
			}
		}


		// make a MoveMap
		core::kinematics::MoveMapOP movemap( new core::kinematics::MoveMap );
		if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::secondary_structure_file ].user() ) {
			for ( core::Size ii = 1; ii <= nucleated_pose.size(); ++ii ) {
				if (  nucleated_pose.secstruct(ii) == 'L' || nucleated_pose.secstruct(ii -1 ) == 'L' ||  nucleated_pose.secstruct(ii + 1 )== 'L' ) { movemap->set_bb( ii, true); }
			}
			std::cout << "setting move mab abf" << std::endl;
		} else { movemap->set_bb(true); }


		core::fragment::ConstantLengthFragSetOP fragset3mer( new core::fragment::ConstantLengthFragSet( 3 ) );
		core::fragment::ConstantLengthFragSetOP fragset9mer( new core::fragment::ConstantLengthFragSet( 9 ) );
		fragset3mer->read_fragment_file( basic::options::option [ basic::options::OptionKeys::in::file::frag3 ], 400);
		fragset9mer->read_fragment_file( basic::options::option [ basic::options::OptionKeys::in::file::frag9 ], 200 );


		core::scoring::ScoreFunctionOP centfxn( ( core::scoring::ScoreFunctionFactory::create_score_function( core::scoring::CENTROID_WTS ) ));

		// if we are doing keep native topology, then skip ab initio steps and go into a fragment insertion
		// maybe i should just do this with a superfastrelax or minimize
		if ( basic::options::option[ basic::options::OptionKeys::DenovoProteinDesign::use_template_topology ].value() == true ) {

			int nmoves = 6;
			double mc_temp = 3.0;

			protocols::simple_moves::SmallMoverOP small_mover( new protocols::simple_moves::SmallMover( movemap, mc_temp, nmoves ) );
			small_mover->angle_max( 'H', 10.0 );
			small_mover->angle_max( 'E', 10.0 );
			small_mover->angle_max( 'L', 10.0 );

			protocols::simple_moves::ShearMoverOP shear_mover( new protocols::simple_moves::ShearMover( movemap, mc_temp, nmoves ) );
			shear_mover->angle_max( 'H', 10.0 );
			shear_mover->angle_max( 'E', 10.0 );
			shear_mover->angle_max( 'L', 10.0 );


			protocols::simple_moves::SmoothFragmentMoverOP smooth_frag_mover( new protocols::simple_moves::SmoothFragmentMover( fragset3mer, movemap, utility::pointer::make_shared< protocols::simple_moves::GunnCost >() ) );


			protocols::moves::RandomMoverOP Moveset( new protocols::moves::RandomMover() );
			Moveset->add_mover( small_mover );
			Moveset->add_mover( shear_mover );
			Moveset->add_mover( smooth_frag_mover );

			protocols::moves::MonteCarloOP  MC( new protocols::moves::MonteCarlo( nucleated_pose , *centfxn , mc_temp ) );
			protocols::moves::TrialMoverOP  Perturb( new protocols::moves::TrialMover( Moveset , MC ) );
			protocols::moves::RepeatMoverOP PerturbCycle( new protocols::moves::RepeatMover( Perturb, 100 ) );


			PerturbCycle->apply( nucleated_pose );
			MC->recover_low( nucleated_pose );

		} else {

			protocols::abinitio::ClassicAbinitio::register_options();
			protocols::abinitio::ClassicAbinitioOP prot_ptr;
			prot_ptr = utility::pointer::make_shared< protocols::abinitio::ClassicAbinitio >( fragset3mer, fragset9mer, movemap );
			protocols::abinitio::ClassicAbinitio& abinitio_protocol( *prot_ptr );

			abinitio_protocol.init( nucleated_pose );
			abinitio_protocol.apply( nucleated_pose );

			protocols::relax::FastRelax relax_step( centfxn );
			relax_step.apply( nucleated_pose );


		}

		numhelix = devel::denovo_protein_design::numberhelices( nucleated_pose );

	} while (  numhelix != 4 );

	pose = nucleated_pose;


}//apply

/// @brief
CreateStartingStructureMover::CreateStartingStructureMover(
) : Mover()
{
	Mover::type( "CreateStartingStructureMover" );
}

CreateStartingStructureMover::~CreateStartingStructureMover() = default;

std::string
CreateStartingStructureMover::get_name() const {
	return "CreateStartingStructureMover";
}

}//DenovoProteinDesign
}//devel

