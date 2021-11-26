#pragma once
//stuff for fft
#include <complex>
#include <valarray>
namespace FFT{
    const double PI = 3.141592653589793238460;

    typedef std::complex<double> Complex;
    typedef std::valarray<Complex> CArray;

    void ifft(CArray& x);
    void fft(CArray& x);
}