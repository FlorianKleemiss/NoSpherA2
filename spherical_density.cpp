#include "spherical_density.h"
#include "convenience.h"
#include "Thakkar_coefs.h"
#include "def2-ECPs.h"
#include <complex>
#include <cmath>

Thakkar::Thakkar(const int g_atom_number) : Spherical_Atom() {
	atomic_number = g_atom_number;
	nex = &(Thakkar_nex[0]);
	ns = &(Thakkar_ns[0]);
	np = &(Thakkar_np[0]);
	nd = &(Thakkar_nd[0]);
	nf = &(Thakkar_nf[0]);
	occ = &(Thakkar_occ[0]);
	n = &(Thakkar_n[0]);
	z = &(Thakkar_z[0]);
	c = &(Thakkar_c[0]);
	charge = 0;
};
Thakkar::Thakkar() : Spherical_Atom() {
	atomic_number = 1;
	nex = &(Thakkar_nex[0]);
	ns = &(Thakkar_ns[0]);
	np = &(Thakkar_np[0]);
	nd = &(Thakkar_nd[0]);
	nf = &(Thakkar_nf[0]);
	occ = &(Thakkar_occ[0]);
	n = &(Thakkar_n[0]);
	z = &(Thakkar_z[0]);
	c = &(Thakkar_c[0]);
	charge = 0;
};

const int Spherical_Atom::first_ex() {
	if (atomic_number == 1) return 0;
	else if (atomic_number > 113) return 200000000;
	int ex = 0;
	for (int i = 0; i < atomic_number - 1; i++)
		ex += nex[i];
	return ex;
};

const int Spherical_Atom::previous_element_coef() {
	if (atomic_number <= 2) return 0;
	int counter = 0;
	for (int temp = atomic_number - 2; temp >= 0; temp--) {
		for (int m = 0; m < 7; m++)
			if (occ[temp * 19 + 0 + m] != 0)
				counter += ns[temp];
		for (int m = 0; m < 6; m++)
			if (occ[temp * 19 + 7 + m] != 0)
				counter += np[temp];
		for (int m = 0; m < 4; m++)
			if (occ[temp * 19 + 13 + m] != 0)
				counter += nd[temp];
		for (int m = 0; m < 2; m++)
			if (occ[temp * 19 + 17 + m] != 0)
				counter += nf[temp];
	}
	return counter;
};

void Thakkar::calc_orbs(
	int& nr_ex,
	int& nr_coef,
	const double& dist,
	const int& offset,
	const int* n_vector,
	const int lower_m,
	const int upper_m,
	double* Orb) {
	double exponent;
	for (int ex = 0; ex < n_vector[atomic_number - 1]; ex++) {
		for (int m = lower_m; m < upper_m; m++) {
			if (occ[offset + m] == 0) continue;
			exponent = -z[nr_ex] * dist;
			if (exponent > -46.5) { // Corresponds to at least 1E-20
				if (n[nr_ex] == 1)
					Orb[m] += c[nr_coef] * exp(exponent);
				else
					Orb[m] += c[nr_coef] * pow(dist, n[nr_ex] - 1) * exp(exponent);
			}
			nr_coef++;
		}
		nr_ex++;
	}
}

const double Thakkar::get_radial_density(double& dist) {
	//Speedup things for H
	if (atomic_number == 1)
		return 6.0835 * exp(-2.3 * dist) / constants::FOUR_PI;

	double Rho = 0.0;
	int nr_ex = first_ex();
	if (nr_ex == 200000000)
		return -20;
	int nr_coef = previous_element_coef();

	double Orb[19] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	const int offset = (atomic_number - 1) * 19;

	calc_orbs(nr_ex, nr_coef, dist, offset, ns, 0, 7, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, np, 7, 13, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, nd, 13, 17, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, nf, 17, 19, Orb);

	for (int m = 0; m < 19; m++) {
		if (Orb[m] == 0 || occ[offset + m] == 0)
			continue;
		Rho += occ[offset + m] * pow(Orb[m], 2);
	}
	return Rho / (constants::FOUR_PI);
};

constexpr double cosinus_integral(const int N, const double z, const double k);

static constexpr double sinus_integral(const int N, const double z, const double k) {
	//Calculates the integral 0 - inf r ^ N e ^ -zr sin(kr) dr through recursion using the general integral int e^ax sin(bx) dx = -e^-ax/(a^2+b^2) * (a sin(bx) + b cos(bx)) and partial integration
	if (N == 0)
		return k / (z * z + k * k);
	else
		return N / (z * z + k * k) * (z * sinus_integral(N - 1, z, k) + k * cosinus_integral(N - 1, z, k));
};

constexpr double cosinus_integral(const int N, const double z, const double k) {
	//Calculates the integral 0 - inf r ^ N e ^ -zr cos(kr) dr through recursion using the general integral int e^ax cos(bx) dx = -e^-ax/(a^2+b^2) * (a cos(bx) - b sin(bx)) and partial integration
	if (N == 0)
		return z / (z * z + k * k);
	else
		return N / (z * z + k * k) * (z * cosinus_integral(N - 1, z, k) - k * sinus_integral(N - 1, z, k));
};

const double Thakkar::get_form_factor(const double& k_vector) {
	return get_custom_form_factor(k_vector, 7, 6, 4, 2, 0, 0, 0, 0);
};

const double Thakkar::get_core_form_factor(const double& k_vector, const int& core_els) {
	int max_s = 0, max_p = 0, max_d = 0, max_f = 0;
	if (core_els == 2) {
		max_s = 1; max_p = 0; max_d = 0; max_f = 0;
	}
	else if (core_els == 10) {
		max_s = 2; max_p = 1; max_d = 0; max_f = 0;
	}
	else if (core_els == 18) {
		max_s = 3; max_p = 2; max_d = 0; max_f = 0;
	}
	else if (core_els == 28) {
		max_s = 3; max_p = 2; max_d = 1; max_f = 0;
	}
	else if (core_els == 46) {
		max_s = 4; max_p = 3; max_d = 2; max_f = 0;
	}
	else if (core_els == 60) {
		max_s = 4; max_p = 3; max_d = 2; max_f = 1;
	}

	return get_custom_form_factor(k_vector, max_s, max_p, max_d, max_f, 0, 0, 0, 0);
};

static double calc_int(const int& occ, const double& coef, const double& exp, const int& radial_exp, const double& k_vector) {
	return occ * coef * sinus_integral(radial_exp, exp, k_vector);
}

static double calc_int_at_k0(const int& occ, const double& coef, const double& exp, const int& radial_exp, const double&) {
	return occ * coef * constants::ft[radial_exp] / pow(exp, radial_exp + 1);
}

double Thakkar::calc_type(
	int& nr_ex,
	int& nr_coef,
	const double& k_vector,
	const int& offset,
	const int* n_vector,
	const int lower_m,
	const int upper_m,
	const int& max,
	const int& min) {

	std::function<double(const int&, const double&, const double&, const int&, const double&)> func;
	if (k_vector == 0)
		func = calc_int_at_k0;
	else
		func = calc_int;
	const int l_n = n_vector[atomic_number - 1];
	double temp, result = 0;
	int i_j_distance = 0;
	for (int m = lower_m; m < upper_m; m++)
		if (occ[offset + m] != 0)
			i_j_distance++;
	for (int m = lower_m + min; m < lower_m + max; m++) {
		if (occ[offset + m] == 0) continue;
		for (int i = 0; i < l_n; i++) {
			for (int j = 0; j < l_n - i; j++) {
				temp = func(occ[offset + m],
					c[nr_coef + m - lower_m + i * i_j_distance] * c[nr_coef + m - lower_m + (i + j) * i_j_distance],
					z[nr_ex + i] + z[nr_ex + i + j],
					n[nr_ex + i] + n[nr_ex + i + j] - 1,
					k_vector);
				if (j != 0)
					result += 2 * temp;
				else
					result += temp;
			}
		}
	}
	nr_coef += i_j_distance * l_n;
	nr_ex += l_n;
	return result;
}

const double Thakkar::get_custom_form_factor(
	const double& k_vector,
	const int& max_s,
	const int& max_p,
	const int& max_d,
	const int& max_f,
	const int& min_s,
	const int& min_p,
	const int& min_d,
	const int& min_f) {

	double result(0.0);
	using namespace std;

	int nr_ex = first_ex();
	if (nr_ex == 200000000)
		return -20;
	int nr_coef = previous_element_coef();
	if (atomic_number == 1) nr_coef = 0;//return 16.0/pow(4.0+pow(k_vector*2.13*PI,2),2);
	const int offset = (atomic_number - 1) * 19;

	result += calc_type(nr_ex, nr_coef, k_vector, offset, ns, 0, 7, max_s, min_s);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, np, 7, 13, max_p, min_p);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, nd, 13, 17, max_d, min_d);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, nf, 17, 19, max_f, min_f);

	if (k_vector != 0)
		return result / k_vector;
	else return result;
};

Thakkar_Anion::Thakkar_Anion(int g_atom_number) {
	if (g_atom_number != 1 && g_atom_number != 6 && g_atom_number != 8 && g_atom_number != 15 && g_atom_number != 17) err_not_impl_f("Only selected anions are currently defined!", std::cout);
	atomic_number = g_atom_number;
	nex = &(Anion_nex[0]);
	ns = &(Anion_ns[0]);
	np = &(Anion_np[0]);
	nd = &(Anion_nd[0]);
	nf = &(Thakkar_nf[0]);
	occ = &(Anion_occ[0]);
	n = &(Anion_n[0]);
	z = &(Anion_z[0]);
	c = &(Anion_c[0]);
	charge = -1;
};

Thakkar_Cation::Thakkar_Cation(int g_atom_number) {
	if (g_atom_number < 3 || g_atom_number > 29) err_not_impl_f("Atoms with Z < 3 or bigger than 29 are not yet done!", std::cout);
	atomic_number = g_atom_number;
	nex = &(Cation_nex[0]);
	ns = &(Cation_ns[0]);
	np = &(Cation_np[0]);
	nd = &(Cation_nd[0]);
	nf = &(Thakkar_nf[0]);
	occ = &(Cation_occ[0]);
	n = &(Cation_n[0]);
	z = &(Cation_z[0]);
	c = &(Cation_c[0]);
	charge = +1;
};

const double sqr_pi = sqrt(3.14159265358979323846);

const double gauss_cos_integral(const int& N, const double& exp, const double& k_vector);


//NEED TO CHECK FOR FACTOR 2 DUE TO 2z or z in exponent
// This function calcualtes the integral of Int_0^Inf r^N exp(-zr^2) sin(kr) dr using a recursion of sinus and cosinus integrals with lower exponents of r
static const double gauss_sin_integral(const int& N, const double& exp, const double& k_vector) {
	if (N == 1) {
		return k_vector * sqr_pi * std::exp(-k_vector * k_vector / 4. / exp) / 4. / pow(exp, 3. / 2.);
	}
	else
		return k_vector / (2. * exp) * gauss_cos_integral(N-1, exp, k_vector) + (N-1) / (2. * exp) * gauss_sin_integral(N - 2, exp, k_vector);

};
const double gauss_cos_integral(const int& N, const double& exp, const double& k_vector) {
	if (N == 0) {
		return sqr_pi * std::exp(-k_vector * k_vector / 4. / exp) / 2. / pow(exp, 1. / 2.);
	}
	else
		return (N - 1) / (2. * exp) * gauss_cos_integral(N - 2, exp, k_vector) + k_vector / (2. * exp) * gauss_sin_integral(N - 1, exp, k_vector);
};

//the integral in case of a gaussian function should be 1/k \int r^(n) e^(-exp * r^2) sin(kr) dr
static double calc_Gaussian_int(const int& occ, const double& coef, const double& exp, const int& radial_exp, const double& k_vector) {
	return occ * coef * gauss_sin_integral(radial_exp, exp, k_vector) / k_vector;
}

static double calc_Gaussian_int_at_k0(const int& occ, const double& coef, const double& exp, const int& radial_exp, const double& k_vector) {
	const int N = radial_exp;
	return -pow(2.0, -N - 2.5) * pow(exp, -N - 1.5) * tgamma(N + 1.5);
}


Gaussian_Atom::Gaussian_Atom(int g_atom_number, std::string &basis) {
	atomic_number = g_atom_number;
	if (basis == "def2-ECP") {
		nex = &(def2_nex[0]);
		ns = &(def2_ns[0]);
		np = &(def2_np[0]);
		nd = &(def2_nd[0]);
		nf = &(def2_nf[0]);
		ng = &(def2_nf[0]);
		nh = &(def2_nf[0]);
		double def2_n_size = 0;
		for (int i = 0; i < 86; i++) {
			def2_n_size += def2_nex[i];
		}
		def2_n.resize(def2_n_size, 2);
		occ = &(def2_occ[0]);
		n = def2_n.data();
		z = &(def2_z[0]);
		c = &(def2_c[0]);
	}
	else {
		err_checkf(false,"Basis set not implemented",std::cout);
	}
	
};

double Gaussian_Atom::calc_type(
	int& nr_ex,
	int& nr_coef,
	const double& k_vector,
	const int& offset,
	const int* n_vector,
	const int lower_m,
	const int upper_m,
	const int& max,
	const int& min) {

	std::function<double(const int&, const double&, const double&, const int&, const double&)> func;
	if (k_vector == 0)
		func = calc_Gaussian_int_at_k0;
	else
		func = calc_Gaussian_int;
	const int l_n = n_vector[atomic_number - 1];
	double temp, result = 0;
	int i_j_distance = 0;
	for (int m = lower_m; m < upper_m; m++)
		if (occ[offset + m] != 0)
			i_j_distance++;
	for (int m = lower_m + min; m < lower_m + max; m++) {
		if (occ[offset + m] == 0) continue;
		for (int i = 0; i < l_n; i++) {
			for (int j = 0; j < l_n - i; j++) {
				temp = func(occ[offset + m],
					c[nr_coef + m - lower_m + i * i_j_distance] * c[nr_coef + m - lower_m + (i + j) * i_j_distance],
					z[nr_ex + i] + z[nr_ex + i + j],
					n[nr_ex + i] + n[nr_ex + i + j] + 1,
					k_vector);
				if (j != 0)
					result += 2 * temp;
				else
					result += temp;
			}
		}
	}
	nr_coef += i_j_distance * l_n;
	nr_ex += l_n;
	return result;
}

const double Gaussian_Atom::get_custom_form_factor(
	const double& k_vector,
	const int& max_s,
	const int& max_p,
	const int& max_d,
	const int& max_f,
	const int& min_s,
	const int& min_p,
	const int& min_d,
	const int& min_f) {

	err_not_impl_SA();
	return -1;
};
const double Gaussian_Atom::get_custom_form_factor(
	const double& k_vector,
	const int& max_s,
	const int& max_p,
	const int& max_d,
	const int& max_f,
	const int& max_g,
	const int& max_h,
	const int& min_s,
	const int& min_p,
	const int& min_d,
	const int& min_f,
	const int& min_g, 
	const int& min_h ) {

	double result(0.0);
	using namespace std;

	int nr_ex = first_ex();
	if (nr_ex == 200000000)
		return -20;
	int nr_coef = previous_element_coef();
	if (atomic_number == 1) nr_coef = 0;
	const int offset = (atomic_number - 1) * 19;

	result += calc_type(nr_ex, nr_coef, k_vector, offset, ns, 0, 7, max_s, min_s);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, np, 7, 13, max_p, min_p);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, nd, 13, 17, max_d, min_d);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, nf, 17, 19, max_f, min_f);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, ng, 17, 19, max_g, min_g);
	result += calc_type(nr_ex, nr_coef, k_vector, offset, nh, 17, 19, max_h, min_h);

	if (k_vector != 0)
		return result / k_vector;
	else return result;
};

const double Gaussian_Atom::get_form_factor(const double& k_vector) {
	return get_custom_form_factor(k_vector, 7, 6, 4, 2, 2, 2, 0, 0, 0, 0, 0, 0);
};

const double Gaussian_Atom::get_core_form_factor(const double& k_vector, const int& core_els) {
	int max_s = 0, max_p = 0, max_d = 0, max_f = 0;
	if (core_els == 2) {
		max_s = 1; max_p = 0; max_d = 0; max_f = 0;
	}
	else if (core_els == 10) {
		max_s = 2; max_p = 1; max_d = 0; max_f = 0;
	}
	else if (core_els == 18) {
		max_s = 3; max_p = 2; max_d = 0; max_f = 0;
	}
	else if (core_els == 28) {
		max_s = 3; max_p = 2; max_d = 1; max_f = 0;
	}
	else if (core_els == 46) {
		max_s = 4; max_p = 3; max_d = 2; max_f = 0;
	}
	else if (core_els == 60) {
		max_s = 4; max_p = 3; max_d = 2; max_f = 1;
	}

	return get_custom_form_factor(k_vector, max_s, max_p, max_d, max_f, 0, 0, 0, 0);
};

void Gaussian_Atom::calc_orbs(
	int& nr_ex,
	int& nr_coef,
	const double& dist,
	const int& offset,
	const int* n_vector,
	const int lower_m,
	const int upper_m,
	double* Orb) {
	double exponent;
	for (int ex = 0; ex < n_vector[atomic_number - 1]; ex++) {
		for (int m = lower_m; m < upper_m; m++) {
			if (occ[offset + m] == 0) continue;
			exponent = -z[nr_ex] * dist * dist;
			if (exponent > -46.5) { // Corresponds to at least 1E-20
				if (n[nr_ex] == 1)
					Orb[m] += c[nr_coef] * exp(exponent);
				else
					Orb[m] += c[nr_coef] * pow(dist, n[nr_ex]) * exp(exponent);
			}
			nr_coef++;
		}
		nr_ex++;
	}
}

const double Gaussian_Atom::get_radial_density(double& dist) {
	double Rho = 0.0;
	int nr_ex = first_ex();
	if (nr_ex == 200000000)
		return -20;
	int nr_coef = previous_element_coef();

	double Orb[19] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	const int offset = (atomic_number - 1) * 19;

	calc_orbs(nr_ex, nr_coef, dist, offset, ns, 0, 7, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, np, 7, 13, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, nd, 13, 17, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, nf, 17, 19, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, ng, 17, 19, Orb);
	calc_orbs(nr_ex, nr_coef, dist, offset, nh, 17, 19, Orb);

	for (int m = 0; m < 19; m++) {
		if (Orb[m] == 0 || occ[offset + m] == 0)
			continue;
		Rho += occ[offset + m] * pow(Orb[m], 2);
	}
	return Rho / (constants::FOUR_PI);
};