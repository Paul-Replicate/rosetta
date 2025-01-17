// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// Unit headers
#include <protocols/ncbb/SecStructMinimizeMultiFunc.hh>
#include <protocols/ncbb/util.hh>

/// Package headers

/// Project headers
#include <core/pose/Pose.hh>
#include <core/optimization/MinimizerMap.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/Energies.hh>
#include <core/scoring/EnergyGraph.fwd.hh>
#include <core/scoring/MinimizationGraph.hh>
#include <core/scoring/methods/LongRangeTwoBodyEnergy.hh>
#include <core/scoring/LREnergyContainer.hh>
#include <basic/Tracer.hh>
#include <core/conformation/Residue.fwd.hh>
#include <core/optimization/atom_tree_minimize.hh>

/// Utility headers
#include <utility>
#include <utility/vector1.hh>

#include <utility/stream_util.hh> // AUTO IWYU For operator<<
#include <core/optimization/DOF_Node.hh> // AUTO IWYU For DOF_Node

static basic::Tracer TR("protocols.ncbb.SecStructMinimizeMultiFunc");

namespace protocols {
namespace ncbb {

using namespace core;
using namespace core::optimization;

bool
SecStructMinimizeMultiFunc::uniq_refers_to_beta (
	char const uniq
) const {
	for ( core::Size i = 1; i <= dihedral_pattern_.length(); ++i ) {
		if ( dihedral_pattern_[i] == uniq ) {
			if ( alpha_beta_pattern_[i] == 'A' || alpha_beta_pattern_[i] == 'P' ) return false;
			else return true;
		}
	}
	return false;
}

SecStructMinimizeMultiFunc::~SecStructMinimizeMultiFunc() = default;

SecStructMinimizeMultiFunc::SecStructMinimizeMultiFunc(
	core::pose::Pose &pose,
	scoring::ScoreFunction & scorefxn_in,
	core::optimization::MinimizerMap & min_map,
	std::string const alpha_beta_pattern,
	std::string const dihedral_pattern
) :
	pose_( pose ),
	scorefxn_( scorefxn_in ),
	min_map_( min_map ),
	pose0_( pose ),
	alpha_beta_pattern_( alpha_beta_pattern ),
	dihedral_pattern_( dihedral_pattern )
{

	setup_minimization_graph( pose_, scorefxn_, min_map_ );

	utility::vector1< char > uniqs;
	core::Size num;
	count_uniq_char( dihedral_pattern_, num, uniqs );
	utility::vector1< core::Size > starters( uniqs.size() );

	starters[ 1 ] = 1;
	for ( core::Size i = 2; i <= uniqs.size(); ++i ) {
		starters[ i ] = starters[ i-1 ] + ( uniq_refers_to_beta( uniqs[ i-1 ] ) ? 3 : 2 );
	}

	nvar_ = 0;
	for ( core::Size i = 1; i <= uniqs.size(); ++i ) {
		if ( uniq_refers_to_beta( uniqs[i] ) ) {
			nvar_ += 3;
		} else {
			nvar_ += 2;
		}
	}
	TR.Trace << "nvar_ = " << nvar_ << std::endl;

	for ( core::Size i = 1; i <= uniqs.size(); ++i ) {
		for ( core::Size j = 1; j <= dihedral_pattern_.size(); ++j ) {
			if ( dihedral_pattern_[ j-1 ] != uniqs[ i ] ) continue;

			//if ( j > 1 ) {
			// assume phi(1) doomed whatever we do
			id::TorsionID bb1( j, id::BB, 1 );
			vars_index_to_torsion_id_[ starters[ i ] ].push_back( bb1 );
			TR.Trace << "So variable " << starters[ i ] << " controls residue " << j << " torsion 1 " << std::endl;
			//}

			//if ( j < pose.size() ) {
			// assume essentially all nonphi(last) doomed whatever we do
			id::TorsionID bb2( j, id::BB, 2 );
			vars_index_to_torsion_id_[ starters[ i ] + 1 ].push_back( bb2 );
			TR.Trace << "So variable " << (starters[ i ]+1) << " controls residue " << j << " torsion 2 " << std::endl;
			if ( alpha_beta_pattern_[ j-1 ] == 'B' ) {
				id::TorsionID bb3( j, id::BB, 3 );
				vars_index_to_torsion_id_[ starters[ i ] + 2 ].push_back( bb3 );
				TR.Trace << "So variable " << (starters[ i ]+2) << " controls residue " << j << " torsion 3 " << std::endl;
			}
			//}
		}
	}

	get_dofs_map();
	get_dofs_for_pose0();
}

Real
SecStructMinimizeMultiFunc::operator ()( Multivec const & vars ) const
{
	Multivec dofs( vars_to_dofs( vars ) );
	min_map_.copy_dofs_to_pose( pose_, dofs );

	TR << "    vars " << vars << std::endl;
	TR << "scorefxn " << scorefxn_( pose_ ) << std::endl;
	return scorefxn_( pose_ );
}

//scoring::MinimizationGraphOP
void
SecStructMinimizeMultiFunc::setup_minimization_graph(
	pose::Pose & pose,
	scoring::ScoreFunction const & sfxn,
	MinimizerMap const & min_map
) const {
	scoring::MinimizationGraphOP mingraph( new scoring::MinimizationGraph( pose.size() ) );

	scoring::EnergyMap dummy;
	for ( core::Size ii = 1; ii <= pose.size(); ++ii ) {
		sfxn.setup_for_minimizing_for_node( * mingraph->get_minimization_node( ii ),
			pose.residue( ii ), pose.residue_data( ii ), min_map, pose, false, dummy );
	}

	for ( utility::graph::Graph::EdgeListIter
			eiter = mingraph->edge_list_begin(), eiter_end = mingraph->edge_list_end();
			eiter != eiter_end; ++eiter ) {
		core::Size const node1 = (*eiter)->get_first_node_ind();
		core::Size const node2 = (*eiter)->get_second_node_ind();

		auto & minedge( static_cast< scoring::MinimizationEdge & > (**eiter) );

		sfxn.setup_for_minimizing_sr2b_enmeths_for_minedge(
			pose.residue( node1 ), pose.residue( node2 ),
			minedge, min_map, pose, true, false,
			static_cast< scoring::EnergyEdge const * > ( nullptr ), dummy );
	}

	/// Now initialize the long-range edges
	for ( core::Size ii = 1; ii <= pose.size(); ++ii ) {
		for ( auto
				iter = sfxn.long_range_energies_begin(),
				iter_end = sfxn.long_range_energies_end();
				iter != iter_end; ++iter ) {

			if ( (*iter)->minimize_in_whole_structure_context( pose ) ) continue;

			scoring::LREnergyContainerCOP lrec = pose.energies().long_range_container( (*iter)->long_range_type() );
			if ( !lrec || lrec->empty() ) continue;

			// Potentially O(N) operation...
			for ( scoring::ResidueNeighborConstIteratorOP
					rni = lrec->const_neighbor_iterator_begin( ii ), // traverse both upper and lower neighbors
					rniend = lrec->const_neighbor_iterator_end( ii );
					(*rni) != (*rniend); ++(*rni) ) {
				core::Size const r1 = rni->lower_neighbor_id();
				core::Size const r2 = rni->upper_neighbor_id();
				core::Size const jj = ( r1 == ii ? r2 : r1 );
				bool const res_moving_wrt_eachother( true );

				if ( jj < ii ) continue; // only setup each edge once.

				conformation::Residue const & lower_res( r1 == ii ? pose.residue( ii ) : pose.residue( jj ) );
				conformation::Residue const & upper_res( r1 == ii ? pose.residue( jj ) : pose.residue( ii ) );
				sfxn.setup_for_lr2benmeth_minimization_for_respair(
					lower_res, upper_res, *iter, *mingraph, min_map, pose,
					res_moving_wrt_eachother, false, rni, dummy );
			}
		}
	}

	pose.energies().set_minimization_graph( mingraph );
	//return mingraph;
}


void
SecStructMinimizeMultiFunc::dfunc( Multivec const & vars, Multivec & dE_dvars ) const
{
	Multivec dE_ddofs;
	Multivec dofs = vars_to_dofs( vars );

	atom_tree_dfunc( pose_, min_map_, scorefxn_, dofs, dE_ddofs );
	dE_dvars = dEddofs_to_dEdvars( dE_ddofs );

	TR.Trace << "    vars " << vars << std::endl;
	TR.Trace << "dE_dvars " << dE_dvars << std::endl;
	return;
}

void
SecStructMinimizeMultiFunc::get_dofs_for_pose0()
{
	dofs_for_pose0_.resize( min_map_.dof_nodes().size() );
	min_map_.copy_dofs_from_pose( pose0_, dofs_for_pose0_ );
}

Multivec
SecStructMinimizeMultiFunc::vars_to_dofs( Multivec const & vars ) const {

	Multivec dofs( dofs_for_pose0_ );

	for ( core::Size i_var = 1; i_var <= vars.size(); ++i_var ) {
		auto dofs_for_this_var = map_BB_to_DOF_.find( i_var );
		if ( dofs_for_this_var == map_BB_to_DOF_.end() ) continue;

		utility::vector1< core::Size > const i_dofs( dofs_for_this_var->second );
		// impose vars on dofs.
		for ( core::Size const dof : i_dofs ) {
			dofs[ dof ] = vars[ i_var ];
		}
	}

	return dofs;
} // end vars_to_dofs

Multivec
SecStructMinimizeMultiFunc::dofs_to_vars( Multivec const & dofs ) const
{
	// Next, iter over vars to convert dofs info to vars info
	Multivec vars( nvar_, 0.0 );

	for ( core::Size i_var = 1; i_var <= nvar_; ++i_var ) {
		auto dofs_for_this_var = map_BB_to_DOF_.find( i_var );
		if ( dofs_for_this_var == map_BB_to_DOF_.end() ) continue;

		utility::vector1< core::Size > const i_dofs( dofs_for_this_var->second );

		for ( core::Size const dof : i_dofs ) {
			vars[ i_var ] += dofs[ dof ];
		}
		vars[ i_var ] /= i_dofs.size();
	}

	return vars;
} // end dof_to_vars

Multivec
SecStructMinimizeMultiFunc::dEddofs_to_dEdvars( Multivec const & dEddofs ) const
{
	Multivec dEdvars( nvar_, 0.0 );

	// i_var: index in reduced coordinate set
	// obtain dofs corresponding
	// derivative d var is average derivative d correpsonding dofs.
	for ( core::Size i_var = 1; i_var <= nvar_; ++i_var ) {
		auto dofs_for_this_var = map_BB_to_DOF_.find( i_var );
		if ( dofs_for_this_var == map_BB_to_DOF_.end() ) continue;
		utility::vector1< core::Size > const i_dofs( dofs_for_this_var->second );

		// average!
		for ( core::Size const dof : i_dofs ) {
			dEdvars[ i_var ] += dEddofs[ dof ];
		}
		//dEdvars[ i_var ] /= i_dofs.size();
	}

	return dEdvars;
}

void
SecStructMinimizeMultiFunc::get_dofs_map()
{
	std::list< DOF_NodeOP > dof_nodes( min_map_.dof_nodes() );

	// Map between var index and DOF
	map_BB_to_DOF_.clear();
	map_DOF_to_BB_.clear();

	core::Size imap = 1;

	for ( auto const & elem : dof_nodes ) {
		id::TorsionID const tor_id( elem->torsion_id() );

		for ( core::Size i = 1; i <= nvar_; ++i ) {
			TR.Trace << "Initializing mapped DOFs for " << i << std::endl;

			utility::vector1< id::TorsionID > torsions = vars_index_to_torsion_id_[ i ];
			TR.Trace << "Grabbed " << torsions.size() << " torsions " << std::endl;
			for ( core::Size j = 1; j <= torsions.size(); ++j ) {
				id::TorsionID id = torsions[ j ];

				if ( id == tor_id ) {
					TR.Trace << "Passing " << i << " to get a dof will also get you dof number " << imap << std::endl;
					map_BB_to_DOF_[ i ].push_back( imap );

					TR.Trace << "Passing " << imap << " to get a var number will get you var number " << i << std::endl;
					map_DOF_to_BB_[ imap ] = i;
					break;
				}
			}
		}

	}
}


/// @details Useful debugging code that can be re-enabled by changing the boolean
/// variables at the top of this function.
void
SecStructMinimizeMultiFunc::dump( Multivec const &/*vars*/, Multivec const &/*vars2*/ ) const {
	return;
}

} // namespace ncbb
} // namespace protocols

