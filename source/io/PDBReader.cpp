#include "io/PDBReader.h"
#include "io/Reader.h"
#include "io/File.h"
#include "data/Terminate.h"
#include "data/Atom.h"
#include "data/Hetatom.h"

#include <fstream>

void PDBReader::read(const string& input_path) {
    // check if file was succesfully opened
    std::ifstream input(input_path);
    if (!input.is_open()) {throw std::ios_base::failure("Error in PDB_file::read: Could not open file \"" + input_path + "\"");}

    string line; // placeholder for the current line
    File& f = *file;
    while(getline(input, line)) {
        string type = line.substr(0, 6); // read the first 6 characters
        switch(Record::get_type(type)) {
            case Record::RecordType::HETATM: {
                Hetatom atom;
                atom.parse_pdb(line);
                f.add(atom);
                break;
            } case Record::RecordType::ATOM: {
                Atom atom;
                atom.parse_pdb(line);
                f.add(atom);
                break;
            } case Record::RecordType::TERMINATE: {
                Terminate term;
                term.parse_pdb(line);
                f.add(term);
                break;
            } case Record::RecordType::HEADER: {
                f.add("HEADER", line);
                break;
            } case Record::RecordType::FOOTER: {
                f.add("FOOTER", line);
                break;
            } default: {
                throw std::ios_base::failure("Error in PDB_file::read: Malformed input file - unrecognized type.");
            }
        };
    }
    input.close();
}