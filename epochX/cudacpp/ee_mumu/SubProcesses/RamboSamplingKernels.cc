#include "RamboSamplingKernels.h"

#include "checkCuda.h"
#include "MemoryAccessMomenta.h"
#include "MemoryAccessRandomNumbers.h"
#include "MemoryAccessWeights.h"
#include "MemoryBuffers.h"
#include "rambo.h" // inline implementation of RAMBO algorithms and kernels

#include <sstream>

#ifdef __CUDACC__
namespace mg5amcGpu
#else
namespace mg5amcCpu
#endif
{
  //--------------------------------------------------------------------------

  RamboSamplingKernelHost::RamboSamplingKernelHost( const fptype energy,                // input: energy
                                                    const BufferRandomNumbers& rnarray, // input: random numbers in [0,1]
                                                    BufferMomenta& momenta,             // output: momenta
                                                    BufferWeights& weights,             // output: weights
                                                    const size_t nevt )
    : SamplingKernelBase( energy, rnarray, momenta, weights )
    , NumberOfEvents( nevt )
  {
#ifdef __CUDACC__
    throw std::runtime_error( "RamboSamplingKernelHost is not yet implemented in CUDA" ); // FIXME!
#endif
    if ( m_rnarray.isOnDevice() ) throw std::runtime_error( "RamboSamplingKernelHost: rnarray must be a host array" );
    if ( m_momenta.isOnDevice() ) throw std::runtime_error( "RamboSamplingKernelHost: momenta must be a host array" );
    if ( m_weights.isOnDevice() ) throw std::runtime_error( "RamboSamplingKernelHost: weights must be a host array" );
  }

  //--------------------------------------------------------------------------

  void RamboSamplingKernelHost::getMomentaInitial()
  {
#ifdef __CUDACC__
    throw std::runtime_error( "RamboSamplingKernelHost is not yet implemented in CUDA" ); // FIXME!
#else
    constexpr auto getMomentaInitial = ramboGetMomentaInitial<HostAccessMomenta>;
    // ** START LOOP ON IEVT **
    for ( size_t ievt = 0; ievt < nevt(); ++ievt )
    {
      // FIXME: document constraints on these memory access functions
      fptype* ievtMomenta = &( MemoryAccessMomenta::ieventAccessIp4Ipar( m_momenta.data(), ievt, 0, 0 ) );
      getMomentaInitial( m_energy, ievtMomenta );
    }
    // ** END LOOP ON IEVT **
#endif
  }

  //--------------------------------------------------------------------------

  void RamboSamplingKernelHost::getMomentaFinal()
  {
#ifdef __CUDACC__
    throw std::runtime_error( "RamboSamplingKernelHost is not yet implemented in CUDA" ); // FIXME!
#else
    constexpr auto getMomentaFinal = ramboGetMomentaFinal<HostAccessRandomNumbers, HostAccessMomenta, HostAccessWeights>;

    // ** START LOOP ON IEVT **
    for ( size_t ievt = 0; ievt < nevt(); ++ievt )
    {
      // FIXME: document constraints on these memory access functions
      const fptype* ievtRnarray = &( MemoryAccessRandomNumbers::ieventConstAccessIp4Iparf( m_rnarray.data(), ievt, 0, 0 ) );
      fptype* ievtMomenta = &( MemoryAccessMomenta::ieventAccessIp4Ipar( m_momenta.data(), ievt, 0, 0 ) );
      fptype* ievtWeights = &( MemoryAccessWeights::ieventAccess( m_weights.data(), ievt ) );
      getMomentaFinal( m_energy, ievtRnarray, ievtMomenta, ievtWeights );
    }
#endif
  }

  //--------------------------------------------------------------------------

#ifdef __CUDACC__
  RamboSamplingKernelDevice::RamboSamplingKernelDevice( const fptype energy,                // input: energy
                                                        const BufferRandomNumbers& rnarray, // input: random numbers in [0,1]
                                                        BufferMomenta& momenta,             // output: momenta
                                                        BufferWeights& weights,             // output: weights
                                                        const size_t gpublocks,
                                                        const size_t gputhreads )
    : SamplingKernelBase( energy, rnarray, momenta, weights )
    , m_gpublocks( gpublocks )
    , m_gputhreads( gputhreads )
  {
    if ( ! m_rnarray.isOnDevice() ) throw std::runtime_error( "RamboSamplingKernelDevice: rnarray must be a device array" );
    if ( ! m_momenta.isOnDevice() ) throw std::runtime_error( "RamboSamplingKernelDevice: momenta must be a device array" );
    if ( ! m_weights.isOnDevice() ) throw std::runtime_error( "RamboSamplingKernelDevice: weights must be a device array" );
    if ( m_gpublocks == 0 ) throw std::runtime_error( "RamboSamplingKernelDevice: gpublocks must be > 0" );
    if ( m_gputhreads == 0 ) throw std::runtime_error( "RamboSamplingKernelDevice: gputhreads must be > 0" );
    // Sanity checks for memory access (are these really strictly needed?)
    constexpr int neppR = MemoryAccessRandomNumbers::neppR; // AOSOA layout
    if ( m_gputhreads%neppR != 0 )
    {
      std::ostringstream sstr;
      sstr << "RamboSamplingKernelDevice: gputhreads should be a multiple of neppR=" << neppR;
      throw std::runtime_error( sstr.str() );
    }
#ifndef __CUDACC__
    constexpr bool ispoweroftwo( int n ){ return ( n > 0 ) && !( n & ( n - 1 ) ); }; // https://stackoverflow.com/a/108360
    static_assert( ispoweroftwo( neppR ) );
#endif
  }
#endif

  //--------------------------------------------------------------------------

#ifdef __CUDACC__
  void RamboSamplingKernelDevice::getMomentaInitial()
  {
    constexpr auto getMomentaInitial = ramboGetMomentaInitial<DeviceAccessMomenta>;
    getMomentaInitial<<<m_gpublocks, m_gputhreads>>>( m_energy, m_momenta.data() );
  }
#endif

  //--------------------------------------------------------------------------

#ifdef __CUDACC__
  void RamboSamplingKernelDevice::getMomentaFinal()
  {
    constexpr auto getMomentaFinal = ramboGetMomentaFinal<DeviceAccessRandomNumbers, DeviceAccessMomenta, DeviceAccessWeights>;
    getMomentaFinal<<<m_gpublocks, m_gputhreads>>>( m_energy, m_rnarray.data(), m_momenta.data(), m_weights.data() );
  }
#endif

  //--------------------------------------------------------------------------

}
