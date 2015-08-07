// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/scoring/methods/AACompositionEnergySetup.cc
/// @brief A helper for the EnergyMethod that penalizes deviation from a desired amino acid composition.
/// @details This energy method is inherently not pairwise decomposible.  However, it is intended for very rapid calculation,
/// and has been designed to plug into Alex Ford's modifications to the packer that permit it to work with non-pairwise scoring
/// terms.
/// @author Vikram K. Mulligan (vmullig@uw.edu).

// Unit headers
#include <core/scoring/methods/AACompositionEnergySetup.hh>

// Package headers
#include <core/scoring/methods/EnergyMethod.hh>
#include <core/scoring/EnergyMap.hh>
#include <core/conformation/Residue.hh>
#include <core/chemical/ResidueType.hh>
#include <core/pose/Pose.hh>

// Options system
#include <basic/options/option.hh>
#include <basic/options/keys/score.OptionKeys.gen.hh>

// File I/O
#include <basic/database/open.hh>
#include <utility/io/ozstream.hh>
#include <utility/io/izstream.hh>
#include <utility/string_util.hh>
#include <utility/file/file_sys_util.hh>

// Other Headers
#include <basic/Tracer.hh>
#include <utility/vector1.hh>
#include <utility/pointer/owning_ptr.hh>

// C++ headers
#include <math.h>

namespace core {
namespace scoring {
namespace methods {

static thread_local basic::Tracer TR("core.scoring.methods.AACompositionEnergySetup");

/**************************************************
AACompositionPropertiesSet class:
**************************************************/


/// @brief Default constructor for AACompositionEnergySetupPropertiesSet.
///
AACompositionPropertiesSet::AACompositionPropertiesSet() :
	included_properties_( ),
	excluded_properties_( )
{}

/// @brief Constructor for AACompositionEnergySetupPropertiesSet that takes lists of
/// included and excluded properties.
AACompositionPropertiesSet::AACompositionPropertiesSet(
	utility::vector1< std::string > const &included_properties_strings,
	utility::vector1< std::string > const &excluded_properties_strings
) :
	included_properties_(),
	excluded_properties_()
{
	if(!included_properties_strings.empty()) parse_included_properites( included_properties_strings );
	if(!excluded_properties_strings.empty()) parse_excluded_properites( excluded_properties_strings );
}

/// @brief Copy constructor for AACompositionEnergySetupPropertiesSet.
///
AACompositionPropertiesSet::AACompositionPropertiesSet( AACompositionPropertiesSet const &src ) :
	utility::pointer::ReferenceCount(),
	included_properties_( src.included_properties_ ),
	excluded_properties_( src.included_properties_ )
{}

/// @brief Default destructor for AACompositionEnergySetupPropertiesSet.
///
AACompositionPropertiesSet::~AACompositionPropertiesSet()
{}

/// @brief Clone: create a copy of this object, and return an owning pointer
/// to the copy.
AACompositionPropertiesSetOP AACompositionPropertiesSet::clone() const {
	return AACompositionPropertiesSetOP( new AACompositionPropertiesSet(*this) );
}

/// @brief Add a property to the list of properties that must be present.
///
void AACompositionPropertiesSet::add_included_property( core::chemical::ResidueProperty const property ) {
	runtime_assert_string_msg( !is_in_list(property, included_properties_), "Error in core::scoring::methods::AACompositionPropertiesSet::add_included_property(): Property has already been added!" );
	included_properties_.push_back(property);
	return;
}

/// @brief Add a property to the list of properties that must not be present.
///
void AACompositionPropertiesSet::add_excluded_property( core::chemical::ResidueProperty const property ) {
	runtime_assert_string_msg( !is_in_list(property, excluded_properties_), "Error in core::scoring::methods::AACompositionPropertiesSet::add_excluded_property(): Property has already been added!" );
	excluded_properties_.push_back(property);
	return;
}

/// @brief Take a list of included property strings and parse it.
/// @details Populates the included_properties_ vector based on the properties named in
/// the list.
void AACompositionPropertiesSet::parse_included_properites( utility::vector1< std::string > const &proplist ) {
	included_properties_.clear();
	core::Size const nprop(proplist.size());
	if(nprop==0) return; //Do nothing if passed no properties.
	for(core::Size i=1; i<=nprop; ++i) { //Loop through the property list.
		if(proplist[i] != "") add_included_property( parse_property( proplist[i] ) ); //Convert the string to a ResidueProperty and store it.
	}
	return;
}

/// @brief Take a list of excluded property strings and parse it.
/// @details Populates the excluded_properties_ vector based on the properties named in
/// the list.
void AACompositionPropertiesSet::parse_excluded_properites( utility::vector1< std::string > const &proplist ) {
	excluded_properties_.clear();
	core::Size const nprop(proplist.size());
	if(nprop==0) return; //Do nothing if passed no properties.
	for(core::Size i=1; i<=nprop; ++i) { //Loop through the property list.
		if(proplist[i] != "") add_excluded_property( parse_property( proplist[i] ) ); //Convert the string to a ResidueProperty and store it.
	}
	return;
}

/// @brief Generate a one-line summary of the properties stored in this AACompositionPropertySet
///
std::string AACompositionPropertiesSet::one_line_report() const {
	std::stringstream outstream("");
	
	outstream << "INCLUDED: {";
	
	for(core::Size i=1, imax=included_properties_.size(); i<=imax; ++i) {
		outstream << core::chemical::ResidueProperties::get_string_from_property(included_properties_[i]);
		if(i<imax) outstream << ",";
	}
	outstream << "} EXCLUDED: {";	
	for(core::Size i=1, imax=excluded_properties_.size(); i<=imax; ++i) {
		outstream << core::chemical::ResidueProperties::get_string_from_property(excluded_properties_[i]);
		if(i<imax) outstream << ",";
	}
	outstream << "}" << std::endl;

	return outstream.str();
}

/**************************************************
AACompositionEnergySetup class:
**************************************************/

/// @brief Default constructor for AACompositionEnergySetup.
///
AACompositionEnergySetup::AACompositionEnergySetup() :
	res_type_index_mappings_(),
	type_penalties_(),
	type_deviation_ranges_(),
	expected_by_type_fraction_(),
	expected_by_type_absolute_(),
	property_sets_(),
	expected_by_properties_fraction_(),
	expected_by_properties_absolute_(),
	property_penalties_(),
	property_deviation_ranges_()
{}

/// @brief Copy constructor for AACompositionEnergySetup.
///
AACompositionEnergySetup::AACompositionEnergySetup( AACompositionEnergySetup const &src ) :
        utility::pointer::ReferenceCount(),
	res_type_index_mappings_( src.res_type_index_mappings_ ),
	type_penalties_( src.type_penalties_ ),
	type_deviation_ranges_( src.type_deviation_ranges_ ),
	expected_by_type_fraction_( src.expected_by_type_fraction_ ),
	expected_by_type_absolute_( src.expected_by_type_absolute_ ),
	property_sets_(), //Cloned below
	expected_by_properties_fraction_( src.expected_by_properties_fraction_ ),
	expected_by_properties_absolute_( src.expected_by_properties_absolute_ ),
	property_penalties_( src.property_penalties_ ),
	property_deviation_ranges_( src.property_deviation_ranges_ )
{
	property_sets_.clear();
	for(core::Size i=1, imax=src.property_sets_.size(); i<=imax; ++i) {
		property_sets_.push_back( src.property_sets_[i]->clone() );
	}
}

/// @brief Default destructor for AACompositionEnergySetup.
///
AACompositionEnergySetup::~AACompositionEnergySetup() {}

/// @brief Clone: create a copy of this object, and return an owning pointer
/// to the copy.
AACompositionEnergySetupOP AACompositionEnergySetup::clone() const {
	return AACompositionEnergySetupOP( new AACompositionEnergySetup(*this) );
}

/// @brief Reset all data in this data storage object.
///
void AACompositionEnergySetup::reset() {
	res_type_index_mappings_.clear();
	type_penalties_.clear();
	type_deviation_ranges_.clear();
	expected_by_type_fraction_.clear();
	expected_by_type_absolute_.clear();
	property_sets_.clear();
	expected_by_properties_fraction_.clear();
	expected_by_properties_absolute_.clear();
	property_penalties_.clear();
	property_deviation_ranges_.clear();
	return;
}

/// @brief Initialize from a .comp file.
///
void AACompositionEnergySetup::initialize_from_file( std::string const &filename ) {
	using namespace utility::io;
	
	//First, reset all data:
	reset();
	
	std::string filename_formatted = filename;
	if(utility::file::file_extension(filename_formatted)!="comp") filename_formatted+= ".comp";

	izstream infile;
	infile.open( filename_formatted );
	if(!infile.good()) {
		filename_formatted = "scoring/score_functions/aa_composition/" + utility::file::file_basename(filename_formatted) + ".comp";
		basic::database::open( infile, filename_formatted );
		runtime_assert_string_msg( infile.good(), "Error in core::scoring::methods::AACompositionEnergySetup::initialize_from_file():  Unable to open .comp file for read!" );
	}

	if(TR.Debug.visible()) TR.Debug << "Reading aa_composition scoring term setup data from " << filename_formatted << "." << std::endl;
	std::string curline(""); //Buffer for current line.
	utility::vector1< std::string > lines; //Storing all lines
	
	//Read the file:
	while(getline(infile, curline)) {
		if(curline.size() < 1) continue; //Ignore blank lines.
		//Find and process comments:
		std::string::size_type pound = curline.find('#', 0);
		if( pound == std::string::npos ) {
			lines.push_back( curline );
		} else {
			lines.push_back(curline.substr(0, pound));
		}
	}
	infile.close();
	
	if(TR.Debug.visible()) TR.Debug << "Read complete.  Parsing penalty definitions." << std::endl;
	parse_penalty_definitions( lines );
	
	check_data();
	
	return;
}

/// @brief Get a summary of the data stored in this object
///
std::string AACompositionEnergySetup::report() const {
	std::stringstream output("");
	
	output << "res_type_index_mappings_:" << std::endl;
	for (std::map<std::string,core::Size>::const_iterator it=res_type_index_mappings_.begin(); it!=res_type_index_mappings_.end(); ++it) {
    output << "\t" << it->first << " => " << it->second << std::endl;
	}
	
	output << "type_penalties_:" << std::endl;
	for(core::Size i=1, imax=type_penalties_.size(); i<=imax; ++i) {
		output << "\t" << i << ":" << "\t";
		for(core::Size j=1, jmax=type_penalties_[i].size(); j<=jmax; ++j) {
			output << type_penalties_[i][j];
			if(j<jmax) output << " ";
			else output << std::endl;
		}
	}
	
	output << "type_deviation_ranges_:" << std::endl;
	for(core::Size i=1, imax=type_deviation_ranges_.size(); i<=imax; ++i) {
		output << "\t" << i << ":\t" << type_deviation_ranges_[i].first << ", " << type_deviation_ranges_[i].second << std::endl;
	}
	
	output << "expected_by_type_fraction_ / expected_by_type_absolute_:" << std::endl;
	for(core::Size i=1, imax=expected_by_type_fraction_.size(); i<=imax; ++i) {
		output << "\t" << i << ":\t" << expected_by_type_fraction_[i] << " / " << expected_by_type_absolute_[i] << std::endl;
	}
	
	output << "property_sets_:" << std::endl;
	for(core::Size i=1, imax=property_sets_.size(); i<=imax; ++i) {
		output << "\t" << i << ":\t" << property_sets_[i]->one_line_report();
	}
	
	output << "property_penalties_:" << std::endl;
	for(core::Size i=1, imax=property_penalties_.size(); i<=imax; ++i) {
		output << "\t" << i << ":" << "\t";
		for(core::Size j=1, jmax=property_penalties_[i].size(); j<=jmax; ++j) {
			output << property_penalties_[i][j];
			if(j<jmax) output << " ";
			else output << std::endl;
		}
	}
	
	output << "property_deviation_ranges_:" << std::endl;
	for(core::Size i=1, imax=property_deviation_ranges_.size(); i<=imax; ++i) {
		output << "\t" << i << ":\t" << property_deviation_ranges_[i].first << ", " << property_deviation_ranges_[i].second << std::endl;
	}
	
	output << "expected_by_properties_fraction_ / expected_by_properties_absolute_:" << std::endl;
	for(core::Size i=1, imax=expected_by_properties_fraction_.size(); i<=imax; ++i) {
		output << "\t" << i << ":\t" << expected_by_properties_fraction_[i] << " / " << expected_by_properties_absolute_[i] << std::endl;
	}

	return output.str();
}


/// @brief Parse out penalty definition blocks from the data read from file.
///
void AACompositionEnergySetup::parse_penalty_definitions( utility::vector1 < std::string > const &lines ) {
	core::Size const nlines( lines.size() );
	runtime_assert_string_msg( nlines>0, "Error in core::scoring::methods::AACompositionEnergySetup::parse_penalty_definitions():  No lines were read from file!" );

	bool in_a_block(false);
	utility::vector1 < std::string > lines_subset;
	
	for(core::Size i=1; i<=nlines; ++i) { //Loop through all lines
		std::istringstream curline(lines[i]);
		std::string oneword("");
		
		curline >> oneword;
		if(!in_a_block) { //If we're not already in a PENALTY_DEFINITION block.
			if( oneword == "PENALTY_DEFINITION" ) {
				in_a_block=true;
				lines_subset.clear(); //Starting with the next line, we'll start filling in this vector
			}
		} else { //If we are already in a PENALTY_DEFINITION block
			runtime_assert_string_msg( oneword != "PENALTY_DEFINITION", "Error in core::scoring::methods::AACompositionEnergySetup::parse_penalty_definitions(): One \"PENALTY_DEFINITION\" line was found inside a PENALTY_DEFINITION...END_PENALTY_DEFINITION block." );
			if( oneword == "END_PENALTY_DEFINITION" ) {
				parse_a_penalty_definition( lines_subset ); //Once we reach the end, we parse the set of lines.
				in_a_block=false;
			} else {
				lines_subset.push_back( lines[i] ); //If we haven't yet reached the end, add this line to the set of lines to parse.
			}
		} //if(!in_a_block)
	} //Loop through all lines

	return;
}

/// @brief Parse out penalty definition from a single block of lines from file.
/// @details The lines vector should be the lines BETWEEN the PENALTY_DEFINITION and END_PENALTY_DEFINITION lines.
void AACompositionEnergySetup::parse_a_penalty_definition( utility::vector1 < std::string > const &lines ) {

	// Bools for whether we've found all the lines we're looking for:
	bool typefound(false);
	bool propertiesfound(false);
	bool notpropertiesfound(false);
	bool deltastartfound(false);
	bool deltaendfound(false);
	bool penaltiesfound(false);
	bool fractionfound(false);
	bool absolutefound(false);
	signed long deltastart(0);
	signed long deltaend(0);

	core::Size const nlines( lines.size() ); //Number of lines we'll be going through.
	
	//Temporary storage for lists of properties.
	utility::vector1 < std::string > properties_list;
	utility::vector1 < std::string > not_properties_list;
	
	//Temporary storage for the penalties.
	utility::vector1 < core::Real > penalties_vector;
	
	//Temporary storage for the fraction
	core::Real fraction(0.0);
	
	//Temporary storage for the absolute value
	signed long absolute(0);
	
	for(core::Size i=1; i<=nlines; ++i) { //Loop through all lines
		std::istringstream curline(lines[i]);
		std::string oneword("");
		
		curline >> oneword;
		if(curline.fail()) continue; //A blank line.
		
		if(oneword == "TYPE") {
			runtime_assert_string_msg( !propertiesfound && !notpropertiesfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"TYPE\" line cannot be present in a \"PENALTY_DEFINITION\" block if a \"PROPERTIES\" or \"NOT_PROPERTIES\" line was present in the same block." );
			runtime_assert_string_msg( !typefound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"TYPE\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			curline >> oneword;
			runtime_assert_string_msg( !curline.fail() && oneword.length()<=3 && oneword.length()>=1, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): A three-letter residue type code must be present after \"TYPE\" on a \"TYPE\" line." );
			core::Size const curindex( res_type_index_mappings_.size() + 1 );
			res_type_index_mappings_[oneword]=curindex;
			typefound=true;
		} else if(oneword == "PROPERTIES") {
			runtime_assert_string_msg( !typefound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"PROPERTIES\" line cannot be present in a \"PENALTY_DEFINITION\" block if a \"TYPE\" line was present in the same block." );
			runtime_assert_string_msg( !propertiesfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"PROPERTIES\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			while(!curline.eof()) {
				curline >> oneword;
				runtime_assert_string_msg( !curline.fail(), "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"PROPERTIES\" line." );
				properties_list.push_back( oneword );
			}
			propertiesfound=true;
		} else if(oneword == "NOT_PROPERTIES") {
			runtime_assert_string_msg( !typefound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"NOT_PROPERTIES\" line cannot be present in a \"PENALTY_DEFINITION\" block if a \"TYPE\" line was present in the same block." );
			runtime_assert_string_msg( !notpropertiesfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"NOT_PROPERTIES\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			while(!curline.eof()) {
				curline >> oneword;
				runtime_assert_string_msg( !curline.fail(), "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"PROPERTIES\" line." );
				not_properties_list.push_back( oneword );
			}
			notpropertiesfound=true;
		} else if(oneword == "DELTA_START") {
			runtime_assert_string_msg( !deltastartfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"DELTA_START\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			curline >> deltastart;
			runtime_assert_string_msg( !curline.fail(), "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"DELTA_START\" line." );
			deltastartfound=true;
		} else if(oneword == "DELTA_END") {
			runtime_assert_string_msg( !deltaendfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"DELTA_END\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			curline >> deltaend;
			runtime_assert_string_msg( !curline.fail(), "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"DELTA_END\" line." );
			deltaendfound=true;
		} else if(oneword == "PENALTIES") {
			runtime_assert_string_msg( !penaltiesfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"PENALTIES\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			core::Real curval;
			while(!curline.eof()) {
				curline >> curval;
				runtime_assert_string_msg( !curline.fail(), "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"PENALTIES\" line." );
				penalties_vector.push_back( curval );
			}
			penaltiesfound=true;
		} else if(oneword == "FRACTION") {
			runtime_assert_string_msg( !absolutefound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"FRACTION\" line cannot be present alongside an \"ABSOLUTE\" line in a \"PENALTY_DEFINITION\" block." );
			runtime_assert_string_msg( !fractionfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"FRACTION\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			curline >> fraction;
			runtime_assert_string_msg( !curline.fail() && fraction >= 0.0, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"FRACTION\" line.  (Note that negative values are not permitted)." );
			absolute=-1; //Negative indicates that fraction should be used.	
			fractionfound=true;
		} else if(oneword == "ABSOLUTE") {
			runtime_assert_string_msg( !fractionfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  A \"FRACTION\" line cannot be present alongside an \"ABSOLUTE\" line in a \"PENALTY_DEFINITION\" block." );
			runtime_assert_string_msg( !absolutefound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition():  An \"ABSOLUTE\" line can only be present only once per \"PENALTY_DEFINITION\" block." );
			curline >> absolute; //Positive value means that absolute should be used
			runtime_assert_string_msg( !curline.fail() && absolute >= 0, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Could not parse \"ABSOLUTE\" line.  (Note that negative values are not permitted)." );
			fraction=0.0;
			absolutefound=true;
		}
	} //End loop through all lines

	runtime_assert_string_msg( typefound || propertiesfound || notpropertiesfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Each \"PENALTY_DEFINITION\" block needs to have a \"TYPE\" line or a \"PROPERTIES\" line or a \"NOT_PROPERTIES\" line." );
	runtime_assert_string_msg( deltastartfound && deltaendfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Each \"PENALTY_DEFINITION\" block needs to have a \"DELTA_START\" line and a \"DELTA_END\" line." );
	runtime_assert_string_msg( penaltiesfound, "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Each \"PENALTY_DEFINITION\" block needs to have a \"PENALTIES\" line." );
	runtime_assert_string_msg( (fractionfound || absolutefound) && !(fractionfound && absolutefound), "Error in core::scoring::methods::AACompositionEnergySetup::parse_a_penalty_definition(): Each \"PENALTY_DEFINITION\" block needs to have a \"FRACTION\" line OR an \"ABSOLUTE\" line (but not both)." );

	if(typefound) { //If we found a residue type.
		type_deviation_ranges_.push_back( std::pair<signed long, signed long>(deltastart, deltaend) );
		type_penalties_.push_back( penalties_vector );
		expected_by_type_fraction_.push_back( fraction );
		expected_by_type_absolute_.push_back( absolute );
	} else { //If we wound properties, instead.
		property_deviation_ranges_.push_back( std::pair<signed long, signed long>(deltastart, deltaend) );
		AACompositionPropertiesSetOP new_property_set( new AACompositionPropertiesSet( properties_list, not_properties_list ) ); //Create a new properties set for the properties.
		property_sets_.push_back( new_property_set );
		property_penalties_.push_back( penalties_vector );
		expected_by_properties_fraction_.push_back( fraction );
		expected_by_properties_absolute_.push_back( absolute );
	}

	return;
}

/// @brief Do some final checks to ensure that data were loaded properly.
///
void AACompositionEnergySetup::check_data() const {
	if(TR.Debug.visible()) TR.Debug << "Checking loaded data." << std::endl;

	core::Size const ntypes( res_type_index_mappings_.size() );
	runtime_assert_string_msg( type_penalties_.size() == ntypes, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Penalty data were not found for all residue types." );
	runtime_assert_string_msg( type_deviation_ranges_.size() == ntypes, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Deviation range data were not found for all residue types." );
	runtime_assert_string_msg( expected_by_type_fraction_.size() == ntypes, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Expected fraction data were not found for all residue types." );
	runtime_assert_string_msg( expected_by_type_absolute_.size() == ntypes, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Absolute expected number data were not found for all residue types." );
	
	for(core::Size i=1; i<=ntypes; ++i) {
		runtime_assert_string_msg( type_deviation_ranges_[i].first <= type_deviation_ranges_[i].second, "Error in core::scoring::methods::AACompositionEnergySetup::check_data():  The delta range min must be less than the delta range max for each type.");
		runtime_assert_string_msg( static_cast< signed long >( type_penalties_[i].size() ) == type_deviation_ranges_[i].second-type_deviation_ranges_[i].first+1, "Error in core::scoring::methods::AACompositionEnergySetup::check_data():  Penalties must be provided for every delta in the range from DELTA_START to DELTA_END.  Too many or too few were found.");
	}

	core::Size const nproperties( property_sets_.size() );
	runtime_assert_string_msg( property_penalties_.size() == nproperties, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Penalty data were not found for all property sets." );
	runtime_assert_string_msg( property_deviation_ranges_.size() == nproperties, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Deviation range data were not found for all property sets." );
	runtime_assert_string_msg( expected_by_properties_fraction_.size() == nproperties, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Expected fraction data were not found for all property sets." );
	runtime_assert_string_msg( expected_by_properties_absolute_.size() == nproperties, "Error in core::scoring::methods::AACompositionEnergySetup::check_data(): Absolute expected number data were not found for all property sets." );
	
	for(core::Size i=1; i<=nproperties; ++i) {
		runtime_assert_string_msg( property_deviation_ranges_[i].first <= property_deviation_ranges_[i].second, "Error in core::scoring::methods::AACompositionEnergySetup::check_data():  The delta range min must be less than the delta range max for each property set.");
		runtime_assert_string_msg( static_cast< signed long >( property_penalties_[i].size() ) == property_deviation_ranges_[i].second-property_deviation_ranges_[i].first+1, "Error in core::scoring::methods::AACompositionEnergySetup::check_data():  Penalties must be provided for every delta in the range from DELTA_START to DELTA_END.  Too many or too few were found.");
	}

	if(TR.Debug.visible()) TR.Debug << "Data checks passed!" << std::endl;	
	return;
}



} // methods
} // scoring
} // core