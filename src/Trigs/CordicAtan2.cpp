
#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "CordicAtan2.hpp"

using namespace std;
/* TODO Debugging:
There are still a few last-bit errors with the current setup in
./flopoco -verbose=3 -pipeline=no CordicAtan2 8 TestBenchFile -2

One is unavoidable, it is atan2 (0,0)
   TODO add a don't care to the test framework?

For the other ones:
 - the negate by not versus negate by - is not the culprit.
 - there are 3 parameters to vary: iterations, gXY, gA. Augmenting them doesn't change much.
TODO : first investigate the problematic values and add them to the regression test
*/
namespace flopoco{

	// TODO Options:
	// An option to output the angle in the (atan(2^-i)) basis
	// an an option for outputting the norm of the vector as well (scaled or not)

#define CORDIC 0

	CordicAtan2::CordicAtan2(Target* target, int w_, int method, map<string, double> inputDelays) 
		: Operator(target), w(w_)
	{
	
		int stage;
		srcFileName="CordicAtan2";
		setCopyrightString ( "Matei Istoan, Florent de Dinechin (2012-...)" );
	
		ostringstream name;
		name << "CordicAtan2_"<< w_;
		if(target->isPipelined())
			name  <<"_f" << target->frequencyMHz();
		else 
			name << "_comb";
		name << "_uid" << getNewUId();
		setName( name.str() );
	
		// everybody needs many digits of Pi (used by emulate etc)
		mpfr_init2(constPi, 10*w);
		mpfr_const_pi( constPi, GMP_RNDN);
	
		mpfr_t  zatan;	// used only by CORDIC but who cares
		mpfr_init2(zatan, 10*w);
	
	
		// declaring inputs. 
		// man atan2 says "atan2(y,x) is atan(y/x) so we will provide the inputs in the same order...
		addInput  ( "Y"  , w, true );
		addInput  ( "X"  , w, true );
	
		// declaring output
		addOutput  ( "A"  , w, 2 );
	
		setCriticalPath(getMaxInputDelays(inputDelays));
		//		manageCriticalPath( target->lutDelay());
	

	
	
		//Defining the various parameters according to method
	
		if(method==CORDIC) {
			negateByComplement=true;
	
#define ROUNDED_ROTATION 0 // 0:trunc 
	
#if ROUNDED_ROTATION
			REPORT(DEBUG, "Using rounded rotation trick");
#endif

			// ulp = weight of the LSB of the result is 2^(-w+1)
			// half-ulp is 2^-w
			// atan(2^-w) < 2^-w therefore after w interations,  method error will be bounded by 2^-w 
			maxIterations = w-1;
	
			//error analysis for the (x,y) datapath
			double eps;  //error in ulp
			eps=1; // initial neg-by-not

			double shift=0.5;
			for(stage=1; stage<=maxIterations; stage++){
#if ROUNDED_ROTATION
				eps = eps + eps*shift + 0.5; // 0.5 assume rounding in the rotation.
#else
				eps = eps + eps*shift + 1.0; // 1.0 assume truncation in the rotation.
#endif
				shift *=0.5;
			}

			// guard bits depend only on the number of iterations
			gXY = (int) ceil(log2(eps));  
			REPORT(DEBUG, "Error analysis computes eps=" << eps << " ulps on the XY datapath, hence  gXY=" << gXY);


			//error analysis for the A datapath
			eps = maxIterations*0.5; // only the rounding error in the atan constant
		
			gA = 1 + (int) ceil(log2(eps)); // +1 for the final rounding 
		
			REPORT(DEBUG, "Error analysis computes eps=" << eps <<  " ulps on the A datapath, hence  gA=" << gA );
		
		} // end if method==CORDIC
		else{
			gXY=0;
			gA=0;
		}

		int sizeXY = w+gXY;
		///////////// VHDL GENERATION
	
		// manage symmetries: we need to reduce (X,Y) to a quadrant (-pi/4, pi/4)
	
		vhdl << tab << declare("sgnX") << " <= X" << of(w-1) << ";" << endl;
		vhdl << tab << declare("sgnY") << " <= Y" << of(w-1) << ";" << endl;
	
		// TODO: replace the following with LUT-based comparators
		// and first maybe experiment with synthesis tools
		vhdl << tab << declare("XmY", w+1) << " <= (sgnX & X)-(sgnY & Y);" << endl;
		vhdl << tab << declare("XpY", w+1) << " <= (sgnX & X)+(sgnY & Y);" << endl;
		vhdl << tab << declare("XltY") << " <= XmY" << of(w) <<";" << endl;
		vhdl << tab << declare("mYltX") << " <= not XpY" << of(w) <<";" << endl;
		// Range reduction: we define 4 quadrants, each centered on one axis (these are not just the sign quadrants)
		// Then each quadrant is decomposed in its positive and its negative octant.
		vhdl << tab << "-- quadrant will also be the angle to add at the end" <<endl;
		vhdl << tab << declare("quadrant", 2) << " <= " << endl;
		vhdl << tab << tab << "\"00\"  when (not sgnX and not XltY and     mYltX)='1' else"    << endl;
		vhdl << tab << tab << "\"01\"  when (not sgnY and     XltY and     mYltX)='1' else"    << endl;
		vhdl << tab << tab << "\"10\"  when (    sgnX and     XltY and not mYltX)='1' else"    << endl;
		vhdl << tab << tab << "\"11\";"    << endl;
	
		vhdl << tab << declare("pX", sizeXY) << " <=      X  & " << zg(gXY)<< ";" << endl;
		vhdl << tab << declare("pY", sizeXY) << " <=      Y  & " << zg(gXY)<< ";" << endl;
		if (negateByComplement)	{		// good enough for Cordic
			vhdl << tab << declare("mX", sizeXY) << " <= (not X) & " << og(gXY)<< ";  -- negation by not, implies one ulp error." << endl;
			vhdl << tab << declare("mY", sizeXY) << " <= (not Y) & " << og(gXY)<< ";  -- negation by not, implies one ulp error. " << endl;
		}else {
			vhdl << tab << declare("mX", sizeXY) << " <= (" << zg(w) << "- X) & " << og(gXY) << ";  -- negation" << endl;
			vhdl << tab << declare("my", sizeXY) << " <= (" << zg(w) << "- Y) & " << og(gXY) << ";  -- negation" << endl;
		}
		vhdl << tab << declare("XR", sizeXY) << " <= " << endl;
		vhdl << tab << tab << "pX when quadrant=\"00\"   else " << endl;
		vhdl << tab << tab << "pY when quadrant=\"01\"   else " << endl;
		vhdl << tab << tab << "mX when quadrant=\"10\"   else " << endl;
		vhdl << tab << tab << "mY;"    << endl;
		vhdl << tab << "-- This XR is always positive, we hope the synthesizer will notice it" << endl;
	
		vhdl << tab << declare("YR", sizeXY) << " <= " << endl;
		vhdl << tab << tab << "pY when quadrant=\"00\" and sgnY='0'  else " << endl;
		vhdl << tab << tab << "mY when quadrant=\"00\" and sgnY='1'  else " << endl;
		vhdl << tab << tab << "pX when quadrant=\"01\" and sgnX='0'  else " << endl;
		vhdl << tab << tab << "mX when quadrant=\"01\" and sgnX='1'  else " << endl;
		vhdl << tab << tab << "pY when quadrant=\"10\" and sgnY='0'  else " << endl;
		vhdl << tab << tab << "mY when quadrant=\"10\" and sgnY='1'  else " << endl;
		vhdl << tab << tab << "pX when quadrant=\"11\" and sgnX='0'  else "    << endl;
		vhdl << tab << tab << "mX ;"    << endl;
		vhdl << tab << "-- This YR is always positive, we hope the synthesizer will notice it" << endl;

		vhdl << tab << declare("finalAdd") << " <= " << endl;
		vhdl << tab << tab << "'1' when (quadrant=\"00\" and sgnY='0') or(quadrant=\"01\" and sgnX='1') or (quadrant=\"10\" and sgnY='1') or (quadrant=\"11\" and sgnX='0')" << endl;
		vhdl << tab << tab << " else '0';  -- this information is sent to the end of the pipeline, better compute it here as one bit"    << endl; 

		//CORDIC iterations

		int sizeZ=w-2+gA; // w-2 because two bits come from arg red 
		int zMSB=-1;      // -1 because these two bits have weight 0 and -1, but we must keep the sign
		int zLSB = zMSB-sizeZ+1;

		vhdl << tab << declare("X1", sizeXY) << " <= XR;" << endl;
		vhdl << tab << declare("Y1", sizeXY) << " <= YR;" << endl;

		stage=1;
		vhdl << tab << declare(join("XShift", stage), sizeXY) << " <= " << zg(stage) << " & X" << stage << range(sizeXY-1, stage) << ";" <<endl;			
		vhdl << tab << declare(join("YShift", stage), sizeXY) << " <= " << zg(stage) << " & Y" << stage << range(sizeXY-1, stage) << ";" <<endl;			
		vhdl << tab << declare(join("X", stage+1), sizeXY) << " <= " << join("X", stage) << " + " << join("YShift", stage) << " ;" << endl;
			
		vhdl << tab << declare(join("Y", stage+1), sizeXY) << " <= " << join("Y", stage) << " - " << join("XShift", stage) << " ;" << endl;
		

		//create the constant signal for the arctan
		mpfr_set_d(zatan, 1.0, GMP_RNDN);
		mpfr_div_2si(zatan, zatan, stage, GMP_RNDN);
		mpfr_atan(zatan, zatan, GMP_RNDN);
		mpfr_div(zatan, zatan, constPi, GMP_RNDN);
		mpfr_t roundbit;
		mpfr_init2(roundbit, 30); // should be enough for anybody
		mpfr_set_d(roundbit, 1.0, GMP_RNDN);
		mpfr_div_2si(roundbit, roundbit, w, GMP_RNDN); // roundbit is in position 2^-w
		
		REPORT(DEBUG, "stage=" << stage << "  atancst=" << printMPFR(zatan));

		mpfr_add(zatan, zatan, roundbit, GMP_RNDN);
		vhdl << tab << declare("Z2", sizeZ) << " <= " << unsignedFixPointNumber(zatan, zMSB, zLSB) << "; -- initial atan, plus round bit" <<endl;

		for(stage=2; stage<=maxIterations; stage++){
			vhdl << tab << "--- Iteration " << stage << " ---" << endl;
			//shift Xin and Yin with 2^n positions to the right
			// From there on X is always positive, but Y may be negative and thus need sign extend
			vhdl << tab << declare(join("sgnY", stage))  << " <= " <<  join("Y", stage)  <<  of(sizeXY-1) << ";" << endl; 
			vhdl << tab << declare(join("XShift", stage), sizeXY) << " <= " << zg(stage) << " & X" << stage << range(sizeXY-1, stage) << ";" <<endl;			
			vhdl << tab << declare(join("YShift", stage), sizeXY) 
			     << " <= " << rangeAssign(sizeXY-1, sizeXY-stage, join("sgnY", stage))   
			     << " & Y" << stage << range(sizeXY-1, stage) << ";" << endl;
			
			// Critical path delay for one stage:
			// The data dependency is from one Z to the next 
			// We may assume that the rotations themselves overlap once the DI are known
			//manageCriticalPath(target->localWireDelay(w) + target->adderDelay(w) + target->lutDelay()));
			// 			manageCriticalPath(target->localWireDelay(sizeZ) + target->adderDelay(sizeZ));
			
			
			
#if ROUNDED_ROTATION  // rounding of the shifted operand, should save 1 bit in each addition
			vhdl << tab << declare(join("XShiftRoundBit", stage)) << " <= " << join("X", stage)  << of(stage-1) << ";" << endl;
			vhdl << tab << declare(join("YShiftRoundBit", stage)) << " <= " << join("Y", stage) << of(stage-1) << ";" <<endl;
			vhdl << tab << declare(join("XShiftNeg", stage), w+1) << " <= " << rangeAssign(w, 0, join("sgnY", stage)) << " xor " << join("XShift", stage)   << " ;" << endl;
			vhdl << tab << declare(join("YShiftNeg", stage), w+1) << " <= (not " << rangeAssign(w, 0, join("sgnY", stage)) << ") xor " << join("YShift", stage)   << " ;" << endl;

			vhdl << tab << declare(join("X", stage+1), w+g) << " <= " 
			     << join("X", stage) << " + " << join("YShiftNeg", stage) << " +  not (" << join("sgnY", stage) << " xor " << join("YShiftRoundBit", stage) << ") ;" << endl;

			vhdl << tab << declare(join("Y", stage+1), w+g) << " <= " 
			     << join("Y", stage) << " + " << join("XShiftNeg", stage) << " + (" << join("sgnY", stage) << " xor " << join("XShiftRoundBit", stage) << ") ;" << endl;

#else
			// truncation of the shifted operand
			vhdl << tab << declare(join("X", stage+1), sizeXY) << " <= " 
			     << join("X", stage) << " - " << join("YShift", stage) << " when " << join("sgnY", stage) << "=\'1\'     else "
			     << join("X", stage) << " + " << join("YShift", stage) << " ;" << endl;
			
			vhdl << tab << declare(join("Y", stage+1), sizeXY) << " <= " 
			     << join("Y", stage) << " + " << join("XShift", stage) << " when " << join("sgnY", stage) << "=\'1\'     else "
			     << join("Y", stage) << " - " << join("XShift", stage) << " ;" << endl;
#endif			
			
			//create the constant signal for the arctan
			mpfr_set_d(zatan, 1.0, GMP_RNDN);
			mpfr_div_2si(zatan, zatan, stage, GMP_RNDN);
			mpfr_atan(zatan, zatan, GMP_RNDN);
			mpfr_div(zatan, zatan, constPi, GMP_RNDN);
			REPORT(DEBUG, "stage=" << stage << "  atancst=" << printMPFR(zatan));		

			// rounding here in unsignedFixPointNumber()
			vhdl << tab << declare(join("atan2PowStage", stage), sizeZ) << " <= " << unsignedFixPointNumber(zatan, zMSB, zLSB) << ";" <<endl;
			vhdl << tab << declare(join("Z", stage+1), sizeZ) << " <= " 
					 << join("Z", stage) << " + " << join("atan2PowStage", stage) << " when " << join("sgnY", stage) << "=\'0\'      else "
					 << join("Z", stage) << " - " << join("atan2PowStage", stage) << " ;" << endl;


		} //end for loop
		
		// Give the time to finish the last rotation
		// manageCriticalPath( target->localWireDelay(w+1) + target->adderDelay(w+1) // actual CP delay
		//                     - (target->localWireDelay(sizeZ+1) + target->adderDelay(sizeZ+1))); // CP delay that was already added

		// reconstruction

		string finalZ=join("Z", maxIterations+1);
		vhdl << tab << declare("qangle", w) << " <= (quadrant & " << zg(w-2) << ");" << endl;
		vhdl << tab << declare("finalZext", w) << " <= " << finalZ << of(sizeZ-1) << " & "<< finalZ << range(sizeZ-1, sizeZ-w+1) << "; -- sign-extended and rounded" << endl;
		vhdl << tab << "A <= "
				 << tab << tab << "     qangle + finalZext  when finalAdd='1'" << endl
				 << tab << tab << "else qangle - finalZext;" << endl;
	};


	CordicAtan2::~CordicAtan2(){
		mpfr_clears (kfactor, constPi, NULL);		
	};


	void CordicAtan2::emulate(TestCase * tc) 
	{
		mpfr_t x,y,a;
		mpfr_init2(x, 10*w);
		mpfr_init2(y, 10*w);
		mpfr_init2(a, 10*w);

		mpz_class az;

		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		// interpret as signed two'ss complement
		if (1==(svX >> (w-1))) // sign bit
			svX -= (1<<w);
		if (1==(svY >> (w-1))) // sign bit
			svY -= (1<<w);
		/* Compute correct value */
		
		mpfr_set_z (x, svX.get_mpz_t(), GMP_RNDN); //  exact
		mpfr_set_z (y, svY.get_mpz_t(), GMP_RNDN); //  exact
	
		mpfr_atan2(a, y, x, GMP_RNDN); // a between -pi and pi
		mpfr_div(a, a, constPi, GMP_RNDN); // a between -1 and 1

		// Now convert a to fix point
		// Align to fix point by adding 6 -- if we just add 4 there is a 1-bit shift in case a<0
		mpfr_add_d(a, a, 6.0, GMP_RNDN);
		mpfr_mul_2si (a, a, w-1, GMP_RNDN); // exact scaling 

		mpz_class mask = (mpz_class(1)<<w) -1; 

		mpfr_get_z (az.get_mpz_t(), a, GMP_RNDD); // there can be a real rounding here
		az -= mpz_class(6)<<(w-1);
		az &= mask; 
 		tc->addExpectedOutput ("A", az);

		mpfr_get_z (az.get_mpz_t(), a, GMP_RNDU); // there can be a real rounding here
		az -= mpz_class(6)<<(w-1);
		az &= mask; 
 		tc->addExpectedOutput ("A", az);
		
		// clean up
		mpfr_clears (x,y,a, NULL);		
	}






	void CordicAtan2::buildStandardTestCases(TestCaseList * tcl) 
	{
		mpfr_t z;
		mpz_class zz;
		TestCase* tc;

		// 0
		tc = new TestCase (this);
		tc -> addInput ("X", mpz_class(1)<< (w-2));
		tc -> addInput ("Y", mpz_class(0));
		emulate(tc);
		tcl->add(tc);

		// pi/2
		tc = new TestCase (this);
		tc -> addInput ("X", mpz_class(0));
		tc -> addInput ("Y", mpz_class(1)<< (w-2));
		emulate(tc);
		tcl->add(tc);

		//Pi/4
		tc = new TestCase (this);
		tc -> addInput ("X", mpz_class(1)<< (w-2));
		tc -> addInput ("Y", mpz_class(1)<< (w-2));
		emulate(tc);
		tcl->add(tc);


	}
	

}

