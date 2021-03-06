
void createGammas(Complex* gammas)
{
  for (int i = 0; i < 64; ++i)
    gammas[i] = Complex(0.0, 0.0);
  
  gammas[2] = Complex(1.0, 0.0);
  gammas[7] = Complex(1.0, 0.0);
  gammas[8] = Complex(1.0, 0.0);
  gammas[13] = Complex(1.0, 0.0);

  gammas[1 * 16 + 3] = Complex(0.0, -1.0);
  gammas[1 * 16 + 6] = Complex(0.0, -1.0);
  gammas[1 * 16 + 9] = Complex(0.0, 1.0);
  gammas[1 * 16 + 12] = Complex(0.0, 1.0);

  gammas[2 * 16 + 3] = Complex(-1.0, 0.0);
  gammas[2 * 16 + 6] = Complex(1.0, 0.0);
  gammas[2 * 16 + 9] = Complex(1.0, 0.0);
  gammas[2 * 16 + 12] = Complex(-1.0, 0.0);

  gammas[3 * 16 + 2] = Complex(0.0, -1.0);
  gammas[3 * 16 + 7] = Complex(0.0, 1.0);
  gammas[3 * 16 + 8] = Complex(0.0, 1.0);
  gammas[3 * 16 + 13] = Complex(0.0, -1.0);
}

void diagonalSpinMatrices(Complex* matrices, Complex factor)
{
  for (int i = 0; i < 64; ++i)
    matrices[i] = Complex(0.0, 0.0);

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      matrices[16 * i + 4 * j + j] = factor;
}

void subtractArray(Complex* y, const Complex* x, const int length)
{
  for (int i = 0; i < length; ++i)
    y[i] -= x[i];
}

void addArray(Complex* y, const Complex* x, const int length)
{
  for (int i = 0; i < length; ++i)
    y[i] += x[i];
}

void createLinks(Complex* links, const int L, const int T)
{
  int numSites = L * L * L * T;

  for (int i = 0; i < 9 * 4 * numSites; ++i)
    links[i] = cusp::complex<float>(0.0, 0.0);

  for (int i = 0; i < 4 * numSites; ++i) {
    links[9 * i] = cusp::complex<float>(1.0, 0.0);
    links[9 * i + 4] = cusp::complex<float>(1.0, 0.0);
    links[9 * i + 8] = cusp::complex<float>(1.0, 0.0);
  }
}

void generateNeighbours(int* indices, const int hopSize, const int L,
			const int T)
{
  int numSites = L * L * L * T;

  for (int index = 0; index < numSites; ++index) {

    int latticeShape[4] = {T, L, L, L};
    
    int site[4];
    int tempIndex = index;
    for (int i = 0; i < 4; ++i) {
      site[3 - i] = mod(tempIndex, latticeShape[3 - i]);
      tempIndex /= latticeShape[3 - i];
    }

    for (int mu = 0; mu < 4; ++mu) {
      int siteBehind[4];
      int siteAhead[4];

      for (int i = 0; i < 4; ++i) {
	siteAhead[i] = site[i];
	siteBehind[i] = site[i];
      }

      siteAhead[mu] += hopSize;
      siteAhead[mu] = mod(siteAhead[mu], latticeShape[mu]);
      siteBehind[mu] -= hopSize;
      siteBehind[mu] = mod(siteBehind[mu], latticeShape[mu]);

      int siteBehindIndex
	= siteBehind[3] 
	+ L * siteBehind[2]
	+ L * L * siteBehind[1]
	+ L * L * L * siteBehind[0];

      int siteAheadIndex
	= siteAhead[3] 
	+ L * siteAhead[2]
	+ L * L * siteAhead[1]
	+ L * L * L * siteAhead[0];

      if (siteAheadIndex >= numSites || siteAheadIndex < 0)
	std::cout << "Warning: site ahead index out of bounds." << std::endl;
      if (siteBehindIndex >= numSites || siteBehindIndex < 0)
	std::cout << "Warning: site ahead index out of bounds." << std::endl;

      indices[8 * index + mu] = siteBehindIndex;
      indices[8 * index + mu + 4] = siteAheadIndex;
    }
  }
}

void generateEvenOddNeighbours(int* indices, const int hopSize,
			       const int remainder, const int L, const int T)
{
  int numSites = L * L * L * T;

  for (int index = 0; index < numSites; ++index) {

    int latticeShape[4] = {T, L, L, L};
    
    int site[4];
    int tempIndex = index;
    for (int i = 0; i < 4; ++i) {
      site[3 - i] = mod(tempIndex, latticeShape[3 - i]);
      tempIndex /= latticeShape[3 - i];
    }

    if ((site[0] + site[1] + site[2] + site[3]) % 2 == remainder) {

      for (int mu = 0; mu < 4; ++mu) {
	int siteBehind[4];
	int siteAhead[4];

	for (int i = 0; i < 4; ++i) {
	  siteAhead[i] = site[i];
	  siteBehind[i] = site[i];
	}

	siteAhead[mu] += hopSize;
	siteAhead[mu] = mod(siteAhead[mu], latticeShape[mu]);
	siteBehind[mu] -= hopSize;
	siteBehind[mu] = mod(siteBehind[mu], latticeShape[mu]);

	int siteBehindIndex
	  = siteBehind[3] 
	  + L * siteBehind[2]
	  + L * L * siteBehind[1]
	  + L * L * L * siteBehind[0];

	int siteAheadIndex
	  = siteAhead[3] 
	  + L * siteAhead[2]
	  + L * L * siteAhead[1]
	  + L * L * L * siteAhead[0];

	if (siteAheadIndex >= numSites || siteAheadIndex < 0)
	  std::cout << "Warning: site ahead index out of bounds." << std::endl;
	if (siteBehindIndex >= numSites || siteBehindIndex < 0)
	  std::cout << "Warning: site ahead index out of bounds." << std::endl;

	indices[8 * (index / 2) + mu] = siteBehindIndex;
	indices[8 * (index / 2) + mu + 4] = siteAheadIndex;
      }
    }
  }
}

void generateBoundaryConditions(Complex* siteBoundaryConditions,
				const int hopSize,
				const Complex* boundaryConditions,
				const int L, const int T)
{
  int numSites = L * L * L * T;

  for (int index = 0; index < numSites; ++index) {

    int latticeShape[4] = {T, L, L, L};
    
    int site[4];
    int tempIndex = index;
    for (int i = 0; i < 4; ++i) {
      site[3 - i] = mod(tempIndex, latticeShape[3 - i]);
      tempIndex /= latticeShape[3 - i];
    }

    for (int mu = 0; mu < 4; ++mu) {

      siteBoundaryConditions[8 * index + mu] = Complex(1.0, 0.0);
      siteBoundaryConditions[8 * index + mu + 4] = Complex(1.0, 0.0);

      if (site[mu] - hopSize < 0 || site[mu] - hopSize >= latticeShape[mu])
	siteBoundaryConditions[8 * index + mu] = boundaryConditions[mu];
      if (site[mu] + hopSize < 0 || site[mu] + hopSize >= latticeShape[mu])
	siteBoundaryConditions[8 * index + mu + 4] = boundaryConditions[mu];
    }
  }
}

void setGridAndBlockSize(int& dimBlock, int& dimGrid, int numThreads)
{
  int device;
  cudaGetDevice(&device);
  cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, device);
  dimBlock = prop.maxThreadsPerBlock;
  dimGrid = numThreads / dimBlock;

  while (dimBlock * dimGrid < numThreads)
    dimGrid++;
}

int mod(const int a, const int b)
{
  return (a % b < 0) ? a % b + b : a % b;
}

__device__
int modDev(const int a, const int b)
{
  return (a % b < 0) ? a % b + b : a % b;
}

__global__
void saxpyDev(Complex* y, const Complex* x, const Complex a, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = a * x[index] + y[index];
  }
}

__global__
void saxpyDev(Complex* y, const Complex* x, const Complex* a, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = a[index] * x[index] + y[index];
  }
}

__global__
void saxDev(Complex* y, const Complex* x, const Complex a, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = a * x[index];
  }
}

__global__
void saxDev(Complex* y, const Complex* x, const Complex* a, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = a[index] * x[index];
  }
}

__global__
void addConstantDev(Complex* y, const Complex a, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N)
    y[index] += a;
}

__global__
void assignDev(Complex* y, const Complex* x, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = x[index];
  }
}

__global__
void assignDev(Complex* y, const Complex x, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = x;
  }
}

__global__
void reciprocalDev(Complex* y, const Complex* x, const int N)
{
  int index = blockDim.x * blockIdx.x + threadIdx.x;

  if (index < N) {
    y[index] = Complex(1.0, 0.0) / x[index];
  }
}

__device__
int shiftSiteIndex(const int index, const int direction, const int numHops,
		   const int L, const int T)
{
  int latticeShape[4] = {T, L, L, L};
  int directionComponent = 1;
  for (int i = 0; i < 3 - direction; ++i)
    directionComponent *= L;
  
  int directionQuotient = index / directionComponent;

  int oldComponent 
    = modDev(directionQuotient, latticeShape[direction])
    * directionComponent;
  
  int newComponent 
    = modDev(directionQuotient + numHops, latticeShape[direction])
    * directionComponent;
  
  return index - oldComponent + newComponent;
}

template<int numHops>
__device__
Complex computeU(const Complex* links, const int siteIndex, const int mu,
		 const int a, const int b, const int L, const int T)
{
  cusp::complex<float> out(0.0, 0.0);

  int newIndex = shiftSiteIndex(siteIndex, mu, 1, L, T);
  out
    = links[36 * siteIndex + 9 * mu + 3 * a]
    * computeU<numHops - 1>(links, newIndex, mu, 0, b, L, T)
    + links[36 * siteIndex + 9 * mu + 3 * a + 1]
    * computeU<numHops - 1>(links, newIndex, mu, 1, b, L, T)
    + links[36 * siteIndex + 9 * mu + 3 * a + 2]
    * computeU<numHops - 1>(links, newIndex, mu, 2, b, L, T);

  return out;
}

template<>
__device__
Complex computeU<1>(const Complex* links, const int siteIndex, const int mu,
		    const int a, const int b, const int L, const int T)
{
  return links[36 * siteIndex + 9 * mu + 3 * a + b];
}
