#pragma once

class MO;
#include "atoms.h"
#include "cube.h"

#include <vector>
#include <string>
#include <fstream>

class WFN
{
private:
  int ncen;
  int nfunc;
  int nmo;
  int nex;
  int charge;
  unsigned int multi;
  int origin; //0=NOT_YET_DEFINED; 1=CRYSTAL; 2=WFN; 3=CUBE; 4=FFN; 5=FCHK; 6=WFX; 7=XYZ; 8=Molden; 9=gbw
  double total_energy;
  double virial_ratio;
  std::string basis_set_name;
  std::string comment;
  std::string path;
  std::string method;

  std::vector<MO> MOs;
  std::vector<int> centers;
  // For cartesian the order of types is: 
  // 1                              = S, 
  // 2,3,4                          = X,Y,Z
  // 5,6,7,8,9,10                   = XX, YY, ZZ, XY, XZ, YZ
  // 11,12,13,14,15,16,17,118,19,20 = XXX, YYY, ZZZ, XXY, XXZ, YYZ, XYY, XZZ, YZZ, XYZ etc..
  // for sphericals the order is:
  // 1 = S
  // 2,3,4 = 0, 1, -1
  // 5,6,7,8,9 = 0, +1, -1, +2, -2
  // 10,11,12,13,14,15,16 = 0, +1, -1, +2, -2, +3, -3 etc...
  std::vector<int> types;
  std::vector<double> exponents;
  std::vector<double> DensityMatrix;
  std::vector<double> SpinDensityMatrix;

  bool erase_center(int nr);
  bool erase_type(int nr);
  bool erase_exponent(int nr);
  bool push_back_center(int cent);
  bool push_back_type(int type);
  bool push_back_exponent(double e);
  void push_back_MO_coef(int nr, double value);
  void assign_MO_coefs(int nr, std::vector<double>& values);
  bool modified;
  bool d_f_switch; //true if spherical harmonics are used for the basis set
  bool distance_switch;
  bool has_ECPs;
  //precomputed factors and helper functions for ESP calc
  int pre[9][5][5][9];
  void fill_pre();
  int Afac_pre[9][5][9];
  void fill_Afac_pre();
  double fj(int& j, int& l, int& m, double& aa, double& bb);
  double Afac(int& l, int& r, int& i, double& PC, double& gamma, double& fjtmp);
  double compute_dens_cartesian(const double& Pos1, const double& Pos2, const double& Pos3, std::vector<std::vector<double>>& d, std::vector<double>& phi, const bool& add_ECP_dens);
  double compute_dens_spherical(const double& Pos1, const double& Pos2, const double& Pos3, std::vector<std::vector<double>>& d, std::vector<double>& phi, const bool& add_ECP_dens);
public:
  WFN();
  WFN(int given_origin);
  std::vector<cube> cub;
  std::vector<atom> atoms;

  //--------------------MO handling--------------------------------------------
  void change_MO_coef(int nr);
  bool change_MO_coef(int nr_mo, int nr_primitive, double value, bool debug);
  double get_MO_coef(const int nr_mo, const int nr_primtive, const bool debug) const;
  double get_MO_coef_f(const int nr_mo, const int nr_primtive) const;
  double* get_MO_coef_ptr(const int nr_mo);
  int get_MO_primitive_count(const int nr_mo) const;
  bool push_back_MO(int nr, double occ, double ener);
  bool push_back_MO_coef(int nr, double value, int nr2);
  double get_MO_energy(const int mo) const;
  int get_center(const int& nr) const { return centers[nr]; };
  int get_type(const int& nr) const { return types[nr]; };
  double get_MO_occ(const int nr);

  //--------------------in and output----------------------------------------
  void change_basis_set_name(std::string name) { basis_set_name = name; };
  bool add_primitive(int cent, int type, double e, double* values);
  bool add_exp(int cent, int type, double e);
  void read_known_wavefunction_format(const std::string fileName, const bool debug = false);
  void read_known_wavefunction_format(const std::string fileName, std::ofstream& file, const bool debug = false);
  bool read_wfn(const std::string fileName, const bool debug = false);
  bool read_wfn(const std::string fileName, const bool debug, std::ofstream& file);
  bool read_wfx(const std::string fileName, const bool debug, std::ofstream& file);
  bool read_wfx(const std::string fileName, const bool debug, std::ostream& file);
  bool read_fchk(const std::string& filename, std::ostream& log, const bool debug = false);
  bool read_fchk(const std::string& filename, std::ofstream& log, const bool debug = false);
  bool read_xyz(const std::string& filename, std::ofstream& file, const bool debug = false);
  bool read_xyz(const std::string& filename, std::ostream& file, const bool debug = false);
  bool read_molden(const std::string& filename, std::ostream& file, const bool debug = false);
  bool read_gbw(const std::string& filename, std::ostream& file, const bool debug = false);
  bool write_wfn(const std::string& fileName, const bool debug, const bool occupied);
  bool set_path(std::string given_path) { path = given_path; return true; };
  void print_primitive(const int nr);
  void assign_charge(const int i_charge) { charge = i_charge; };
  void assign_multi(const int i_multi) { multi = i_multi; };
  int get_charge() const { return charge; };
  int get_multi() const { return multi; };
  int get_nex() const { return nex; };
  int get_ncen() const { return ncen; };
  int get_nmo() const { return nmo; };
  int get_nmo(const bool only_occ) const;
  int get_origin() const { return origin; };
  double get_exponent(int nr) const { return exponents[nr]; };
  unsigned int get_nr_electrons(const bool& debug);
  unsigned int get_nr_ECP_electrons();
  double count_nr_electrons(void);
  /*int get_type(int nr) { if(nr>types.size()||nr<0) return -1; else return types[nr]; };
  int get_center(int nr) { if(nr>centers.size()||nr<0) return -1; else return centers[nr]; };					NOT NEEDED AT THIS POINT!
  double get_exponent(int nr) { if(nr>exponents.size()||nr<0) return -1; else return exponents[nr]; }; */
  std::string get_centers(const bool bohr);
  std::string get_basis_set_name() { return basis_set_name; };
  void set_basis_set_name(const std::string input) { basis_set_name = input; };
  std::string get_path() { return path; };
  std::string hdr(const bool occupied);
  void set_method(const std::string& input) { method = input; };
  std::string get_method() { return method; };
  bool erase_atom(const int nr);
  void list_primitives();
  void list_centers();
  bool remove_center(const int nr);
  bool remove_primitive(const int nr);
  void change_center(const int nr);
  void change_type(const int nr);
  void change_exponent(const int nr);
  /*bool change_center(int nr, int value);
  bool change_type(int nr, int value);					NOT NEEDED AT THIS POINT!
  bool change_exponent(int nr, double value);*/
  void set_modified() { modified = true; };
  bool get_modified() { return modified; };
  void set_d_f_switch(bool in) { d_f_switch = in; };
  bool get_d_f_switch() { return d_f_switch; };
  int check_order(bool debug);
  bool sort_wfn(int order, bool debug);
  void set_dist_switch() { distance_switch = true; };
  void set_has_ECPs(const bool& in, const bool& apply_to_aotms = true);
  bool get_has_ECPs() { return has_ECPs; };
  void operator=(const WFN& right);
  int calculate_charge();
  int calculate_charge(std::ofstream& file);
  bool guess_multiplicity(const bool expert = false);
  bool guess_multiplicity(std::ofstream& file, const bool expert = false);
  std::vector<double> get_norm_const(std::ofstream& file, const bool debug = false);
  //-------------------atom handling--------------------------------------------------------------
  double get_atom_coordinate(const int nr, const int axis, const bool debug);
  std::string get_atom_label(const int nr);
  int get_nr_basis_set_loaded();
  bool get_atom_basis_set_loaded(const int nr);
  double get_atom_basis_set_exponent(const int nr_atom, const int nr_prim);
  double get_atom_basis_set_coefficient(const int nr_atom, const int nr_prim);
  bool change_atom_basis_set_exponent(const int nr_atom, const int nr_prim, const double value);
  bool change_atom_basis_set_coefficient(const int nr_atom, const int nr_prim, const double value);
  int get_atom_primitive_count(const int nr);
  int get_atom_primitive_type(const int nr_atom, const int nr_prim)
  {
    if (nr_atom < atoms.size() && nr_atom >= 0 && nr_prim < atoms[nr_atom].basis_set.size() && nr_prim >= 0)
      return atoms[nr_atom].basis_set[nr_prim].type; else return -1;
  };
  bool erase_atom_primitive(const int nr, const int nr_prim);
  int get_atom_shell_count(const int nr);
  int get_atom_shell_primitives(const int nr_atom, const int nr_shell);
  int get_shell_type(const int nr_atom, const int nr_shell);
  int get_shell_center(const int nr_atom, const int nr_shell);
  int get_basis_set_shell(const int nr_atom, const int nr_prim);
  int get_shell_start(const int nr_atom, const int nr_shell, const bool debug = false);
  int get_shell_start_in_primitives(const int nr_atom, const int nr_shell);
  int get_shell_end(const int nr_atom, const int nr_shell, const bool debug = false);
  bool push_back_atom(const std::string& label, const double x, const double y, const double z, const int charge);
  //atom get_atom(int nr) { if(nr >= 0 && nr < ncen) return atoms[nr]; else return atom(); };
  bool push_back_atom_basis_set(const int nr, const double exponent, const double coefficient, const int type, const int shell)
  {
    if (nr <= ncen && nr >= 0) return atoms[nr].push_back_basis_set(exponent, coefficient, type, shell);
    else return false;
  };
  void print_atom_long(const int nr) { if (nr <= ncen && nr >= 0) atoms[nr].print_values_long(); };
  int get_atom_charge(const int& nr) const;
  unsigned int get_atom_integer_mass(const unsigned int atomnr);
  double get_atom_real_mass(const int atomnr);
  //----------Calcualtion of Properties
  //double compute_dens(const double* PosGrid, const int atom = -1);
  //This second version will use phi[nmo] and d[4][ncen] as scratch instead of allocating new ones
  double compute_dens(const double& Pos1, const double& Pos2, const double& Pos3, const bool& add_ECP_dens = true);
  double compute_dens(const double& Pos1, const double& Pos2, const double& Pos3, std::vector<std::vector<double>>& d, std::vector<double>& phi, const bool& add_ECP_dens = true);
  void computeValues(const double* PosGrid, double& Rho, double& normGrad, double* Hess, double& Elf, double& Eli, double& Lap, const bool& add_ECP_dens = true);
  void computeLapELIELF(const double* PosGrid, double& Elf, double& Eli, double& Lap);
  void computeELIELF(const double* PosGrid, double& Elf, double& Eli);
  void computeLapELI(const double* PosGrid, double& Eli, double& Lap);
  void computeELI(const double* PosGrid, double& Eli);
  void computeELF(const double* PosGrid, double& Elf);
  double computeMO(const double* PosGrid, const int& mo);
  double compute_MO_spherical(const double& Pos1, const double& Pos2, const double& Pos3, const int& MO);
  double computeESP(double* PosGrid, std::vector<std::vector<double> >& d2);
  //----------DM Handling--------------------------------
  void push_back_DM(double value);
  bool set_DM(int nr, double value);
  double get_DM(int nr);
  int get_DM_size() { return DensityMatrix.size(); };
  //----------S_DM Handling--------------------------------
  void push_back_SDM(double value);
  bool set_SDM(int nr, double value);
  double get_SDM(int nr);
  int get_SDM_size() { return SpinDensityMatrix.size(); };
  //-----------Cube handling-------------------------
  bool push_back_cube(std::string filepath, bool full, bool expert = false);
  void push_back_cube(cube given) { cub.push_back(given); };
  void pop_back_cube();

  //-----------Pointer to members---------------------
  int* get_ptr_types() { return &types[0]; };
  int* get_ptr_centers() { return &centers[0]; };
  double* get_ptr_exponents() { return &exponents[0]; };
  double* get_ptr_mo_coefficients(int mo);

};
#include "mo_class.h"