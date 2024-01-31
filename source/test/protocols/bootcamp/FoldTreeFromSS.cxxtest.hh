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
#include <string>

/// Project headers
#include <core/types.hh>

// C++ headers

//Auto Headers
#include <core/pack/dunbrack/DunbrackRotamer.hh>


// --------------- Test Class --------------- //

class FoldTreeFromSS : public CxxTest::TestSuite {

public:


	// --------------- Fixtures --------------- //

	// Define a test fixture (some initial state that several tests share)
	// In CxxTest, setUp()/tearDown() are executed around each test case. If you need a fixture on the test
	// suite level, i.e. something that gets constructed once before all the tests in the test suite are run,
	// suites have to be dynamically created. See CxxTest sample directory for example.

	
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
		//std::string sample1 = "   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   ";
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
		/*
		"   EEEEE   HHHHHHHH  EEEEE   IGNOR EEEEEE   HHHHHHHHHHH  EEEEE  HHHH   "
			with 7 secondary structure elements, spanning residues 4 to 8, 12 to 19, 22 to 26, 36 to 41, 45 to 55, 58 to 62, and 65 to 68 (notice there are some non H/E elements that you should ignore.
			"HHHHHHH   HHHHHHHHHHHH      HHHHHHHHHHHHEEEEEEEEEEHHHHHHH EEEEHHH "
			with 7 secondary structure elements, spanning residues 1 to 7, 11 to 22, 29 to 40, 41 to 50, 51 to 57, 59 to 62, and 63 to 65.
			"EEEEEEEEE EEEEEEEE EEEEEEEEE H EEEEE H H H EEEEEEEE"
			with 9 secondary structure elements, spanning residues 1 to 9, 11 to 18, 20 to 28, 30 to 30, 32 to 36, 38 to 38, 40 to 40, 42 to 42, and 44 to 51.
		*/

	}
private:
	utility::vector1< std::pair< core::Size, core::Size > > sample1;
	utility::vector1< std::pair< core::Size, core::Size > > sample2;
	utility::vector1< std::pair< core::Size, core::Size > > sample3;
};
