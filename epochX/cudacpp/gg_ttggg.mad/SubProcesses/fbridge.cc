#include "Bridge.h"
#include "CPPProcess.h"
#include "CudaRuntime.h"

extern "C"
{
  /**
   * The namespace where the Bridge class is taken from.
   *
   * In the current implementation, two separate shared libraries are created for the GPU/CUDA and CPU/C++ implementations.
   * Actually, two shared libraries for GPU and CPU are created for each of the five SIMD implementations on CPUs (none, sse4, avx2, 512y, 512z).
   * A single fcreatebridge_ symbol is created in each library with the same name, connected to the appropriate Bridge on CPU or GPU.
   * The Fortran MadEvent code is always the same: the choice whether to use a CPU or GPU implementation is done by linking the appropriate library.
   * As the names of the two CPU/GPU libraries are the same in the five SIMD implementations, the choice of SIMD is done by setting LD_LIBRARY_PATH.
   *
   * In a future implementation, a single heterogeneous shared library may be created, with the same interface.
   * Using the same Fortran MadEvent code, linking to the hetrerogeneous library would allow access to both CPU and GPU implementations.
   * The specific heterogeneous configuration (how many GPUs, how many threads on each CPU, etc) could be loaded in CUDA/C++ from a data file.
   */
#ifdef __CUDACC__
  using namespace mg5amcGpu;
#else
  using namespace mg5amcCpu;
#endif

  /**
   * The floating point precision used in Fortran arrays.
   * This is presently hardcoded to double precision (REAL*8).
   */
  using FORTRANFPTYPE = double; // for Fortran double precision (REAL*8) arrays
  //using FORTRANFPTYPE = float; // for Fortran single precision (REAL*4) arrays

  /**
   * Create a Bridge and return its pointer.
   * This is a C symbol that should be called from the Fortran code (in auto_dsig1.f).
   *
   * @param ppbridge the pointer to the Bridge pointer (the Bridge pointer is handled in Fortran as an INTEGER*8 variable)
   * @param nevtF the pointer to the number of events in the Fortran arrays
   * @param nparF the pointer to the number of external particles in the Fortran arrays (KEPT FOR SANITY CHECKS ONLY)
   * @param np4F the pointer to the number of momenta components, usually 4, in the Fortran arrays (KEPT FOR SANITY CHECKS ONLY)
   */
  void fbridgecreate_( CppObjectInFortran** ppbridge, const int* pnevtF, const int* pnparF, const int* pnp4F )
  {
#ifdef __CUDACC__
    CudaRuntime::setUp();
#endif
    // Create a process object, read parm card and set parameters
    // FIXME: the process instance can happily go out of scope because it is only needed to read parameters?
    // FIXME: the CPPProcess should really be a singleton? what if fbridgecreate is called from several Fortran threads?
    CPPProcess process( /*verbose=*/false );
    process.initProc( "../../Cards/param_card.dat" );
    // FIXME: disable OMP in Bridge when called from Fortran
    *ppbridge = new Bridge<FORTRANFPTYPE>( *pnevtF, *pnparF, *pnp4F );
  }

  /**
   * Delete a Bridge.
   * This is a C symbol that should be called from the Fortran code (in auto_dsig1.f).
   *
   * @param ppbridge the pointer to the Bridge pointer (the Bridge pointer is handled in Fortran as an INTEGER*8 variable)
   */
  void fbridgedelete_( CppObjectInFortran** ppbridge )
  {
    Bridge<FORTRANFPTYPE>* pbridge = dynamic_cast<Bridge<FORTRANFPTYPE>*>( *ppbridge );
    if( pbridge == 0 ) throw std::runtime_error( "fbridgedelete_: invalid Bridge address" );
    delete pbridge;
#ifdef __CUDACC__
    CudaRuntime::tearDown();
#endif
  }

  /**
   * Execute the matrix-element calculation "sequence" via a Bridge on GPU/CUDA or CUDA/C++.
   * This is a C symbol that should be called from the Fortran code (in auto_dsig1.f).
   *
   * @param ppbridge the pointer to the Bridge pointer (the Bridge pointer is handled in Fortran as an INTEGER*8 variable)
   * @param momenta the pointer to the input 4-momenta
   * @param gs the pointer to the input Gs (running QCD coupling constant alphas)
   * @param mes the pointer to the output matrix elements
   */
  void fbridgesequence_( CppObjectInFortran** ppbridge, const FORTRANFPTYPE* momenta, const FORTRANFPTYPE* gs, FORTRANFPTYPE* mes )
  {
    Bridge<FORTRANFPTYPE>* pbridge = dynamic_cast<Bridge<FORTRANFPTYPE>*>( *ppbridge );
    if( pbridge == 0 ) throw std::runtime_error( "fbridgesequence_: invalid Bridge address" );
#ifdef __CUDACC__
    // Use the device/GPU implementation in the CUDA library
    // (there is also a host implementation in this library)
    pbridge->gpu_sequence( momenta, gs, mes );
#else
    // Use the host/CPU implementation in the C++ library
    // (there is no device implementation in this library)
    pbridge->cpu_sequence( momenta, gs, mes );
#endif
  }
}
