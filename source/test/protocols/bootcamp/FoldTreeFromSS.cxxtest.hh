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

	std::string fold_tree_from_ss( core::pose::Pose & in_pose ) {
		// Take pose return FoldTree
		core::Size numres = in_pose.size();
		core::scoring::dssp::Dssp DSSP = core::scoring::dssp::Dssp( in_pose );

		std::string dssp_string = "";
		for (core::Size res=1; res <= numres; res++) {
			dssp_string + DSSP.get_dssp_secstruct(res);
		}
		
		core::kinematics::FoldTree pose_from_dssp = fold_tree_from_dssp_string( dssp_string );
		TR << pose_from_dssp;
		return dssp_string;
	}

	core::kinematics::FoldTree
	fold_tree_from_dssp_string( std::string dssp_string ) {
		// Take string and return FoldTree
		utility::vector1< std::pair< core::Size, core::Size > > ss_bounds;
		ss_bounds = identify_secondary_structure_spans( dssp_string );
		core::Size nres = dssp_string.size();

		// Construct FoldTree from number of residues only
		core::kinematics::FoldTree ft = core::kinematics::FoldTree(nres);

		// Set the midpoint of the first SS element
		// sub half of the ss.first and ss.second difference to ss.first to get the mid
		core::Size first_ss_mid = curr_ss.second - ((curr_ss.second - curr_ss.first) / 2);

		utility::vector1< std::pair< core::Size, core::Size > > loop_boundaries;

		for (core::Size i=1; i <= ss_bounds.size(); i++) { 
			// Loop over all but first SS elem.
			std::pair curr_ss = ss_bounds[i];

			
		}

		//core::Size pep_edge_ct = 4 * ss_bounds.size() - 2;
		//code::Size jump_edge_ct = 2 * ss_bounds.size() - 2;

		return ft;
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

	void test_hello_world() {
		TS_ASSERT( true );
	}

	void test_identify_secondary_structure_spans() {
		TS_ASSERT_EQUALS(sample1.size(), 7);
		TS_ASSERT_EQUALS(sample2.size(), 7);
		TS_ASSERT_EQUALS(sample3.size(), 9);
		
		// Test the pairs in sample 1
		utility::vector1< std::pair< core::Size, core::Size > > sample1_compare;
		sample1_compare.push_back(std::make_pair(12,19));
		sample1_compare.push_back(std::make_pair(22,26));
		sample1_compare.push_back(std::make_pair(36,41));
		sample1_compare.push_back(std::make_pair(45,55));
		sample1_compare.push_back(std::make_pair(58,62));
		sample1_compare.push_back(std::make_pair(65,68));
		for( core::Size i=0; i <= sample1.size(); i++) {
			std::cout << sample1[i].first << std::endl;
			std::cout << sample1_compare[i].first << std::endl;
			TS_ASSERT_EQUALS(sample1[i].first, sample1_compare[i].first);
			TS_ASSERT_EQUALS(sample1[i].second, sample1_compare[i].second);
		}
		
		utility::vector1< std::pair< core::Size, core::Size > > sample2_compare;
		sample2_compare.push_back(std::make_pair(1,7));
		sample2_compare.push_back(std::make_pair(11,22));
		sample2_compare.push_back(std::make_pair(29,40));
		sample2_compare.push_back(std::make_pair(41,50));
		sample2_compare.push_back(std::make_pair(51,57));
		sample2_compare.push_back(std::make_pair(59,62));
		sample2_compare.push_back(std::make_pair(63,65));
		for( core::Size i=0; i <= sample2.size(); i++) {
			std::cout << sample1[i].first << std::endl;
			std::cout << sample2_compare[i].first << std::endl;
			TS_ASSERT_EQUALS(sample2[i].first, sample2_compare[i].first);
			TS_ASSERT_EQUALS(sample2[i].second, sample2_compare[i].second);
		}

		utility::vector1< std::pair< core::Size, core::Size > > sample3_compare;
		sample3_compare.push_back(std::make_pair(1,9));
		sample3_compare.push_back(std::make_pair(11,18));
		sample3_compare.push_back(std::make_pair(20,28));
		sample3_compare.push_back(std::make_pair(30,30));
		sample3_compare.push_back(std::make_pair(32,36));
		sample3_compare.push_back(std::make_pair(38,38));
		sample3_compare.push_back(std::make_pair(40,40));
		sample3_compare.push_back(std::make_pair(42,42));
		sample3_compare.push_back(std::make_pair(44,51));
		for( core::Size i=0; i <= sample3.size(); i++) {
			std::cout << sample3[i].first << std::endl;
			std::cout << sample3_compare[i].first << std::endl;
			TS_ASSERT_EQUALS(sample3[i].first, sample3_compare[i].first);
			TS_ASSERT_EQUALS(sample3[i].second, sample3_compare[i].second);
		}
	}

	void test_fold_tree_from_ss() {
		core::pose::Pose my_pose = create_test_in_pdb_pose();
		std::string pose_seq = fold_tree_from_ss(my_pose);
		TR << pose_seq << std::endl;
		std::cout << pose_seq << std::endl;
		
	}

	void test_fold_tree_from_dssp_string() {

	}

private:
	utility::vector1< std::pair< core::Size, core::Size > > sample1;
	utility::vector1< std::pair< core::Size, core::Size > > sample2;
	utility::vector1< std::pair< core::Size, core::Size > > sample3;
	std::string string1;
	std::string string2;
	std::string string3;
};
