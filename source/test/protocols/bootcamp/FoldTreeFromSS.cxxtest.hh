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
#include <protocols/bootcamp/fold_tree_from_ss.hh>
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

//Namespace
using namespace protocols::bootcamp;

// --------------- Test Class --------------- //

class FoldTreeFromSS : public CxxTest::TestSuite {

public:


	// --------------- Fixtures --------------- //

	// Define a test fixture (some initial state that several tests share)
	// In CxxTest, setUp()/tearDown() are executed around each test case. If you need a fixture on the test
	// suite level, i.e. something that gets constructed once before all the tests in the test suite are run,
	// suites have to be dynamically created. See CxxTest sample directory for example.

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
		//TR << "INPUT POSE TREE: " << pose_in.fold_tree() << std::endl;
		// core::Size pose_in_ft_size = pose_in.fold_tree().size();
		core::kinematics::FoldTree fold_tree = fold_tree_from_ss(pose_in);
		// core::Size pose_out_ft_size = pose_out.fold_tree().size();
		TS_ASSERT_EQUALS(pose_in.size(), fold_tree.size());
		
		
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
