// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file Patch.cc
///
/// @brief Implementation class for abstract class Residue
///
/// @details
/// My understanding of this piece of code comes from various conversations with Oliver Lang, Summer Thyme, and Andrew
/// Leaver-Fay. If the ideas are incorrect, feel free to correct.
///
/// General Overview
///
/// What are patches? Patches are modifications to the original amino acid residue types.  These modifications include
/// capping the N and C terminus, adding phosphorylation to residues, etc, etc.  The actual files that are read and used
/// are found in: database/chemical/residue_type_sets/fa_standard/patches
///
/// All of these patches are read when ResidueTypeSet is instantiated and are applied to all residues that are
/// identified as needing to be patched.  (Identification of residues is explained later.)  Identification of residues
/// that need to be patched varies by what is in the actual patch file.  For example, peptide residues that are C and/or
/// N termini are patched with the N and/or C terminus patches, which are defined in the files NtermProteinFull.txt and
/// CtermProteinFull.txt.
///
/// Overview of a Patch File
///
/// Let's look at a patch file to see what goes on.  The file we are looking at is NtermProteinFull.txt.  The file is
/// shortened to make it easier to read.  Important parts are shown:
///
/// NAME NtermProteinFull  # actual name of the patch
/// TYPES LOWER_TERMINUS   # the type that this patch belongs to.  Types are defined in VariantType.cc/.hh
///
/// # This section is for general selection rules to apply the patch.
/// BEGIN_SELECTOR  # Here is where we define how to select amino acids for the patch.
///                 # Properties/variant types needed for the patch are found between the BEGIN_SELECTOR and
///                 # END_SELECTOR "titles"
/// PROPERTY PROTEIN  # For this patch to apply, the residue must be a protein, as defined in the paramaters file.
/// NOT VARIANT_TYPE LOWER_TERMINUS  # We do not want to patch the variant type LOWER_TERMINUS.  This is because, we do
///                                  # not want to double patch a residue (if the residue is already variant type
///                                  # lower_terminus).
/// END_SELECTOR  # ending the selection process for the patch
///
/// # This section is to modify specific residues that the patch encounters.
/// BEGIN_CASE ## PROLINE  # Within the BEGIN_CASE and END_CASE section is where the residue is modified.
///                        # These are the operations that occur to apply the patch.
/// BEGIN_SELECTOR  # Once again, we are defining what to do in this specific BEGIN_CASE/END_CASE block by using the
///                 # selector.
/// AA PRO        # We only want to modify aa PRO in the following way between the BEGIN_CASE and END_CASE block.
/// END_SELECTOR  # End selection requirements
///
/// ADD_ATOM 1H   Hpol HC 0.24  # straight forward; add atom 1H
/// ADD_ATOM 2H   Hpol HC 0.24
/// ADD_BOND N 1H               # Atoms need bonds; add those bonds.
/// ADD_BOND N 2H
/// SET_POLYMER_CONNECT LOWER NONE # setting a property
///
/// ## totally making these up:
/// SET_ICOOR 1H 120 60 1 N CA C ## like to use CD but CD's parent is CG  # Once new atoms are placed,
/// SET_ICOOR 2H 120 60 1 N CA 1H                                         # new internal cooridantes need to be made.
///
/// ## modify properties of existing atoms
/// SET_ATOM_TYPE N Nlys
/// SET_MM_ATOM_TYPE N NP
/// SET_ATOMIC_CHARGE N -0.07
/// SET_ATOMIC_CHARGE CA 0.16
/// SET_ATOMIC_CHARGE HA 0.09
/// SET_ATOMIC_CHARGE CD 0.16
/// SET_ATOMIC_CHARGE 1HD 0.09
/// SET_ATOMIC_CHARGE 2HD 0.09
/// ADD_PROPERTY LOWER_TERMINUS ## implies terminus
/// END_CASE
///
/// BEGIN_CASE ### THE GENERAL CASE  # Here is the general case, specified at the very beginning of the patch file.
///
/// ## these are the operations involved
/// DELETE_ATOM H ## deletes all bonds to this atom
/// ADD_ATOM 1H   Hpol HC 0.33
/// ADD_ATOM 2H   Hpol HC 0.33
/// ADD_ATOM 3H   Hpol HC 0.33
/// ADD_BOND N 1H
/// ADD_BOND N 2H
/// ADD_BOND N 3H
/// SET_POLYMER_CONNECT LOWER NONE
///
/// ## totally making these up:
/// SET_ICOOR 1H 120 60 1 N CA C
/// SET_ICOOR 2H 120 60 1 N CA 1H
/// SET_ICOOR 3H 120 60 1 N CA 2H
///
/// ## modify properties of existing atoms
/// SET_ATOM_TYPE N Nlys
/// SET_MM_ATOM_TYPE N NH3
/// SET_ATOMIC_CHARGE N -0.3
/// SET_ATOMIC_CHARGE CA 0.21
/// SET_ATOMIC_CHARGE HA 0.10
/// ADD_PROPERTY LOWER_TERMINUS ## implies terminus
/// END_CASE
///
///
/// So, let's go through what was put up there.  In general, you name your patch and assign it a type.  Then, you tell
/// the patch system in the BEGIN_SELECTOR / END_SELECTOR block what type of properties that you are looking for to
/// apply the patch.  You then, in the BEGIN_CASE / END_CASE block, add operations to the residue.  You can specify
/// specific selectors within the BEGIN_CASE / END_CASE by having another BEGIN_SELECTOR / END_SELECTOR block.  People
/// generally specify specific amino acids within the BEGIN_SELECTOR / END_SELECTOR blocks within the BEGIN_CASE /
/// END_CASE blocks.  At the end, there is one last BEGIN_CASE / END_CASE with no BEGIN_SELECTOR / END_SELECTOR block.
/// This block is used to specify what to do with the general case, which was defined at the very beginning of the file.
/// A few more details follow below.
///
/// First, you must name your patch.  Then you have to give it a "type".  This type is defined in VariantType.cc/.hh.
/// All the type does is adds a name that can be used later on in different protocols.  It also helps the patch system
/// keep track of what residues are patched with what type.  The type has no magical meaning; it's a name that you give
/// it that is "defined" in the variantType.cc.  In short, it's a name handler.  Once the name and type have been
/// assigned, you must define a general case in which your patch applies.  Generally, this means that you want it to
/// apply to a specific type of aa, or all residues of a specific type or property.  After this block comes the
/// BEGIN_CASE / END_CASE statements.  In these statements, you specify what modifications you want to apply to the
/// residues.  If you have different requirements for different amino acids, you must specify this in a BEGIN_SELECTOR /
/// END_SELECTOR block within the BEGIN_CASE / END_CASE block.  You must specify specific amino acids first, then in a
/// new BEGIN_CASE / END_CASE block, you specify the general modifications defined by the first selector.  Confusing,
/// huh?  The reason for having specific selectors defined before the general case is because..., I don't know.  I
/// suspect it's because of how the file is read in and applied.  Regardless, if you want to do something to a specific
/// amino acid/residue before you get to the general case, you need to have a BEGIN_CASE / END_CASE block with a
/// selector in there before you get to the general BEGIN_CASE / END_CASE block.
///
/// Using Patch Selector to Specify Application of a Patch to Specific Residues
///
/// Woah there!  Now that you know how to use patches, you don't need to go all gun-ho crazy and create a ton of
/// patches. This is because all patches are read when ResidueTypeSet is created and applied.  This creates overhead if
/// you have a ton of patch files.  To circumvent this, someone wrote an option called -chemical:patch_selectors.  I
/// don't know who wrote it, or where it's located, sorry.  This option allows you to create patches that are only
/// loaded when you use the option -chemical:patch_selectors.  To get this to work, you create a patch like normal.
/// Then, in the BEGIN_SELECTOR / END_SELECTOR line, you add a line CMDLINE_SELECTOR <name of command line>.  This
/// means, if you use the CMDLINE_SELECTOR sc_orbitals, your patch will only be loaded if you use the option
/// -chemical:patch_selectors sc_orbitals.  Cool, huh?
///
///
/// Trouble Shooting!!!
/// So, you added a new patch and used the command line selector and nothing happens.  Did you modify VariantTypes.cc to
/// have your new type?  Did you name things correctly?  Did you edit the patches.txt to include your patch?  Are you
/// trying to use a command that does not exist?  Look at patchoperations.cc for commands that exist.  These are all
/// things to check for.
///
/// So, you have added a new patch, and you get segmentation faults.  Most common seg fault for me is in the stub atom.
/// This is because your patch might add atoms/delete atoms that are used in other patches.  In order to fix this, you
/// must modify other patches to include your modifications.  Remember patches are all loaded whenever ResidueTypeSet is
/// instantiated, which is usually near the beginning of a Rosetta protocol when a pose is 1st loaded!  If you have
/// conflicts with other patches in your patch, even if you are using the command line selector, you must resolve those
/// problems.  Write an integration test so that people won't screw up your patches.
///
///
/// @author
/// Phil Bradely
/// Steven Combs only added comments
/////////////////////////////////////////////////////////////////////////


// Unit headers
#include <core/chemical/Patch.hh>
#include <core/chemical/PatchOperationFactory.hh>
#include <core/chemical/PatchOperation.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/chemical/ResidueType.hh>
#include <core/chemical/MutableResidueType.hh>

// Basic header
#include <basic/Tracer.hh>

// Utility headers
#include <utility/vector1.hh>
#include <utility/io/izstream.hh>

#include <basic/options/keys/chemical.OptionKeys.gen.hh>
#include <basic/options/option.hh>

#include <core/chemical/ResidueProperties.hh>

// C++ headers



#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/vector1.srlz.hh>
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/string.hpp>
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

namespace core {
namespace chemical {

/// @brief Constructor
///
Patch::Patch() = default;

/// @brief Constructor with ResidueTypeSet name.
///
Patch::Patch( TypeSetMode res_type_set_mode ) :
	res_type_set_mode_(res_type_set_mode)
{}

/// @details Auto-generated virtual destructor
Patch::~Patch() = default;

/// @details Auto-generated virtual destructor
PatchCase::~PatchCase() = default;

static basic::Tracer tr( "core.chemical.Patch" );

std::string tag_from_line( std::string const & line ) {
	std::string tag;
	std::istringstream l( line );
	l >> tag;
	if ( l.fail() ) return "";
	else return tag;
}

std::string
residue_type_base_name( ResidueTypeBase const & rsd_type )
{
	std::string base_name = rsd_type.name().substr( 0, rsd_type.name().find( PATCH_LINKER ) );
	return base_name;
}

std::string
residue_type_all_patches_name( ResidueType const & rsd_type )
{
	Size spos = rsd_type.name().find( PATCH_LINKER );
	if ( spos < rsd_type.name().length() ) return rsd_type.name().substr( spos );
	else return "";
}

utility::vector1< std::string > get_patch_names( ResidueType const & rsd_type ) {
	std::stringstream ss( residue_type_all_patches_name(rsd_type) );
	std::string item;
	utility::vector1< std::string > elems;
	while ( std::getline( ss, item, ':' ) ) {
		if ( item != "" ) elems.push_back( item );
	}
	return elems;
}

/// @brief override patch partial charges with values from commandline
void
setup_patch_atomic_charge_reassignments_from_commandline(
	std::string const & patch_name,
	TypeSetMode rsd_type_set_mode,
	ResidueTypeSelector const & rsd_selector,
	std::map< std::string, Real > & atomic_charge_reassignments
)
{
	if ( !basic::options::option[ basic::options::OptionKeys::chemical::set_patch_atomic_charge ].user() ) return;

	if ( patch_name.empty() ) { return; }

	utility::vector1< std::string > mods
		( basic::options::option[ basic::options::OptionKeys::chemical::set_patch_atomic_charge ] );

	std::string const errmsg( "-set_patch_atomic_charge format should be::  -chemical:set_patch_atomic_charge <rsd-type-set1-name>:<rsd-type1-name>:<patch-name>:<atom1-name>:<new-charge> ... For example: '-chemical:set_atomic_charge fa_standard:PRO:NtermProteinFull:1H:-1' ");

	for ( std::string const & mod : mods ) {
		///
		/// mod should look like (for example):  "fa_standard:PRO:NtermProteinFull:1H:-1"
		///
		utility::vector1<std::string> tokens = utility::string_split( mod, ':');
		if ( tokens.size() != 5 ) utility_exit_with_message(errmsg);

		// 1: check rsd_type_set
		TypeSetMode option_cat( type_set_mode_from_string( tokens[1] ) );
		if ( option_cat != rsd_type_set_mode ) continue;

		// 2: check patch name
		if ( tokens[3] != patch_name ) continue;

		// 3: check residue
		//    this is tricky since we can't get a representative (since we are in the process of building the residue type set)
		//    instead, we make make a dummy residue type with only name1, name3, and aa set
		//    this could be expanded to deal with more complicated selector logic
		//    it is possible as well more complicated selector logic (which queries ResidueType in more detail) could break this ...
		MutableResidueType dummy( nullptr,nullptr,nullptr,nullptr );
		AA aatype = aa_from_name(tokens[2]);
		dummy.aa(aatype);
		dummy.name(tokens[2]);
		dummy.name3(tokens[2]);
		dummy.name1(oneletter_code_from_aa(is_canonical_D_aa(aatype) ? get_L_equivalent(aatype) : aatype));
		if ( core::chemical::is_canonical_D_aa(aatype) ) {
			dummy.add_property("D_AA");
		} else if ( core::chemical::is_canonical_L_aa_or_gly(aatype) ) {
			dummy.add_property("L_AA");
		}

		if ( !rsd_selector[ dummy ] ) continue;

		// 4: add to list
		if ( !ObjexxFCL::is_float( tokens[5] ) ) utility_exit_with_message(errmsg);
		Real const new_atomic_charge( ObjexxFCL::float_of( tokens[5] ) );

		tr.Trace << "setup_atomic_charge_reassignments_from_commandline: setting charge of " << rsd_type_set_mode << ' ' << patch_name << ' ' << tokens[4] << " to " << new_atomic_charge << std::endl;

		atomic_charge_reassignments[ tokens[4] ] = new_atomic_charge;
	}
}


/// @brief create a PatchCase from input lines
/// @details add selector_ from lines enclosed by "BEGIN_SELECTOR" and "END_SELECTOR".\n
/// add operations_ from each input line containing a single operation
PatchCaseOP
case_from_lines(
	utility::vector1< std::string > const & lines,
	TypeSetMode res_type_set_mode,
	std::string const & patch_name
)
{
	PatchCaseOP pcase( new PatchCase() );

	PatchOperationFactory const & pofact = *PatchOperationFactory::get_instance();

	bool in_selector( false );
	for ( std::string const & line: lines ) {
		std::string tag( tag_from_line( line ) );

		if ( tag == "BEGIN_SELECTOR" ) {
			debug_assert( !in_selector );
			in_selector = true;
		} else if ( tag == "END_SELECTOR" ) {
			in_selector = false;
		} else if ( in_selector ) {
			pcase->selector().add_line( line );
		} else {
			//fd : use phil's logic for flag-based changes to patch charges and atom types
			//   : use both selector and patch name to figure out charge mapping
			std::map< std::string, Real > atomic_charge_reassignments;
			setup_patch_atomic_charge_reassignments_from_commandline( patch_name, res_type_set_mode, pcase->selector(), atomic_charge_reassignments );

			std::istringstream l( line );
			l >> tag; // Need to pull the tag to advance the stream to the next location.
			PatchOperationOP operation = pofact.newPatchOperation( tag, line, l, atomic_charge_reassignments );
			if ( operation ) pcase->add_operation( operation );
		}
	}
	return pcase;
}

MutableResidueTypeOP
PatchCase::apply( ResidueType const & rsd_in, bool const instantiate /* = true */ ) const
{
	MutableResidueTypeOP mut_type( new MutableResidueType( rsd_in ) );
	return apply( *mut_type, instantiate );
}

/// @brief add one more operation in this PatchCase
void
PatchCase::add_operation( PatchOperationOP operation )
{
	if ( operation->generates_base_residue_type() ) {
		generates_base_residue_type_ = true; //If the operation generates a new base residue type, then the patch case does.
	}
	operations_.push_back( operation );
}

/// @details First clone the base ResidueType.  Then patching for this case is done by applying all the operations.
/// finalize() is called after the VariantTypes and name are set by Patch::apply().
/// @note    If you call this method without calling finalize(), your ResidueType may not have the correct derived data!
MutableResidueTypeOP
PatchCase::apply( MutableResidueType const & rsd_in, bool const instantiate /* = true */ ) const
{
	MutableResidueTypeOP rsd;
	if ( instantiate ) {
		rsd = rsd_in.clone();
	} else {
		rsd = rsd_in.placeholder_clone();
	}

	for ( auto const & operation : operations_ ) {
		if ( !instantiate && !operation->applies_to_placeholder() ) { continue; }
		bool const fail( operation->apply( *rsd ) );
		if ( fail ) {
			tr.Debug << "Failed when applying PatchOperation " << operation->name() << " to residue " << rsd->name() << std::endl;
			// A more explicit message will be printed in Patch::apply()
			return nullptr;
		}
	}

	return rsd;
}


/// @details Go through patch operations in this PatchCase, and compile list of any atom names that are added.
/// Also, add any atom aliases for those atoms.
utility::vector1< std::string >
PatchCase::adds_atoms() const
{
	utility::vector1< std::string > atom_names;
	for ( auto const & operation : operations_ ) {
		atom_names.append( operation->adds_atoms() );
	}
	return atom_names;
}

/// @details Go through patch operations in this PatchCase, and compile list of any atom names that are deleted.
utility::vector1< std::string >
PatchCase::deletes_atoms() const
{
	utility::vector1< std::string > atom_names;
	for ( auto const & operation : operations_ ) {
		std::string const atom_name = operation->deletes_atom();
		if ( atom_name.size() > 0 ) atom_names.push_back( atom_name );
	}
	return atom_names;
}

/// @details Go through patch operations in this PatchCase, and compile list of any property names that are added.
utility::vector1< std::string >
PatchCase::adds_properties() const
{
	utility::vector1< std::string > property_names;
	for ( auto const & operation : operations_ ) {
		std::string const property_name = operation->adds_property();
		if ( property_name.size() > 0 ) property_names.push_back( property_name );
	}
	return property_names;
}

/// @details Loop through patch operations in this PatchCase, and compile a list of any ResidueProperties that are added.
/// Note that this version uses enums, and only works for canonical properties (not on-the-fly properties).
utility::vector1< ResidueProperty >
PatchCase::adds_properties_enums() const
{
	utility::vector1< ResidueProperty > properties;
	for ( auto const & operation : operations_ ) {
		ResidueProperty const & property( operation->adds_property_enum() );
		if ( property != NO_PROPERTY ) properties.push_back( property );
	}
	return properties;
}

/// @details Go through patch operations in this PatchCase, and compile list of any property names that are deleted.
utility::vector1< std::string >
PatchCase::deletes_properties() const
{
	utility::vector1< std::string > property_names;
	for ( auto const & operation : operations_ ) {
		std::string const & property_name = operation->deletes_property();
		if ( property_name.size() > 0 ) property_names.push_back( property_name );
	}
	return property_names;
}

/// @details Loop through patch operations in this PatchCase, and compile a list of any ResidueProperties that are deleted.
/// Note that this version uses enums, and only works for canonical properties (not on-the-fly properties).
utility::vector1< ResidueProperty >
PatchCase::deletes_properties_enums() const
{
	utility::vector1< ResidueProperty > property_enums;
	for ( auto const & operation : operations_ ) {
		ResidueProperty const & property = operation->deletes_property_enum();
		if ( property != NO_PROPERTY ) property_enums.push_back( property );
	}
	return property_enums;
}

/// @details Go through patch operations in this PatchCase, and compile list of any property names that are deleted.
utility::vector1< std::string >
PatchCase::deletes_variants() const
{
	utility::vector1< std::string > variant_names;
	for ( auto const & operation : operations_ ) {
		std::string const & variant_name = operation->deletes_variant();
		if ( variant_name.size() > 0 ) variant_names.push_back( variant_name );
	}
	return variant_names;
}

/// @details Go through patch operations in this PatchCase, and compile list of any property names that are deleted.
/// This version works by enum, and doesn't support on-the-fly VariantTypes.
utility::vector1< core::chemical::VariantType >
PatchCase::deletes_variants_by_enum() const
{
	utility::vector1< core::chemical::VariantType > variants;
	for ( auto const & operation : operations_ ) {
		VariantType const & varianttype( operation->deletes_variant_enum() );
		if ( varianttype != core::chemical::NO_VARIANT ) variants.push_back( varianttype );
	}
	return variants;
}

/// @brief returns list of new name3, useful for identifying patches that go with PDB residues
std::string
PatchCase::generates_new_name3() const {
	std::string new_name3( "" );
	for ( auto const & operation : operations_ ) {
		std::string const & name3 = operation->generates_name3();
		if ( name3.size() > 0 ) {
			runtime_assert( new_name3.size() == 0 ); // do not allow Patch to define more than one new name3
			new_name3 =  name3;
		}
	}
	return new_name3;
}

/// @brief returns list of new interchangeability_group, useful for identifying patches that go with PDB residues
std::string
PatchCase::generates_interchangeability_group() const {
	std::string new_interchangeability_group( "" );
	for ( auto const & operation : operations_ ) {
		std::string const interchangeability_group = operation->generates_interchangeability_group();
		if ( interchangeability_group.size() > 0 ) {
			runtime_assert( new_interchangeability_group.size() == 0 ); // do not allow Patch to define more than one new interchangeability_group
			new_interchangeability_group =  interchangeability_group;
		}
	}
	return new_interchangeability_group;
}

/// @brief returns list of new interchangeability_group, useful for identifying patches that go with PDB residues
bool
PatchCase::may_change_aa() const {
	for ( auto const & operation : operations_ ) {
		if ( operation->may_change_aa() ) return true;
	}
	return false;
}

bool
PatchCase::changes_connections_on( ResidueType const & rsd_type, std::string const & atom ) const {
	for ( auto const & operation : operations_ ) {
		if ( operation->changes_connections_on( rsd_type, atom ) ) return true;
	}
	return false;
}


/// @details - first read in all lines from the file, discarding # comment lines
/// - parse input lines for Patch name and variant types (NAME, TYPES)
/// - parse input lines to create each case accordingly (BEGIN_CASE, END_CASE)
/// @note keep the order to avoid triggering parsing errors
void
Patch::read_file( std::string const & filename )
{
	// clear old data
	tr.Debug << "Reading patch file: " << filename << std::endl;

	name_ = "";
	types_.clear();
	custom_types_.clear();
	selector_.clear();
	cases_.clear();
	replaces_residue_type_ = false;

	utility::vector1< std::string > lines;
	{ // read the lines file
		utility::io::izstream data( filename.c_str() );
		if ( !data.good() ) {
			utility_exit_with_message("Cannot find patch file: "+filename);
		}
		std::string line;
		while ( getline( data,line ) ) {
			std::string const tag( tag_from_line( line ) );
			if ( tag.size() && tag[0] != '#' ) lines.push_back( line );
		}
	}

	// misc parsing
	for ( uint i=1; i<= lines.size(); ++i ) {
		std::istringstream l(lines[i]);
		std::string tag;
		l >> tag;
		if ( tag == "NAME" ) {
			l >> name_;
			is_metapatch_ = (name_.substr( 0, 3 ) == "MP-");
		} else if ( tag == "TYPES" ) {
			std::string t;
			l >> t;
			while ( !l.fail() ) {
				types_.push_back( ResidueProperties::get_variant_from_string(t) );
				l >> t;
			}
		} else if ( tag == "REPLACE_RES_TYPE" ) {
			replaces_residue_type_ = true;
		}
	}

	// build the residue selector
	{
		bool in_selector( false );
		for ( uint i=1; i<= lines.size(); ++i ) {
			std::string tag( tag_from_line( lines[i] ) );
			if ( tag == "BEGIN_CASE" ) {
				break;
			} else if ( tag == "BEGIN_SELECTOR" ) {
				debug_assert( !in_selector );
				in_selector = true;
			} else if ( tag == "END_SELECTOR" ) {
				debug_assert( in_selector );
				in_selector = false;
			} else if ( in_selector ) {
				selector_.add_line( lines[i] );
			}
		}
	}

	// get the cases
	utility::vector1< std::string > case_lines;
	bool in_case( false );
	while ( !lines.empty() ) {
		// look for a case
		std::string tag( tag_from_line( lines[1] ) );
		if ( tag == "BEGIN_CASE" ) {
			debug_assert( case_lines.empty() );
			debug_assert( !in_case );
			in_case = true;
		} else if ( tag == "END_CASE" ) {
			PatchCaseOP new_case( case_from_lines( case_lines, res_type_set_mode_, name_ ) );
			if ( new_case ) { add_case( new_case ); }
			case_lines.clear();
			in_case = false;
		} else if ( in_case ) case_lines.push_back( lines[1] );

		lines.erase( lines.begin() );
	}
}

/// @brief Add a VariantType to the list that this patch applies.
/// @author Vikram K. Mulligan (vmullig@uw.edu).
/// @author Andy Watkins (amw579@stanford.edu)
void
Patch::add_type( core::chemical::VariantType const type ) {
	if ( types_.contains( type ) ) return;
	types_.push_back(type);
}

/// @brief Add a custom VariantType to the list that this patch applies.
/// @author Vikram K. Mulligan (vmullig@uw.edu).
/// @author Andy Watkins (amw579@stanford.edu)
void
Patch::add_custom_type( std::string const & custom_type ) {
	if ( custom_types_.contains( custom_type ) ) return;
	custom_types_.push_back(custom_type);
}

MutableResidueTypeOP
Patch::apply( ResidueType const & rsd_type, bool const instantiate /* = true */ ) const
{
	MutableResidueTypeOP mut_type( new MutableResidueType( rsd_type ) );
	return apply( *mut_type, instantiate );
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, the corresponding patch
/// operations are applied to create a new variant type of the basic ResidueType.  The new types's name and its
/// variant type info are updated together with all other primary and derived ResidueType data.
/// Finally, call finalize() to update all primary and derived data for the new ResidueType.
MutableResidueTypeOP
Patch::apply( MutableResidueType const & rsd_type, bool const instantiate /* = true */ ) const
{
	if ( !applies_to( rsd_type ) ) { return nullptr; }  // I don't know how to patch this residue.
	using namespace basic;

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			tr.Trace << "Attempting to patch: " << rsd_type.name() << " with: " << name() << std::endl;

			MutableResidueTypeOP patched_rsd_type;
			try {
				patched_rsd_type = iter->apply( rsd_type, instantiate );
			} catch ( utility::excn::Exception & excn ) {
				tr.Warning << "Patch " << name() << " implies it can apply to residue type " << rsd_type.name() << ", but actually applying it fails with error message:" << std::endl;
				tr.Warning << excn.msg() << std::endl;
				tr.Warning << "   You may want to check your patch definitions." << std::endl;
				continue;
			} catch ( std::exception const & excn ) {
				tr.Warning << "Patch " << name() << " implies it can apply to residue type " << rsd_type.name() << ", but actually applying it fails with error message:" << std::endl;
				tr.Warning << excn.what() << std::endl;
				tr.Warning << "   You may want to check your patch definitions." << std::endl;
				throw;
			} catch (...) {
				tr.Warning << "Patch " << name() << " implies it can apply to residue type " << rsd_type.name() << ", but actually applying it fails with an unknown exception." << std::endl;
				tr.Warning << "   You may want to check your patch definitions." << std::endl;
				throw;
			}

			if ( ! patched_rsd_type ) {
				tr.Warning << "Patch " << name() << " implies it can apply to residue type " << rsd_type.name() << ", but actually applying it fails." << std::endl;
				tr.Warning << "   You may want to check your patch definitions." << std::endl;
				continue;
			}

			// Each patch should leave the residue type in a valid state - catch that failure here to be more robust to patch failures.
			if ( ! patched_rsd_type->validate_residue_type() ) {
				tr.Warning << "Patch " << name() << " implies it can apply to residue type " << rsd_type.name() << ", but actually applying it results in an invalid residue type." << std::endl;
				tr.Warning << "   You may want to check your patch definitions." << std::endl;
				continue;
			}

			// patch succeeded!
			if ( !replaces_residue_type_ ) { // This is bananas. Shouldn't just forget that patch was applied. -- rhiju.
				for ( auto const & type : types_ ) { // Just the enum types.
					patched_rsd_type->add_variant_type( type );
				}
				if ( ! custom_types_.empty() ) {
					// Custom variant type--must account for.
					// Issue: this means that variants in Patch files that are misspelled or
					// something won't be caught--they'll just silently and ineffectually
					// be added.
					// AMW: No longer an issue. Check if this Patch is metapatch-derived
					// if so, enable custom variant types. if not, let it burn!
					if ( name_.substr( 0, 3 ) == "MP-" ) { // Are we a metapatch?
						patched_rsd_type->enable_custom_variant_types();
					} // If not enabled, the add_variant_type() will error out.
					for ( auto const & type: custom_types_ ) {
						patched_rsd_type->add_variant_type( type ); // Robust addition if it already has it.
					}
				}
				patched_rsd_type->name( patched_name(*patched_rsd_type) );
			}

			if ( instantiate ) {
				tr.Debug << "successfully patched: " << rsd_type.name() <<
					" to: " << patched_rsd_type->name() << std::endl;
			}

			return patched_rsd_type;
		}
	}

	// I am commenting out this exit below, because I have patches with complicated selection criteria.  It can happen
	// that a patch's main selector applies to a ResidueType yet none of the sub-cases apply.  This should not fail;
	// the ResidueType should just be skipped.  If this point is reached, a null pointer is returned, which will be
	// properly handled by ResidueTypeSet, which checks that the new ResidueType exists before adding it to the Set.
	// If there is a genuine error in patching, the PatchOperations (or ResidueType) will error during the actual
	// patching, so I believe this is completely safe. ~Labonte
	//utility_exit_with_message( "no patch applied? " + name_ + " to " + rsd_type.name() );
	return nullptr;
}

std::string
Patch::patched_name( ResidueTypeBase const & rsd ) const {
	// AMW: Special case for the chiral flip patches. In ONLY THESE CASE,
	// application PREpends the letter D or L. No ':'.
	if ( name_ == "D" ) {
		return "D" + rsd.name();
	} else if ( name_ == "L" ) {
		return "L" + rsd.name();
	} else if ( name_ == "RPEPTOID" ) {
		// Added by VKM, 14 February 2018: When patching an S-peptoid to an R-peptoid, increment the peptoid number by 1 and return the name with the increment.
#ifndef NDEBUG
		int peptoid_number( std::atoi( rsd.base_name().c_str() ) );
#endif
		debug_assert( peptoid_number > 600 && peptoid_number < 700 );
		debug_assert( peptoid_number % 2 == 0 );
		size_t const colon_pos( rsd.name().find( PATCH_LINKER ) );
		return rsd.base_name() /*Note that the RPEPTOID patch has already incremented the base name number*/ + ( colon_pos == std::string::npos ? "" : rsd.name().substr( colon_pos ) );
	} else if ( generates_base_residue_type() ) {
		// In the case of other patches that generate a new residue type, the residue name should
		// already be set by the patch.
		return rsd.name();
	} else {
		return rsd.name() + PATCH_LINKER + name_;
	}
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any atom_names that are added.
utility::vector1< std::string >
Patch::adds_atoms( ResidueType const & rsd_type ) const
{
	utility::vector1< std::string > atom_names;
	if ( ! applies_to( rsd_type ) ) { return atom_names; }  // I don't know how to patch this residue.

	for ( auto const & patch_case : cases_ ) {
		if ( patch_case->applies_to( rsd_type ) ) {
			// This patch case applies to this rsd_type.
			atom_names.append( patch_case->adds_atoms() );
		}
	}

	return atom_names;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any atom_names that are deleted.
utility::vector1< std::string >
Patch::deletes_atoms( ResidueType const & rsd_type ) const
{
	utility::vector1< std::string > atom_names;
	if ( !applies_to( rsd_type ) ) return atom_names;  // I don't know how to patch this residue.

	for ( auto const & iter : cases_ ) {

		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< std::string > atom_names_for_patch_case = iter->deletes_atoms();
			for ( Size n = 1; n <= atom_names_for_patch_case.size(); n++ ) {
				atom_names.push_back(  atom_names_for_patch_case[ n ] );
			}
		}
	}

	return atom_names;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any properties that are added.
utility::vector1< std::string >
Patch::adds_properties( ResidueType const & rsd_type ) const
{
	utility::vector1< std::string > properties;
	if ( !applies_to( rsd_type ) ) return properties;  // I don't know how to patch this residue.

	for ( auto const & iter : cases_ ) {

		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< std::string > properties_for_patch_case = iter->adds_properties();
			for ( Size n = 1; n <= properties_for_patch_case.size(); n++ ) {
				properties.push_back(  properties_for_patch_case[ n ] );
			}
		}
	}

	return properties;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any properties that are added, by enum.  Faster than string version.
utility::vector1< ResidueProperty >
Patch::adds_properties_enums( ResidueType const & rsd_type ) const
{
	utility::vector1< ResidueProperty > properties;
	if ( !applies_to( rsd_type ) ) return properties;  // I don't know how to patch this residue.

	for ( auto const & patch_case : cases_ ) {
		if ( patch_case->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< ResidueProperty > const & properties_for_patch_case = patch_case->adds_properties_enums();
			properties.insert( properties.end(), properties_for_patch_case.begin(), properties_for_patch_case.end() );
		}
	}

	return properties;
}


/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any properties that are deleted.
utility::vector1< std::string >
Patch::deletes_properties( ResidueType const & rsd_type ) const
{
	utility::vector1< std::string > properties;
	if ( !applies_to( rsd_type ) ) return properties;  // I don't know how to patch this residue.

	for ( auto const & iter : cases_ ) {

		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< std::string > const & properties_for_patch_case = iter->deletes_properties();
			properties.insert( properties.end(), properties_for_patch_case.begin(), properties_for_patch_case.end() );
		}
	}

	return properties;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any properties that are deleted, by enum.  Faster than string version.
utility::vector1< ResidueProperty >
Patch::deletes_properties_enums( ResidueType const & rsd_type ) const
{
	utility::vector1< ResidueProperty > properties;
	if ( !applies_to( rsd_type ) ) return properties;  // I don't know how to patch this residue.

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< ResidueProperty > const & properties_for_patch_case = iter->deletes_properties_enums();
			properties.insert( properties.end(), properties_for_patch_case.begin(), properties_for_patch_case.end() );
		}
	}

	return properties;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any variants that are deleted.
utility::vector1< std::string >
Patch::deletes_variants( ResidueType const & rsd_type ) const
{
	utility::vector1< std::string > variants;
	if ( !applies_to( rsd_type ) ) return variants;  // I don't know how to patch this residue.

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< std::string > const & variants_for_patch_case = iter->deletes_variants();
			variants.insert( variants.end(), variants_for_patch_case.begin(), variants_for_patch_case.end() );
		}
	}

	return variants;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, compile
/// a list of any variants that are deleted.  This version returns a list of VariantType enums (and does
/// not support on-the-fly types).
utility::vector1< core::chemical::VariantType >
Patch::deletes_variants_by_enum( ResidueType const & rsd_type ) const
{
	utility::vector1< core::chemical::VariantType > variants;
	if ( !applies_to( rsd_type ) ) return variants;  // I don't know how to patch this residue.

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			utility::vector1< core::chemical::VariantType > const & variants_for_patch_case = iter->deletes_variants_by_enum();
			variants.insert( variants.end(), variants_for_patch_case.begin(), variants_for_patch_case.end() );
		}
	}
	return variants;
}

bool
Patch::changes_connections_on( ResidueType const & rsd_type, std::string const & atom ) const {
	if ( !applies_to( rsd_type ) ) return false;  // Can't change what you can't alter

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			// this patch case applies to this rsd_type
			if ( iter->changes_connections_on( rsd_type, atom ) ) {
				// This doesn't 'add a CONNECT' in the proper sense, but for the purpose of
				// relating LINKs to patch assignment, we need to encourage cutpoint patches
				// if we find backbone LINKs
				//|| ( atom == " N  " && ( name_ == "protein_cutpoint_upper" || name_ == "peptoid_cutpoint_upper" ) )
				//|| ( atom == " P  " && ( name_ == "rna_cutpoint_upper" ) ) ) {
				//tr << " aaaa " << atom << "  " << name_ << std::endl;
				return true;
			}
		}
	}
	return false;
}

/// @brief Add a patch case to this patch.
void
Patch::add_case(
	PatchCaseOP pcase
) {
	if ( pcase->generates_base_residue_type() ) {
		generates_base_residue_type_ = true;
	}
	cases_.push_back( pcase );
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, discover
/// if there is a new name3 generated by the patch.
std::string
Patch::generates_new_name3( ResidueType const & rsd_type ) const
{
	std::string new_name3( "" );
	if ( !applies_to( rsd_type ) ) return new_name3;

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			std::string name3 = iter->generates_new_name3();
			if ( name3.size() > 0 ) {
				runtime_assert( new_name3.size() == 0 );
				new_name3 = name3;
			}
			break; // do not go to later selectors.
		}
	}

	return new_name3;
}

/// @details loop through the cases in this patch and if it is applicable to this ResidueType, discover
/// if there is a new interchangeability_group generated by the patch.
std::string
Patch::generates_interchangeability_group( ResidueType const & rsd_type ) const
{
	std::string new_interchangeability_group( "" );
	if ( !applies_to( rsd_type ) ) return new_interchangeability_group;

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) ) {
			std::string interchangeability_group = iter->generates_interchangeability_group();
			if ( interchangeability_group.size() > 0 ) {
				runtime_assert( new_interchangeability_group.size() == 0 );
				new_interchangeability_group = interchangeability_group;
			}
			break; // do not go to later selectors.
		}
	}

	return new_interchangeability_group;
}


/// @details loop through the cases in this patch and if it is applicable to this ResidueType, discover
/// if there is a new name3 generated by the patch.
chemical::AA
Patch::generates_aa( ResidueType const & rsd_type ) const
{
	chemical::AA new_aa( aa_none );
	if ( !applies_to( rsd_type ) ) return new_aa;

	for ( auto const & iter : cases_ ) {
		if ( iter->applies_to( rsd_type ) && iter->may_change_aa() ) {
			MutableResidueTypeOP rsd_type_apply = apply( rsd_type, false /* do not need to instantiate */ );
			chemical::AA aa = rsd_type_apply->aa();
			if ( aa == rsd_type.aa() ) aa = aa_none;
			runtime_assert( new_aa == aa_none );
			new_aa = aa;
			break; // do not go to later selectors.
		}
	}

	return new_aa;
}


} // chemical
} // core

#ifdef    SERIALIZATION

/// @brief Automatically generated serialization method
template< class Archive >
void
core::chemical::PatchCase::save( Archive & arc ) const {
	arc( CEREAL_NVP( selector_ ) ); // class core::chemical::ResidueTypeSelector
	arc( CEREAL_NVP( operations_ ) ); // utility::vector1<PatchOperationOP>
	arc( CEREAL_NVP( generates_base_residue_type_ ) ); //bool
}

/// @brief Automatically generated deserialization method
template< class Archive >
void
core::chemical::PatchCase::load( Archive & arc ) {
	arc( selector_ ); // class core::chemical::ResidueTypeSelector
	arc( operations_ ); // utility::vector1<PatchOperationOP>
	arc( generates_base_residue_type_ ); //bool
}

SAVE_AND_LOAD_SERIALIZABLE( core::chemical::PatchCase );
CEREAL_REGISTER_TYPE( core::chemical::PatchCase )


/// @brief Automatically generated serialization method
template< class Archive >
void
core::chemical::Patch::save( Archive & arc ) const {
	arc( CEREAL_NVP( res_type_set_mode_ ) ); // enum core::chemical::TypeSetMode
	arc( CEREAL_NVP( name_ ) ); // std::string
	arc( CEREAL_NVP( is_metapatch_ ) ); //bool
	arc( CEREAL_NVP( types_ ) ); // utility::vector1<std::string>
	arc( CEREAL_NVP( custom_types_ ) ); // utility::vector1<std::string>
	arc( CEREAL_NVP( selector_ ) ); // class core::chemical::ResidueTypeSelector
	arc( CEREAL_NVP( cases_ ) ); // utility::vector1<PatchCaseOP>
	arc( CEREAL_NVP( replaces_residue_type_ ) ); // _Bool
	arc( CEREAL_NVP( generates_base_residue_type_ ) ); //bool
}

/// @brief Automatically generated deserialization method
template< class Archive >
void
core::chemical::Patch::load( Archive & arc ) {
	arc( res_type_set_mode_ ); // enum core::chemical::TypeSetMode
	arc( name_ ); // std::string
	arc( is_metapatch_ ); //bool
	arc( types_ ); // utility::vector1<std::string>
	arc( custom_types_ );
	arc( selector_ ); // class core::chemical::ResidueTypeSelector
	arc( cases_ ); // utility::vector1<PatchCaseOP>
	arc( replaces_residue_type_ ); // _Bool
	arc( generates_base_residue_type_ ); //bool
}

SAVE_AND_LOAD_SERIALIZABLE( core::chemical::Patch );
CEREAL_REGISTER_TYPE( core::chemical::Patch )

CEREAL_REGISTER_DYNAMIC_INIT( core_chemical_Patch )
#endif // SERIALIZATION
