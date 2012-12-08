#ifndef flopoco_random_utils_convolve_mpreal_hpp
#define flopoco_random_utils_convolve_mpreal_hpp

#include <complex>

#include "random/utils/fft/fft.hpp"
#include "random/utils/fft/fft_mpreal.hpp"

#include "random/utils/fft/convolve.hpp"

namespace flopoco
{
namespace random
{
	inline std::vector<mpfr::mpreal> convolve_mpreal(const std::vector<mpfr::mpreal> &a, const std::vector<mpfr::mpreal> &b, unsigned wPrec)
	{
		unsigned n=a.size()+b.size()-1;
		unsigned nn=detail::NextBinaryPower(n);
		
		std::vector<mpfr::mpreal> aa(nn, mpfr::mpreal(0,wPrec)), bb(nn, mpfr::mpreal(0,wPrec));
		for(unsigned i=0;i<a.size();i++){
			mpfr_set(aa[2*i].mpfr_ptr(), a[i].mpfr_srcptr(), MPFR_RNDN);
		}
		for(unsigned i=0;i<b.size();i++){
			mpfr_set(bb[2*i].mpfr_ptr(), b[i].mpfr_srcptr(), MPFR_RNDN);
		}
		
		fft_radix2_complex(&aa[0], nn, wPrec);
		fft_radix2_complex(&bb[0], nn, wPrec);
		
		for(unsigned i=0;i<nn;i++){
			std::complex<mpfr::mpreal> av(aa[2*i], aa[2*i+1]);
			std::complex<mpfr::mpreal> bv(bb[2*i], bb[2*i+1]);
			av=av*bv;
			aa[2*i]=real(av);
			aa[2*i+1]=imag(av);
		}
		
		ifft_radix2_complex(&aa[0], nn, wPrec);
		
		std::vector<mpfr::mpreal> res(n);
		for(unsigned i=0;i<n;i++){
			std::swap(res[i], aa[2*i]);
		}
		
		return res;
	}
	
	inline std::vector<mpfr::mpreal> self_convolve_mpreal(const std::vector<mpfr::mpreal> &a, unsigned k, unsigned wPrec)
	{
		if(k==0)
			throw std::string("self_convolve_mpreal - Self convolution of degree 0 is probably not intended.");
		if(k==1)
			return a;
		
		unsigned n=a.size()*k-1;
		unsigned nn=detail::NextBinaryPower(n);
		
		std::vector<mpfr::mpreal> aa(nn, mpfr::mpreal(0,wPrec));
		for(unsigned i=0;i<a.size();i++){
			mpfr_set(aa[2*i].mpfr_ptr(), a[i].mpfr_srcptr(), MPFR_RNDN);
		}
		
		fft_radix2_complex(&aa[0], nn, wPrec);
		
		for(unsigned i=0;i<nn;i++){
			std::complex<mpfr::mpreal> av(aa[2*i], aa[2*i+1]);
			av=pow(av,k);
			aa[2*i]=real(av);
			aa[2*i+1]=imag(av);
		}
		
		ifft_radix2_complex(&aa[0], nn, wPrec);
		
		std::vector<mpfr::mpreal> res(n);
		for(unsigned i=0;i<n;i++){
			std::swap(res[i], aa[2*i]);
		}
		
		return res;
	}
	
	template<>
	inline std::vector<mpfr::mpreal> convolve(const std::vector<mpfr::mpreal> &a, const std::vector<mpfr::mpreal> &b)
	{
		mpfr_prec_t wPrec=mpfr_get_default_prec();
		for(unsigned i=0;i<a.size();i++){
			wPrec=std::max(wPrec, a[i].get_prec());
		}
		for(unsigned i=0;i<b.size();i++){
			wPrec=std::max(wPrec, b[i].get_prec());
		}
		return convolve_mpreal(a,b,wPrec);
	}
	
	template<>
	inline std::vector<mpfr::mpreal> self_convolve(const std::vector<mpfr::mpreal> &a, unsigned k)
	{
		mpfr_prec_t wPrec=mpfr_get_default_prec();
		for(unsigned i=0;i<a.size();i++){
			wPrec=std::max(wPrec, a[i].get_prec());
		}
		return self_convolve_mpreal(a,k,wPrec);
	}
	
	
};
};

#endif
