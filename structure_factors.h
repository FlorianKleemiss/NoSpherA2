#ifndef __c_structure_factors__
#define __c_structure_factors__

#include <string>
#include <fstream>


class WFN;

bool calculate_structure_factors(
	std::string &hkl_filename, 
	std::string &cif_file, 
	std::string &asym_cif, 
	std::string &symm, 
	WFN &wave, 
	bool debug, 
	int accuracy, 
	std::ofstream &file, 
	int cpus = -1,
	bool electron_diffraction = false,
	int pbc = 0);

#include "wfn_class.h"

#endif
