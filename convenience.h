#ifndef __CONVENIENCE_H__
#define __CONVENIENCE_H__

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>

class WFN;
class cell;


constexpr double bohr2aang(double inp) {
	return inp * 0.529177249;
}

constexpr double ang2bohr(double inp) {
	return inp / 0.529177249;
}

//------------------general functions for easy use of terminal input--------------------
const double bragg_angstrom[114]{
0.00, 
	0.35, 0.35, 
	1.45, 1.05,																																					0.85, 0.70, 0.65, 0.60, 0.50, 0.45,
	1.80, 1.50,																																					1.25, 1.10, 1.00, 1.00, 1.00, 1.00,
	2.20, 1.80,																						1.60, 1.40, 1.35, 1.40, 1.40, 1.40, 1.35, 1.35, 1.35, 1.35, 1.30, 1.25, 1.15, 1.15, 1.15, 1.10,
	2.35, 2.00,																						1.80, 1.55, 1.45, 1.45, 1.35, 1.30, 1.35, 1.40, 1.60, 1.55, 1.55, 1.45, 1.45, 1.40, 1.40, 1.40,
	2.60, 2.15, 1.95, 1.85, 1.85, 1.85, 1.85, 1.85, 1.85, 1.80, 1.75, 1.75, 1.75, 1.75, 1.75, 1.75, 1.75, 1.55, 1.45, 1.35, 1.30, 1.30, 1.35, 1.35, 1.35, 1.50, 1.90, 1.75, 1.60, 1.90, 1.50, 1.50,
	2.80, 2.35, 2.15, 2.05, 2.05, 2.05, 2.05, 2.05, 2.05, 2.00, 1.95, 1.95, 1.95, 1.95, 1.95, 1.95};
bool yesno();
bool is_similar_rel(double first, double second, double tolerance);
bool is_similar(double first, double second, double tolerance);
void Enter();
void cls();
std::string get_home_path(void);
void join_path(std::string &s1, std::string &s2);

bool generate_sph2cart_mat(std::vector<std::vector<double>>& d, std::vector<std::vector<double>>& f, std::vector<std::vector<double>>& g, std::vector<std::vector<double>>& h);
std::string go_get_string(std::ifstream& file, std::string search, bool rewind = true);

inline const int sht2nbas(const int type) {
	const int st2bas[6]{ 1,3,6,10,15,21 };
	const int nst2bas[6]{ 11,9,7,5,4,1 };
	if (type >= 0)
		return st2bas[type];
	else
		return nst2bas[5+type];
};

inline const int shell2function(int type, int prim) {
	switch (type) {
	case (-5):
		return -32 + prim;
	case (-4):
		return -21 + prim;
	case(-3):
		return -12 + prim;
	case(-2):
		return -5 + prim;
	case(-1):
		return 1 + prim;
	case(0):
		return 1;
	case(1):
		return 2 + prim;
	case(2):
		return 5 + prim;
	case(3):
		if (prim == 0) return 11;
		if (prim == 1) return 12;
		if (prim == 2) return 13;
		if (prim == 3) return 17;
		if (prim == 4) return 14;
		if (prim == 5) return 15;
		if (prim == 6) return 18;
		if (prim == 7) return 19;
		if (prim == 8) return 16;
		if (prim == 9) return 20;
		break;
	case(4):
		return 21 + prim;
	case(5):
		return 36 + prim;
	default:
		return 0;
	}
	return 0;
}

const double normgauss(const int type, const double exp);

template<class T> std::string toString(const T& t)
{
     std::ostringstream stream;
     stream << t;
     return stream.str();
}

template<class T> T fromString(const std::string& s)
{
     std::istringstream stream (s);
     T t;
     stream >> t;
     return t;
}

inline void copyright();

inline int CountWords(const char* str)
{
	if (str == NULL)
		return -1;

	bool inSpaces = true;
	int numWords = 0;

	while (*str != '\0')
	{
		if (std::isspace(*str))
		{
			inSpaces = true;
		}
		else if (inSpaces)
		{
			numWords++;
			inSpaces = false;
		}

		++str;
	}

	return numWords;
};

inline bool copyright(std::ofstream& file);

inline bool exists(const std::string &name){
	std::ifstream f(name.c_str());
	return f.good();
};

std::string atnr2letter(const int nr);
void copy_file(std::string from, std::string to);
std::string shrink_string(std::string &input);
std::string shrink_string_to_atom(std::string &input, const int atom_number);
std::vector<std::string> split_string(const std::string& input, const std::string delimiter);
std::string get_filename_from_path(const std::string &input);
std::string get_foldername_from_path(const std::string &input);
std::string get_basename_without_ending(const std::string &input);
//------------------Functions to handle .wfn files--------------------------------------
bool writewfn(WFN &wavefunction, const std::string &path, bool &debug, bool occ);
bool readwfn(WFN &wavefunction, const std::string &path, bool &debug);
bool readwfn(WFN& wavefunction, const std::string& path, bool& debug, std::ofstream &file);
//------------------Functions to read from .fchk files----------------------------------
bool read_fchk_integer_block(std::ifstream& in, std::string heading, std::vector<int>& result, bool rewind = true);
bool read_fchk_double_block(std::ifstream& in, std::string heading, std::vector<double>& result, bool rewind = true);
int read_fchk_integer(std::string in);
int read_fchk_integer(std::ifstream& in, std::string search, bool rewind = true);
double read_fchk_double(std::string in);
double read_fchk_double(std::ifstream& in, std::string search, bool rewind = true);
//------------------Functions to work with configuration files--------------------------
void write_template_confi();
int program_confi(std::string &gaussian_path, std::string &turbomole_path, 
                   std::string &basis, int &ncpus, float &mem, bool debug = false, bool expert = false, unsigned int counter = 0);
bool check_bohr(WFN &wave, bool interactive, bool debug);
int filetype_identifier(std::string &file, bool debug = false);

/*bool open_file_dialog(std::string &path, bool debug, std::vector <std::string> filter);
bool save_file_dialog(std::string &path, bool debug, const std::vector<std::string> &endings, const std::string &filename_given);
bool save_file_dialog(std::string &path, bool debug, const std::vector<std::string> &endings);*/
void select_cubes(std::vector <std::vector <unsigned int> > &selection, std::vector<WFN> &wavy, unsigned int nr_of_cubes=1, bool wfnonly=false, bool debug = false);
bool unsaved_files(std::vector<WFN> &wavy);
int get_Z_from_label(const char * tmp);

inline int sum_of_bools(const std::vector<bool> in) {
	int result = 0;
	for (int i = 0; i < in.size(); i++)
		if (in[i]) result++;
	return result;
}

inline std::string trim(const std::string& s)
{
	if (s == "") return "";
	auto start = s.begin();
	while (start != s.end() && std::isspace(*start)) {
		start++;
	}

	auto end = s.end();
	do {
		end--;
	} while (std::distance(start, end) > 0 && std::isspace(*end));

	return std::string(start, end + 1);
}

inline void err(const char message, std::ofstream log) {
	log << message << std::endl;
	exit(-1);
}

inline void err(std::string& message, std::ofstream log) {
	log << message << std::endl;
	exit(-1);
}

inline void err(std::string& message) {
	std::cout << message << std::endl;
	exit(-1);
}


//-------------------------Progress_bar--------------------------------------------------

class progress_bar
{
	static const auto overhead = sizeof " [100%]";

	std::ofstream& os;
	const std::size_t bar_width;
	std::string message;
	const std::string full_bar;

public:
	progress_bar(std::ofstream& os, std::size_t line_width,
		std::string message_, const char symbol = '=')
		: os{ os },
		bar_width{ line_width - overhead },
		message{ std::move(message_) },
		full_bar{ std::string(bar_width, symbol) + std::string(bar_width, ' ') }
	{
		if (message.size() + 1 >= bar_width || message.find('\n') != message.npos) {
			os << message << '\n';
			message.clear();
		}
		else {
			message += ' ';
		}
		write(0.0);
	}

	// not copyable
	progress_bar(const progress_bar&) = delete;
	progress_bar& operator=(const progress_bar&) = delete;

	~progress_bar()
	{
		write(1.0);
		os << '\n';
	}

	void write(double fraction);
};

const double MPI2 = 2 * 3.14159265358979323844;

class cosinus_annaeherung
{
public:
	cosinus_annaeherung();
	inline double get(double x) const
	{
////////////////double xa = abs(x);
////////////////size_t pos = static_cast<size_t>((xa * mSize) / MPI2); // Stueststelle bestimmen (Wird fuer grosse X ungenau, aber passt fuer x
////////////////double dx = xa - pos * mStepwidth;
////////////////pos = pos % mSize; // Modulo, da sinus periodisch ist.
////////////////double y1 = mBase_values[pos];
////////////////double y2 = mBase_values[pos + 1];
////////////////return y1 + dx * (y2 - y1) / mStepwidth;
		return 0.0;
	}

	void   resize(size_t size);
	double calculate_error_at(double x) const;
private:
	size_t mSize;
	double* mBase_values;
	double mStepwidth;
};
struct sinus
{
	sinus(cosinus_annaeherung& helper) : helper(helper) {};
	double get(double x) { return helper.get(x - 1.57079632679489661922l); }
	cosinus_annaeherung& helper;
};

struct cosinus
{
	cosinus(cosinus_annaeherung& helper) : helper(helper) {};
	double get(double x) { return helper.get(x); }
	cosinus_annaeherung& helper;
};

void readxyzMinMax_fromWFN(
	WFN& wavy,
	double* CoordMinMax,
	double* NbSteps,
	double Radius,
	double Increments);
void readxyzMinMax_fromCIF(
	std::string cif,
	double* CoordMinMax,
	double* NbSteps,
	std::vector < std::vector < double > > &cm,
	double Resolution,
	std::ofstream& file,
	bool debug = false);
void type2vector(
	const int index,
	int* vector);

bool read_fracs_ADPs_from_CIF(std::string cif, WFN& wavy, cell& unit_cell, std::ofstream& log3, bool debug);

inline double double_from_string_with_esd(std::string in) {
	if (in.find('(') == std::string::npos)
		return stod(in);
	else
		return stod(in.substr(0, in.find('(')));
}

constexpr double Laguerre_ploynom(int n, double x) {
	if (n == 0) return 1;
	else if (n == 1) return 1 - x;
	else return (((int(2) * n + int(1) - x) * Laguerre_ploynom(n - 1, x) - n * Laguerre_ploynom(n - 2, x)) / (n + int(1)));
}


template< typename order_iterator, typename value_iterator >
void reorder_destructive(order_iterator order_begin, order_iterator order_end, value_iterator v) {
//https://stackoverflow.com/questions/838384/reorder-vector-using-a-vector-of-indices
	typedef typename std::iterator_traits< value_iterator >::value_type value_t;
	typedef typename std::iterator_traits< order_iterator >::value_type index_t;
	typedef typename std::iterator_traits< order_iterator >::difference_type diff_t;

	diff_t remaining = order_end - 1 - order_begin;
	for (index_t s = index_t(); remaining > 0; ++s) {
		index_t d = order_begin[s];
		if (d == (diff_t)-1) continue;
		--remaining;
		value_t temp = v[s];
		for (index_t d2; d != s; d = d2) {
			swap(temp, v[d]);
			swap(order_begin[d], d2 = (diff_t)-1);
			--remaining;
		}
		v[s] = temp;
	}
}

#include "wfn_class.h"

#endif
