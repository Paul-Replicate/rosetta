// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/match/ScaffoldBuildPoint.hh
/// @brief  Class declarations for the launch point geometry on the Scaffold.
/// @author Alex Zanghellini (zanghell@u.washington.edu)
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com), porting to mini

#ifndef INCLUDED_protocols_match_upstream_OriginalScaffoldBuildPoint_hh
#define INCLUDED_protocols_match_upstream_OriginalScaffoldBuildPoint_hh

// Unit headers
#include <protocols/match/upstream/OriginalScaffoldBuildPoint.fwd.hh>

// Package headers
#include <protocols/match/upstream/ScaffoldBuildPoint.hh>
#include <protocols/match/upstream/UpstreamBuilder.fwd.hh>

// Project headers
#include <core/types.hh>
#include <core/conformation/Residue.fwd.hh>
#include <core/pose/Pose.fwd.hh>

// Utility headers

// Numeric headers
#include <numeric/xyzVector.hh>



namespace protocols {
namespace match {
namespace upstream {


class ProteinBackboneBuildPoint : public ScaffoldBuildPoint
{
public:
	typedef core::Real         Real;
	typedef core::Vector       Vector;
	typedef ScaffoldBuildPoint parent;
public:
	ProteinBackboneBuildPoint();
	ProteinBackboneBuildPoint( core::Size index );
	~ProteinBackboneBuildPoint() override;

	Real phi() const { return phi_; }
	Real psi() const { return psi_; }

	Vector N_pos()  const { return N_pos_; }
	Vector CA_pos() const { return CA_pos_; }
	Vector C_pos()  const { return C_pos_; }
	Vector O_pos()  const { return O_pos_; }

	Vector H_pos() const { return H_pos_; }
	Vector HA_pos() const { return HA_pos_; }

public:
	/// Setters
	void phi( Real setting );
	void psi( Real setting );

	void N_pos( Vector const & setting );
	void CA_pos( Vector const & setting );
	void C_pos( Vector const & setting );
	void O_pos( Vector const & setting );

	void H_pos( Vector const & setting );
	void HA_pos( Vector const & setting );

private:

	Real phi_, psi_;
	Vector N_pos_;
	Vector CA_pos_;
	Vector C_pos_;
	Vector O_pos_;

	Vector H_pos_;
	Vector HA_pos_;

};

class OriginalBackboneBuildPoint : public ProteinBackboneBuildPoint
{
public:
	typedef ProteinBackboneBuildPoint parent;

public:
	OriginalBackboneBuildPoint( core::Size index );
	~OriginalBackboneBuildPoint() override;
	OriginalBackboneBuildPoint( core::conformation::Residue const & res );
	OriginalBackboneBuildPoint( core::conformation::Residue const & res, core::Size index);

	void
	initialize_from_residue(
		core::conformation::Residue const & res
	);

	bool
	compatible( ScaffoldBuildPoint const & other, bool first_dispatch = true ) const override;

	bool
	compatible( OriginalBackboneBuildPoint const & other, bool first_dispatch = true ) const override;

	core::Size
	original_insertion_point() const override;

	void
	insert(
		core::Size seqpos_to_insert_at,
		Hit const & hit,
		UpstreamBuilderCOP builder,
		core::pose::Pose & pose
	) const override;

	core::Size original_resid() const { return original_resid_; }
	//void original_resid( core::Size setting ) const;

	core::conformation::Residue const &
	input_conformation() const {
		return *input_conformation_;
	}

private:

	core::Size original_resid_;

	core::conformation::ResidueOP input_conformation_;

};

}
}
}

#endif
