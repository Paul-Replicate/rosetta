// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/scoring/EnergyMap.hh
/// @brief  Vector of scores declaration
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)


#ifndef INCLUDED_core_scoring_EnergyMap_hh
#define INCLUDED_core_scoring_EnergyMap_hh

// Unit Headers
#include <core/scoring/EnergyMap.fwd.hh>

// Package Headers
#include <core/scoring/ScoreType.hh>

#include <iosfwd>

#include <core/types.hh>
#include <utility/numbers.hh>

#include <array>

namespace core {
namespace scoring {

// silly class that's lightweight, manages its memory, and initializes its
// energies to 0.

/// @brief A vector for storing energy data, initially all values are 0
/// @note: several methods using EMapVector objects add in values,
/// be sure to use the zero method between uses
///
/// Common Methods:
///     EMapVector.zero
class EMapVector
{
	using MemberType = std::array< Real, n_score_types >;
public:
	using const_iterator = MemberType::const_iterator;
	using iterator = MemberType::iterator;

public:
	/// @brief default constructor, initializes the energies to 0
	EMapVector()
	{
		clear();
	}

	/// @brief const-iterator to the begining of the vector of energies
	const_iterator begin() const { return map_.cbegin();}
	/// @brief const-iterator to the end of the vector of energies
	const_iterator end() const { return map_.cend(); }
	/// @brief non-const-iterator to the begining of the vector of energies
	iterator begin() { return map_.begin(); }
	/// @brief non-const-iterator to the end of the vector of energies
	iterator end() { return map_.end(); }

	/// @brief Returns the value for ScoreType  <st>
	///
	/// example(s):
	///     emap.get(fa_sol)
	/// See also:
	///     EMapVector
	///     EMapVector.set
	///     ScoreFunction
	///     create_score_function
	Real get(ScoreType st) const { return (*this)[st]; }
	/// @brief Sets the value for ScoreType  <st>  to  <val>
	///
	/// example(s):
	///     emap.set(fa_sol,13.37)
	/// See also:
	///     EMapVector
	///     EMapVector.get
	///     ScoreFunction
	///     create_score_function
	Real set(ScoreType st, Real val) { return (*this)[st] = val; }

	/// @brief [] operator for getting a non-const reference to the energy for a ScoreType
	Real & operator[] ( ScoreType st )
	{
		debug_assert( st > 0 && st <= n_score_types );
		return map_[ st-1 ];
	}

	/// @brief [] operator for getting the value for a ScoreType
	Real operator[] ( ScoreType st ) const
	{
		debug_assert( st > 0 && st <= n_score_types );
		return map_[ st-1 ];
	}

	/// @brief Zero a subset of the positions, as in calibrating a scale
	void
	zero( ScoreTypes const & l )
	{
		for ( auto iter : l ) {
			operator[]( iter ) = 0.0;
		}
	}

	/// @brief Set every value to zero
	///
	/// example(s):
	///     emap.zero()
	/// See also:
	///     EMapVector
	///     EMapVector.get
	///     EMapVector.set
	///     ScoreFunction
	///     create_score_function
	void
	zero()
	{
		clear();
	}

	/// @brief Set every value to zero
	///
	/// example(s):
	///     emap.zero()
	/// See also:
	///     EMapVector
	///     EMapVector.get
	///     EMapVector.set
	///     ScoreFunction
	///     create_score_function
	void
	clear()
	{
		map_.fill( 0 );
	}

	/// @brief Returns the dot product of this object with EMapVector  <src>
	/// @note: useful for multiplying weights with scores
	///
	/// example(s):
	///     we = scorefxn.weights()
	///     emap.dot(we)
	/// See also:
	///     EMapVector
	///     ScoreFunction
	///     ScoreFunction.weights
	///     create_score_function
	Real
	dot( EMapVector const & src ) const
	{
		Real total(0.0);
		for ( int ii=0; ii< n_score_types; ++ii ) {
			total += map_[ii] * src.map_[ii];
		}
		return total;
	}

	/// @brief dot product of two EMapVectors
	/// over a subset of the score types
	Real
	dot( EMapVector const & src,  ScoreTypes const & l ) const
	{
		Real total(0.0);
		for ( auto iter : l ) {
			total += operator[]( iter ) * src[ iter ];
		}
		return total;
	}

	/// @brief += operator, for summing energies
	EMapVector &
	operator += ( EMapVector const & src )
	{
		for ( int ii=0; ii< n_score_types; ++ii ) {
			map_[ii] += src.map_[ii];
		}
		return *this;
	}

	/// @brief -= operator, for subtracting energies
	EMapVector &
	operator -= ( EMapVector const & src )
	{
		for ( int ii=0; ii< n_score_types; ++ii ) {
			map_[ii] -= src.map_[ii];
		}
		return *this;
	}

	/// @brief *= operator, for performing multiplication of a vector by a scalar
	EMapVector &
	operator *= ( Real scalar )
	{
		for ( Real & ii : map_ ) {
			ii *= scalar;
		}
		return *this;
	}

	/// @brief *= operator, for performing element-by-element multiplication of two vectors
	EMapVector &
	operator *= ( EMapVector const & src )
	{
		for ( int ii = 0; ii < n_score_types; ++ii ) {
			map_[ii] *= src.map_[ii];
		}
		return *this;
	}


	/// @brief * operator, for performing multiplication of a vector by a scalar
	EMapVector
	operator * ( Real scalar ) const
	{
		EMapVector result(*this);
		result *= scalar;
		return result;
	}

	/// @brief * operator, for performing element-by-element multiplication of two vectors
	EMapVector
	operator * ( EMapVector const & src ) const
	{
		EMapVector result(*this);
		result *= src;
		return result;
	}


	/// @brief == operator for comparing two energy maps element by element
	bool
	operator == ( EMapVector const & src ) const
	{
		return map_ == src.map_;
	}

	/// @brief != operator for comparing two energy maps element by element
	bool
	operator != ( EMapVector const & src ) const
	{
		return map_ != src.map_;
	}


	/// @brief print the contents of an emap vector to standard out
	void
	print() const;

	/// @brief accumulate a subset of the positions
	void
	accumulate( EMapVector const & src, ScoreTypes const & l )
	{
		for ( auto iter : l ) {
			operator[]( iter ) += src[ iter ];
		}
	}

	/// @brief accumulate a subset of the positions with a common weight factor
	void
	accumulate( EMapVector const & src, ScoreTypes const & l, Real const wt )
	{
		for ( auto iter : l ) {
			operator[]( iter ) += wt * src[ iter ];
		}
	}

	/// @brief Returns the sum of this vector
	///
	/// example(s):
	///     emap.sum()
	/// See also:
	///     EMapVector
	///     EMapVector.get
	///     EMapVector.set
	///     ScoreFunction
	///     create_score_function
	Real
	sum() const
	{
		Real total( 0.0 );
		for ( Real ii : map_ ) {
			total += ii;
		}
		return total;
	}

	/// @brief accumulate the squared values of a subset of the positions
	Real
	norm_squared( ScoreTypes const & l ) const
	{
		Real total( 0.0 );
		for ( auto iter : l ) {
			// could use numeric::square
			Real const val( operator[]( iter ) );
			total += val * val;
		}
		return total;
	}

	/// @brief Check that there aren't any non-finite (inf, NaN) entries.
	bool
	is_finite() const {
		for ( Real const val: map_ ) {
			// We check for zero first here, because we expect that most entries in the map are likely
			// to be zero, and checking for zero is much faster than doing the finite check.
			if ( val != 0.0 && ! utility::isfinite( val ) ) {
				return false;
			}
		}
		return true;
	}

	/// @brief Prints the non-zero positions of the energy map
	/// @brief Set every value to zero
	///
	/// example(s):
	///     emap.show_nonzero()
	/// See also:
	///     EMapVector
	///     EMapVector.get
	///     ScoreFunction
	///     create_score_function
	void
	show_nonzero( std::ostream & out ) const;

	/// @brief convert the non-zero positions of the energy map to a string
	std::string
	show_nonzero() const;

	/// @brief write the energies in this energy map to the output stream for those
	/// score types that have non-zero values in the "weights" energy map.
	void
	show_if_nonzero_weight( std::ostream & out, EMapVector const & weights ) const;

	/// @brief write the weighted energies in this energy map to the output stream for those
	/// score types that have non-zero values in the "weights" energy map.
	void
	show_weighted( std::ostream & out, EMapVector const & weights ) const;

	/// @brief convert the weighted energies in this energy map to a string
	/// for those score types that have non-zero values in the "weights" energy map.
	std::string
	weighted_string_of( EMapVector const & weights ) const;

	/// Grant access to private data to the TwoBodyEMapVector
	//friend class TwoBodyEMapVector;

private:

	/// EMapVector is an array. EMapVector[score_type] = value. Can be used for storing either energy or weight for each score_type.
	MemberType map_;
#ifdef    SERIALIZATION
public:
	template< class Archive > void save( Archive & arc ) const;
	template< class Archive > void load( Archive & arc );
#endif // SERIALIZATION

};


/// output operator (index;value)
std::ostream &
operator << ( std::ostream & ost, EMapVector const &  emap );


} // namespace scoring
} // namespace core

#endif // INCLUDED_core_scoring_EnergyMap_HH
