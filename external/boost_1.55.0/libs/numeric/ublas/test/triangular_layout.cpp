// Copyright 2008 Gunter Winkler <guwi17@gmx.de>
// Thanks to Tiago Requeijo for providing this test
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/cstdlib.hpp>

using namespace std;
namespace ublas = boost::numeric::ublas;

int main(int argc, char* argv[])
{
    int sz = 4;
    ublas::symmetric_matrix<int, ublas::upper, ublas::column_major>  UpCol (sz, sz);
    ublas::symmetric_matrix<int, ublas::upper, ublas::row_major>     UpRow (sz, sz);
    ublas::symmetric_matrix<int, ublas::lower, ublas::column_major>  LoCol (sz, sz);
    ublas::symmetric_matrix<int, ublas::lower, ublas::row_major>     LoRow (sz, sz);

    ublas::triangular_matrix<int, ublas::upper, ublas::column_major>  TrUpCol (sz, sz);
    ublas::triangular_matrix<int, ublas::upper, ublas::row_major>     TrUpRow (sz, sz);
    ublas::triangular_matrix<int, ublas::lower, ublas::column_major>  TrLoCol (sz, sz);
    ublas::triangular_matrix<int, ublas::lower, ublas::row_major>     TrLoRow (sz, sz);

    for(int i=0; i<sz; ++i)
        for(int j=i; j<sz; ++j)
        {
            // Symmetric
            UpCol(i,j) = 10*i + j;
            UpRow(i,j) = 10*i + j;
            LoCol(i,j) = 10*i + j;
            LoRow(i,j) = 10*i + j;
            // Triangular
            TrUpCol(i,j) = 10*i + j;
            TrUpRow(i,j) = 10*i + j;
            TrLoCol(j,i) = 10*i + j;
            TrLoRow(j,i) = 10*i + j;
        }

    //get pointers to data
    int* uc = &(UpCol.data()[0]);
    int* ur = &(UpRow.data()[0]);
    int* lc = &(LoCol.data()[0]);
    int* lr = &(LoRow.data()[0]);
    int* tuc = &(TrUpCol.data()[0]);
    int* tur = &(TrUpRow.data()[0]);
    int* tlc = &(TrLoCol.data()[0]);
    int* tlr = &(TrLoRow.data()[0]);

    // upper, column_major
    //   storage should be:  0 1 11 2 12 22 3 13 23 33
    int uc_correct[] = {0, 1, 11, 2, 12, 22, 3, 13, 23, 33};

    // upper, row_major
    //   storage should be:  0 1 2 3 11 12 13 22 23 33
    int ur_correct[] = {0, 1, 2, 3, 11, 12, 13, 22, 23, 33};

    // lower, column_major
    //   storage should be:  0 1 2 3 11 12 13 22 23 33
    int lc_correct[] = {0, 1, 2, 3, 11, 12, 13, 22, 23, 33};

    // lower, row_major
    //   storage should be:  0 1 11 2 12 22 3 13 23 33
    int lr_correct[] = {0, 1, 11, 2, 12, 22, 3, 13, 23, 33};

    bool success = true;

    // Test Symmetric
    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(uc[i] != uc_correct[i])
        {
            cout << "Storage error (Symmetric, Upper, Column major)" << endl;
            success = false;
            break;
        }

    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(ur[i] != ur_correct[i])
        {
            cout << "Storage error (Symmetric, Upper, Row major)" << endl;
            success = false;
            break;
        }

    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(lc[i] != lc_correct[i])
        {
            cout << "Storage error (Symmetric, Lower, Column major)" << endl;
            success = false;
            break;
        }

    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(lr[i] != lr_correct[i])
        {
            cout << "Storage error (Symmetric, Lower, Row major)" << endl;
            success = false;
            break;
        }

    // Test Triangular
    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(tuc[i] != uc_correct[i])
        {
            cout << "Storage error (Triangular, Upper, Column major)" << endl;
            success = false;
            break;
        }

    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(tur[i] != ur_correct[i])
        {
            cout << "Storage error (Triangular, Upper, Row major)" << endl;
            success = false;
            break;
        }

    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(tlc[i] != lc_correct[i])
        {
            cout << "Storage error (Triangular, Lower, Column major)" << endl;
            success = false;
            break;
        }

    for(int i=0; i<sz*(sz+1)/2; ++i)
        if(tlr[i] != lr_correct[i])
        {
            cout << "Storage error (Triangular, Lower, Row major)" << endl;
            success = false;
            break;
        }

        
        return (success)?boost::exit_success:boost::exit_failure;
}
