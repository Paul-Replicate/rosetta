// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/bootcamp/BootCampMover.cc
/// @brief Simple mover to apply mc weighted Phi/Psi perturbations.
/// @author Paul-Replicate (ptoth512@gmail.com)

// Unit headers
#include <protocols/bootcamp/BootCampMover.hh>
#include <protocols/bootcamp/BootCampMoverCreator.hh>
#include <protocols/jd2/JobDistributor.hh>

// Core headers
#include <core/pose/Pose.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>
#include <utility/pointer/memory.hh>

// XSD Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <protocols/moves/mover_schemas.hh>

// Citation Manager
#include <utility/vector1.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>

#include <iostream>
#include <core/types.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <devel/init.hh>
#include <utility/pointer/owning_ptr.hh>

#include <core/import_pose/import_pose.hh>
#include <core/scoring/ScoreFunction.hh>

#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/scoring/ScoreType.hh>
#include <numeric/random/random.hh>
#include <protocols/moves/MonteCarlo.hh>

#include <protocols/moves/PyMOLMover.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/pack_rotamers.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/optimization/MinimizerOptions.hh>
#include <core/optimization/AtomTreeMinimizer.hh>
#include <core/pose/variant_util.hh>

#include <protocols/bootcamp/fold_tree_from_ss.hh>


static basic::Tracer TR( "protocols.bootcamp.BootCampMover" );

namespace protocols {
namespace bootcamp {

	/////////////////////
	/// Constructors  ///
	/////////////////////

/// @brief Default constructor
BootCampMover::BootCampMover():
	protocols::moves::Mover( BootCampMover::mover_name() )
{
	//protocols::jd2::JobDistributor::get_instance();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer members)
BootCampMover::~BootCampMover(){}

////////////////////////////////////////////////////////////////////////////////
	/// Mover Methods ///
	/////////////////////

/// @brief Apply the mover
void
BootCampMover::apply( core::pose::Pose& mypose ){

	//mypose = *mypose;
	mypose.fold_tree(protocols::bootcamp::fold_tree_from_ss( mypose ));
	//mypose = protocols::bootcamp::fold_tree_from_ss( mypose );
	core::pose::correctly_add_cutpoint_variants( mypose );

	// Add linear_chainbreak to score function to punish
	sfxn_->set_weight( core::scoring::linear_chainbreak, 1.0);

	//core::Real score = sfxn->score( * mypose );

	protocols::moves::MonteCarlo mc = protocols::moves::MonteCarlo( mypose,  *sfxn_, 1.0 );
	protocols::moves::PyMOLObserverOP the_observer = protocols::moves::AddPyMOLObserver( mypose, true, 0 );
	the_observer->pymol().apply( mypose );

	core::kinematics::MoveMap mm;
	mm.set_bb( true );
	mm.set_chi( true );
	core::optimization::MinimizerOptions min_opts( "lbfgs_armijo_atol", 0.01, true );
	core::optimization::AtomTreeMinimizer atm;

	//mypose = protocols::bootcamp::fold_tree_from_ss( mypose );
	core::pose::Pose copy_pose = mypose;

	core::Size num_res = mypose.size();
	//core::Size mc_cycles = 100;
	core::Size accepted_count = 0;
	core::Real running_score_avg = 0.0;
	for (core::Size i=0; i <= num_iterations_; i++ ) {
		TR << "Running MC Cycle " << i << std::endl;
		core::Size randres = numeric::random::uniform() * num_res + 1;
		core::Real pert1 = numeric::random::gaussian();
		core::Real pert2 = numeric::random::gaussian();
		core::Real orig_phi = mypose.phi( randres );
		core::Real orig_psi = mypose.psi( randres );
		mypose.set_phi( randres, orig_phi + pert1 );
		mypose.set_psi( randres, orig_psi + pert2 );
		core::pack::task::PackerTaskOP repack_task = core::pack::task::TaskFactory::create_packer_task( mypose );
		repack_task->restrict_to_repacking();
		core::pack::pack_rotamers( mypose, *sfxn_, repack_task );
		copy_pose = mypose;
		atm.run( copy_pose, mm, *sfxn_, min_opts );
		mypose = copy_pose;

		TR << "TESTING TESTING TESTING TESTING" << std::endl;
		
		bool accepted = mc.boltzmann( mypose );
		running_score_avg += mc.last_score();  // Add score each cycle. Div below to get avg score
		if (accepted) {
			accepted_count += 1;
			std::cout << "accepted" << std::endl;
		} else {
			std::cout << "rejected" << std::endl;
		}
	}

	std::cout << accepted_count << " cycles were accepted out of " << num_iterations_ << std::endl;
	std::cout << "Percent Accepted: " << static_cast<core::Real>(accepted_count) / static_cast<core::Real>(num_iterations_) << std::endl;
	std::cout << "Average Score: " << running_score_avg / static_cast<core::Real>(num_iterations_) << std::endl;
}

core::scoring::ScoreFunctionOP
BootCampMover::get_score_function() {
	return sfxn_;
}

core::Size
BootCampMover::get_num_iterations() {
	return num_iterations_;
}

void 
BootCampMover::set_score_function( core::scoring::ScoreFunctionOP & sfxn ) {
	sfxn_ = sfxn;
}

void 
BootCampMover::set_num_iterations( core::Size & num_iterations ) {
	num_iterations_ = num_iterations;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Show the contents of the Mover
void
BootCampMover::show(std::ostream & output) const
{
	protocols::moves::Mover::show(output);
}

////////////////////////////////////////////////////////////////////////////////
	/// Rosetta Scripts Support ///
	///////////////////////////////

/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
void
BootCampMover::parse_my_tag(
	utility::tag::TagCOP ,
	basic::datacache::DataMap&
) {

}
void BootCampMover::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{

	using namespace utility::tag;
	AttributeList attlist;

	//here you should write code to describe the XML Schema for the class.  If it has only attributes, simply fill the probided AttributeList.

	protocols::moves::xsd_type_definition_w_attributes( xsd, mover_name(), "Simple mover to apply mc weighted Phi/Psi perturbations.", attlist );
}


////////////////////////////////////////////////////////////////////////////////
/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
BootCampMover::fresh_instance() const
{
	return utility::pointer::make_shared< BootCampMover >();
}

/// @brief required in the context of the parser/scripting scheme
protocols::moves::MoverOP
BootCampMover::clone() const
{
	return utility::pointer::make_shared< BootCampMover >( *this );
}

std::string BootCampMover::get_name() const {
	return mover_name();
}

std::string BootCampMover::mover_name() {
	return "BootCampMover";
}



/////////////// Creator ///////////////

protocols::moves::MoverOP
BootCampMoverCreator::create_mover() const
{
	return utility::pointer::make_shared< BootCampMover >();
}

std::string
BootCampMoverCreator::keyname() const
{
	return BootCampMover::mover_name();
}

void BootCampMoverCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	BootCampMover::provide_xml_schema( xsd );
}

/// @brief This mover is unpublished.  It returns Paul-Replicate as its author.
void
BootCampMover::provide_citation_info(basic::citation_manager::CitationCollectionList & citations ) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"BootCampMover", basic::citation_manager::CitedModuleType::Mover,
		"Paul-Replicate",
		"TODO: institution",
		"ptoth512@gmail.com",
		"Wrote the BootCampMover."
		)
	);
}


////////////////////////////////////////////////////////////////////////////////
	/// private methods ///
	///////////////////////

std::ostream &
operator<<( std::ostream & os, BootCampMover const & mover )
{
	mover.show(os);
	return os;
}


} //bootcamp
} //protocols
