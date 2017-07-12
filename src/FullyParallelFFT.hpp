#ifdef HAVE_PAGLIB

#include "Operator.hpp"
#include "utils.hpp"

namespace flopoco {

	// new operator class declaration
	class FullyParallelFFT : public Operator {
	public:
        int wIn;
        string rotatorFileName,FFTRealizationFileName;


	public:

        FullyParallelFFT(Target* target, int wIn_ = 16,  string rotatorFileName_=0, string FFTRealizationFileName_=0);

		// destructor
		~FullyParallelFFT() {};
		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);

		/** Factory method that parses arguments and calls the constructor */
		static OperatorPtr parseArguments(Target *target , vector<string> &args);
		
		/** Factory register method */ 
		static void registerFactory();

	};

}//namespace
#endif // HAVE_PAGLIB
