// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file core/simple_metrics/per_residue_metrics/PerResidueBFactorMetric.cc
/// @brief Simple Per Residue Real Metric to get the per residue b-factor/temperature column from a pose.
/// @author Paul-Replicate (ptoth512@gmail.com)

// Unit headers
#include <core/simple_metrics/per_residue_metrics/PerResidueBFactorMetric.hh>
#include <core/simple_metrics/per_residue_metrics/PerResidueBFactorMetricCreator.hh>
#include <core/simple_metrics/simple_metric_creators.hh>

// Core headers
#include <core/simple_metrics/PerResidueRealMetric.hh>
#include <core/simple_metrics/util.hh>
#include <core/pose/Pose.hh>
#include <core/pose/PDBInfo.hh>
#include <core/select/residue_selector/ResidueSelector.hh>
#include <core/select/residue_selector/util.hh>
#include <core/conformation/Residue.hh>
#include <core/select/util.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <basic/datacache/DataMap.hh>
#include <utility/tag/Tag.hh>
#include <utility/string_util.hh>
#include <utility/pointer/memory.hh>

// XSD Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>
#include <basic/citation_manager/CitationCollection.hh>

#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

static basic::Tracer TR( "core.simple_metrics.per_residue_metrics.PerResidueBFactorMetric" );


namespace core {
namespace simple_metrics {
namespace per_residue_metrics {

using namespace core::select;
using namespace core::select::residue_selector;

	/////////////////////
	/// Constructors  ///
	/////////////////////

/// @brief Default constructor
PerResidueBFactorMetric::PerResidueBFactorMetric():
	core::simple_metrics::PerResidueRealMetric()
{}

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer members)
PerResidueBFactorMetric::~PerResidueBFactorMetric(){}

core::simple_metrics::SimpleMetricOP
PerResidueBFactorMetric::clone() const {
	return utility::pointer::make_shared< PerResidueBFactorMetric >( *this );
}

std::string
PerResidueBFactorMetric::name() const {
	return name_static();
}

std::string
PerResidueBFactorMetric::name_static() {
	return "PerResidueBFactorMetric";

}
std::string
PerResidueBFactorMetric::metric() const {

	return "bfactor";
}

void
PerResidueBFactorMetric::parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & datamap)
{

	SimpleMetric::parse_base_tag( tag );
	PerResidueRealMetric::parse_per_residue_tag( tag, datamap );
	if( tag->hasOption("atom_type") ) {
		set_atom_type( tag->getOption<std::string>("atom_type") );
	}

	//if (tag->hasOption("bogus_option")){
	//	return;
	//}
}

void
PerResidueBFactorMetric::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) {
	using namespace utility::tag;
	using namespace core::select::residue_selector;

	AttributeList attlist;
	attlist + XMLSchemaAttribute::attribute_w_default("atom_type", xs_string, 
					"An atom type sting to specify which atoms to get b-factor/temperature column values from.", "CA");

	//attributes_for_parse_residue_selector( attlist, "residue_selector",
	//	"Selector specifying residues." );

	core::simple_metrics::xsd_per_residue_real_metric_type_definition_w_attributes(xsd, name_static(),
		"Simple Per Residue Real Metric to get the per residue b-factor/temperature column from a pose.", attlist);
}

std::map< core::Size, core::Real >
PerResidueBFactorMetric::calculate(const core::pose::Pose & pose) const {
	// Apply a mask of the residues by user selection.
	utility::vector1< core::Size > selection1 = selection_positions(get_selector()->apply(pose));

	std::map< core::Size, core::Real > bfactor_map; // Create empty real map for residues and their bfactors.
	
	// Loop over residues in selection1 mask by index.
	for(core::Size res=1; res <= selection1.size(); ++res) {
		// Check if atom_type_ selection exists within the currently selected residue.
		// Defaults to atom_type "CA" if non is specified.
		if(pose.residue(selection1[res]).has(atom_type_)) {
			// Get bfactor from pdb_info of current pose. 
			// Use bfactor method from PDBInfoOP: 
			// 	Give core::Size as res index
			// 	Give core::Size as atom index
			// 		Select atom index based on atom_type specification.
			bfactor_map[selection1[res]] = pose.pdb_info()->bfactor( 
				selection1[res], 
				pose.residue_type(selection1[res]).atom_index(atom_type_) );
		} else {
				// Skip residues that don't contain the specified atom_type
				TR << "Atom Type \"" << atom_type_ << "\" missing from residue " 
					 << selection1[res] << ": Skipping." << std::endl;
		}
	}
	return bfactor_map;	
}

std::string
PerResidueBFactorMetric::get_atom_type() const {
	return atom_type_;
}

void
PerResidueBFactorMetric::set_atom_type( std::string atom_type ) {
	atom_type_ = atom_type;
}

/// @brief This simple metric is unpublished.  It returns Paul-Replicate as its author.
void
PerResidueBFactorMetric::provide_citation_info( basic::citation_manager::CitationCollectionList & citations ) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"PerResidueBFactorMetric", basic::citation_manager::CitedModuleType::SimpleMetric,
		"Paul Toth",
		"TODO: The Ohio State University",
		"ptoth512@gmail.com",
		"Wrote the PerResidueBFactorMetric."
		)
	);
}

void
PerResidueBFactorMetricCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const {
	PerResidueBFactorMetric::provide_xml_schema( xsd );
}

std::string
PerResidueBFactorMetricCreator::keyname() const {
	return PerResidueBFactorMetric::name_static();
}

core::simple_metrics::SimpleMetricOP
PerResidueBFactorMetricCreator::create_simple_metric() const {
	return utility::pointer::make_shared< PerResidueBFactorMetric >();
}

} //per_residue_metrics
} //simple_metrics
} //core


#ifdef    SERIALIZATION



template< class Archive >
void
core::simple_metrics::per_residue_metrics::PerResidueBFactorMetric::save( Archive & arc ) const {
	arc( cereal::base_class< core::simple_metrics::PerResidueRealMetric>( this ) );
	//arc( CEREAL_NVP( output_as_pdb_nums_ ) );

}

template< class Archive >
void
core::simple_metrics::per_residue_metrics::PerResidueBFactorMetric::load( Archive & arc ) {
	arc( cereal::base_class< core::simple_metrics::PerResidueRealMetric >( this ) );
	//arc( output_as_pdb_nums_ );


}

SAVE_AND_LOAD_SERIALIZABLE( core::simple_metrics::per_residue_metrics::PerResidueBFactorMetric );
CEREAL_REGISTER_TYPE( core::simple_metrics::per_residue_metrics::PerResidueBFactorMetric )

CEREAL_REGISTER_DYNAMIC_INIT( core_simple_metrics_per_residue_metrics_PerResidueBFactorMetric )
#endif // SERIALIZATION




