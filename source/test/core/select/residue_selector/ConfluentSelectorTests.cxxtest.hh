// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  core/select/residue_selector/ConfluentSelectorTests.cxxtest.hh
/// @brief  tests the BlockSelector
/// @author Frank Teets (frankdt@email.unc.edu)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <core/select/residue_selector/ConfluentSelector.hh>
#include <core/select/residue_selector/SecondaryStructureSelector.hh>
#include <core/conformation/Residue.hh>


// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>

static basic::Tracer TR("ConfluentSelectorTests");


class ConfluentSelectorTests : public CxxTest::TestSuite {
	//Define Variables

public:

	void setUp() {
		core_init();

	}
	void test_confluentselector() {
		core::select::residue_selector::SecondaryStructureSelectorOP in_selector( new core::select::residue_selector::SecondaryStructureSelector );
		in_selector->set_selected_ss("H");
		in_selector->set_use_dssp(true);
		core::select::residue_selector::ConfluentSelectorOP selector( new core::select::residue_selector::ConfluentSelector );
		core::pose::Pose trpcage = create_trpcage_ideal_pose();

		selector->set_terminus_selector(in_selector);

		core::select::residue_selector::ResidueSubset subset = selector->apply( trpcage );

		TS_ASSERT( !subset[1] );
		for ( core::Size i=3; i<=14; ++i ) {
			TS_ASSERT( subset[i] );
		}
		core::select::residue_selector::SecondaryStructureSelectorOP in_selector_2( new core::select::residue_selector::SecondaryStructureSelector );
		in_selector_2->set_selected_ss("L");
		in_selector_2->set_use_dssp(true);
		selector->set_breaking_selector(in_selector_2);
		subset = selector->apply( trpcage );
		for ( core::Size i=8; i<=10; ++i ) {
			TS_ASSERT( !subset[i] );
		}

		TR.flush();
	}

	void tearDown() {

	}






};
