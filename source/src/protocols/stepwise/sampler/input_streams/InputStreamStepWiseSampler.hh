// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/stepwise/sampler/input_streams/InputStreamStepWiseSampler.hh
/// @brief
/// @details
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_sampler_input_streams_InputStreamStepWiseSampler_HH
#define INCLUDED_protocols_sampler_input_streams_InputStreamStepWiseSampler_HH

#include <protocols/stepwise/sampler/StepWiseSamplerSized.hh>
#include <protocols/stepwise/sampler/input_streams/InputStreamStepWiseSampler.fwd.hh>
#include <protocols/stepwise/modeler/protein/InputStreamWithResidueInfo.fwd.hh>
#include <protocols/stepwise/legacy/modeler/protein/StepWiseProteinPoseSetup.fwd.hh>

namespace protocols {
namespace stepwise {
namespace sampler {
namespace input_streams {

class InputStreamStepWiseSampler: public StepWiseSamplerSized {

public:

	//constructor
	InputStreamStepWiseSampler( stepwise::modeler::protein::InputStreamWithResidueInfoOP input_stream );

	InputStreamStepWiseSampler( stepwise::modeler::protein::InputStreamWithResidueInfoOP input_stream,
		stepwise::legacy::modeler::protein::StepWiseProteinPoseSetupCOP stepwise_pose_setup );

	//destructor
	~InputStreamStepWiseSampler() override;

public:

	/// @brief Reset to the first (or random if random()) rotamer.
	void reset() override;

	/// @brief Get the total number of rotamers in sampler
	core::Size size() const override{ return size_;}

	/// @brief set ID -- how StepWiseSamplerSizedComb controls StepWiseSamplerSized. Need some extra work here with InputStreamStepWiseSampler.
	void set_id( core::Size const setting ) override;

	/// @brief Move to next rotamer
	void operator++() override;

	/// @brief Apply the i-th rotamer to pose
	void apply( core::pose::Pose&, core::Size const ) override;

	/// @brief Name of the class
	std::string get_name() const override { return "InputStreamStepWiseSampler"; }

	/// @brief Type of class (see enum in toolbox::SamplerPlusPlusTypes.hh)
	toolbox::SamplerPlusPlusType type() const override { return toolbox::INPUT_STREAM; }

private:

	stepwise::modeler::protein::InputStreamWithResidueInfoOP input_stream_;
	stepwise::legacy::modeler::protein::StepWiseProteinPoseSetupCOP stepwise_pose_setup_; // this is really a legacy of SWA protein code.
	core::Size size_;

};

} //input_streams
} //sampler
} //stepwise
} //protocols

#endif
