// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/scoring/rna/RNP_LowResPairDistEnergy.cc
/// @brief  Statistically derived rotamer pair potential class implementation
/// @author Kalli Kappel


// Unit headers
#include <core/energy_methods/RNP_LowResPairDistEnergy.hh>
#include <core/scoring/rna/RNP_LowResPairDistPotential.hh>
#include <core/energy_methods/RNP_LowResPairDistEnergyCreator.hh>

// Package headers
#include <basic/Tracer.hh>
#include <core/scoring/ScoringManager.hh>

// Project headers
#include <core/pose/Pose.fwd.hh>
#include <core/conformation/Residue.hh>

#include <basic/options/option.hh>
#include <basic/options/keys/score.OptionKeys.gen.hh>

// Utility headers


//Auto Headers

#include <core/scoring/EnergyMap.hh> // AUTO IWYU For EMapVector


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is a statistically derived low-resolution potential for RNA/protein interactions
// For RNA/protein modeling, this is meant to supplement the RNA low-res and protein low-res score
// functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static basic::Tracer TR( "core.energy_methods.RNP_LowResPairDistEnergy" );
using namespace core::chemical;
using namespace core::chemical::rna;
using namespace basic::options;
using namespace basic::options::OptionKeys::score;

namespace core {
namespace energy_methods {

/// @details This must return a fresh instance of the RNP_LowResPairDistEnergy class,
/// never an instance already in use
core::scoring::methods::EnergyMethodOP
RNP_LowResPairDistEnergyCreator::create_energy_method(
	core::scoring::methods::EnergyMethodOptions const &
) const {
	return utility::pointer::make_shared< RNP_LowResPairDistEnergy >();
}

core::scoring::ScoreTypes
RNP_LowResPairDistEnergyCreator::score_types_for_method() const {
	using namespace core::scoring;
	ScoreTypes sts;
	sts.push_back( rnp_pair_dist );
	return sts;
}


/// c-tor
RNP_LowResPairDistEnergy::RNP_LowResPairDistEnergy() :
	parent( utility::pointer::make_shared< RNP_LowResPairDistEnergyCreator >() ),
	potential_( core::scoring::ScoringManager::get_instance()->get_RNP_LowResPairDistPotential() )
{
	//std::cout << "Constructed the RNP pair dist energy" << std::endl;
}

//clone
core::scoring::methods::EnergyMethodOP
RNP_LowResPairDistEnergy::clone() const
{
	return utility::pointer::make_shared< RNP_LowResPairDistEnergy >();
}

/////////////////////////////////////////////////////////////////////////////
// scoring
/////////////////////////////////////////////////////////////////////////////
void
RNP_LowResPairDistEnergy::setup_for_scoring( pose::Pose & /*pose*/, core::scoring::ScoreFunction const & /*scfxn*/ ) const
{
	// don't need to do anything for now
}

//void
//RNP_LowResPairDistEnergy::setup_for_derivatives( pose::Pose & pose, core::scoring::ScoreFunction const & ) const
//{
//}

//////////////////////////////////////////////////////////////////////////////////////////
void
RNP_LowResPairDistEnergy::residue_pair_energy(
	conformation::Residue const & rsd1,
	conformation::Residue const & rsd2,
	pose::Pose const & /*pose*/, // need this back for rnp_pair
	core::scoring::ScoreFunction const &,
	core::scoring::EnergyMap & emap
) const
{
	if ( rsd1.has_variant_type( REPLONLY ) ) return;
	if ( rsd2.has_variant_type( REPLONLY ) ) return;

	// Only evaluate these score terms between RNA and protein residues
	if ( !(( rsd1.is_RNA() && rsd2.is_protein() ) || ( rsd1.is_protein() && rsd2.is_RNA() )) ) return;

	// this option is false by default
	bool const use_actual_centroid( basic::options::option[ basic::options::OptionKeys::score::rna::FA_low_res_rnp_scoring ]() );

	// Just give a warning and return if the protein isn't coarse
	if ( rsd1.is_protein() ) {
		bool is_centroid = rsd1.type().mode() == core::chemical::CENTROID_t;
		if ( !is_centroid && !use_actual_centroid ) {
			//TR.Warning << "rnp low res pair energy not computed b/c protein is not centroid" << std::endl;
			return;
		}
	} else { // rsd2 is protein
		bool is_centroid = rsd2.type().mode() == core::chemical::CENTROID_t;
		if ( !is_centroid && !use_actual_centroid ) {
			//TR.Warning << "rnp low res pair energy not computed b/c protein is not centroid" << std::endl;
			return;
		}
	}


	Real rnp_pair_dist_score = 0.0;
	potential_.evaluate_rnp_pair_dist_score( rsd1, rsd2, rnp_pair_dist_score );


	emap[ core::scoring::rnp_pair_dist ] += rnp_pair_dist_score;

}


/// @brief RNA_PairwiseLowResolutionEnergy distance cutoff
Distance
RNP_LowResPairDistEnergy::atomic_interaction_cutoff() const
{
	// For testing
	return 0.0; /// Uh, I don't know.
}

core::Size
RNP_LowResPairDistEnergy::version() const
{
	return 1; // Initial versioning
}


} //scoring
} //core
