#ifndef CUDA_CudaDWF_H
#define CUDA_CudaDWF_H

#include <utils.h>
#include <kernels.h>

#include <linear_operator.h>
#include <wilson.h>
#include <hamber_wu.h>
#include <naik.h>

class CudaDWF : public CudaLinearOperator
{
public:
  CudaDWF(const float mass, const float M5, const int Ls, const int kernelType,
      const int L, const int T, const bool precondition, const bool hermitian,
      const Complex boundaryConditions[4], Complex* links);
  ~CudaDWF();

  void apply(Complex* y, const Complex* x) const;
  void applyHermitian(Complex* y, const Complex* x) const;
  void makeHermitian(Complex* y, const Complex* x) const;

  void makeEvenOdd(Complex* y, const Complex* x) const;
  void removeEvenOdd(Complex* y, const Complex* x) const;

  void applyEvenEvenInv(Complex* y, const Complex* x) const;
  void applyOddOdd(Complex* y, const Complex* x) const;
  void applyEvenOdd(Complex* y, const Complex* x) const;
  void applyEvenOddDagger(Complex* y, const Complex* x) const;
  void applyOddEven(Complex* y, const Complex* x) const;
  void applyOddEvenDagger(Complex* y, const Complex* x) const;

  void applyPreconditionedHermitian(Complex* y, const Complex* x) const;

private:
  float mass_;
  float M5_;
  int Ls_;

  CudaLinearOperator* kernel_;
};

#include <dwf.cu>

#endif
