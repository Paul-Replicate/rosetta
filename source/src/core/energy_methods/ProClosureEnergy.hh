// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/energy_methods/ProClosureEnergy.hh
/// @brief  Proline ring closure energy method class declaration
/// @note This energy term supports L-proline, D-proline, and L- or D-oligourea-proline.
/// @author Andrew Leaver-Fay
/// @author Vikram K. Mulligan (vmullig@uw.edu) -- added support for D-proline and for oligoureas.


#ifndef INCLUDED_core_energy_methods_ProClosureEnergy_hh
#define INCLUDED_core_energy_methods_ProClosureEnergy_hh

// Unit headers
#include <core/energy_methods/ProClosureEnergy.fwd.hh>

// Package headers
#include <core/scoring/methods/ContextIndependentTwoBodyEnergy.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/scoring/MinimizationData.fwd.hh>


#include <utility/vector1.hh>


namespace core {
namespace energy_methods {



class ProClosureEnergy : public core::scoring::methods::ContextIndependentTwoBodyEnergy  {
public:
	typedef core::scoring::methods::ContextIndependentTwoBodyEnergy  parent;

public:

	/// ctor
	ProClosureEnergy();

	// dstor
	~ProClosureEnergy() override;

	/// clone
	core::scoring::methods::EnergyMethodOP
	clone() const override;

	/////////////////////////////////////////////////////////////////////////////
	// methods for ContextIndependentTwoBodyEnergies
	/////////////////////////////////////////////////////////////////////////////

	/// @brief Pro-closure terms only apply between bonded residues where i+1 is
	/// proline -- skip residue pairs that don't apply during minimization.
	bool
	defines_score_for_residue_pair(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2,
		bool res_moving_wrt_eachother
	) const override;

	/// @brief Evaluate the interaction between a given residue pair
	void
	residue_pair_energy(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2,
		pose::Pose const & pose,
		core::scoring::ScoreFunction const & sfxn,
		core::scoring::EnergyMap & emap
	) const override;

	bool
	minimize_in_whole_structure_context( pose::Pose const & ) const override { return false; }

	/// @brief Evaluate the derivative for the input residue pair.
	/// This will only be called if rsd1 and rsd2 are bonded and one of them is a proline
	/// because ProClosureEnergy defines the "defines_score_for_residue_pair" method.
	/*virtual
	void
	eval_atom_derivative_for_residue_pair(
	Size const atom_index,
	conformation::Residue const & rsd1,
	conformation::Residue const & rsd2,
	core::scoring::ResSingleMinimizationData const & minsingle_data1,
	core::scoring::ResSingleMinimizationData const & minsingle_data2,
	core::scoring::ResPairMinimizationData const & min_data,
	pose::Pose const & pose, // provides context
	kinematics::DomainMap const & domain_map,
	core::scoring::ScoreFunction const & sfxn,
	core::scoring::EnergyMap const & weights,
	Vector & F1,
	Vector & F2
	) const;*/

	void
	eval_residue_pair_derivatives(
		conformation::Residue const & rsd1,
		conformation::Residue const & rsd2,
		core::scoring::ResSingleMinimizationData const &,
		core::scoring::ResSingleMinimizationData const &,
		core::scoring::ResPairMinimizationData const &,
		pose::Pose const &,
		core::scoring::EnergyMap const & weights,
		utility::vector1< core::scoring::DerivVectorPair > & r1_atom_derivs,
		utility::vector1< core::scoring::DerivVectorPair > & r2_atom_derivs
	) const override;

	/// @brief Non-virtual interface; takes only the needed parameters.
	/*void
	eval_atom_derivative_for_residue_pair2(
	Size const atom_index,
	conformation::Residue const & rsd1,
	conformation::Residue const & rsd2,
	core::scoring::EnergyMap const & weights,
	Vector & F1,
	Vector & F2
	) const;*/

	/// @brief Penalize the pucker-up residue type if its chi1 is positive;
	/// penalize the pucker-down residue type if its chi1 is negative.  Only
	/// applies this penalty when the other_residue is the next polymeric residue
	/// after pro_residue (i+1), unless pro_residue is an upper_term,
	/// in which case it applies the penalty for pro_residue's previous polymeric
	/// residue.
	void
	bump_energy_full(
		conformation::Residue const & pro_residue,
		conformation::Residue const & other_residue,
		pose::Pose const &,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap &
	) const override;

	/// @brief Penalize the pucker-up residue type if its chi1 is positive;
	/// penalize the pucker-down residue type if its chi1 is negative.  Only
	/// applies this penalty when the other_residue is the next polymeric residue
	/// after pro_residue (i+1), unless pro_residue is an upper_term,
	/// in which case it applies the penalty for pro_residue's previous polymeric
	/// residue.
	void
	bump_energy_backbone(
		conformation::Residue const & pro_residue,
		conformation::Residue const & other_residue,
		pose::Pose const &,
		core::scoring::ScoreFunction const &,
		core::scoring::EnergyMap &
	) const override;


	bool
	defines_intrares_energy( core::scoring::EnergyMap const & weights ) const override;

	void
	eval_intrares_energy(
		conformation::Residue const & rsd,
		pose::Pose const & pose,
		core::scoring::ScoreFunction const & sfxn,
		core::scoring::EnergyMap & emap
	) const override;


	/// @brief Returns false if res is not a proline.
	bool
	defines_intrares_energy_for_residue(
		conformation::Residue const & res
	) const override;

	/// @brief This should only be handed a proline.
	void
	eval_intrares_derivatives(
		conformation::Residue const & rsd,
		core::scoring::ResSingleMinimizationData const & min_data,
		pose::Pose const & pose,
		core::scoring::EnergyMap const & weights,
		utility::vector1< core::scoring::DerivVectorPair > & atom_derivs
	) const override;

	/// @brief ProClosure Energy is context independent and thus
	/// indicates that no context graphs need to
	/// be maintained by class Energies
	void indicate_required_context_graphs(
		utility::vector1< bool > & /*context_graphs_required*/
	) const override;

	Distance
	atomic_interaction_cutoff() const override
	{ return 4.0; }

	/// @brief Queries whether the user has set the -score::no_pro_close_ring_closure flag.
	/// If he/she has, this sets skip_ring_closure_ to 'true'.
	void set_skip_ring_closure_from_flags();

	/// @brief Sets skip_ring_closure.
	///
	void set_skip_ring_closure( bool const val=true ) { skip_ring_closure_=val; return; };

	/// @brief Gets skip_ring_closure.
	///
	inline bool skip_ring_closure() const { return skip_ring_closure_; };


private:

	/// @brief measure in radians the chi4 between two residues;
	/// upper_residue must be a proline chi4 is wrapped to be in
	/// the range [-pi_over_2, 3/2*pi )
	Real
	measure_chi4(
		conformation::Residue const & lower_residue,
		conformation::Residue const & upper_residue
	) const;

	Real
	chi4E(
		Real chi4
	) const;

	Real
	dchi4E_dchi4(
		Real chi4
	) const;

	// data
private:

	/// @brief The pro_close term does two things: it holds the proline ring closed, and it also
	/// has some logic based on the psi value of the previous residue.  If this flag is set to
	/// 'true', the term only does the torsional stuff, allowing the ring closure to be handled
	/// by other terms (e.g. cart_bonded or ring_close).  Set to 'false' by default.
	/// @details The -score::no_pro_close_ring_closure flag can be used to set this to true.
	bool skip_ring_closure_;

	/// @brief The SQUARE of the coordinate variation standard deviation.
	///
	Real const n_nv_dist_sd_;
	Real const ca_cav_dist_sd_;

	Real const trans_chi4_mean_;
	Real const trans_chi4_sd_;
	Real const cis_chi4_mean_;
	Real const cis_chi4_sd_;

	std::string const bbN_;
	std::string const scNV_;
	std::string const bbCA_;
	std::string const scCAV_;
	std::string const scCD_;
	std::string const bbC_;
	std::string const bbO_;
	core::Size version() const override;

};

} // scoring
} // core


#endif // INCLUDED_core_scoring_EtableEnergy_HH
