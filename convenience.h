#ifndef __CONVENIENCE_H__
#define __CONVENIENCE_H__

#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <fstream>

class WFN;

//------------------general functions for easy use of terminal input--------------------
bool yesno();
bool is_similar_rel(double first, double second, double tolerance);
bool is_similar(double first, double second, double tolerance);
void Enter();
void cls();
std::string get_home_path(void);
void join_path(std::string &s1, std::string &s2);
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

inline void QCT();
inline bool cuQCT(std::ofstream& file);
inline void cuQCT();

inline void copyright();
inline bool copyright(std::ofstream& file);

inline bool exists(const std::string &name){
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
};

std::string atnr2letter(const int nr);
void copy_file(std::string from, std::string to);
std::string shrink_string(std::string &input);
std::string shrink_string_to_atom(std::string &input, const int atom_number);
std::string get_filename_from_path(const std::string &input);
std::string get_foldername_from_path(const std::string &input);
std::string get_basename_without_ending(const std::string &input);
//------------------Functions to handle .wfn files--------------------------------------
bool writewfn(WFN &wavefunction, const std::string &path, bool &debug, bool occ);
bool readwfn(WFN &wavefunction, const std::string &path, bool &debug);
bool readwfn(WFN& wavefunction, const std::string& path, bool& debug, std::ofstream &file);
//------------------Functions to work with configuration files--------------------------
void write_template_confi();
int program_confi(std::string &gaussian_path, std::string &turbomole_path, 
                   std::string &basis, int &ncpus, float &mem, bool debug = false, bool expert = false, unsigned int counter = 0);
bool check_bohr(WFN &wave, bool interactive, bool debug);
int filetype_identifier(std::string &file, bool debug = false);

bool open_file_dialog(std::string &path, bool debug, std::vector <std::string> filter);
bool save_file_dialog(std::string &path, bool debug, const std::vector<std::string> &endings, const std::string &filename_given);
bool save_file_dialog(std::string &path, bool debug, const std::vector<std::string> &endings);
void select_cubes(std::vector <std::vector <unsigned int> > &selection, std::vector<WFN> &wavy, unsigned int nr_of_cubes=1, bool wfnonly=false, bool debug = false);
bool unsaved_files(std::vector<WFN> &wavy);
int get_Z_from_label(const char * tmp);

//-------------------------Progress_bar--------------------------------------------------

class progress_bar
{
	static const auto overhead = sizeof " [100%]";

	std::ostream& os;
	const std::size_t bar_width;
	std::string message;
	const std::string full_bar;

public:
	progress_bar(std::ostream& os, std::size_t line_width,
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

#include "wfn_class.h"

#endif
