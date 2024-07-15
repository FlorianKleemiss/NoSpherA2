#include "SALTED_math.h"
#ifdef _WIN32
#include "DLL_Helper.h"
#endif
#if has_RAS
#include "cblas.h"
#endif
#include <cassert>

using namespace std;

template <typename T>
vector<vector<T>> reshape(vector<T> flatVec, Shape2D sizes)
{
    vector<vector<T>> reshapedVec(sizes.rows, vector<T>(sizes.cols));
    for (int i = 0; i < sizes.rows; ++i)
    {
        for (int j = 0; j < sizes.cols; ++j)
        {
            reshapedVec[i][j] = flatVec[i * sizes.cols + j];
        }
    }
    return reshapedVec;
}
template vector<vector<double>> reshape(vector<double> flatVec, Shape2D sizes);
template vector<vector<cdouble>> reshape(vector<cdouble> flatVec, Shape2D sizes);
template vector<vector<int>> reshape(vector<int> flatVec, Shape2D sizes);

// To_3D
template <typename T>
vector<vector<vector<T>>> reshape(vector<T> flatVec, Shape3D sizes)
{
    vector<vector<vector<T>>> reshapedVec(sizes.depth, vector<vector<T>>(sizes.rows, vector<T>(sizes.cols)));
    for (int i = 0; i < sizes.depth; ++i)
    {
        for (int j = 0; j < sizes.rows; ++j)
        {
            for (int k = 0; k < sizes.cols; ++k)
            {
                reshapedVec[i][j][k] = flatVec[i * sizes.rows * sizes.cols + j * sizes.cols + k];
            }
        }
    }
    return reshapedVec;
}
template vector<vector<vector<double>>> reshape(vector<double> flatVec, Shape3D sizes);
template vector<vector<vector<cdouble>>> reshape(vector<cdouble> flatVec, Shape3D sizes);
template vector<vector<vector<int>>> reshape(vector<int> flatVec, Shape3D sizes);

// Flatten Vectors 2D
template <typename T>
vector<T> flatten(const vector<vector<T>> &vec2D)
{
    vector<T> flatVec;
    size_t totalSize = 0;
    for (const auto &row : vec2D)
    {
        totalSize += row.size();
    }
    flatVec.reserve(totalSize);
    for (const vector<T> &row : vec2D)
    {
        // Use std::copy to copy the entire row at once
        flatVec.insert(flatVec.end(), row.begin(), row.end());
    }
    return flatVec;
}
template vector<double> flatten(const vector<vector<double>> &vec2D);
template vector<cdouble> flatten(const vector<vector<cdouble>> &vec2D);
template vector<int> flatten(const vector<vector<int>> &vec2D);

// Flatten Vectors 3D
template <typename T>
vector<T> flatten(const vector<vector<vector<T>>> &vec3D)
{
    vector<T> flatVec;
    size_t totalSize = 0;
    for (const auto &row : vec3D)
    {
        for (const auto &innerRow : row)
        {
            totalSize += innerRow.size();
        }
    }
    flatVec.reserve(totalSize);
    for (const vector<vector<T>> &row : vec3D)
    {
        for (const vector<T> &innerRow : row)
        {
            // Use std::copy to copy the entire innerRow at once
            flatVec.insert(flatVec.end(), innerRow.begin(), innerRow.end());
        }
    }
    return flatVec;
}
template vector<double> flatten(const vector<vector<vector<double>>> &vec3D);
template vector<cdouble> flatten(const vector<vector<vector<cdouble>>> &vec3D);
template vector<int> flatten(const vector<vector<vector<int>>> &vec3D);

// SLICE Operation
vector<double> slice(const vector<double> &vec, size_t start, size_t length)
{
    if (start + length > vec.size())
    {
        throw std::out_of_range("Slice range is out of bounds.");
    }

    std::vector<double> result;
    for (size_t i = start; i < start + length; ++i)
    {
        result.push_back(vec[i]);
    }

    return result;
}

// Matrix multiplication
// 2D x 2D MATRIX MULTIPLICATION
template <typename T>
vector<vector<T>> self_dot(const vector<vector<T>> &mat1, const vector<vector<T>> &mat2)
{
    // if either of the matrices is empty, return a empty matrix
    if (mat1.empty() || mat2.empty())
    {
        return {};
    }

    size_t rows1 = mat1.size();
    size_t cols1 = mat1[0].size();
    size_t rows2 = mat2.size();
    size_t cols2 = mat2[0].size();

    // Check if matrix multiplication is possible
    if (cols1 != rows2)
    {
        throw std::invalid_argument("Matrix dimensions do not match for multiplication");
    }

    vector<vector<T>> result(rows1, vector<T>(cols2, 0.0));
    const long long int totalIterations = static_cast<long long int>(rows1 * cols2 * cols1);
    size_t total_size = rows1 * cols2;
#pragma omp parallel
    {
        vector<T> local_result(total_size, 0.0);
        int i, j, k, flatIndex;

#pragma omp for schedule(static) private(i, j, k, flatIndex) nowait
        for (long long int n = 0; n < totalIterations; ++n)
        {
            i = static_cast<int>(n / (cols2 * cols1));
            j = static_cast<int>((n / cols1) % cols2);
            k = static_cast<int>(n % cols1);
            flatIndex = i * cols2 + j;
            local_result[flatIndex] += mat1[i][k] * mat2[k][j];
        }

#pragma omp critical
        {
            for (i = 0; i < rows1; ++i)
            {
                for (j = 0; j < cols2; ++j)
                {
                    flatIndex = i * cols2 + j;
                    result[i][j] += local_result[flatIndex];
                }
            }
        }
    }

    return result;
}
template vector<vector<double>> self_dot(const vector<vector<double>> &mat1, const vector<vector<double>> &mat2);
template vector<vector<cdouble>> self_dot(const vector<vector<cdouble>> &mat1, const vector<vector<cdouble>> &mat2);

typedef void (*ExampleFunctionType)(void);

// Matrix multiplication
// 2D x 2D MATRIX MULTIPLICATION
template <typename T>
vector<vector<T>> dot(const vector<vector<T>> &mat1, const vector<vector<T>> &mat2)
{
    // if either of the matrices is empty, return a empty matrix
    if (mat1.empty() || mat2.empty())
    {
        return {};
    }

    size_t rows1 = mat1.size();
    size_t cols1 = mat1[0].size();
    size_t rows2 = mat2.size();
    size_t cols2 = mat2[0].size();

    // Check if matrix multiplication is possible
    if (cols1 != rows2)
    {
        throw std::invalid_argument("Matrix dimensions do not match for multiplication");
    }

#if has_RAS
#ifdef _WIN32
    HMODULE hOpenBlas = LoadLibrary(TEXT("libopenblas.dll"));
    if (hOpenBlas != NULL)
    {
        ExampleFunctionType eF = (ExampleFunctionType)GetProcAddress(hOpenBlas, "cblas_sgemm");
        if (eF != NULL)
        {
            // If the function is found, use OpenBLAS
            std::cout << "Using OpenBLAS." << std::endl;
            vector<T> flatMat1 = flatten(mat1);
            vector<T> flatMat2 = flatten(mat2);

            vector<T> result_flat(rows1 * cols2, 0.0);
            if constexpr (std::is_same_v<T, float>)
            {
                cblas_sgemm(CblasRowMajor,
                            CblasNoTrans,
                            CblasNoTrans,
                            rows1,
                            cols2,
                            cols1,
                            1.0f,
                            flatMat1.data(),
                            cols1,
                            flatMat2.data(),
                            cols2,
                            0.0f,
                            result_flat.data(),
                            cols2);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                cblas_dgemm(CblasRowMajor,
                            CblasNoTrans,
                            CblasNoTrans,
                            rows1,
                            cols2,
                            cols1,
                            1.0,
                            flatMat1.data(),
                            cols1,
                            flatMat2.data(),
                            cols2,
                            0.0,
                            result_flat.data(),
                            cols2);
            }
            else if constexpr (std::is_same_v<T, cdouble>)
            {
                cdouble one = cdouble(1.0, 0.0);
                cdouble zero = cdouble(0.0, 0.0);
                cblas_zgemm(CblasRowMajor,
                            CblasNoTrans,
                            CblasNoTrans,
                            rows1,
                            cols2,
                            cols1,
                            &(one),
                            reinterpret_cast<const cdouble *>(flatMat1.data()),
                            cols1,
                            reinterpret_cast<const cdouble *>(flatMat2.data()),
                            cols2,
                            &(zero),
                            reinterpret_cast<cdouble *>(result_flat.data()),
                            cols2);
            }
            else
            {
                throw std::invalid_argument("Unsupported data type for matrix multiplication");
            }
            Shape2D sizes = {rows1, cols2};
            return reshape(result_flat, sizes);
        }
        else
        {
            // Function not found, fallback
            std::cout << "OpenBLAS function not found, using fallback." << std::endl;
            return self_dot(mat1, mat2);
        }
        FreeLibrary(hOpenBlas);
    }
    else
    {
        // DLL not found, fallback
        std::cout << "OpenBLAS DLL not found, using fallback." << std::endl;
        return self_dot(mat1, mat2);
    }
#else
    vector<T> flatMat1 = flatten(mat1);
    vector<T> flatMat2 = flatten(mat2);

    vector<T> result_flat(rows1 * cols2, 0.0);
    if constexpr (std::is_same_v<T, float>)
    {
        cblas_sgemm(CblasRowMajor,
                    CblasNoTrans,
                    CblasNoTrans,
                    rows1,
                    cols2,
                    cols1,
                    1.0f,
                    flatMat1.data(),
                    cols1,
                    flatMat2.data(),
                    cols2,
                    0.0f,
                    result_flat.data(),
                    cols2);
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        cblas_dgemm(CblasRowMajor,
                    CblasNoTrans,
                    CblasNoTrans,
                    rows1,
                    cols2,
                    cols1,
                    1.0,
                    flatMat1.data(),
                    cols1,
                    flatMat2.data(),
                    cols2,
                    0.0,
                    result_flat.data(),
                    cols2);
    }
    else if constexpr (std::is_same_v<T, cdouble>)
    {
        cdouble one = cdouble(1.0, 0.0);
        cdouble zero = cdouble(0.0, 0.0);
        cblas_zgemm(CblasRowMajor,
                    CblasNoTrans,
                    CblasNoTrans,
                    rows1,
                    cols2,
                    cols1,
                    &(one),
                    reinterpret_cast<const cdouble *>(flatMat1.data()),
                    cols1,
                    reinterpret_cast<const cdouble *>(flatMat2.data()),
                    cols2,
                    &(zero),
                    reinterpret_cast<cdouble *>(result_flat.data()),
                    cols2);
    }
    else
    {
        throw std::invalid_argument("Unsupported data type for matrix multiplication");
    }
    Shape2D sizes = {rows1, cols2};
    return reshape(result_flat, sizes);
#endif
#else

    return self_dot(mat1, mat2);
#endif
}
template vector<vector<double>> dot(const vector<vector<double>> &mat1, const vector<vector<double>> &mat2);
template vector<vector<cdouble>> dot(const vector<vector<cdouble>> &mat1, const vector<vector<cdouble>> &mat2);

void test_dot()
{
    vector<vector<double>> mat1 = {{1, 2, 3}, {4, 5, 6}};
    vector<vector<double>> mat2 = {{7, 8}, {9, 10}, {11, 12}};
    vector<vector<double>> result = dot(mat1, mat2);

    size_t rows1 = mat1.size();
    size_t cols1 = mat1[0].size();
    size_t rows2 = mat2.size();
    size_t cols2 = mat2[0].size();

    vector<vector<double>> result_old(rows1, vector<double>(cols2, 0.0));
    const long long int totalIterations = static_cast<long long int>(rows1 * cols2 * cols1);
    size_t total_size = rows1 * cols2;
#pragma omp parallel
    {
        vector<double> local_result(total_size, 0.0);
        int i, j, k, flatIndex;

#pragma omp for schedule(static) private(i, j, k, flatIndex) nowait
        for (long long int n = 0; n < totalIterations; ++n)
        {
            i = static_cast<int>(n / (cols2 * cols1));
            j = static_cast<int>((n / cols1) % cols2);
            k = static_cast<int>(n % cols1);
            flatIndex = i * cols2 + j;
            local_result[flatIndex] += mat1[i][k] * mat2[k][j];
        }

#pragma omp critical
        {
            for (i = 0; i < rows1; ++i)
            {
                for (j = 0; j < cols2; ++j)
                {
                    flatIndex = i * cols2 + j;
                    result_old[i][j] += local_result[flatIndex];
                }
            }
        }
    }
    for (int i = 0; i < rows1; ++i)
    {
        for (int j = 0; j < cols2; ++j)
        {
            cout << result[i][j] << " " << result_old[i][j] << endl;
            assert(result[i][j] == result_old[i][j]);
        }
    }
    exit(0);
}

// 2D x 1D MATRIX MULTIPLICATION
template <typename T>
vector<T> dot(const vector<vector<T>> &mat, const vector<T> &vec)
{
    int mat_rows = static_cast<int>(mat.size());
    int mat_cols = static_cast<int>(mat[0].size());
    int vec_size = static_cast<int>(vec.size());

    // Check if matrix multiplication is possible
    if (mat_cols != vec_size)
    {
        throw std::invalid_argument("Matrix dimensions do not match for multiplication");
    }

    vector<T> result(mat_rows, 0.0);
    int totalIterations = mat_rows * mat_cols;
#pragma omp parallel
    {
        vector<T> local_result(mat_rows, 0.0);
#pragma omp for schedule(static)
        for (int n = 0; n < totalIterations; ++n)
        {
            int i = n / mat_cols;
            int j = n % mat_cols;
            local_result[i] += mat[i][j] * vec[j];
        }
#pragma omp critical
        {
            for (int i = 0; i < mat_rows; ++i)
            {
                result[i] += local_result[i];
            }
        }
    }

    return result;
}
template vector<double> dot(const vector<vector<double>> &mat, const vector<double> &vec);
template vector<cdouble> dot(const vector<vector<cdouble>> &mat, const vector<cdouble> &vec);

// TRANSPOSES
// 3D MATRIX
template <typename T>
vector<vector<vector<T>>> transpose(const vector<vector<vector<T>>> &originalVec)
{
    if (originalVec.empty() || originalVec[0].empty() || originalVec[0][0].empty())
    {
        return {}; // Return an empty vector if the original vector is empty or not 3D
    }

    int newDim1 = originalVec[0][0].size(); // New first dimension is the old third dimension
    int newDim2 = originalVec.size();       // New second dimension is the old first dimension
    int newDim3 = originalVec[0].size();    // New third dimension is the old second dimension

    vector<vector<vector<T>>> transposedVec(newDim1, vector<vector<T>>(newDim2, std::vector<T>(newDim3)));

    for (int i = 0; i < originalVec.size(); ++i)
    {
        for (int j = 0; j < originalVec[i].size(); ++j)
        {
            for (int k = 0; k < originalVec[i][j].size(); ++k)
            {
                transposedVec[k][i][j] = originalVec[i][j][k];
            }
        }
    }

    return transposedVec;
}
template vector<vector<vector<double>>> transpose(const vector<vector<vector<double>>> &originalVec);
template vector<vector<vector<cdouble>>> transpose(const vector<vector<vector<cdouble>>> &originalVec);
template vector<vector<vector<int>>> transpose(const vector<vector<vector<int>>> &originalVec);

// 2D MATRIX
template <class T>
vector<vector<T>> transpose(const vector<vector<T>> &mat)
{
    int rows = static_cast<int>(mat.size());
    int cols = static_cast<int>(mat[0].size());
    vector<vector<T>> result(cols, vector<T>(rows));

    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            result[j][i] = mat[i][j];
        }
    }
    return result;
}
template vector<vector<double>> transpose(const vector<vector<double>> &mat);
template vector<vector<cdouble>> transpose(const vector<vector<cdouble>> &mat);
template vector<vector<int>> transpose(const vector<vector<int>> &mat);

// Reorder 3D Vectors following a given order
template <typename T>
vector<vector<vector<T>>> reorder3D(const vector<vector<vector<T>>> &original)
{
    if (original.empty() || original[0].empty() || original[0][0].empty())
    {
        return {}; // Return an empty vector if the original vector is empty or not properly sized
    }

    size_t size1 = original.size();       // Original first dimension size
    size_t size2 = original[0].size();    // Original second dimension size
    size_t size3 = original[0][0].size(); // Original third dimension size

    // New vector with dimensions rearranged according to (2, 0, 1)
    vector<vector<vector<T>>> transposed(size3, vector<vector<T>>(size1, vector<T>(size2)));

    for (size_t i = 0; i < size1; ++i)
    {
        for (size_t j = 0; j < size2; ++j)
        {
            for (size_t k = 0; k < size3; ++k)
            {
                transposed[k][i][j] = original[i][j][k];
            }
        }
    }

    return transposed;
}
template vector<vector<vector<double>>> reorder3D(const vector<vector<vector<double>>> &original);

// Function to collect rows from a matrix based on a vector of indices
vec2 collectRows(const vec2 &matrix, const vector<int> &indices)
{
    // If no indices are provided, return empty matrix
    if (indices.empty())
    {
        return {};
    }
    vec2 result;
    for (int index : indices)
    {
        if (index < matrix.size())
        {
            result.push_back(matrix[index]);
        }
        else
        {
            // Handle the case where index is out of bounds
            throw std::out_of_range("Index out of range");
        }
    }
    return result;
}

// Function to collect rows from a Cube based on a vector of indices
vec3 collectRows(const vec3 &cube, const vector<int> &indices)
{
    // If no indices are provided, return empty matrix
    if (indices.empty())
    {
        return {};
    }
    vec3 result;
    for (int index : indices)
    {
        if (index < cube.size())
        {
            result.push_back(cube[index]);
        }
        else
        {
            // Handle the case where index is out of bounds
            throw std::out_of_range("Index out of range");
        }
    }
    return result;
}

// Element-wise exponentiation of a matrix
vec2 elementWiseExponentiation(const vec2 &matrix, double exponent)
{
    vec2 result = matrix; // Copy the original matrix to preserve its dimensions

    for (size_t i = 0; i < matrix.size(); ++i)
    { // Iterate over rows
        for (size_t j = 0; j < matrix[i].size(); ++j)
        {                                                    // Iterate over columns
            result[i][j] = std::pow(matrix[i][j], exponent); // Apply exponentiation
        }
    }

    return result;
}