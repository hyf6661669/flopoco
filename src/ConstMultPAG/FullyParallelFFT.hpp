#ifndef __FULLYPARALLELFFT_HPP
#define __FULLYPARALLELFFT_HPP

#ifdef HAVE_PAGLIB

#include "Operator.hpp"
#include "utils.hpp"

namespace flopoco {

	// new operator class declaration
	class FullyParallelFFT : public Operator {
	public:
        int wIn, fftSize, bC, b, wLE;
        string alg,rotatorFileName,FFTAlgorithmFileName;
        bool intPip;


	public:

        FullyParallelFFT(Target* target, int wIn_ = 16, int fftSize_=0, int bC_  =12, int b_=0, int wLE_=0, string alg_=0, string rotatorFileName_=0, string FFTAlgorithmFileName_=0, bool intPip_=true);

		// destructor
		~FullyParallelFFT() {};
		void emulate(TestCase * tc);
        int getBitReverse(int value, int bits);
		void buildStandardTestCases(TestCaseList* tcl);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);
		
		/** Factory register method */ 
		static void registerFactory();

	};

}//namespace
#endif // HAVE_PAGLIB
#endif //__FULLYPARALLELFFT_HPP