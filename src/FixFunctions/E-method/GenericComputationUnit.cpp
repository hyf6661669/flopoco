/*
 * GenericComputationUnit.cpp
 *
 *  Created on: 13 Jan 2018
 *      Author: mistoan
 */

#include "GenericComputationUnit.hpp"

namespace flopoco {

	GenericComputationUnit::GenericComputationUnit(Target* target, int _radix, int _maxDigit, int _index,
			Signal *_W, Signal *_X, Signal *_Di, string _qi, int _specialCase, map<string, double> inputDelays)
	: Operator(target), radix(_radix), index(_index), maxDigit(_maxDigit),
	  msbW(_W->MSB()), lsbW(_W->LSB()),
	  msbX(_X->MSB()), lsbX(_X->LSB()),
	  msbD(_Di->MSB()), lsbD(_Di->LSB()),
	  qi(_qi),
	  specialCase(_specialCase)
	{
		ostringstream name;

		srcFileName = "GenericComputationUnit";
		name << "GenericComputationUnit_radix" << radix << "_index_" << index
				<< "_qi_" << std::setprecision(5) << qi << "_msbIn_" << vhdlize(msbW) << "_lsbIn_" << vhdlize(lsbW);
		setName(name.str()+"_uid"+vhdlize(getNewUId()));

		//safety checks
		if((radix != 2) && (radix != 4) && (radix != 8))
		{
			THROWERROR("GenericComputationUnit: radixes higher than 8 currently not supported!");
		}

		setCopyrightString("Matei Istoan, 2017");

		useNumericStd_Signed();

		//determine the MSB and the LSb for the internal computations
		msbInt = maxInt(3, msbW, msbX, msbD);
		lsbInt = minInt(3, lsbW, lsbX, lsbD);

		REPORT(DEBUG, "using internal format: msbInt=" << msbInt << ", lsbInt=" << lsbInt);

		//determine the MSB and the LSB for the DiMultX signals
		msbDiMX = msbX+(int)ceil(log2(maxDigit));
		lsbDiMX = lsbX;

		REPORT(DEBUG, "using the following format for the DiMultX signals: msbDiMX="
				<< msbDiMX << ", lsbDiMX=" << lsbDiMX);

		//create the inputs and the output
		//	the inputs
		addFixInput("Wi", true, msbW, lsbW);
		addFixInput("D0", true, msbD, lsbD);
		addFixInput("Di", true, msbD, lsbD);
		addFixInput("Dip1", true, msbD, lsbD);
		addFixInput("X", true, msbX, lsbX);
		//	the inputs for D_{i+1}[j-1]*X
		//		if required
		if(specialCase != 1)
		{
			//	the inputs for D_{i+1}[j-1]*X
			for(int i=(-maxDigit); i<=maxDigit; i++)
			{
				addFixInput(join("X_Mult_", vhdlize(i)), true, msbDiMX, lsbDiMX);
			}
		}
		// the outputs
		addFixOutput("Wi_next", true, msbInt, lsbInt, 2);

		//create the bitheap
		REPORT(DEBUG, "creating the bitheap");
		bitheap = new BitHeap(
								this,											// parent operator
								msbInt-lsbInt+1,								// maximum weight
								false, 											// enable supertiles
								join("Bitheap_"+name.str()+"_", getNewUId())	// bitheap name
								);

		//add W_i[j-1] to the bitheap
		REPORT(DEBUG, "add W_i[j-1] to the bitheap");
		bitheap->addSignedBitVector(
									lsbW-lsbInt,					//weight
									"Wi",							//input signal name
									msbW-lsbW+1						//size
									);

		//create the multiplication D_0[j-1] * (-1)*q_i
		//	if required
		if(specialCase != -1)
		{
			//create the multiplication D_0[j-1] * (-1)*q_i
			REPORT(DEBUG, "create the multiplication D_0[j-1] * (-1)*q_i");
			FixRealKCM *constMult = new FixRealKCM(
													this,						//parent operator
													"std_logic_vector(D0)",		//input signal name
													true,						//signedness
													msbD,						//msbIn
													lsbD,						//lsbIn
													lsbInt,						//lsbOut
													"(-1)*"+qi,					//constant
													false,						//add round bit
													1.0							//target ulp error
													);
			//add the result of the multiplication to the bitheap
			REPORT(DEBUG, "add the result of the multiplication to the bitheap");
			constMult->addToBitHeap(bitheap, 0);
		}
		else
		{
			//this term is not required for iteration 0
			REPORT(DEBUG, "no need to create the multiplication D_0[j-1] * (-1)*q_i for iteration 0");
		}

		//subtract D_i[j-1]
		REPORT(DEBUG, "subtract D_i[j-1]");
		bitheap->subtractSignedBitVector(
										lsbD-lsbInt,				//weight
										"Di",						//input signal name
										msbD-lsbD+1					//size
										);

		//create the multiplication D_{i+1}[j-1] * X
		//	if required
		if(specialCase != 1)
		{
			//create the multiplication D_{i+1}[j-1] * X
			//	not actual multiplication is done here, as we're multiplying by a digit
			//	instead, all the possible products are generated upstream
			//	and here we only have to choose which one to add
			REPORT(DEBUG, "create the multiplication D_{i+1}[j-1] * X");
			vhdl << tab << declareFixPoint("Dip1_Mult_X", true, msbDiMX, lsbDiMX) << " <= " << endl;
			for(int i=(-maxDigit); i<=maxDigit; i++)
			{
				mpz_class digitValue = i;

				//handle negative digits
				if(digitValue < 0)
					digitValue = mpz_class(1<<radix) + digitValue;

				vhdl << tab << tab << join("X_Mult_", vhdlize(i)) << " when Dip1="
						<< "\"" << unsignedBinary(digitValue, radix) << "\" else" << endl;
			}
			vhdl << tab << tab << "(others => '-');" << endl;
			//	add the result of the selection to the bitheap
			REPORT(DEBUG, "add the result of the selection to the bitheap");
			bitheap->addSignedBitVector(
										lsbDiMX-lsbInt,					//weight
										"Dip1_Mult_X",					//input signal name
										msbDiMX-lsbDiMX+1				//size
										);
		}
		else
		{
			//this term is not required for iteration n
			REPORT(DEBUG, "no need to create the multiplication D_{i+1}[j-1] * X for iteration n");
		}

		//compress the bitheap
		bitheap->generateCompressorVHDL();

		// Retrieve the bits we want from the bit heap
		REPORT(DEBUG, "Retrieve the bits we want from the bit heap");
		vhdl << tab << declareFixPoint("sum", true, msbInt, lsbInt) << " <= signed(" <<
				bitheap->getSumName() << range(msbInt-lsbInt, 0) << ");" << endl;

		REPORT(DEBUG, "write to the output");
		// multiply by radix, a constant shift by radix positions to the left
		vhdl << tab << "Wi_next <= sum" << range(msbInt-lsbInt-ceil(log2(radix)), 0)
				<< " & " << zg(ceil(log2(radix))) << ";" << endl;
	}


	GenericComputationUnit::~GenericComputationUnit() {

	}


	void GenericComputationUnit::emulate(TestCase * tc)
	{
		//get the inputs from the TestCase
		mpz_class svWi   = tc->getInputValue("Wi");
		mpz_class svD0   = tc->getInputValue("D0");
		mpz_class svDi   = tc->getInputValue("Di");
		mpz_class svDip1 = tc->getInputValue("Dip1");
		mpz_class svX    = tc->getInputValue("X");
		mpz_class svXMultDip1[2*maxDigit+1];
		// these inputs do not exist for iteration n
		if(specialCase != 1)
		{
			for(int i=(-maxDigit); i<=maxDigit; i++)
			{
				svXMultDip1[i+maxDigit] = tc->getInputValue(join("X_Mult_", vhdlize(i)));
			}
		}

		//manage signed digits
		mpz_class big1W      = (mpz_class(1) << (msbW-lsbW+1));
		mpz_class big1Wp     = (mpz_class(1) << (msbW-lsbW));
		mpz_class big1D      = (mpz_class(1) << radix);
		mpz_class big1Dp     = (mpz_class(1) << (radix-1));
		mpz_class big1X      = (mpz_class(1) << (msbX-lsbX+1));
		mpz_class big1Xp     = (mpz_class(1) << (msbX-lsbX));
		mpz_class big1Xmult  = (mpz_class(1) << (msbDiMX-lsbDiMX+1));
		mpz_class big1Xmultp = (mpz_class(1) << (msbDiMX-lsbDiMX));
		mpz_class big1out    = (mpz_class(1) << (msbInt-lsbInt+1));
		mpz_class big1outp   = (mpz_class(1) << (msbInt-lsbInt));

		//handle the signed inputs
		if(svWi >= big1Wp)
			svWi -= big1W;
		if(svD0 >= big1Dp)
			svD0 -= big1D;
		if(svDi >= big1Dp)
			svDi -= big1D;
		if(svDip1 >= big1Dp)
			svDip1 -= big1D;
		if(svX >= big1Xp)
			svX -= big1X;
		// these inputs do not exist for iteration n
		if(specialCase != 1)
		{
			for(int i=(-maxDigit); i<=maxDigit; i++)
			{
				if(svXMultDip1[i+maxDigit] >= big1Xmultp)
					svXMultDip1[i+maxDigit] -= big1Xmult;
			}
		}

		// compute the multiple-precision output
		mpz_class svW_next, svW_nextRd, svW_nextRu;
		mpfr_t mpW_next, mpSum, mpQi, mpTmp;

		//initialize the variables
		mpfr_inits2(10000, mpW_next, mpSum, mpQi, mpTmp, (mpfr_ptr)nullptr);

		//initialize the sum
		mpfr_set_zero(mpSum, 0);

		//add W_i[j-1]
		mpfr_set_z(mpTmp, svWi.get_mpz_t(), GMP_RNDN);
		//	scale W_i appropriately, by the amount given by lsbW
		mpfr_mul_2si(mpTmp, mpTmp, lsbW, GMP_RNDN);
		mpfr_add(mpSum, mpSum, mpTmp, GMP_RNDN);

		//subtract D_0[j-1]*q_i
		//	if required
		if(specialCase != -1)
		{
			//subtract D_0[j-1]*q_i
			//	parse q_i using Sollya
			sollya_obj_t node;
			node = sollya_lib_parse_string(qi.c_str());
			/* If  parse error throw an exception */
			if (sollya_lib_obj_is_error(node))
			{
				THROWERROR("emulate: Unable to parse string "<< qi << " as a numeric constant");
			}
			sollya_lib_get_constant(mpQi, node);
			free(node);
			// scale D_0 appropriately, by the amount given by lsbD
			mpfr_set_z(mpTmp, svD0.get_mpz_t(), GMP_RNDN);
			mpfr_mul_2si(mpTmp, mpTmp, lsbD, GMP_RNDN);
			//	subtract from the sum
			mpfr_mul(mpTmp, mpTmp, mpQi, GMP_RNDN);
			mpfr_sub(mpSum, mpSum, mpTmp, GMP_RNDN);
		}

		//subtract D_i[j-1]
		// scale D_i appropriately, by the amount given by lsbD
		mpfr_set_z(mpTmp, svDi.get_mpz_t(), GMP_RNDN);
		mpfr_mul_2si(mpTmp, mpTmp, lsbD, GMP_RNDN);
		//	subtract from the sum
		mpfr_sub(mpSum, mpSum, mpTmp, GMP_RNDN);

		//add D_{i+1}[j-1]*X
		//	if required
		if(specialCase != 1)
		{
			//add D_{i+1}[j-1]*X
			// select the signal to add, depending on the value of Dip1
			mpfr_set_z(mpTmp, svXMultDip1[svDip1.get_si()+maxDigit].get_mpz_t(), GMP_RNDN);
			//	scale XMultDip1 appropriately, by the amount given by lsbDiMX
			mpfr_mul_2si(mpTmp, mpTmp, lsbDiMX, GMP_RNDN);
			mpfr_add(mpSum, mpSum, mpTmp, GMP_RNDN);
		}

		//multiply by radix
		mpfr_mul_si(mpSum, mpSum, radix, GMP_RNDN);

		//scale the result back to an integer
		mpfr_mul_2si(mpSum, mpSum, -lsbInt, GMP_RNDN);

		//round the result
		mpfr_get_z(svW_nextRd.get_mpz_t(), mpSum, GMP_RNDD);
		mpfr_get_z(svW_nextRu.get_mpz_t(), mpSum, GMP_RNDU);

		//handle the signed outputs
		if(svW_nextRd < 0)
			svW_nextRd += big1out;
		if(svW_nextRu < 0)
			svW_nextRu += big1out;

		//only use the required bits
		svW_nextRd &= (big1out-1);
		svW_nextRu &= (big1out-1);

		// complete the TestCase with this expected output
		tc->addExpectedOutput("Wi_next", svW_nextRd);
		tc->addExpectedOutput("Wi_next", svW_nextRu);

		//cleanup
		mpfr_clears(mpW_next, mpSum, mpQi, mpTmp, (mpfr_ptr)nullptr);
	}

	OperatorPtr GenericComputationUnit::parseArguments(Target *target, std::vector<std::string> &args) {
		int radix, index, maxDigit;
		int msbW, lsbW, msbX, lsbX, msbD, lsbD;
		string qi;
		int specialCase;

		UserInterface::parseInt(args, "radix", &radix);
		UserInterface::parseInt(args, "index", &index);
		UserInterface::parseInt(args, "maxDigit", &maxDigit);
		UserInterface::parseInt(args, "msbW", &msbW);
		UserInterface::parseInt(args, "lsbW", &lsbW);
		UserInterface::parseInt(args, "msbX", &msbX);
		UserInterface::parseInt(args, "lsbX", &lsbX);
		UserInterface::parseInt(args, "msbD", &msbD);
		UserInterface::parseInt(args, "lsbD", &lsbD);
		UserInterface::parseString(args, "q_i", &qi);
		UserInterface::parseInt(args, "specialCase", &specialCase);

		Signal *W  = new Signal("W", Signal::wire, true, msbW, lsbW);
		Signal *X  = new Signal("X", Signal::wire, true, msbX, lsbX);
		Signal *Di = new Signal("D", Signal::wire, true, msbD, lsbD);

		return new GenericComputationUnit(target, radix, index, maxDigit, W, X, Di, qi, specialCase);
	}

	void GenericComputationUnit::registerFactory(){
		UserInterface::add("GenericComputationUnit", // name
				"Generic computation unit for the E-method.", //description
				"FunctionApproximation", // category
				"",
				"radix(int): the radix of the digit set being used;\
				 index(int): the index of the unit;\
				 maxDigit(int): the maximum digit in the used digit set;\
				 msbW(int): MSB of the W input signal;\
				 lsbW(int): LSB of the W input signal;\
				 msbX(int): MSB of the X input signal;\
				 lsbX(int): LSB of the X input signal;\
				 msbD(int): MSB of the D input signals;\
				 lsbD(int): LSB of the D input signals;\
				 q_i(string): the q_i constant, given in arbitrary-precision decimal, or as a Sollya expression, e.g \"log(2)\";\
				 specialCase(int): flag indicating special cases"
				"",
				"",
				GenericComputationUnit::parseArguments,
				GenericComputationUnit::unitTest
		) ;

	}

	TestList GenericComputationUnit::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		return testStateList;
	}

} /* namespace flopoco */
