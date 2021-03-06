#include <linear_operators/dwf.hpp>

DWF::DWF(
  const double mass, const double M5, const int Ls, const int kernelType,
  const vector<complex<double> >& boundaryConditions,
  const Lattice* lattice) : LinearOperator::LinearOperator()
{
  // Class constructor - we set the fermion mass, create a pointer to the 
  // lattice and compute the frequently used spin structures used within the
  // Dirac operator.
  this->mass_ = mass;
  this->Ls_ = Ls;
  this->operatorSize_ 
    = 12 * Ls * int(pow(lattice->spatialExtent, 3)) * lattice->temporalExtent;
  this->lattice_ = lattice;

  if (kernelType == pyQCD::wilson)
    this->kernel_ = new Wilson(-M5, boundaryConditions, lattice);
  else if (kernelType == pyQCD::hamberWu)
    this->kernel_ = new HamberWu(-M5, boundaryConditions, lattice);
  else if (kernelType == pyQCD::naik)
    this->kernel_ = new Naik(-M5, boundaryConditions, lattice);
  else
    this->kernel_ = new Wilson(-M5, boundaryConditions, lattice);    
}



DWF::~DWF()
{
  // Just the hopping matrix to destroy
  delete this->kernel_;
}



VectorXcd DWF::apply(const VectorXcd& psi)
{
  // Right multiply a vector by the operator

  int size4d = this->operatorSize_ / this->Ls_;

  // The output vector
  VectorXcd eta = VectorXcd::Zero(this->operatorSize_);

  // If psi's the wrong size, get out of here before we segfault
  if (psi.size() != this->operatorSize_)
    return eta;

  unsigned long long nKernelFlopsOld = this->kernel_->getNumFlops();

  for (int i = 0; i < this->Ls_; ++i) {
    eta.segment(i * size4d, size4d)
      = this->kernel_->apply(psi.segment(i * size4d, size4d))
      + psi.segment(i * size4d, size4d);

    if (i == 0) {
      eta.segment(i * size4d, size4d)
	-= pyQCD::multiplyPminus(psi.segment(size4d, size4d));
      eta.segment(i * size4d, size4d) 
	+= this->mass_
	* pyQCD::multiplyPplus(psi.segment((this->Ls_ - 1) * size4d, size4d));
    }
    else if (i == this->Ls_ - 1) {
      eta.segment(i * size4d, size4d)
	-= pyQCD::multiplyPplus(psi.segment((this->Ls_ - 2) * size4d, size4d));
      eta.segment(i * size4d, size4d)
	+= this->mass_ * pyQCD::multiplyPminus(psi.segment(0, size4d));
    }
    else {
      eta.segment(i * size4d, size4d)
	-= pyQCD::multiplyPminus(psi.segment((i + 1) * size4d, size4d));
      eta.segment(i * size4d, size4d)
	-= pyQCD::multiplyPplus(psi.segment((i - 1) * size4d, size4d));
    }
  }

  this->nFlops_ += this->kernel_->getNumFlops() - nKernelFlopsOld;

  return eta;
}



VectorXcd DWF::applyHermitian(const VectorXcd& psi)
{
  return this->makeHermitian(this->apply(psi));
}



VectorXcd DWF::makeHermitian(const VectorXcd& psi)
{
  // Right multiply a vector by the operator daggered

  bool precondition = psi.size() == (this->operatorSize_ / 2);

  if (precondition) {
    return this->applyOddOdd(psi)
      - this->applyOddEvenDagger(this->applyEvenEvenInv(
	  this->applyEvenOddDagger(psi)));
  }
  else {

    int size4d = this->operatorSize_ / this->Ls_;

    // The output vector
    VectorXcd eta = VectorXcd::Zero(this->operatorSize_);

    // If psi's the wrong size, get out of here before we segfault
    if (psi.size() != this->operatorSize_)
      return eta;

    unsigned long long nKernelFlopsOld = this->kernel_->getNumFlops();

    for (int i = 0; i < this->Ls_; ++i) {
      eta.segment(i * size4d, size4d)
	= pyQCD::multiplyGamma5(psi.segment(i * size4d, size4d));
      eta.segment(i * size4d, size4d)
	= this->kernel_->apply(eta.segment(i * size4d, size4d))
	+ eta.segment(i * size4d, size4d);
      eta.segment(i * size4d, size4d)
	= pyQCD::multiplyGamma5(eta.segment(i * size4d, size4d));

      if (i == 0) {
	eta.segment(i * size4d, size4d)
	  -= pyQCD::multiplyPplus(psi.segment(size4d, size4d));
	eta.segment(i * size4d, size4d) 
	  += this->mass_
	  * pyQCD::multiplyPminus(psi.segment((this->Ls_ - 1) * size4d,
					      size4d));
      }
      else if (i == this->Ls_ - 1) {
	eta.segment(i * size4d, size4d)
	  -= pyQCD::multiplyPminus(psi.segment((this->Ls_ - 2) * size4d,
					       size4d));
	eta.segment(i * size4d, size4d)
	  += this->mass_ * pyQCD::multiplyPplus(psi.segment(0, size4d));
      }
      else {
	eta.segment(i * size4d, size4d)
	  -= pyQCD::multiplyPplus(psi.segment((i + 1) * size4d, size4d));
	eta.segment(i * size4d, size4d)
	  -= pyQCD::multiplyPminus(psi.segment((i - 1) * size4d, size4d));
      }
    }

    this->nFlops_ += this->kernel_->getNumFlops() - nKernelFlopsOld;

    return eta;
  }
}



VectorXcd DWF::makeEvenOdd(const VectorXcd& psi)
{
  // Permutes the supplied spinor, shuffling it so that all of the 5D lattice
  // points are split into even and odd sites

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_);

  if (psi.size() != this->operatorSize_)
    return eta;

  int size4d = this->operatorSize_ / this->Ls_;
  int halfSize4d = size4d / 2;

  for (int i = 0; i < this->Ls_; ++i) {
    VectorXcd eta4d
      = this->kernel_->makeEvenOdd(psi.segment(i * size4d, size4d));

    if (i % 2 == 0) {
      eta.segment(i * halfSize4d, halfSize4d)
	= eta4d.head(halfSize4d);
      eta.segment((i + this->Ls_) * halfSize4d, halfSize4d)
	= eta4d.tail(halfSize4d);
    }
    else {
      eta.segment(i * halfSize4d, halfSize4d)
	= eta4d.tail(halfSize4d);
      eta.segment((i + this->Ls_) * halfSize4d, halfSize4d)
	= eta4d.head(halfSize4d);      
    }
  }

  return eta;
}



VectorXcd DWF::removeEvenOdd(const VectorXcd& psi)
{
  // Permutes the supplied spinor, shuffling it and putting it back into
  // lexicographic order

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_);

  if (psi.size() != this->operatorSize_)
    return eta;

  int size4d = this->operatorSize_ / this->Ls_;
  int halfSize4d = size4d / 2;

  for (int i = 0; i < this->Ls_; ++i) {
    VectorXcd eta4d = VectorXcd::Zero(size4d);

    if (i % 2 == 0) {
      eta4d.head(halfSize4d)
	= psi.segment(i * halfSize4d, halfSize4d);
      eta4d.tail(halfSize4d)
	= psi.segment((i + this->Ls_) * halfSize4d, halfSize4d);
    }
    else {
      eta4d.tail(halfSize4d)
	= psi.segment(i * halfSize4d, halfSize4d);
      eta4d.head(halfSize4d)
	= psi.segment((i + this->Ls_) * halfSize4d, halfSize4d);
    }
    eta.segment(i * size4d, size4d) = this->kernel_->removeEvenOdd(eta4d);
  }

  return eta;
}



VectorXcd DWF::applyEvenEvenInv(const VectorXcd& psi)
{
  // Applies the inverse of the even-even part of the domain wall operator
  // to the supplied vector.

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_ / 2);

  if (psi.size() != this->operatorSize_ / 2)
    return eta;

  int halfSize4d = this->operatorSize_ / (2 * this->Ls_);

  VectorXcd evenEvenDiagonal
    = this->kernel_->applyEvenEven(VectorXcd::Ones(halfSize4d)) 
    + VectorXcd::Ones(halfSize4d);
  VectorXcd oddOddDiagonal
    = this->kernel_->applyOddOdd(VectorXcd::Ones(halfSize4d)) 
    + VectorXcd::Ones(halfSize4d);

  for (int i = 0; i < this->Ls_; ++i) {
    if (i % 2 == 0)
      eta.segment(i * halfSize4d, halfSize4d)
	= psi.segment(i * halfSize4d, halfSize4d).array()
	/ evenEvenDiagonal.array();
    else
      eta.segment(i * halfSize4d, halfSize4d)
	= psi.segment(i * halfSize4d, halfSize4d).array()
	/ oddOddDiagonal.array();
  }

  return eta;
}



VectorXcd DWF::applyOddOdd(const VectorXcd& psi)
{
  // Applies the odd diagonal piece to the supplied spinor

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_ / 2);

  if (psi.size() != this->operatorSize_ / 2)
    return eta;

  int halfSize4d = this->operatorSize_ / (2 * this->Ls_);

  for (int i = 0; i < this->Ls_; ++i) {
    if (i % 2 == 0)
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyOddOdd(psi.segment(i * halfSize4d, halfSize4d))
	+ psi.segment(i * halfSize4d, halfSize4d);
    else
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyEvenEven(psi.segment(i * halfSize4d, halfSize4d))
	+ psi.segment(i * halfSize4d, halfSize4d);
  }

  return eta;
}



VectorXcd DWF::applyEvenOdd(const VectorXcd& psi)
{
  // Apply the even-odd piece of the Dirac operator to the supplied spinor

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_ / 2);

  if (psi.size() != this->operatorSize_ / 2)
    return eta;

  int halfSize4d = this->operatorSize_ / (2 * this->Ls_);

  for (int i = 0; i < this->Ls_; ++i) {
    if (i % 2 == 0)
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyEvenOdd(psi.segment(i * halfSize4d, halfSize4d));
    else
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyOddEven(psi.segment(i * halfSize4d, halfSize4d));

    if (i == 0) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment(halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d) 
	+= this->mass_
	* pyQCD::multiplyPplus(psi.segment((this->Ls_ - 1) * halfSize4d,
					   halfSize4d));
    }
    else if (i == this->Ls_ - 1) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment((this->Ls_ - 2) * halfSize4d,
					    halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	+= this->mass_ * pyQCD::multiplyPminus(psi.segment(0, halfSize4d));
    }
    else {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment((i + 1) * halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment((i - 1) * halfSize4d, halfSize4d));
    }
  }

  return eta;
}



VectorXcd DWF::applyEvenOddDagger(const VectorXcd& psi)
{
  // Apply the daggered even-odd piece of the Dirac operator to the supplied
  // spinor

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_ / 2);

  if (psi.size() != this->operatorSize_ / 2)
    return eta;

  int halfSize4d = this->operatorSize_ / (2 * this->Ls_);

  for (int i = 0; i < this->Ls_; ++i) {

    eta.segment(i * halfSize4d, halfSize4d)
      = pyQCD::multiplyGamma5(psi.segment(i * halfSize4d, halfSize4d));

    if (i % 2 == 0)
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyEvenOdd(eta.segment(i * halfSize4d, halfSize4d));
    else
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyOddEven(eta.segment(i * halfSize4d, halfSize4d));
    
    eta.segment(i * halfSize4d, halfSize4d)
      = pyQCD::multiplyGamma5(eta.segment(i * halfSize4d, halfSize4d));
      
    if (i == 0) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment(halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d) 
	+= this->mass_
	* pyQCD::multiplyPminus(psi.segment((this->Ls_ - 1) * halfSize4d,
					    halfSize4d));
    }
    else if (i == this->Ls_ - 1) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment((this->Ls_ - 2) * halfSize4d,
					    halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	+= this->mass_ * pyQCD::multiplyPplus(psi.segment(0, halfSize4d));
    }
    else {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment((i + 1) * halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment((i - 1) * halfSize4d, halfSize4d));
    }
  }

  return eta;
}



VectorXcd DWF::applyOddEven(const VectorXcd& psi)
{
  // Apply the even-odd piece of the Dirac operator to the supplied spinor

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_ / 2);

  if (psi.size() != this->operatorSize_ / 2)
    return eta;

  int halfSize4d = this->operatorSize_ / (2 * this->Ls_);

  for (int i = 0; i < this->Ls_; ++i) {
    if (i % 2 == 0)
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyOddEven(psi.segment(i * halfSize4d, halfSize4d));
    else
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyEvenOdd(psi.segment(i * halfSize4d, halfSize4d));

    if (i == 0) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment(halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d) 
	+= this->mass_
	* pyQCD::multiplyPplus(psi.segment((this->Ls_ - 1) * halfSize4d,
					   halfSize4d));
    }
    else if (i == this->Ls_ - 1) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment((this->Ls_ - 2) * halfSize4d,
					    halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	+= this->mass_ * pyQCD::multiplyPminus(psi.segment(0, halfSize4d));
    }
    else {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment((i + 1) * halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment((i - 1) * halfSize4d, halfSize4d));
    }
  }

  return eta;
}



VectorXcd DWF::applyOddEvenDagger(const VectorXcd& psi)
{
  // Apply the even-odd piece of the Dirac operator to the supplied spinor

  VectorXcd eta = VectorXcd::Zero(this->operatorSize_ / 2);

  if (psi.size() != this->operatorSize_ / 2)
    return eta;

  int halfSize4d = this->operatorSize_ / (2 * this->Ls_);

  for (int i = 0; i < this->Ls_; ++i) {

    eta.segment(i * halfSize4d, halfSize4d)
      = pyQCD::multiplyGamma5(psi.segment(i * halfSize4d, halfSize4d));

    if (i % 2 == 0)
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyOddEven(eta.segment(i * halfSize4d, halfSize4d));
    else
      eta.segment(i * halfSize4d, halfSize4d)
	= this->kernel_->applyEvenOdd(eta.segment(i * halfSize4d, halfSize4d));

    eta.segment(i * halfSize4d, halfSize4d)
      = pyQCD::multiplyGamma5(eta.segment(i * halfSize4d, halfSize4d));

    if (i == 0) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment(halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d) 
	+= this->mass_
	* pyQCD::multiplyPminus(psi.segment((this->Ls_ - 1) * halfSize4d,
					    halfSize4d));
    }
    else if (i == this->Ls_ - 1) {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment((this->Ls_ - 2) * halfSize4d,
					     halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	+= this->mass_ * pyQCD::multiplyPplus(psi.segment(0, halfSize4d));
    }
    else {
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPplus(psi.segment((i + 1) * halfSize4d, halfSize4d));
      eta.segment(i * halfSize4d, halfSize4d)
	-= pyQCD::multiplyPminus(psi.segment((i - 1) * halfSize4d, halfSize4d));
    }
  }

  return eta;
}
