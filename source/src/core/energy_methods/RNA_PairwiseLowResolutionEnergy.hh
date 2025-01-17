// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/scoring/rna/RNA_PairwiseLowResolutionEnergy.hh
/// @brief  Statistically derived rotamer pair potential class declaration
/// @author Rhiju Das


#ifndef INCLUDED_core_scoring_rna_RNA_PairwiseLowResolutionEnergy_hh
#define INCLUDED_core_scoring_rna_RNA_PairwiseLowResolutionEnergy_hh

// Unit Headers
#include <core/energy_methods/RNA_PairwiseLowResolutionEnergy.fwd.hh>

// Package headers
#include <core/scoring/methods/ContextIndependentTwoBodyEnergy.hh>
#include <core/scoring/rna/RNA_LowResolutionPotential.fwd.hh>
#include <core/pose/rna/RNA_RawBaseBaseInfo.fwd.hh>

// Project headers
#include <core/pose/Pose.fwd.hh>

#include <core/kinematics/Stub.fwd.hh>
#include <utility/vector1.hh>


// Utility headers


namespace core {
namespace energy_methods {


class RNA_PairwiseLowResolutionEnergy : public core::scoring::methods::ContextIndependentTwoBodyEnergy  {
public:
	typedef core::scoring::methods::ContextIndependentTwoBodyEnergy  parent;
public:


	RNA_PairwiseLowResolutionEnergy();


	/// clone
	core::scoring::methods::EnergyMethodOP
	clone() const override;

	/////////////////////////////////////////////////////////////////////////////
	// scoring
	/////////////////////////////////////////////////////////////////////////////

	void
	setup_for_scoring( pose::Pose & pose, core::scoring::ScoreFunction const & ) const override;

	void
	setup_for_derivatives( pose::Pose & pose, core::scoring::ScoreFunction const & ) const override;

	void
	setup_for_packing( pose::Pose & pose, utility::vector1< bool > const &, utility::vector1< bool > const & designing_residues ) const override;

	void
	residue_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2,
		pose::Pose const & pose,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap & emap
	) const override;

	void
	eval_intrares_energy(
		conformation::Residue const &,
		pose::Pose const &,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap &
	) const override {}

	void
	eval_atom_derivative(
		id::AtomID const & atom_id,
		pose::Pose const & pose,
		kinematics::DomainMap const & domain_map,
		core::scoring::ScoreFunction const & scorefxn,
		core::scoring::EnergyMap const & weights,
		Vector & F1,
		Vector & F2
	) const override;

	bool
	defines_intrares_energy( core::scoring::EnergyMap const & /*weights*/ ) const override { return false; }

	void
	finalize_total_energy(
		pose::Pose & pose,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap &// totals
	) const override;

	Distance
	atomic_interaction_cutoff() const override;

	void indicate_required_context_graphs( utility::vector1< bool > & ) const override {}


	/////////////////////////////////////////////////////////////////////////////
	// data
	/////////////////////////////////////////////////////////////////////////////

private:
	void
	get_centroid_information(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2,
		pose::Pose const & pose,
		Vector & centroid1,
		Vector & centroid2,
		kinematics::Stub & stub1,
		kinematics::Stub & stub2 ) const;

	//Need to get rid of the following...
	Real
	rna_base_pair_pairwise_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2
	) const;

	Real
	rna_base_axis_pairwise_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2
	) const;

	Real
	rna_base_stagger_pairwise_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2
	) const;

	Real
	rna_base_stack_pairwise_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2
	) const;

	Real
	rna_base_stack_axis_pairwise_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2
	) const;

	void
	clean_up_rna_two_body_energy_tables(
		pose::rna::RNA_RawBaseBaseInfo & raw_base_base_info,
		pose::Pose & pose ) const;

private:

	// const-ref to scoring database
	core::scoring::rna::RNA_LowResolutionPotential const & rna_low_resolution_potential_;
	mutable pose::rna::RNA_RawBaseBaseInfo * rna_raw_base_base_info_;
	mutable bool might_be_designing_;
	core::Size version() const override;

};


} //scoring
} //core

#endif // INCLUDED_core_scoring_ScoreFunction_HH
