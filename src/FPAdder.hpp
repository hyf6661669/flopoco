#ifndef FPADDER_HPP
#define FPADDER_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "LZOC.hpp"
#include "Shifters.hpp"
#include "FPNumber.hpp"
#include "IntAdder.hpp"
#include "LZOCShifterSticky.hpp"

/** The FPAdder class */
class FPAdder : public Operator
{
public:
	/**
	 * The FPAdder constructor
	 * @param[in]		target		the target device
	 * @param[in]		wEX			the the with of the exponent for the f-p number X
	 * @param[in]		wFX			the the with of the fraction for the f-p number X
	 * @param[in]		wEY			the the with of the exponent for the f-p number Y
	 * @param[in]		wFY			the the with of the fraction for the f-p number Y
	 * @param[in]		wER			the the with of the exponent for the addition result
	 * @param[in]		wFR			the the with of the fraction for the addition result
	 */
	FPAdder(Target* target, int wEX, int wFX, int wEY, int wFY, int wER, int wFR);

	/**
	 * FPAdder destructor
	 */
	~FPAdder();

	/**
	 * Method belonging to the Operator class overloaded by the FPAdder class
	 * @param[in,out] o     the stream where the current architecture will be outputed to
	 * @param[in]     name  the name of the entity corresponding to the architecture generated in this method
	 **/
	void outputVHDL(std::ostream& o, std::string name);

	/** Method for setting the operator name
	*/
	void setOperatorName();	

	/**
	 * Gets the signals which are interesting for TestCases.
	 * @see TestIOMap
	 */
	TestIOMap getTestIOMap();

	/**
	 * Gets the correct value associated to one or more inputs.
	 * @param a the array which contains both already filled inputs and
	 *          to be filled outputs in the order specified in getTestIOMap.
	 */
	void fillTestCase(mpz_class a[]);
	
private:
	/** The width of the exponent for the input X */
	int wEX; 
	/** The width of the fraction for the input X */
	int wFX; 
	/** The width of the exponent for the input Y */
	int wEY; 
	/** The width of the fraction for the input Y */
	int wFY; 
	/** The width of the exponent for the output R */
	int wER; 
	/** The width of the fraction for the output R */
	int wFR;
	/** Signal if the output of the operator is to be or not normalized*/

	/** The integer adder object */
	IntAdder *intadd1; 
	/** The integer adder object */
	IntAdder *intadd2; 
	/** The integer adder object */
	IntAdder *intaddClose1; 
	/** The integer adder object */
	IntAdder *intaddClose2; 
	/** The integer adder object */
	IntAdder *intaddClose3; 

	/** The integer adder object */
	IntAdder *intaddFar1; 
	/** The integer adder object */
	IntAdder *intaddFar2; 
	/** The integer adder object */
	IntAdder *intaddFar3; 

	LZOC* leadingZeroCounter;
	Shifter* leftShifter;
	Shifter* rightShifter;
	
	IntAdder* adderList[8];
	
	LZOCShifterSticky* lzocs; 

	int wF;
	int wE;
	int wOutLZC;
	int sizeRightShift;
	
	int swapDifferencePipelineDepth;
	int closePathDepth;
	int farPathDepth;
	int maxPathDepth;
	
};

#endif
