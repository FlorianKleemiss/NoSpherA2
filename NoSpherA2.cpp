#define WIN32_LEAN_AND_MEAN
#include "convenience.h"
#include "fchk.h"
#include "cube.h"
#include "basis_set.h"
#include "structure_factors.h"
#include "properties.h"
#include "cell.h"
#include "spherical_density.h"
#include "test_functions.h"

using namespace std;
#ifdef _WIN32
typedef enum omp_sched_t {
    // schedule kinds 
    omp_sched_static = 0x1,
    omp_sched_dynamic = 0x2,
    omp_sched_guided = 0x3,
    omp_sched_auto = 0x4,

    // schedule modifier 
    omp_sched_monotonic = 0x80000000u
} omp_sched_t;
#endif

int main(int argc, char** argv)
{
  ofstream log_file("NoSpherA2.log", ios::out);
  auto coutbuf = std::cout.rdbuf(log_file.rdbuf()); //save and redirect
  vector<WFN> wavy;
  options opt(argc, argv);
  opt.digest_options();
  if (opt.threads != -1) {
    omp_set_num_threads(opt.threads);
    //omp_set_dynamic(0);
#ifdef _WIN32
    _putenv("OMP_SCHEDULE=dynamic,2");
#else
    putenv("OMP_SCHEDULE=dynamic,2");
#endif
  }
  log_file << NoSpherA2_message();
  if (!opt.no_date) {
    log_file << build_date();
  }
  log_file.flush();
  //Perform fractal dimensional analysis and quit 
  if (opt.fract) {
    WFN wav(6);
    cube residual(opt.fract_name, true, wav, std::cout, opt.debug);
    residual.fractal_dimension(0.01);
    log_file.flush();
    log_file.close();
    return 0;
  }
  //perform combine_mo and quit
  if (opt.combine_mo.size() != 0) {
    WFN wavy1(2);
    WFN wavy2(2);
    WFN wavy3(2);
    wavy1.read_wfn(opt.combine_mo[0], false, cout);
    wavy2.read_wfn(opt.combine_mo[1], false, cout);
    for (int i = 0; i < wavy1.get_ncen(); i++) {
      wavy3.push_back_atom(wavy1.get_atom(i));
    }
    for (int i = 0; i < wavy2.get_ncen(); i++) {
      wavy3.push_back_atom(wavy2.get_atom(i));
    }
    cout << "In total we have " << wavy3.get_ncen() << " atoms" << endl;

    double MinMax1[6];
    int steps1[3];
    readxyzMinMax_fromWFN(wavy1, MinMax1, steps1, opt.radius, opt.resolution, true);
    double MinMax2[6];
    int steps2[3];
    readxyzMinMax_fromWFN(wavy2, MinMax2, steps2, opt.radius, opt.resolution, true);

    cout << "Read input\nCalculating for MOs ";
    for (int v1 = 0; v1 < opt.cmo1.size(); v1++) {
      cout << opt.cmo1[v1] << " ";
    }
    cout << "of fragment 1 and MOs ";
    for (int v1 = 0; v1 < opt.cmo2.size(); v1++) {
      cout << opt.cmo2[v1] << " ";
    }
    cout << "of fragment 2" << endl;
    double MinMax[6]{ 100,100,100,-100,-100,-100 };
    int steps[3]{ 0,0,0 };
    for (int i = 0; i < 3; i++) {
      if (MinMax1[i] < MinMax[i])
        MinMax[i] = MinMax1[i];
      if (MinMax1[i + 3] > MinMax[i + 3])
        MinMax[i + 3] = MinMax1[i + 3];
    }
    for (int i = 0; i < 3; i++) {
      if (MinMax2[i] < MinMax[i])
        MinMax[i] = MinMax2[i];
      if (MinMax2[i + 3] > MinMax[i + 3])
        MinMax[i + 3] = MinMax2[i + 3];
      steps[i] = (int)ceil(constants::bohr2ang(MinMax[i + 3] - MinMax[i]) / 0.1);
    }
    int counter = 0;
    cube total(steps[0], steps[1], steps[2], 0, true);
    cube MO1(steps[0], steps[1], steps[2], 0, true);
    MO1.give_parent_wfn(wavy3);
    MO1.set_na(wavy3.get_ncen());
    cube MO2(steps[0], steps[1], steps[2], 0, true);
    vector<string> fns;
    for (int i = 0; i < 3; i++) {
      MO1.set_origin(i, MinMax[i]);
      MO1.set_vector(i, i, (MinMax[i + 3] - MinMax[i]) / steps[i]);
      total.set_origin(i, MinMax[i]);
      total.set_vector(i, i, (MinMax[i + 3] - MinMax[i]) / steps[i]);
      MO2.set_origin(i, MinMax[i]);
      MO2.set_vector(i, i, (MinMax[i + 3] - MinMax[i]) / steps[i]);
    }
    for (int v1 = 0; v1 < opt.cmo1.size(); v1++) {
      MO1.set_zero(); 
      Calc_MO(MO1, opt.cmo1[v1]-1, wavy1, -1, 400, std::cout);
      for (int j = 0; j < opt.cmo2.size(); j++) {
        counter++;
        cout << "Running: " << counter << " of " << opt.cmo2.size() * opt.cmo1.size() << endl;
        string filename("");
        MO2.set_zero();
        Calc_MO(MO2, opt.cmo2[j]-1, wavy2, -1, 400, std::cout);
        cout << "writing files..." << flush;
        filename = get_basename_without_ending(wavy1.get_path()) + "_" + std::to_string(opt.cmo1[v1]) + "+" + get_basename_without_ending(wavy2.get_path()) + "_" + std::to_string(opt.cmo2[j]) + ".cube";
        fns.push_back(filename);
        total.set_zero();
        total = MO1;
        total += MO2;
        total.write_file(filename, false);
        filename = get_basename_without_ending(wavy1.get_path()) + "_" + std::to_string(opt.cmo1[v1]) + "-" + get_basename_without_ending(wavy2.get_path()) + "_" + std::to_string(opt.cmo2[j]) + ".cube";
        fns.push_back(filename);
        total.set_zero();
        total = MO1;
        total -= MO2;
        total.write_file(filename, false);
        cout << " ... done!" << endl;
      }
    }
    ofstream vmd("read_files.vmd");
    vmd << "mol addrep 0\nmol new {" + fns[0] + "} type {cube} first 0 last -1 step 1 waitfor 1 volsets {0 }\n";
    vmd << "animate style Loop\n";
    for(int i=1; i<fns.size(); i++)
      vmd << "mol addfile {" + fns[i] + "} type {cube} first 0 last -1 step 1 waitfor 1 volsets {0 } 0\n";
    vmd << "animate style Loop\ndisplay projection Orthographic\ndisplay depthcue off\n";
    vmd << "axes location Off\ndisplay rendermode GLSL\ncolor Display Background white\ncolor Element P purple\n";
    vmd << "color Element Ni green\ncolor Element C gray\nmol modstyle 0 0 CPK 1.000000 0.300000 12.000000 12.000000\n";
    vmd << "mol modcolor 0 0 Element\nmol color Element\nmol representation CPK 1.000000 0.300000 22.000000 22.000000\n";
    vmd << "mol selection all\nmol material Transparent\nmol addrep 0\nmol modstyle 1 0 Isosurface 0.020000 0 0 0 1 1\n";
    vmd << "mol modcolor 1 0 ColorID 0\nmol selection all\nmol material Transparent\nmol addrep 0\nmol modstyle 2 0 Isosurface -0.020000 0 0 0 1 1\nmol modcolor 2 0 ColorID 1\n";
    vmd << "mol selection all\nmol material Transparent\n";
    vmd.flush();
    vmd.close();
    exit(0);
  }
  //Performs MTC and CMTC calcualtions, that is multiple wfns with either one or multiple cifs and 1 common hkl.
  if (opt.cif_based_combined_tsc_calc || opt.combined_tsc_calc) {
    err_checkf(opt.hkl != "" || opt.dmin != 99.0, "No hkl specified and no dmin value given", log_file);
    if (opt.combined_tsc_calc) err_checkf(opt.cif != "", "No cif specified", log_file);
    //First make sure all files exist
    if (opt.cif_based_combined_tsc_calc) err_checkf(opt.combined_tsc_calc_files.size() == opt.combined_tsc_calc_cifs.size(), "Unequal number of CIFs and WFNs impossible!", log_file);
    for (int i = 0; i < opt.combined_tsc_calc_files.size(); i++) {
      err_checkf(exists(opt.combined_tsc_calc_files[i]), "Specified file for combined calculation doesn't exist! " + opt.combined_tsc_calc_files[i], log_file);
      if(opt.cif_based_combined_tsc_calc) err_checkf(exists(opt.combined_tsc_calc_cifs[i]), "Specified file for combined calculation doesn't exist! " + opt.combined_tsc_calc_cifs[i], log_file);
    }
    wavy.resize(opt.combined_tsc_calc_files.size());
    for (int i = 0; i < opt.combined_tsc_calc_files.size(); i++) {
      log_file << "Reading: " << setw(44) << opt.combined_tsc_calc_files[i] << flush;
      wavy[i].read_known_wavefunction_format(opt.combined_tsc_calc_files[i], log_file);
      if (opt.ECP) {
        wavy[i].set_has_ECPs(true);
      }
      if (opt.set_ECPs) {
        wavy[i].set_ECPs(opt.ECP_nrs, opt.ECP_elcounts);
      }
      log_file << " done!\nNumber of atoms in Wavefunction file: " << wavy[i].get_ncen() << " Number of MOs: " << wavy[i].get_nmo() << endl;
    }

    vector<string> known_scatterer;
    vector<vec> known_kpts;
    tsc_block result;
    for (int i = 0; i < opt.combined_tsc_calc_files.size(); i++) {
      known_scatterer = result.get_scatterers();
      if (wavy[i].get_origin() != 7) {
        result.append(calculate_structure_factors_MTC(
          opt,
          wavy,
          log_file,
          known_scatterer,
          i,
          &known_kpts
        ), log_file);
      }
      else {
        result.append(MTC_thakkar_sfac(
          opt,
          log_file,
          known_scatterer,
          wavy,
          i
        ), log_file);
      }
    }

    known_scatterer = result.get_scatterers();
    log_file << "Final number of atoms in .tsc file: " << known_scatterer.size() << endl;
#ifdef _WIN64
    time_t start = time(NULL);
    time_t end_write;
#endif
    log_file << "Writing tsc file... " << flush;
    if (opt.binary_tsc)
      result.write_tscb_file();
    if (opt.old_tsc) {
      result.write_tsc_file(opt.cif);
    }
    log_file << " ... done!" << endl;
#ifdef _WIN64
    end_write = time(NULL);

    if (end_write - start < 60) log_file << "Writing Time: " << fixed << setprecision(0) << end_write - start << " s\n";
    else if (end_write - start < 3600) log_file << "Writing Time: " << fixed << setprecision(0) << floor((end_write - start) / 60) << " m " << (end_write - start) % 60 << " s\n";
    else log_file << "Writing Time: " << fixed << setprecision(0) << floor((end_write - start) / 3600) << " h " << ((end_write - start) % 3600) / 60 << " m\n";
    log_file << endl;
#endif
    log_file.flush();
    log_file.close();
    return 0;
  }
  //Performs the Thakkar IAM
  if (opt.iam_switch) {
    if (opt.debug) {
      log_file << "I am doing a Thakkar IAM!" << endl;
    }
    err_checkf(opt.xyz_file != "", "No xyz specified", log_file);
    err_checkf(exists(opt.xyz_file), "xyz doesn't exist", log_file);
    wavy.push_back(WFN(0));
    wavy[0].read_known_wavefunction_format(opt.xyz_file, log_file, opt.debug);

    if (opt.electron_diffraction && opt.debug)
      log_file << "Making Electron diffraction scattering factors, be carefull what you are doing!" << endl;
    if (opt.debug)
      log_file << "Entering Structure Factor Calculation!" << endl;
    err_checkf(thakkar_sfac(opt, log_file, wavy[0]), "Error during SF Calculation!", log_file);
    log_file.flush();
    log_file.close();
    return 0;
  }
  //This one has conversion to fchk and calculation of one single tsc file
  if (opt.wfn != "" && !opt.calc && !opt.gbw2wfn && opt.sfac_scan == 0.0) {
    wavy.push_back(WFN(0));
    log_file << "Reading: " << setw(44) << opt.wfn << flush;
    wavy[0].read_known_wavefunction_format(opt.wfn, log_file, opt.debug);
    wavy[0].set_method(opt.method);
    wavy[0].set_multi(opt.mult);
    if (opt.ECP) {
      wavy[0].set_has_ECPs(true);
    }
    if (opt.set_ECPs) {
      log_file << "Adding ECPs" << endl;
      wavy[0].set_ECPs(opt.ECP_nrs, opt.ECP_elcounts);
    }
    log_file << " done!\nNumber of atoms in Wavefunction file: " << wavy[0].get_ncen() << " Number of MOs: " << wavy[0].get_nmo() << endl;

    if (opt.basis_set != "" || opt.fchk != "") {
      //Make a fchk out of the wfn/wfx file
      join_path(opt.basis_set_path, opt.basis_set);
      if (opt.debug) log_file << "Checking for " << opt.basis_set_path << endl;
      err_checkf(exists(opt.basis_set_path), "Basis set file does not exist!", log_file);
      wavy[0].set_basis_set_name(opt.basis_set_path);

      string outputname;
      if (opt.fchk != "")
        outputname = opt.fchk;
      else {
        outputname = wavy[0].get_path();
        if (opt.debug) log_file << "Loaded path..." << endl;
        size_t where(0);
        int len = 4;
        if (wavy[0].get_origin() == 2) where = outputname.find(".wfn");
        else if (wavy[0].get_origin() == 4) where = outputname.find(".ffn");
        else if (wavy[0].get_origin() == 6) where = outputname.find(".wfx");
        else if (wavy[0].get_origin() == 8) where = outputname.find(".molden"), len = 7;
        else if (wavy[0].get_origin() == 9) where = outputname.find(".gbw");
        if (where >= outputname.length() && where != string::npos) err_checkf(false, "Cannot make output file name!", log_file);
        else outputname.erase(where, len);
      }
      wavy[0].assign_charge(wavy[0].calculate_charge());
      if (opt.mult == 0) err_checkf(wavy[0].guess_multiplicity(log_file), "Error guessing multiplicity", log_file);
      else wavy[0].assign_multi(opt.mult);
      free_fchk(log_file, outputname, "", wavy[0], opt.debug, true);
    }
    if (opt.cif != "" || opt.hkl != "") {
      // Calculate tsc file from given files
      if (opt.debug)
        log_file << "Entering Structure Factor Calculation!" << endl;
      if (opt.electron_diffraction)
        log_file << "Making Electron diffraction scattering factors, be carefull what you are doing!" << endl;
      if (wavy[0].get_origin() != 7)
        err_checkf(calculate_structure_factors_HF(
          opt,
          wavy[0],
          log_file), "Error during SF Calcualtion", log_file);
      else
        err_checkf(thakkar_sfac(
          opt,
          log_file,
          wavy[0]), "Error during SF Calcualtion", log_file);
    }
    log_file.flush();
    log_file.close();
    return 0;
  }
  //Contains all calculations of properties and cubes
  if (opt.calc) {
    properties_calculation(opt);
    log_file.flush();
    log_file.close();
    return 0;
  }
  //Test for molden and ECP files
  if (opt.density_test_cube) {
    test_density_cubes(opt, log_file);
    return 0;
  }
  //Converts gbw file to wfn file and leaves
  if (opt.gbw2wfn) {
    err_checkf(opt.wfn != "", "No Wavefunction given!", log_file);
    wavy.push_back(WFN(9));
    wavy[0].read_known_wavefunction_format(opt.wfn, log_file);
    wavy[0].write_wfn("converted.wfn", false, false);
    log_file.flush();
    log_file.close();
    return 0;
  }
  //Calcualtes data for Thakkar IAM Paper
  if (opt.thakkar_d_plot) {
    thakkar_d_test(opt);
    return 0;
  }
  //Calcualtes data sampling a full sphere for the first atom in a wavefunction and tries to compare to 
  //Thakkar densities. Make sure Thakkar ions are present if your selected atom
  if (opt.sfac_scan > 0.0) {
    sfac_scan(opt, log_file);
    return 0;
  }
  if (opt.spherical_harmonic) {
    spherical_harmonic_test();
    return 0;
  }
  if (opt.ML_test) {
    ML_test();
    return 0;
  }
  if (opt.coef_file != "") {
    if (opt.cif != "" && opt.xyz_file != "") {
      wavy.push_back(WFN(7));
      //Read the xyzfile
      wavy[0].read_xyz(opt.xyz_file, std::cout, opt.debug);
      //Fill WFN wil the primitives of the JKFit basis (currently hardcoded)
      int nr_coefs = load_basis_into_WFN(wavy[0], TZVP_JKfit);
      //Run generation of tsc file
      if(opt.SALTED_BECKE || opt.SALTED)
        err_checkf(calculate_structure_factors_RI_No_H(
          opt,
          wavy[0],
          log_file,
          nr_coefs), "Error during ML-SF Calcualtion", log_file);
      else
        err_checkf(calculate_structure_factors_RI(
          opt,
          wavy[0],
          log_file,
          nr_coefs), "Error during ML-SF Calcualtion", log_file);
    }
    else {
      cube_from_coef_npy(opt.coef_file, opt.xyz_file);
    }
    return 0;
  }
  std::cout << NoSpherA2_message();
  if (!opt.no_date) std::cout << build_date();
  std::cout << "Did not understand the task to perform!\n" << help_message() << endl;
  log_file.flush();
  log_file.close();
  return 0;
}