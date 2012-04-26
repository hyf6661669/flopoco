#ifndef LogicIntMultiplier_HPP
#define LogicIntMultiplier_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"
#include "../Table.hpp"
#include "../IntAdder.hpp"
#include "../IntMultiAdder.hpp"

namespace flopoco{


	/** 
	 * The Integer Multiplier class. Receives at input two numbers of 
	 * wInX and wInY widths and outputs a result having the width wOut=wInX+wInY 
	 **/
	class LogicIntMultiplier : public Operator
	{

	public:

		/** An elementary LUT-based multiplier, written as a table so that synthesis tools don't infer DSP blocks for it*/
		class SmallMultTable: public Table {
		public:
			int dx, dy; 			
			SmallMultTable(Target* target, int dx, int dy );
			mpz_class function(int x);
		};
		


		/** 
		 * The constructor of the LogicIntMultiplier class
		 * @param target argument of type Target containing the data for which this operator will be optimized
		 * @param wInX integer argument representing the width in bits of the input X 
		 * @param wInY integer argument representing the width in bits of the input Y
		 **/
		LogicIntMultiplier(Target* target, int wInX, int wInY, bool sign=false, map<string, double> inputDelays = emptyDelayMap);
		
		/** LogicIntMultiplier destructor */
		~LogicIntMultiplier();
		
		//		void outputVHDL(std::ostream& o, std::string name);


		void emulate(TestCase* tc);
		
	protected:

		int wInX_; /**< the width (in bits) of the input X  */  
		int wInY_; /**< the width (in bits) of the input Y  */
		int wInX; /**< the width (in bits) of the larger input, renamed  XX  */
		int wInY; /**< the width (in bits) of the smaller input, renamed YY  */
		int wOut_; /**< the width (in bits) of the output R  */

	private:

		IntAdder *intAdd_;            /**< The integer adder object */
		int      IntAddPipelineDepth; /**< The pipeline depth of the adder */
		int      partsX_; 	          /**< The number of parts that the input X will be split in */
		int      partsY_; 	          /**< The number of parts that the input Y will be split in */
		int      k_;                  /**< The size of the elementary logic-based square mult exploiting LUT sizes optimally (2 or 3 these days) */
		int      numberOfZerosX_; 	  /**< The number of zeros that the input X will be padded with so that it's length reaches a multiple of the suggested multiplier width */
		int      numberOfZerosY_; 	  /**< The number of zeros that the input Y will be padded with so that it's length reaches a multiple of the suggested multiplier width */
		int      multiplierWidthX_;   /**< The X width of the multiplier */
		int      multiplierWidthY_;   /**< The Y width of the multiplier */
		bool     reverse_; 	          /**< Signals if we are doing the multiplication X*Y of Y*X */
		IntAdder *intAdd1_; 
		IntAdder *intAdd2_; 
	
		bool sign_;
	};

}
#endif
