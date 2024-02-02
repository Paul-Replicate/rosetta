// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  protocols/bootcamp/BootCampMover.cxxtest.hh
/// @brief  Unit tests for BootCampMover methods.
/// @author Paul-Replicate (ptoth512@gmail.com)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <protocols/moves/MoverFactory.hh>
#include <protocols/bootcamp/BootCampMover.hh>

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
#include <core/scoring/ScoreFunction.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>
#include <utility/pointer/deep_copy.hh>

// C++ Headers
#include <string>

static basic::Tracer TR("BootCampMover");


class BootCampMover : public CxxTest::TestSuite {
	//Define Variables

public:

	void setUp() {
		core_init();
		TR << "TESTING" << std::endl;
	}

	void tearDown() {

	}

	

	void test_BootCampMover() {
		TS_ASSERT( true );
		protocols::moves::MoverOP mover_op = protocols::moves::MoverFactory::get_instance()->newMover( "BootCampMover" );
		TS_ASSERT_EQUALS(mover_op->get_name(), "BootCampMover");
		protocols::bootcamp::BootCampMoverOP bcm_op = utility::pointer::dynamic_pointer_cast< protocols::bootcamp::BootCampMover >(mover_op);
		std::cout << "TEST TEST TEST" << std::endl;
		TS_ASSERT_DIFFERS(bcm_op, nullptr);
		TR << "Differs Passed" << std::endl;
		TS_ASSERT_EQUALS(bcm_op->mover_name(), "BootCampMover");
		TR << "Equals Passed" << std::endl;
	}

	void test_sfxn_get_set() {
		protocols::bootcamp::BootCampMoverOP bcm_op = protocols::moves::MoverFactory::get_instance()->newMover( "BootCampMover" );
		TR << bcm_op->get_name() << std::endl;
		//core::scoring::ScoreFunctionOP orig_sfxn = bcm_op->get_score_function();
		//core::scoring::ScoreFunctionOP new_sfxn = core::scoring::get_score_function();
		//TS_ASSERT_EQUALS( orig_sfxn->get_name(), new_sfxn->get_name() );

	}

	void num_iterations_get_set() {
		
	}

};
