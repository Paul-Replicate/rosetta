// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   test/protocols/match/FoldTreeFromSS.cxxtest.hh
/// @brief
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)


// Test headers
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Utility headers
#include <basic/Tracer.hh>

/// Project headers
#include <core/types.hh>
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
#include <core/scoring/dssp/Dssp.hh>
#include <core/kinematics/FoldTree.hh>

// C++ headers
#include <string>

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>

//Tracer
static basic::Tracer TR( "protocols.bootcamp.FoldTreeFromSS.cxxtest" );

// --------------- Test Class --------------- //

class FoldTreeFromSS : public CxxTest::TestSuite {

public:


	// --------------- Fixtures --------------- //

	// Define a test fixture (some initial state that several tests share)
	// In CxxTest, setUp()/tearDown() are executed around each test case. If you need a fixture on the test
	// suite level, i.e. something that gets constructed once before all the tests in the test suite are run,
	// suites have to be dynamically created. See CxxTest sample directory for example.

	core::pose::Pose 
	fold_tree_from_ss( core::pose::Pose & pose ) {
		// Take pose return FoldTree
		core::scoring::dssp::Dssp DSSP = core::scoring::dssp::Dssp( pose );

		std::string dssp_string = DSSP.get_dssp_secstruct();
		
		core::kinematics::FoldTree ft_from_dssp = fold_tree_from_dssp_string( dssp_string );
		//pose.fold_tree(ft_from_dssp);
		return pose;
	}

	core::kinematics::FoldTree
	fold_tree_from_dssp_string( std::string dssp_string ) {
		// Take string and return FoldTree
		utility::vector1< std::pair< core::Size, core::Size > > ss_bounds;
		ss_bounds = identify_secondary_structure_spans( dssp_string );

		// Construct FoldTree from number of residues only
		core::kinematics::FoldTree ft = core::kinematics::FoldTree();

		core::Size root_res = calculate_ss_center(ss_bounds[1].first, ss_bounds[1].second);
		if (root_res != 1) {
			ft.add_edge( root_res, 1, core::kinematics::Edge::PEPTIDE );
		}
		if (root_res != ss_bounds[1].second) {
			ft.add_edge( root_res, ss_bounds[1].second, core::kinematics::Edge::PEPTIDE );
		}
		
		core::Size jump_counter = 1;
		for (core::Size i=2; i <= ss_bounds.size(); i++) { 

			// loop jump then RL
			core::Size loop_start = ss_bounds[i-1].second + 1;
			core::Size loop_end = ss_bounds[i].first - 1;	
			core::Size loop_mid = calculate_ss_center(loop_start, loop_end);
			ft.add_edge( root_res,  loop_mid, jump_counter);
			++jump_counter;
			if (loop_mid != loop_start) {
				ft.add_edge( loop_mid, loop_start, core::kinematics::Edge::PEPTIDE );
			}
			if (loop_mid != loop_end) {
				ft.add_edge( loop_mid, loop_end, core::kinematics::Edge::PEPTIDE );
			}

			// ss jump then RL
			core::Size ss_start = ss_bounds[i].first;
			core::Size ss_end = ss_bounds[i].second;	
			core::Size ss_mid = calculate_ss_center(ss_start, ss_end);
			ft.add_edge( root_res, ss_mid, jump_counter);
			++jump_counter;
			if (ss_mid != ss_start) {
				ft.add_edge( ss_mid, ss_start, core::kinematics::Edge::PEPTIDE );
			}
			if (ss_mid != ss_end) {
				ft.add_edge( ss_mid, ss_end, core::kinematics::Edge::PEPTIDE );
			}	
		}
		
		std::cout << "We made " << jump_counter << " jumps" << std::endl;
		std::cout << ft << std::endl;

		//core::Size pep_edge_ct = 4 * ss_bounds.size() - 2;
		//code::Size jump_edge_ct = 2 * ss_bounds.size() - 2;

		return ft;
	}



	core::Size 
	calculate_ss_center( core::Size first, core::Size second ) {
		return second - ((second - first) / 2);
	}
	
	utility::vector1< std::pair< core::Size, core::Size > >
	identify_secondary_structure_spans( std::string const & ss_string )
	{
	utility::vector1< std::pair< core::Size, core::Size > > ss_boundaries;
	core::Size strand_start = -1;
	for ( core::Size ii = 0; ii < ss_string.size(); ++ii ) {
		if ( ss_string[ ii ] == 'E' || ss_string[ ii ] == 'H'  ) {
		if ( int( strand_start ) == -1 ) {
			strand_start = ii;
		} else if ( ss_string[ii] != ss_string[strand_start] ) {
			ss_boundaries.push_back( std::make_pair( strand_start+1, ii ) );
			strand_start = ii;
		}
		} else {
		if ( int( strand_start ) != -1 ) {
			ss_boundaries.push_back( std::make_pair( strand_start+1, ii ) );
			strand_start = -1;
		}
		}
	}
	if ( int( strand_start ) != -1 ) {
		// last residue was part of a ss-eleemnt                                                                                                                                
		ss_boundaries.push_back( std::make_pair( strand_start+1, ss_string.size() ));
	}
	for ( core::Size ii = 1; ii <= ss_boundaries.size(); ++ii ) {
		std::cout << "SS Element " << ii << " from residue "
		<< ss_boundaries[ ii ].first << " to "
		<< ss_boundaries[ ii ].second << std::endl;
	}
	return ss_boundaries;
	}

	// Shared initialization goes here.
	void setUp() {

		std::cout << "TEST" << std::endl;
		std::string string1 = "   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   ";
		std::string string2 = "HHHHHHH   HHHHHHHHHHHH      HHHHHHHHHHHHEEEEEEEEEEHHHHHHH EEEEHHH ";
		std::string string3 = "EEEEEEEEE EEEEEEEE EEEEEEEEE H EEEEE H H H EEEEEEEE";
		core_init();
		sample1 = identify_secondary_structure_spans("   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   ");
		sample2 = identify_secondary_structure_spans("HHHHHHH   HHHHHHHHHHHH      HHHHHHHHHHHHEEEEEEEEEEHHHHHHH EEEEHHH ");
		sample3 = identify_secondary_structure_spans("EEEEEEEEE EEEEEEEE EEEEEEEEE H EEEEE H H H EEEEEEEE");
	}

	// Shared finalization goes here.
	void tearDown() {
	}

	void test_identify_secondary_structure_spans() {
		TS_ASSERT_EQUALS(sample1.size(), 7);
		TS_ASSERT_EQUALS(sample2.size(), 7);
		TS_ASSERT_EQUALS(sample3.size(), 9);
	}

	void test_fold_tree_from_ss() {
		core::pose::Pose pose_in = create_test_in_pdb_pose();
		core::pose::Pose pose_out = fold_tree_from_ss(pose_in);
		TS_ASSERT_EQUALS(pose_in.size(), pose_out.size());
		//TS_ASSERT_DIFFERS(pose_in.fold_tree(), pose_out.fold_tree())
		
	}

	void test_fold_tree_from_dssp_string() {
		std::string test_dssp = "   EEEEEEE    EEEEEEE         EEEEEEEEE    EEEEEEEEEE   HHHHHH         EEEEEEEEE         EEEEE     ";
		TS_ASSERT_EQUALS(fold_tree_from_dssp_string(test_dssp).size(), 38);
		TS_ASSERT(fold_tree_from_dssp_string(test_dssp).check_fold_tree());
	}
	

private:
	utility::vector1< std::pair< core::Size, core::Size > > sample1;
	utility::vector1< std::pair< core::Size, core::Size > > sample2;
	utility::vector1< std::pair< core::Size, core::Size > > sample3;
	std::string string1;
	std::string string2;
	std::string string3;
};
