#include <io/PDBReader.h>
#include <io/Reader.h>
#include <data/detail/AtomCollection.h>
#include <io/ExistingFile.h>
#include <data/record/Terminate.h>
#include <data/record/Atom.h>
#include <data/record/Water.h>
#include <utility/Exceptions.h>
#include <settings/GeneralSettings.h>
#include <constants/ConstantsFwd.h>
#include <utility/Console.h>

#include <fstream>

using namespace io::detail;
using namespace data::record;

PDBReader::PDBReader(data::detail::AtomCollection* const file) : file(file) {}

PDBReader::~PDBReader() = default;

void PDBReader::read(const io::ExistingFile& path) {
    if (settings::general::verbose) {
        console::print_info("\nReading PDB file from \"" + path + "\"");
    }

    // check if file was succesfully opened
    std::ifstream input(path);
    if (!input.is_open()) {throw except::io_error("PDBReader::read: Could not open file \"" + path + "\"");}

    std::string line; // placeholder for the current line
    data::detail::AtomCollection& f = *file;
    while(getline(input, line)) {
        std::string type = line.substr(0, std::min(6, int(line.size()))); // read the first 6 characters
        switch(Record::get_type(type)) {
            case RecordType::ATOM: {
                // first just parse it as an atom; we can reuse it anyway even if it is a water molecule
                Atom atom;
                atom.parse_pdb(line);

                // check if this is a hydrogen atom
                if (atom.element == constants::atom_t::H && !settings::general::keep_hydrogens) {continue;}

                // check if this is a water molecule
                if (atom.is_water()) {f.add(Water(std::move(atom)));} 
                else {f.add(atom);}
                break;
            } case RecordType::TERMINATE: {
                Terminate term;
                term.parse_pdb(line);
                f.add(term);
                break;
            } case RecordType::HEADER: {
                f.add(RecordType::HEADER, line);
                break;
            } case RecordType::FOOTER: {
                f.add(RecordType::FOOTER, line);
                break;
            } case RecordType::NOTYPE: {
                break;
            } default: {
                throw except::io_error("PDBReader::read: Malformed input file - unrecognized type \"" + type + "\".");
            }
        };
    }
    input.close();

    unsigned int n_pa = f.protein_atoms.size();
    unsigned int n_ha = f.hydration_atoms.size();
    
    if (settings::general::verbose) {
        std::cout << "\tSuccessfully read " << n_pa + n_ha << " atomic records." << std::endl;
        if (n_ha != 0) {
            std::cout << "\t\t" << f.hydration_atoms.size() << " of these are hydration atoms." << std::endl;
        }
    }
}