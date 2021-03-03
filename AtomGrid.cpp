#include <cmath>
#include <limits>
#include <fstream>

#include "AtomGrid.h"
#include "becke_partitioning.h"
#include "error_handling.h"
#include "grid_radial.h"
#include "convenience.h"
#include "parameters.h"
#include "sphere_lebedev_rule.h"

#ifdef _WIN32
    #define NOMIMAX
    #include <algorithm>
    #include <io.h>
#endif
const double PI = 3.14159265358979323846;   /* pi */

//                       3,     5     7,    9,    11,   13,   15,   17
//                      19,    21
int lebedev_table[33] = {6,    14,   26,   38,   50,   74,   86,   110,
                         146,  170,  194,  230,  266,  302,  350,  434,
                         590,  770,  974,  1202, 1454, 1730, 2030, 2354,
                         2702, 3074, 3470, 3890, 4334, 4802, 5294, 5810};

int get_closest_num_angular(int n)
{
    int m;

    for (int i = 0; i < MAX_ANGULAR_ORDER; i++)
    {
        m = lebedev_table[i];
        if (m >= n)
            return m;
    }

    NUMGRID_ERROR("Input n too high in get_closest_num_angular");

    // this statement is unreachable and only here to not see a compiler warning
    return -1;
}

int get_angular_order(int n)
{
    for (int i = 0; i < MAX_ANGULAR_ORDER; i++)
    {
        if (lebedev_table[i] == n)
            return i;
    }

    NUMGRID_ERROR("No match found in get_angular_offset");

    // this statement is unreachable and only here to not see a compiler warning
    return -1;
}

AtomGrid::AtomGrid(const double radial_precision,
                   const int min_num_angular_points,
                   const int max_num_angular_points,
                   const int proton_charge,
                   const double alpha_max,
                   const int max_l_quantum_number,
                   const double alpha_min[])
{
    int min_num_angular_points_closest =
        get_closest_num_angular(min_num_angular_points);
    int max_num_angular_points_closest =
        get_closest_num_angular(max_num_angular_points);

    double *angular_x = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];
    double *angular_y = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];
    double *angular_z = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];
    double *angular_w = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];

    for (int i = get_angular_order(min_num_angular_points_closest);
         i < get_angular_order(max_num_angular_points_closest) + 1;
         ++i)
    {
        int angular_off = i * MAX_ANGULAR_GRID;
        ld_by_order(lebedev_table[i],
                    &angular_x[angular_off],
                    &angular_y[angular_off],
                    &angular_z[angular_off],
                    &angular_w[angular_off]);
    }

    // obtain radial parameters
    double r_inner = get_r_inner(radial_precision,
                                 alpha_max * 2.0); // factor 2.0 to match DIRAC
	double h = (std::numeric_limits<double>::max)();
    double r_outer = 0.0;
    for (int l = 0; l <= max_l_quantum_number; l++)
    {
        if (alpha_min[l] > 0.0)
        {
            r_outer =
                (std::max)(r_outer,
                         get_r_outer(radial_precision,
                                     alpha_min[l],
                                     l,
                                     4.0 * bragg_angstrom[proton_charge]));
            NUMGRID_ASSERT(r_outer > r_inner);
            h = (std::min)(h,
                         get_h(radial_precision, l, 0.1 * (r_outer - r_inner)));
        }
    }
    NUMGRID_ASSERT(r_outer > h);

    num_radial_grid_points_ = 0;

    double rb = bragg_angstrom[proton_charge] /
                (5.0 * 0.529177249); // factors match DIRAC code
    double c = r_inner / (exp(h) - 1.0);
    int num_radial = int(log(1.0 + (r_outer / c)) / h);
    for (int irad = 0; irad < num_radial; irad++)
    {
        double radial_r = c * (exp(double(irad + 1.0) * h) - 1.0);
        double radial_w = (radial_r + c) * radial_r * radial_r * h;

        radial_atom_grid_r_bohr_.push_back(radial_r);
        radial_atom_grid_w_.push_back(radial_w);
        num_radial_grid_points_++;

        int num_angular = max_num_angular_points_closest;
        if (radial_r < rb)
        {
            num_angular = static_cast<int>(max_num_angular_points_closest *
                                           (radial_r / rb));
            num_angular = get_closest_num_angular(num_angular);
            if (num_angular < min_num_angular_points_closest)
                num_angular = min_num_angular_points_closest;
        }

        int angular_off = get_angular_order(num_angular) * MAX_ANGULAR_GRID;

        for (int iang = 0; iang < num_angular; iang++)
        {
            atom_grid_x_bohr_.push_back(angular_x[angular_off + iang] * radial_r);
            atom_grid_y_bohr_.push_back(angular_y[angular_off + iang] * radial_r);
            atom_grid_z_bohr_.push_back(angular_z[angular_off + iang] * radial_r);

            atom_grid_w_.push_back(4.0 * PI * angular_w[angular_off + iang] * radial_w);
        }
    }

    delete[] angular_x;
    delete[] angular_y;
    delete[] angular_z;
    delete[] angular_w;
}

AtomGrid::AtomGrid(const double radial_precision,
                   const int min_num_angular_points,
                   const int max_num_angular_points,
                   const int proton_charge,
                   const double alpha_max,
                   const int max_l_quantum_number,
                   const double alpha_min[],
                   const bool debug,
                   std::ofstream& file)
 {
    using namespace std;
     int min_num_angular_points_closest =
         get_closest_num_angular(min_num_angular_points);
     int max_num_angular_points_closest =
         get_closest_num_angular(max_num_angular_points);

     double* angular_x = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];
     double* angular_y = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];
     double* angular_z = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];
     double* angular_w = new double[MAX_ANGULAR_ORDER * MAX_ANGULAR_GRID];

     for (int i = get_angular_order(min_num_angular_points_closest);
         i < get_angular_order(max_num_angular_points_closest) + 1;
         ++i)
     {
         int angular_off = i * MAX_ANGULAR_GRID;
         ld_by_order(lebedev_table[i],
             &angular_x[angular_off],
             &angular_y[angular_off],
             &angular_z[angular_off],
             &angular_w[angular_off]);
     }

     // obtain radial parameters
     double r_inner = get_r_inner(radial_precision,  alpha_max * 2.0); // factor 2.0 to match DIRAC
     double h = (std::numeric_limits<double>::max)();
     double r_outer = 0.0;
     for (int l = 0; l <= max_l_quantum_number; l++)
     {
         if (alpha_min[l] > 0.0)
         {
             if (debug) 
                 file << "ATOM GRID: " 
                    << "l= " << l 
                    << " r_inner: " << r_inner 
                    << " alpha_min: " << alpha_min[l] << endl;
             r_outer = std::max(r_outer, 
                                get_r_outer(radial_precision, 
                                    alpha_min[l], 
                                    l, 
                                    4.0 * bragg_angstrom[proton_charge])
                                );
             NUMGRID_ASSERT(r_outer > r_inner);
             h = std::min(h, get_h(radial_precision, 
                                    l, 
                                    0.1 * (r_outer - r_inner))
                          );
         }
     }
     NUMGRID_ASSERT(r_outer > h);

     if(debug) 
         file << "ATOM GRID: " 
            << "r_inner: " << r_inner 
            << " h: " << h 
            << " r_outer: " << r_outer << endl;

     num_radial_grid_points_ = 0;

     double rb = bragg_angstrom[proton_charge] / (5.0 * 0.529177249); // factors match DIRAC code
     double c = r_inner / (exp(h) - 1.0);
     int num_radial = int(log(1.0 + (r_outer / c)) / h);

     if (debug) 
         file << "ATOM GRID: " 
            << "rb: " << rb 
            << " c: " << c 
            << " num_radial: " << num_radial << endl;

     for (int irad = 0; irad < num_radial; irad++)
     {
         double radial_r = c * (exp(double(irad + 1.0) * h) - 1.0);
         double radial_w = (radial_r + c) * radial_r * radial_r * h;

         radial_atom_grid_r_bohr_.push_back(radial_r);
         radial_atom_grid_w_.push_back(radial_w);
         num_radial_grid_points_++;

         int num_angular = max_num_angular_points_closest;
         if (radial_r < rb)
         {
             num_angular = static_cast<int>(max_num_angular_points_closest *
                 (radial_r / rb));
             num_angular = get_closest_num_angular(num_angular);
             if (num_angular < min_num_angular_points_closest)
                 num_angular = min_num_angular_points_closest;
         }

         int angular_off = get_angular_order(num_angular) * MAX_ANGULAR_GRID;

         for (int iang = 0; iang < num_angular; iang++)
         {
             atom_grid_x_bohr_.push_back(angular_x[angular_off + iang] * radial_r);
             atom_grid_y_bohr_.push_back(angular_y[angular_off + iang] * radial_r);
             atom_grid_z_bohr_.push_back(angular_z[angular_off + iang] * radial_r);

             atom_grid_w_.push_back(4.0 * PI * angular_w[angular_off + iang] * radial_w);
         }
     }

     delete[] angular_x;
     delete[] angular_y;
     delete[] angular_z;
     delete[] angular_w;
 }

AtomGrid::~AtomGrid() {}

int AtomGrid::get_num_grid_points() const { return atom_grid_x_bohr_.size(); }

int AtomGrid::get_num_radial_grid_points() const
{
    return num_radial_grid_points_;
}

void AtomGrid::get_grid(const int num_centers,
    const int center_index,
    const double* x_coordinates_bohr,
    const double* y_coordinates_bohr,
    const double* z_coordinates_bohr,
    const int* proton_charges,
    double grid_x_bohr[],
    double grid_y_bohr[],
    double grid_z_bohr[],
    double grid_aw[],
    double grid_mw[]) const
{

    for (size_t ipoint = 0; ipoint < get_num_grid_points(); ipoint++)
    {
        grid_x_bohr[ipoint] =
            atom_grid_x_bohr_[ipoint] + x_coordinates_bohr[center_index];
        grid_y_bohr[ipoint] =
            atom_grid_y_bohr_[ipoint] + y_coordinates_bohr[center_index];
        grid_z_bohr[ipoint] =
            atom_grid_z_bohr_[ipoint] + z_coordinates_bohr[center_index];

        double becke_w = 1.0;

        // if there are no other centers
        // no point in partitioning the space
        if (num_centers > 1)
        {
            becke_w = get_becke_w(num_centers,
                proton_charges,
                x_coordinates_bohr,
                y_coordinates_bohr,
                z_coordinates_bohr,
                center_index,
                grid_x_bohr[ipoint],
                grid_y_bohr[ipoint],
                grid_z_bohr[ipoint]);
        }

        grid_mw[ipoint] = atom_grid_w_[ipoint] * becke_w;
        grid_aw[ipoint] = atom_grid_w_[ipoint];
    }
}

void AtomGrid::get_grid(const int num_centers,
                        const int center_index,
                        const double* x_coordinates_bohr,
                        const double* y_coordinates_bohr,
                        const double* z_coordinates_bohr,
                        const int* proton_charges,
                        double grid_x_bohr[],
                        double grid_y_bohr[],
                        double grid_z_bohr[],
                        double grid_w[]) const
{

    for (size_t ipoint = 0; ipoint < get_num_grid_points(); ipoint++)
    {
        grid_x_bohr[ipoint] =
            atom_grid_x_bohr_[ipoint] + x_coordinates_bohr[center_index];
        grid_y_bohr[ipoint] =
            atom_grid_y_bohr_[ipoint] + y_coordinates_bohr[center_index];
        grid_z_bohr[ipoint] =
            atom_grid_z_bohr_[ipoint] + z_coordinates_bohr[center_index];

        double becke_w = 1.0;

        // if there are no other centers
        // no point in partitioning the space
        if (num_centers > 1)
        {
            becke_w = get_becke_w(num_centers,
                                  proton_charges,
                                  x_coordinates_bohr,
                                  y_coordinates_bohr,
                                  z_coordinates_bohr,
                                  center_index,
                                  grid_x_bohr[ipoint],
                                  grid_y_bohr[ipoint],
                                  grid_z_bohr[ipoint]);
        }

        grid_w[ipoint] = atom_grid_w_[ipoint] * becke_w;
    }
}

void AtomGrid::get_radial_grid(double grid_r_bohr[], double grid_w[]) const
{
    for (size_t ipoint = 0; ipoint < num_radial_grid_points_; ipoint++)
    {
        grid_r_bohr[ipoint] = radial_atom_grid_r_bohr_[ipoint];
        grid_w[ipoint] = radial_atom_grid_w_[ipoint];
    }
}
