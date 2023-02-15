#include <dataset/detail/DATConstructor.h>
#include <dataset/Dataset.h>
#include <dataset/SimpleDataset.h>
#include <dataset/Dataset2D.h>
#include <utility/Settings.h>
#include <utility/Utility.h>
#include <utility/Exceptions.h>
#include <math/Statistics.h>

#include <vector>
#include <string>
#include <fstream>

std::shared_ptr<Dataset> detail::DATConstructor::construct(std::string path) {
    if (setting::general::verbose) {
        utility::print_info("Loading dataset from \"" + path + "\"");
    }

    // check if file was succesfully opened
    std::ifstream input(path);
    if (!input.is_open()) {throw std::ios_base::failure("DATConstructor::construct: Could not open file \"" + path + "\"");}

    std::string line;
    std::vector<std::string> header;
    std::vector<std::vector<double>> row_data;
    std::vector<unsigned int> col_number;
    while(getline(input, line)) {
        // skip empty lines
        if (line.empty()) {continue;}

        // remove leading whitespace
        std::vector<std::string> tokens = utility::split(line, " ,\t\n\r"); // spaces, commas, and tabs can be used as separators

        // remove empty tokens
        for (unsigned int i = 0; i < tokens.size(); i++) {
            if (tokens[i].empty()) {
                tokens.erase(tokens.begin() + i);
                i--;
            }
        }

        // check if all tokens are numbers
        bool skip = false;
        for (unsigned int i = 0; i < tokens.size(); i++) {
            if (tokens[i].find_first_not_of("0123456789+-.Ee\n\r") != std::string::npos) {
                skip = true;
            }
        }
        if (skip) {
            header.push_back(line);
            continue;
        }

        // add values to dataset
        std::vector<double> vals(tokens.size());
        for (unsigned int i = 0; i < tokens.size(); i++) {
            vals[i] = std::stod(tokens[i]);
        }
        row_data.push_back(vals);
        col_number.push_back(vals.size());
    }

    // determine the most common number of columns, since that will likely be the data
    unsigned int mode = stats::mode(col_number);
    std::shared_ptr<Dataset> dataset;
    switch (mode) {
        case 2: {
            if (setting::general::verbose) {std::cout << "\t2 columns detected. Assuming the format is x | y" << std::endl;}
            dataset = std::make_shared<Dataset>();
            break;
        }
        case 3: {
            if (setting::general::verbose) {std::cout << "\t3 columns detected. Assuming the format is x | y | yerr" << std::endl;}
            dataset = std::make_shared<Dataset>(SimpleDataset());
            break;
        }
        case 4: {
            if (setting::general::verbose) {std::cout << "\t4 columns detected. Assuming the format is x | y | yerr | xerr" << std::endl;}
            dataset = std::make_shared<Dataset>(Dataset2D());
            break;
        }
        default: {
            throw except::io_error("DATConstructor::construct: File has an unsupported number of columns (" + std::to_string(mode) + ").");
        }
    }

    // copy all rows with the correct number of columns
    unsigned int count = 0;
    for (unsigned int i = 0; i < row_data.size(); i++) {
        if (row_data[i].size() != mode) {continue;}
        if (count++ < setting::axes::skip) {continue;}
        dataset->push_back(row_data[i]);
    }
    if (setting::axes::skip != 0 && setting::general::verbose) {
        std::cout << "\tSkipped " << count - dataset->size() << " data points from beginning of file." << std::endl;
    }

    // verify that at least one row was read correctly
    if (dataset->empty()) {
        throw except::io_error("DATConstructor::construct: No data could be read from the file.");
    }

    // scan the headers for units. must be either [Å] or [nm]
    bool found_unit = false;
    for (auto& s : header) {
        if (s.find("[nm]") != std::string::npos) {
            if (setting::general::verbose) {std::cout << "\tUnit [nm] detected. Scaling all q values by 1/10." << std::endl;}
            for (unsigned int i = 0; i < dataset->size(); i++) {
                dataset->index(i, 0) /= 10;
            }
            found_unit = true;
            break;
        } else if ((s.find("[Å]") != std::string::npos) || (s.find("[AA]") != std::string::npos)) {
            if (setting::general::verbose) {std::cout << "\tUnit [Å] detected. No scaling necessary." << std::endl;}
            found_unit = true;
            break;
        }
    }
    if (!found_unit) {
        if (setting::general::verbose) {std::cout << "\tNo unit detected. Assuming [Å]." << std::endl;}
    }

    // check if the file is abnormally large
    if (dataset->size() > 300) {
        // reread first line
        input.clear();
        input.seekg(0, input.beg);
        getline(input, line);

        // check if file has already been rebinned
        if (line.find("REBINNED") == std::string::npos) {
            // if not, suggest it to the user
            if (setting::general::verbose) {
                std::cout << "\tFile contains more than 300 rows. Consider rebinning the data." << std::endl;
            }
        }
    }

    if (setting::general::verbose) {
        std::cout << "\tSuccessfully read " << dataset->size() << " data points from " << path << std::endl;
    }
    return dataset;
}