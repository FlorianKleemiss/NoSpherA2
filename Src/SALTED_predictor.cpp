#include "SALTED_predictor.h"
#include <filesystem>

using namespace std;
//std::string find_first_h5_file(const std::string& directory_path)
SALTEDPredictor::SALTEDPredictor(const WFN &wavy_in, const options &opt_in) : wavy(wavy_in), opt(opt_in)
{
    string _path = this->opt.SALTED_DIR;
    this->config.h5_filename = find_first_h5_file(this->opt.SALTED_DIR);
    if (this->config.h5_filename == "")
	{
        string _f_path("inputs.txt");
        join_path(_path, _f_path);
        this->config.populateFromFile(_path);
	}
	else
	{
#if has_RAS
    join_path(_path, this->config.h5_filename);
    H5::H5File config_file(_path, H5F_ACC_RDONLY);
    this->config.populateFromFile(config_file);
#else
    err_not_impl_f("HDF5 files are not supported by this build", std::cout);
#endif
	}
    this->config.predict_filename = this->wavy.get_path();
}

const string SALTEDPredictor::get_dfbasis_name()
{
	return this->config.dfbasis;
}



void SALTEDPredictor::setup_atomic_environment()
{
    
    SALTED_Utils::set_lmax_nmax(this->lmax, this->nmax, *(this->wavy.get_basis_set_ptr()), this->config.species);

    for (int i = 0; i < this->wavy.atoms.size(); i++)
    {
        this->atomic_symbols.push_back(this->wavy.atoms[i].label);
    }
    // # Define system excluding atoms that belong to species not listed in SALTED input
    this->atomic_symbols = SALTED_Utils::filter_species(this->atomic_symbols, this->config.species);

    // Print all Atomic symbols
    if (this->opt.debug)
    {
        cout << "Atomic symbols: ";
        for (const auto &symbol : atomic_symbols)
        {
            cout << symbol << " ";
        }
        cout << endl;
    }

    this->natoms = static_cast<int>(this->atomic_symbols.size());
    for (int i = 0; i < this->atomic_symbols.size(); i++)
    {
        this->atom_idx[this->atomic_symbols[i]].push_back(i);
        this->natom_dict[this->atomic_symbols[i]] += 1;
    }

    // RASCALINE (Generate descriptors)
#if has_RAS
    v1 = Rascaline_Descriptors(this->config.predict_filename, this->config.nrad1, this->config.nang1, this->config.sig1, this->config.rcut1, this->natoms, this->config.neighspe1, this->config.species).calculate_expansion_coeffs();
    v2 = Rascaline_Descriptors(this->config.predict_filename, this->config.nrad2, this->config.nang2, this->config.sig2, this->config.rcut2, this->natoms, this->config.neighspe2, this->config.species).calculate_expansion_coeffs();
#else
    err_not_impl_f("RASCALINE is not supported by this build", std::cout);
#endif
    // END RASCALINE
}

void SALTEDPredictor::read_model_data()
{
    // Define zeta as a string with one decimal
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << this->config.zeta;
    std::string zeta_str = stream.str();

    H5::H5File features(this->opt.SALTED_DIR + "/GPR_data/FEAT_M-" + to_string(this->config.Menv) + ".h5", H5F_ACC_RDONLY);
    H5::H5File projectors(this->opt.SALTED_DIR + "/GPR_data/projector_M" + to_string(this->config.Menv) + "_zeta" + zeta_str + ".h5", H5F_ACC_RDONLY);
    vector<hsize_t> dims_out;
    for (string spe : config.species)
    {
        for (int lam = 0; lam < lmax[spe] + 1; lam++)
        {
            vec temp_power = readHDF5<double>(features, "sparse_descriptors/" + spe + "/" + to_string(lam), dims_out);
            this->power_env_sparse[spe + to_string(lam)] = reshape<double>(temp_power, Shape2D{dims_out[0], dims_out[1]});
            vec temp_Vmat = readHDF5<double>(projectors, "projectors/" + spe + "/" + to_string(lam), dims_out);
            this->Vmat[spe + to_string(lam)] = reshape<double>(temp_Vmat, Shape2D{dims_out[0], dims_out[1]});

            if (lam == 0)
            {
                this->Mspe[spe] = static_cast<int>(this->power_env_sparse[spe + to_string(lam)].size());
            }
            if (this->config.zeta == 1)
            {
                this->power_env_sparse[spe + to_string(lam)] = dot<double>(this->Vmat[spe + to_string(lam)], this->power_env_sparse[spe + to_string(lam)], true, false);
            }
        }
    }
    features.close();
    projectors.close();

    for (int lam = 0; lam < SALTED_Utils::get_lmax_max(lmax) + 1; lam++)
    {
        this->wigner3j[lam] = readVectorFromFile<double>(this->opt.SALTED_DIR + "/wigners/wigner_lam-" + to_string(lam) + "_lmax1-" + to_string(this->config.nang1) + "_lmax2-" + to_string(this->config.nang2) + ".dat");
    }
    


    if (config.sparsify)
    {
        this->vfps = read_fps<int64_t>(this->opt.SALTED_DIR + "/GPR_data/fps" + to_string(this->config.ncut) + "-", SALTED_Utils::get_lmax_max(this->lmax));
    };
    if (config.average) {
        for (string spe : this->config.species)
        {
            read_npy(this->opt.SALTED_DIR + "/averages/averages_" + spe + ".npy", this->av_coefs[spe]);
        }
    }


    int ntrain = static_cast<int>(config.Ntrain * config.trainfrac);

    if (config.field)
    {
        cout << "Field" << endl;
        err_not_impl_f("Calculations using 'Field = True' are not yet supported", std::cout);
    }
    else
    {
        read_npy(this->opt.SALTED_DIR + "/GPR_data/weights_N" + to_string(ntrain) + "_reg-6.npy", this->weights);
    }
}

#if has_RAS
void SALTEDPredictor::read_model_data_h5() {
    string _H5path = this->opt.SALTED_DIR;
    join_path(_H5path, this->config.h5_filename);
    H5::H5File input(_H5path, H5F_ACC_RDONLY);
    vector<hsize_t> dims_out;
    for (string spe : config.species)
    {
        for (int lam = 0; lam < lmax[spe] + 1; lam++)
        {
            vec temp_power = readHDF5<double>(input, "sparse_descriptors/" + spe + "/" + to_string(lam), dims_out);
            this->power_env_sparse[spe + to_string(lam)] = reshape(temp_power, Shape2D{dims_out[0], dims_out[1]});
            vec temp_proj = readHDF5<double>(input, "projectors/" + spe + "/" + to_string(lam), dims_out);
            this->Vmat[spe + to_string(lam)] = reshape(temp_proj, Shape2D{dims_out[0], dims_out[1]});

            if (lam == 0)
            {
                this->Mspe[spe] = static_cast<int>(this->power_env_sparse[spe + to_string(lam)].size());
            }
            if (this->config.zeta == 1)
            {
                this->power_env_sparse[spe + to_string(lam)] = dot<double>(this->Vmat[spe + to_string(lam)], this->power_env_sparse[spe + to_string(lam)], true, false);
            }
        }
    }

    for (int lam = 0; lam < SALTED_Utils::get_lmax_max(lmax) + 1; lam++)
    {
        this->wigner3j[lam] = readHDF5<double>(input, "wigners/lam-" + to_string(lam), dims_out);
    }

    if (config.sparsify)
    {
        for (int lam = 0; lam < SALTED_Utils::get_lmax_max(this->lmax) + 1; lam++)
        {
            this->vfps[lam] = readHDF5<int64_t>(input, "fps/lam-" + to_string(lam), dims_out);
        }
    };
    if (config.average) {
        for (string spe : this->config.species)
        {
            this->av_coefs[spe] = readHDF5<double>(input, "averages/" + spe, dims_out);
        }
    }

    if (config.field)
    {
        cout << "Field" << endl;
        err_not_impl_f("Calculations using 'Field = True' are not yet supported", std::cout);
    }
    else
    {
        this->weights = readHDF5<double>(input, "weights", dims_out);
    }
}
#endif

vec SALTEDPredictor::predict()
{
    // Compute equivariant descriptors for each lambda value entering the SPH expansion of the electron density
    vec2 pvec_l0{};
    unordered_map<int, vec3> pvec{};
    for (int lam = 0; lam < SALTED_Utils::get_lmax_max(lmax) + 1; lam++)
    {
        int llmax = 0;
        unordered_map<int, ivec> lvalues{};
        for (int l1 = 0; l1 < this->config.nang1 + 1; l1++)
        {
            for (int l2 = 0; l2 < this->config.nang2 + 1; l2++)
            {
                // keep only even combination to enforce inversion symmetryc
                if ((lam + l1 + l2) % 2 == 0)
                {
                    if (abs(l2 - lam) <= l1 && l1 <= (l2 + lam))
                    {
                        lvalues[llmax] = {l1, l2};
                        llmax += 1;
                    }
                }
            }
        }
        // Fill dense array from dictionary
        ivec2 llvec(llmax, ivec(2));
        for (int i = 0; i < llmax; i++)
        {
            llvec[i] = lvalues[i];
        }
        
        cvec2 c2r = SALTED_Utils::complex_to_real_transformation({2 * lam + 1})[0];

        int featsize = this->config.nspe1 * this->config.nspe2 * this->config.nrad1 * this->config.nrad2 * llmax;
        vec3 p;
        ivec2 llvec_t = transpose<int>(llvec);
        if (this->config.sparsify)
        {
            int nfps = static_cast<int>(vfps[lam].size());
            equicomb(this->natoms, this->config.nang1, this->config.nang2, (this->config.nspe1 * this->config.nrad1), (this->config.nspe2 * this->config.nrad2), this->v1, this->v2, this->wigner3j[lam], llmax, llvec_t, lam, c2r, featsize, nfps, this->vfps[lam], p);
            featsize = config.ncut;
        }
        else
        {
            equicomb(this->natoms, this->config.nang1, this->config.nang2, (this->config.nspe1 * this->config.nrad1), (this->config.nspe2 * this->config.nrad2), this->v1, this->v2, this->wigner3j[lam], llmax, llvec_t, lam, c2r, featsize, p);
        }

        vec3 p_t = reorder3D<double>(p);
        vec flat_p = flatten<double>(p_t);

        if (lam == 0)
        {
            pvec_l0 = reshape<double>(flat_p, Shape2D{this->natoms, featsize});
        }
        else
        {
            pvec[lam] = reshape<double>(flat_p, Shape3D{this->natoms, 2 * lam + 1, featsize});
        }
        flat_p.clear();
    }

    unordered_map<string, vec2> psi_nm{};

    for (const string &spe : this->config.species)
    {
        if (this->atom_idx.find(spe) == this->atom_idx.end())
        {
            continue;
        }
        vec2 kernell0_nm;
        if (config.zeta == 1)
        {
            psi_nm[spe + "0"] = dot<double>(collectRows(pvec_l0, this->atom_idx[spe]), this->power_env_sparse[spe + "0"], false, true);
        }
        else
        {
            kernell0_nm = dot<double>(collectRows(pvec_l0, this->atom_idx[spe]), this->power_env_sparse[spe + "0"], false, true);
            vec2 kernel_nm = elementWiseExponentiation(kernell0_nm, this->config.zeta);
            psi_nm[spe + "0"] = dot<double>(kernel_nm, this->Vmat[spe + "0"], false, false);
        }

        for (int lam = 1; lam <= lmax[spe]; ++lam)
        {
            int featsize = static_cast<int>(pvec[lam][0][0].size());
            vec3 pVec_Rows = collectRows(pvec[lam], this->atom_idx[spe]);
            vec2 pvec_lam = reshape<double>(flatten<double>(pVec_Rows), Shape2D{this->natom_dict[spe] * (2 * lam + 1), featsize});

            if (this->config.zeta == 1)
            {
                psi_nm[spe + to_string(lam)] = dot<double>(pvec_lam, this->power_env_sparse[spe + to_string(lam)], false, true);
            }
            else
            {
                vec2 kernel_nm = dot<double>(pvec_lam, this->power_env_sparse[spe + to_string(lam)], false, true);
                for (size_t i1 = 0; i1 < this->natom_dict[spe]; ++i1)
                {
                    for (size_t i2 = 0; i2 < this->Mspe[spe]; ++i2)
                    {
                        for (size_t i = 0; i < 2 * lam + 1; ++i)
                        {
                            for (size_t j = 0; j < 2 * lam + 1; ++j)
                            {
                                kernel_nm[i1 * (2 * lam + 1) + i][i2 * (2 * lam + 1) + j] *= pow(kernell0_nm[i1][i2], this->config.zeta - 1);
                            }
                        }
                    }
                }
                psi_nm[spe + to_string(lam)] = dot<double>(kernel_nm, Vmat[spe + to_string(lam)], false, false);
            }
        }
    }

    unordered_map<string, vec> C{};
    unordered_map<string, int> ispe{};
    int isize = 0;
    for (const string &spe : this->config.species)
    {
        if (this->atom_idx.find(spe) == this->atom_idx.end())
        {
            for (int l = 0; l <= this->lmax[spe]; ++l)
            {
                for (int n = 0; n < this->nmax[spe + to_string(l)]; ++n)
                {
                    isize += static_cast<int>(this->Vmat[spe + to_string(l)].size());
                }
            }
            continue;
        }
        ispe[spe] = 0;
        for (int l = 0; l <= this->lmax[spe]; ++l)
        {
            for (int n = 0; n < this->nmax[spe + to_string(l)]; ++n)
            {
                int Mcut = static_cast<int>(psi_nm[spe + to_string(l)][0].size());
                vec weights_subset(this->weights.begin() + isize, this->weights.begin() + isize + Mcut);
                C[spe + to_string(l) + to_string(n)] = dot(psi_nm[spe + to_string(l)], weights_subset);
                isize += Mcut;
            }
        }
    }

    int Tsize = 0;
    for (int iat = 0; iat < this->natoms; iat++)
    {
        string spe = this->atomic_symbols[iat];
        for (int l = 0; l < this->lmax[spe] + 1; l++)
        {
            for (int n = 0; n < this->nmax[spe + to_string(l)]; n++)
            {
                Tsize += 2 * l + 1;
            }
        }
    }

    vec Av_coeffs(Tsize, 0.0);

    // fill vector of predictions
    int i = 0;
    vec pred_coefs(Tsize, 0.0);
    for (int iat = 0; iat < this->natoms; ++iat)
    {
        string spe = this->atomic_symbols[iat];
        for (int l = 0; l <= this->lmax[spe]; ++l)
        {
            for (int n = 0; n < this->nmax[spe + to_string(l)]; ++n)
            {
                for (int ind = 0; ind < 2 * l + 1; ++ind)
                {
                    pred_coefs[i + ind] = C[spe + to_string(l) + to_string(n)][ispe[spe] * (2 * l + 1) + ind];
                }
                if (this->config.average && l == 0)
                {
                    Av_coeffs[i] = this->av_coefs[spe][n];
                }
                i += 2 * l + 1;
            }
        }
        ispe[spe] += 1;
    }

    if (this->config.average)
    {
        for (i = 0; i < Tsize; i++)
        {
            pred_coefs[i] += Av_coeffs[i];
        }
    }

    cout << "          ... done!\nNumber of predicted coefficients: " << pred_coefs.size() << endl;
    // npy::npy_data<double> coeffs;
    // coeffs.data = pred_coefs;
    // coeffs.fortran_order = false;
    // coeffs.shape = { unsigned long(pred_coefs.size()) };
    // npy::write_npy("coeffs_by_black_magic.npy", coeffs);
    return pred_coefs;
}

vec SALTEDPredictor::gen_SALTED_densities()
{
    if (this->opt.coef_file != "")
	{
        vec coefs{};
		cout << "Reading coefficients from file: " << this->opt.coef_file << endl;
		read_npy<double>(this->opt.coef_file, coefs);
		return coefs;
	}

    // Run generation of tsc file
    time_point start;
    if (this->opt.debug) start = get_time();

    this->setup_atomic_environment();

    if (!this->config.from_h5) {
        this->read_model_data();
    }
    else {
#if has_RAS
        this->read_model_data_h5();
#else
        err_not_impl_f("HDF5 files are not supported by this build", std::cout);
#endif
        
    }
    
    vec coefs = this->predict();


    if (this->opt.debug)
    {
        time_point end = get_time();
        long long int dur = get_sec(start, end);
        cout << "Finished ML Density Prediction!" << endl;
        if (dur < 1)
            cout << "Time for SALTED prediction: " << fixed << setprecision(0) << get_msec(start, end) << " ms" << endl;
        else
            cout << "Time for SALTED prediction: " << fixed << setprecision(0) << dur << " s" << endl;

        cout << "Number of coefficients: " << coefs.size() << endl;

        if (this->opt.wfn == string("test_cysteine.xyz") || this->opt.wfn == string("test_sucrose.xyz"))
        {
            vector<unsigned long> shape{};
            bool fortran_order;
            vec ref_coefs{};
            // Depending on the value of opt.wfn read either the cysteine or the sucrose reference coefs
            if (this->opt.wfn == string("test_sucrose.xyz"))
                npy::LoadArrayFromNumpy("sucrose_ref_Combined_v1.npy", shape, fortran_order, ref_coefs);
            else
                npy::LoadArrayFromNumpy("cysteine_def2_qzvppd.npy", shape, fortran_order, ref_coefs);
            // Compare coefs with the reference
            vector<double> diff_vec;
            double diff = 0.0;
            for (int i = 0; i < coefs.size(); i++)
            {
                diff += abs(coefs[i] - ref_coefs[i]);
                if (abs(coefs[i] - ref_coefs[i]) > 1e-4)
                {
                    cout << "Difference in coef " << fixed << setprecision(3) << i << " : " << coefs[i] << " - " << ref_coefs[i] << " = " << abs(coefs[i] - ref_coefs[i]) << endl;
                }
                diff_vec.push_back((coefs[i] / ref_coefs[i]) - 1);
            }
            cout << "Difference between calculated and reference coefs: " << fixed << setprecision(3) << diff << endl;
            cout << "Maximum ((pred / ref) -1): " << fixed << setprecision(3) << *max_element(diff_vec.begin(), diff_vec.end()) << endl;
            if (diff > 0.1)
            {
                cout << "WARNING: The difference between the calculated and reference coefficients is too large!" << endl;
                exit(1);
            }
        }
    }

    return coefs;
}