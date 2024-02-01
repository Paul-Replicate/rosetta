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

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
utility::pointer::dynamic_pointer_cast

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
		
	}

	void tearDown() {

	}


	void test_get_correct_mover() {
		TS_ASSERT( true );
		//protocols::moves::MoverOP mover_op = protocols::moves::MoverFactory::get_instance()->newMover( "BootCampMover" );
		//protocols::bootcamp::BootCampMover bcm_op = BootCampMover( utility::pointer::dynamic_pointer_cast< protocols::bootcamp::BootCampMover > (mover_op));
	}



};
