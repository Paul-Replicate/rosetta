// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file core/simple_metrics/per_residue_metrics/PerResidueBFactorMetric.hh
/// @brief Simple Per Residue Real Metric to get the per residue b-factor/temperature column from a pose.
/// @author Paul-Replicate (ptoth512@gmail.com)

#ifndef INCLUDED_core_simple_metrics_per_residue_metrics_PerResidueBFactorMetric_HH
#define INCLUDED_core_simple_metrics_per_residue_metrics_PerResidueBFactorMetric_HH

#include <core/simple_metrics/per_residue_metrics/PerResidueBFactorMetric.fwd.hh>
#include <core/simple_metrics/PerResidueRealMetric.hh>

// Core headers
#include <core/types.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/pose/PDBInfo.hh>
#include <core/conformation/Residue.hh>
// Utility headers
#include <utility/tag/XMLSchemaGeneration.fwd.hh>

// C++ headers
#include <map>
#include <string>

#ifdef    SERIALIZATION
// Cereal headers
#include <cereal/types/polymorphic.fwd.hpp>
#endif // SERIALIZATION

namespace core {
namespace simple_metrics {
namespace per_residue_metrics {

///@brief Simple Per Residue Real Metric to get the per residue b-factor/temperature column from a pose.
class PerResidueBFactorMetric : public core::simple_metrics::PerResidueRealMetric{

public:

	/////////////////////
	/// Constructors  ///
	/////////////////////

	/// @brief Default constructor
	PerResidueBFactorMetric();

	/// @brief Destructor (important for properly forward-declaring smart-pointer members)
	~PerResidueBFactorMetric() override;


public:

	/////////////////////
	/// Metric Methods ///
	/////////////////////

	///Defined in RealMetric:
	///
	/// @brief Calculate the metric and add it to the pose as a score.
	///           labeled as prefix+metric+suffix.
	///
	/// @details Score is added through setExtraScorePose and is output
	///            into the score tables/file at pose output.
	//void
	//apply( pose::Pose & pose, prefix="", suffix="" ) override;


	///Defined in PerResidueRealMetric
	//void
	//set_residue_selector( select::residue_selector::ResidueSelectorCOP selector );

	///@brief Calculate the metric.
	/// This map is Rosetta Resnum->value and includes only those residues selected.
	///
	///@details
	/// Return by value as this function can not STORE the result, it only calculates.
	/// Store the result in the pose by using the apply method, which calls this method and stores the result
	/// in the pose as ExtraScoreValues.
	std::map< core::Size, core::Real >
	calculate( core::pose::Pose const & pose ) const override;

	std::string
	get_atom_type() const;

	void
	set_atom_type( std::string atom_type );


public:

	///@brief Name of the class
	std::string
	name() const override;

	///@brief Name of the class for creator.
	static
	std::string
	name_static();

	///@brief Name of the metric
	std::string
	metric() const override;

	/// @brief This simple metric is unpublished.  It returns Paul-Replicate as its author.
	void provide_citation_info(basic::citation_manager::CitationCollectionList & ) const override;

public:

	/// @brief called by parse_my_tag -- should not be used directly
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data ) override;

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

	core::simple_metrics::SimpleMetricOP
	clone() const override;

#ifdef    SERIALIZATION
public:
	template< class Archive > void save( Archive & arc ) const;
	template< class Archive > void load( Archive & arc );
#endif // SERIALIZATION

private:
	std::string atom_type_;

};

} //per_residue_metrics
} //simple_metrics
} //core

#ifdef    SERIALIZATION
CEREAL_FORCE_DYNAMIC_INIT( core_simple_metrics_per_residue_metrics_PerResidueBFactorMetric )
#endif // SERIALIZATION

#endif //core_simple_metrics_per_residue_metrics_PerResidueBFactorMetric_HH
