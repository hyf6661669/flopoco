/*
 * FixEMethodEvaluatorTrunc.cpp
 *
 *  Created on: 18 Sep 2018
 *      Author: mistoan
 */

#include "FixEMethodEvaluatorTrunc.hpp"

namespace flopoco {

	FixEMethodEvaluatorTrunc::FixEMethodEvaluatorTrunc(Target* target, size_t _radix, size_t _maxDigit, int _msbInOut, int _lsbInOut,
		vector<string> _coeffsP, vector<string> _coeffsQ,
		double _delta, bool _scaleInput, double _inputScaleFactor,
		map<string, double> inputDelays)
	: Operator(target), radix(_radix), maxDigit(_maxDigit),
	  	  n(_coeffsP.size()), m(_coeffsQ.size()),
	  	  msbInOut(_msbInOut), lsbInOut(_lsbInOut),
		  coeffsP(_coeffsP), coeffsQ(_coeffsQ),
		  delta(_delta), scaleInput(_scaleInput), inputScaleFactor(_inputScaleFactor),
		  maxDegree(n>m ? n : m)
	{
		ostringstream name;

		srcFileName = "FixEMethodEvaluatorTrunc";
		name << "FixEMethodEvaluatorTrunc_n_" << n << "_m_" << m
				<< "_msbInOut_" << vhdlize(msbInOut) << "_lsbInOut_" << vhdlize(lsbInOut);
		setName(name.str()+"_uid"+vhdlize(getNewUId()));

		useNumericStd_Signed();

		setCopyrightString("Matei Istoan, 2018");

		//safety checks and warnings
		if(n != m)
			REPORT(INFO, "WARNING: degree of numerator and of denominator are different! "
					<< "This will lead to a less efficient implementation.");
#if RADIX8plusSUPPORT==0
		if((radix != 2) && (radix != 4) && (radix != 8))
			THROWERROR("radixes higher than 8 currently not supported!");
#endif
		if(maxDigit < (int)(radix-1))
			REPORT(INFO, "WARNING: used digit set is not maximal!");
		if(maxDigit > (int)(radix-1))
			THROWERROR("maximum digit larger than the maximum digit in the redundant digit set!");
		if((delta<0 || delta>1))
			THROWERROR("delta must be in the interval [0, 1)!");

		//create a copy of the coefficients of P and Q
		copyVectors();

		REPORT(DEBUG, "coefficients of P, as mp numbers:");
		for(size_t i=0; i<n; i++)
		{
			double tmpD = mpfr_get_d(mpCoeffsP[i], GMP_RNDN);
			REPORT(DEBUG, "  " << tmpD);
		}
		REPORT(DEBUG, "coefficients of Q, as mp numbers:");
		for(size_t i=0; i<m; i++)
		{
			double tmpD = mpfr_get_d(mpCoeffsQ[i], GMP_RNDN);
			REPORT(DEBUG, "  " << tmpD);
		}

		//compute the parameters of the algorithm
		REPORT(DEBUG, "compute the parameters of the algorithm");
		setAlgorithmParameters();

		//checks to ensure algorithm correctness
		//	check P's coefficients
		REPORT(DEBUG, "checking the coefficients of P");
		checkPCoeffs();
		//	check Q's coefficients
		REPORT(DEBUG, "checking the coefficients of Q");
		checkQCoeffs();
		//	check the ranges on the input
		REPORT(DEBUG, "checking X");
		checkX();

		//compute the number of iterations needed
		nbIter = msbInOut - lsbInOut + 1;
		//	adjust the number of iterations according to the radix
		if(radix > 2)
			nbIter = ceil(1.0*nbIter/log2(radix));
		//add an additional number of iterations to compensate for the errors
		//g = intlog2(nbIter);
		g = 0;

		//set the format of the internal signals
		REPORT(DEBUG, "set the format of the internal signals");
		//	W^Hat
		GenericSimpleSelectionFunction::getWHatFormat(radix, maxDigit, &msbWHat, &lsbWHat);
		dWHat  = new Signal("dWHat", Signal::wire, true, msbWHat, lsbWHat);
		//	W
		msbW = msbWHat;
		lsbW = lsbInOut - g;
		dW   = new Signal("dW", Signal::wire, true, msbW, lsbW);
		//	compute the datapath sizes for the truncated W paths
		setWDatapathLengths();
		//	D
		msbD = ceil(log2(radix));
		lsbD = 0;
		dD   = new Signal("dD", Signal::wire, true, msbD, lsbD);
		//	X
		msbX = msbInOut;
		lsbX = lsbInOut;
		dX   = new Signal("dX", Signal::wire, true, msbX, lsbX);
		// DiMultX
		msbDiMX = msbX + (int)ceil(log2(maxDigit));
		lsbDiMX = lsbX;
		dDiMX   = new Signal("dDiMX", Signal::wire, true, msbDiMX, lsbDiMX);

//		//--------- pipelining
//		setCriticalPath(getMaxInputDelays(inputDelays));
//		//--------- pipelining

		//add the inputs
		addFixInput("X", true, msbInOut, lsbInOut);
		//add the outputs
		addFixOutput("Y", true, msbInOut, lsbInOut, 2);

		//scale the input X by the factor delta, if necessary
		if(scaleInput)
		{
			int xScaleSize;

			//a helper signal
			vhdl << tab << declare("X_std_lv", msbX-lsbX+1) << " <= std_logic_vector(X);" << endl;

			//scale the input
			//	multiply by inputScaleFactor
			//		set by default to 1/2*alpha, if not otherwise specified by the user
			FixRealKCM *scaleMult = new FixRealKCM(
												 	 target,							//target
													 true,								//signed input
													 msbX,								//msbIn
													 lsbX,								//lsbIn
													 lsbX-g,							//lsbOut
													 std::to_string(inputScaleFactor),	//the constant
													 1.0								//target ulp error
												 	 );
			addSubComponent(scaleMult);
			inPortMap (scaleMult, "X", "X_std_lv");
			outPortMap(scaleMult, "R", "X_scaled_int");
			vhdl << tab << instance(scaleMult, "ScaleConstMult");

//			//--------- pipelining
//			syncCycleFromSignal("X_scaled_int", true);
//			//--------- pipelining

			//extend the result of the multiplication to the size of the original signal, if necessary
			//	inputScaleFactor<1, so the scaled signal's msb <= original signal's msb
			xScaleSize = getSignalByName("X_scaled_int")->width();
			if(xScaleSize < (msbX-lsbX+1+g))
			{
				//extension required
				ostringstream xScaleExtension;

//				//--------- pipelining
//				manageCriticalPath(target->localWireDelay(msbX-lsbX+1), true);
//				syncCycleFromSignal("X_scaled_int", true);
//				//--------- pipelining

				for(int i=xScaleSize; i<(msbX-lsbX+1+g); i++)
					xScaleExtension << "X_scaled_int(" << xScaleSize-1 << ") & ";
				vhdl << tab << declare("X_scaled", msbX-lsbX+1+g) << " <= "
						<< xScaleExtension.str() << "X_scaled_int;" << endl;
			}
			else
			{
				vhdl << tab << declare("X_scaled", msbX-lsbX+g+1) << " <= X_scaled_int;" << endl;
			}

//			//--------- pipelining
//			syncCycleFromSignal("X_scaled", true);
//			//--------- pipelining

			//update the size of the signals based on X
			//	X
			msbX = msbInOut;
			lsbX = lsbInOut-g;
			dX   = new Signal("dX", Signal::wire, true, msbX, lsbX);
			// DiMultX
			msbDiMX = msbX + (int)ceil(log2(maxDigit));
			lsbDiMX = lsbX;
			dDiMX   = new Signal("dDiMX", Signal::wire, true, msbDiMX, lsbDiMX);
		}
		else
		{
			//no scaling required
			//	just copy the input to an intermediate signal
			vhdl << tab << declare("X_scaled", msbX-lsbX+1) << " <= std_logic_vector(X);" << endl;
		}

		//a helper signal
		vhdl << tab << declare("X_scaled_std_lv", msbX-lsbX+1) << " <= X_scaled;" << endl;
		//a helper signal
		vhdl << tab << declareFixPoint("X_scaled_signed", true, msbX, lsbX) << " <= signed(X_scaled);" << endl;

		//create the DiMX signals
		REPORT(DEBUG, "create the DiMX signals");
		addComment(" ---- create the DiMX signals ----", tab);
		//	the multipliers by constants
		IntConstMult *dimxMult[maxDigit+1];

//		//--------- pipelining
//		currentCycle = getCurrentCycle();
//		currentCriticalPath = getCriticalPath();
//		//--------- pipelining

		//multiply by the positive constants
		REPORT(DEBUG, "multiply by the positive constants");
		addComment(" ---- multiply by the positive constants ----", tab);
		for(int i=0; i<=maxDigit; i++)
		{
			//create the multiplication between X and the constant given by i
			REPORT(DEBUG, "create the multiplication between X and the constant " << i);
			if(i == 0)
			{
//				//--------- pipelining
//				manageCriticalPath(target->localWireDelay(), true);
//				//--------- pipelining

				vhdl << tab << declareFixPoint(join("X_Mult_", i, "_std_lv"), true, msbDiMX, lsbDiMX)
						<< " <= " << zg(msbDiMX-lsbDiMX+1) << ";" << endl;
			}
			else
			{
//				//--------- pipelining
//				setCycle(currentCycle);
//				setCriticalPath(currentCriticalPath);
//				//--------- pipelining

				dimxMult[i] = new IntConstMult(
												target,					//target
												msbX-lsbX+1,		 	//size of X
												mpz_class(i)			//the constant
												);
				addSubComponent(dimxMult[i]);
				inPortMap  (dimxMult[i], "X", "X_scaled_std_lv");
				outPortMap (dimxMult[i], "R", join("X_Mult_", i, "_std_lv"));
				vhdl << tab << instance(dimxMult[i], join("ConstMult_", i));
			}

			vhdl << tab << declareFixPoint(join("X_Mult_", i, "_int"), true,
					getSignalByName(join("X_Mult_", i, "_std_lv"))->width()+lsbX-1, lsbX)
					<< " <= signed(X_Mult_" << i << "_std_lv);" << endl;
			resizeFixPoint(join("X_Mult_", i), join("X_Mult_", i, "_int"), msbDiMX, lsbDiMX, 1);
		}
		//multiply by the negative constants
		REPORT(DEBUG, "multiply by the negative constants");
		addComment(" ---- multiply by the negative constants ----", tab);
		for(int i=1; i<=(int)maxDigit; i++)
		{
//			//--------- pipelining
//			setCycleFromSignal(join("X_Mult_", vhdlize(i)), true);
//			manageCriticalPath(target->adderDelay(msbDiMX-lsbDiMX+1)+target->localWireDelay(), true);
//			//--------- pipelining

			REPORT(DEBUG, "create the multiplication between X and the constant " << -i);
			vhdl << tab << declareFixPoint(join("X_Mult_", vhdlize(-i)), true, msbDiMX, lsbDiMX)
					<< " <= -X_Mult_" << i << ";" << endl;
		}

//		//--------- pipelining
//		manageCriticalPath(target->localWireDelay(msbW-lsbW+1), true);
//		//--------- pipelining

		//create the computation units
		REPORT(DEBUG, "create the computation units");
		GenericComputationUnitTrunc *cu0, *cuI[maxDegree-2], *cuN;

		//target->setPipelined(false);

		//compute unit 0
		REPORT(DEBUG, "create the computation unit 0");
		cu0 = new GenericComputationUnitTrunc(
										target,			//target
										radix, 			//radix
										maxDigit,		//maximum digit
										0,				//index
										-1,				//special case
										dWTrunc[0],		//signal W
										dX,				//signal X
										dD, 			//signal Di
										coeffsQ[0]		//constant q_i
										);
		addSubComponent(cu0);
		//compute units 1 - n-1
		for(size_t i=1; i<=(maxDegree-2); i++)
		{
			//compute unit i
			REPORT(DEBUG, "create the computation unit " << i);
			cuI[i-1] = new GenericComputationUnitTrunc(
												target,			//target
												radix, 			//radix
												maxDigit, 		//maximum digit
												i,				//index
												0,				//special case
												dWTrunc[i],		//signal W
												dX,				//signal X
												dD, 			//signal Di
												coeffsQ[i]		//constant q_i
												);
			addSubComponent(cuI[i-1]);
		}
		//compute unit n
		REPORT(DEBUG, "create the computation unit n");
		cuN = new GenericComputationUnitTrunc(
										target,					//target
										radix, 					//radix
										maxDigit, 				//maximum digit
										maxDegree-1,			//index
										+1,						//special case
										dWTrunc[maxDegree-1],	//signal W
										dX,						//signal X
										dD, 					//signal Di
										coeffsQ[maxDegree-1]	//constant q_i
										);
		addSubComponent(cuN);

		//create the selection units
		REPORT(DEBUG, "create the selection units");
		GenericSimpleSelectionFunction *sel[maxDegree];

		for(size_t i=0; i<maxDegree; i++)
		{
			//selection unit i
			REPORT(DEBUG, "create selection unit " << i);
			sel[i] = new GenericSimpleSelectionFunction(
					target,			//target
					radix,	 		//radix
					maxDigit, 		//maximum digit
					dWTrunc[i]		//signal W[i]
			);
			addSubComponent(sel[i]);
		}

		//target->setPipelined(true);

		//iteration 0
		//	initialize the elements of the residual vector
		addComment(" ---- iteration 0 ----", tab);
		REPORT(DEBUG, "iteration 0");
		for(size_t i=0; i<maxDegree; i++)
		{
			vhdl << tab << declareFixPoint(join("W_0_", i), true, msbW, lsbW) << " <= "
					<< signedFixPointNumber(mpCoeffsP[i], msbW, lsbW, 0) << ";" << endl;
			vhdl << tab << declareFixPoint(join("D_0_", i), true, msbD, lsbD) << " <= "
					<< zg(msbD-lsbD+1, 0) << ";" << endl;
		}

		//iteration 1
		//	the elements of the residual vector are the ones at the previous iteration
		//	shifted by log2(radix) positions to the left
		if(nbIter >= 1)
		{
			addComment(" ---- iteration 1 ----", tab);
			REPORT(DEBUG, "iteration 1");
			for(size_t i=0; i<maxDegree; i++)
			{
				//create the residual vector
				mpfr_t mpTmp;

				mpfr_init2(mpTmp, LARGEPREC);
				mpfr_mul_ui(mpTmp, mpCoeffsP[i], radix, GMP_RNDN);

//				//--------- pipelining
//				manageCriticalPath(target->localWireDelay(msbW-lsbW+1), true);
//				//--------- pipelining

				vhdl << tab << declareFixPoint(join("W_1_", i), true, msbW, lsbW) << " <= "
						<< signedFixPointNumber(mpTmp, msbW, lsbW, 0) << ";" << endl;

				//create the selection unit
				vhdl << tab << declareFixPoint(join("D_1_", i), true, msbD, lsbD) << " <= "
						<< signedFixPointNumber(mpTmp, msbD, lsbD, 0) << ";" << endl;

				mpfr_clear(mpTmp);
			}
		}


		//iteration 2
		//	the elements of the residual vector and the select digits can almost be pre-computed
		//	except for the operations including X
		if(nbIter >= 2)
		{
			addComment(" ---- iteration 2 ----", tab);
			REPORT(DEBUG, "iteration 2");

			//create the residual vector
			mpfr_t mpTmp, mpSum, w_i, d_0, d_i, d_ip1;
			mpz_class sv_d_0, sv_d_i, sv_d_ip1;

			//initialize the MPFR variables
			mpfr_inits2(LARGEPREC, mpTmp, mpSum, w_i, d_0, d_i, d_ip1, (mpfr_ptr)nullptr);

			for(size_t i=0; i<maxDegree; i++)
			{
				//create w_i^{j-1} = radix*p[i]
				mpfr_mul_ui(w_i, mpCoeffsP[i], radix, GMP_RNDN);

				//create d_i^{j-1} = select(radix*w_i^{j-1}) = select(w_i))
				//	round to the closest integer
				mpfr_get_z(sv_d_i.get_mpz_t(), w_i, GMP_RNDN);
				//	save it back in the MPFR version of the variable
				mpfr_set_z(d_i, sv_d_i.get_mpz_t(), GMP_RNDN);

				//create d_0^{j-1} = select(radix*d_0^{j-1}) = select(radix*p[0])
				mpfr_mul_ui(d_0, mpCoeffsP[0], radix, GMP_RNDN);
				//	round to the closest integer
				mpfr_get_z(sv_d_0.get_mpz_t(), d_0, GMP_RNDN);
				//	save it back in the MPFR version of the variable
				mpfr_set_z(d_0, sv_d_0.get_mpz_t(), GMP_RNDN);

				if(i < (maxDegree-1))
				{
					//create d_{i+1}^{j-1} = select(radix*w_{i+1}^{j-1}) = select(w_{i+1}))
					mpfr_mul_ui(d_ip1, mpCoeffsP[i+1], radix, GMP_RNDN);
					//	round to the closest integer
					mpfr_get_z(sv_d_ip1.get_mpz_t(), d_ip1, GMP_RNDN);
					//	save it back in the MPFR version of the variable
					mpfr_set_z(d_ip1, sv_d_ip1.get_mpz_t(), GMP_RNDN);
				}

				//create the sum
				//	w_i^{j-1} - q_i*d_0^{j-1} - d_i^{j-1}
				mpfr_set(mpSum, w_i, GMP_RNDN);
				if(i > 0)
				{
					mpfr_mul(mpTmp, d_0, mpCoeffsQ[i], GMP_RNDN);
					mpfr_sub(mpSum, mpSum, mpTmp, GMP_RNDN);
				}
				mpfr_sub(mpSum, mpSum, d_i, GMP_RNDN);

//				//--------- pipelining
//				manageCriticalPath(target->localWireDelay(msbW-lsbW+1), true);
//				//--------- pipelining

				//create the signal for the sum
				vhdl << tab << declareFixPoint(join("sum_2_", i), true, msbW, lsbW) << " <= "
						<< signedFixPointNumber(mpSum, msbW, lsbW, 0) << ";" << endl;

				//create the signal for x*d_{i+1}^{(1)}
				if(i < (maxDegree-1))
				{
					resizeFixPoint(join("sum_2_", i, "_term2"),
							join("X_Mult_", vhdlize(sv_d_ip1.get_d())), msbW, lsbW, 1);
				}

//				//--------- pipelining
//				manageCriticalPath(target->adderDelay(msbW-lsbW+1), true);
//				//--------- pipelining

				//create w_i^{(2)}
				if(i < (maxDegree-1))
				{
					vhdl << tab << declareFixPoint(join("W_2_", i, "_int"), true, msbW, lsbW) << " <= "
							<< "sum_2_" << i << " + "
							<< "sum_2_" << i << "_term2"
							<< ";" << endl;
				}
				vhdl << tab << declareFixPoint(join("W_2_", i), true, dWTrunc[i]->MSB(), dWTrunc[i]->LSB()) << " <= ";
				if(i < (maxDegree-1))
				{
					vhdl << "W_2_" << i << "_int";
				}
				else
				{
					vhdl << "sum_2_" << i;
				}
				if(dWTrunc[i]->LSB() > lsbW)
				{
					vhdl << range(dWTrunc[i]->MSB()-lsbW-ceil(log2(radix)), dWTrunc[i]->LSB()-lsbW-ceil(log2(radix)))
							<< ";" << endl;
				}else{
					vhdl << range(dWTrunc[i]->MSB()-dWTrunc[i]->LSB()-ceil(log2(radix)), 0)
							<< " & " << zg(ceil(log2(radix))) << ";" << endl;
				}

				//create the selection unit
				//inputs
				inPortMap(sel[i],  "W", join("W_2_", i));
				//outputs
				outPortMap(sel[i], "D", join("D_2_", i));
				//the instance
				vhdl << tab << instance(sel[i], join("SEL_2_", i));
			}

			mpfr_clears(mpTmp, mpSum, w_i, d_0, d_i, d_ip1, (mpfr_ptr)nullptr);
		}

//		//--------- pipelining
//		nextCycle(true);
//		//--------- pipelining

		//iterations 3 to nbIter
		REPORT(DEBUG, "iterations 3 to nbIter");
		for(size_t iter=3; iter<=nbIter; iter++)
		{
			REPORT(DEBUG, "iteration " << iter);
			addComment(join(" ---- iteration ", iter, " ----"), tab);

//			//--------- pipelining
//			if(iter > 2)
//			{
//				for(size_t i=0; i<maxDegree; i++)
//				{
//					//after iteration nbIter-m, we can stop generating some of the SELs
//					if((i > nbIter-iter) && (iter > nbIter-maxDegree))
//						continue;
//
//					syncCycleFromSignal(join("W_", iter-1, "_0"), true);
//					syncCycleFromSignal(join("D_", iter-1, "_0"), true);
//				}
//				setCriticalPath(sel->getOutDelayMap()["D"]);
//			}
//			//--------- pipelining

			//create computation unit index 0
			REPORT(DEBUG, "create computation unit index 0");
			//	a special case
			//inputs
			inPortMap(cu0, "Wi",   join("W_", iter-1, "_0"));
			inPortMap(cu0, "D0",   join("D_", iter-1, "_0"));
			inPortMap(cu0, "Di",   join("D_", iter-1, "_0"));
			inPortMap(cu0, "Dip1", join("D_", iter-1, "_1"));
			inPortMap(cu0, "X",    "X_scaled_signed");
			for(int i=(-(int)maxDigit); i<=(int)maxDigit; i++)
			{
				inPortMap(cu0, join("X_Mult_", vhdlize(i)), join("X_Mult_", vhdlize(i)));
			}
			//outputs
			outPortMap(cu0, "Wi_next", join("W_", iter, "_0"));
			//the instance
			vhdl << tab << instance(cu0, join("CU_", iter, "_0"));

			//create computation units index 1 to maxDegree-2
			REPORT(DEBUG, "create computation units index 1 to maxDegree-2");
			for(size_t i=1; i<=(maxDegree-2); i++)
			{
				//after iteration nbIter-m, we can stop generating some of the CUs
				if((i > nbIter-iter) && (iter > nbIter-maxDegree))
					break;

				REPORT(DEBUG, "create computation unit index " << i);
				//inputs
				inPortMap(cuI[i-1], "Wi",   join("W_", iter-1, "_", i));
				inPortMap(cuI[i-1], "D0",   join("D_", iter-1, "_0"));
				inPortMap(cuI[i-1], "Di",   join("D_", iter-1, "_", i));
				inPortMap(cuI[i-1], "Dip1", join("D_", iter-1, "_", i+1));
				inPortMap(cuI[i-1], "X",    "X_scaled_signed");
				for(int j=(-(int)maxDigit); j<=(int)maxDigit; j++)
				{
					inPortMap(cuI[i-1], join("X_Mult_", vhdlize(j)), join("X_Mult_", vhdlize(j)));
				}
				//outputs
				outPortMap(cuI[i-1], "Wi_next", join("W_", iter, "_", i));
				//the instance
				vhdl << tab << instance(cuI[i-1], join("CU_", iter, "_", i));
			}

			//after iteration nbIter-m, this CU is no longer needed
			if(iter <= (nbIter-maxDegree+1))
			{
				//create computation unit index maxDegree
				REPORT(DEBUG, "create computation unit index maxDegree-1");
				//	a special case
				//inputs
				inPortMap(cuN, "Wi",   join("W_", iter-1, "_", maxDegree-1));
				inPortMap(cuN, "D0",   join("D_", iter-1, "_0"));
				inPortMap(cuN, "Di",   join("D_", iter-1, "_", maxDegree-1));
				inPortMap(cuN, "X",    "X_scaled_signed");
				//outputs
				outPortMap(cuN, "Wi_next", join("W_", iter, "_", maxDegree-1));
				//the instance
				vhdl << tab << instance(cuN, join("CU_", iter-1, "_", maxDegree-1));
			}

			//create the selection units index 0 to maxDegree-1
			REPORT(DEBUG, "create the selection units index 0 to maxDegree-1");
			for(size_t i=0; i<maxDegree; i++)
			{
				//after iteration nbIter-m, we can stop generating some of the SELs
				if((i > nbIter-iter) && (iter > nbIter-maxDegree))
					continue;

				REPORT(DEBUG, "create the selection unit " << i);

//				//--------- pipelining
//				setCycleFromSignal(join("W_", iter, "_", i), true);
//				setCriticalPath(cu0->getOutDelayMap()["Wi_next"]);
//				//--------- pipelining

				//inputs
				inPortMap(sel[i],  "W", join("W_", iter, "_", i));
				//outputs
				outPortMap(sel[i], "D", join("D_", iter, "_", i));
				//the instance
				vhdl << tab << instance(sel[i], join("SEL_", iter, "_", i));
			}

//			//--------- pipelining
//			for(size_t i=0; i<maxDegree; i++)
//			{
//				//after iteration nbIter-m, we can stop generating some of the SELs
//				if((i > nbIter-iter) && (iter > nbIter-maxDegree))
//					continue;
//
//				syncCycleFromSignal(join("D_", iter, "_", i), true);
//			}
//			setCriticalPath(sel->getOutDelayMap()["D"]);
//
//			//if((iter > 2) && (iter%2 == 0))
//			//if((iter > 2) && (iter%3 == 0))
//			//if((iter > 2) && (iter%4 == 0))
//				nextCycle(true);
//			//--------- pipelining
		}

//		//--------- pipelining
//		syncCycleFromSignal(join("W_", nbIter, "_0"), true);
//		syncCycleFromSignal(join("D_", nbIter, "_0"), true);
//		setCriticalPath(sel->getOutDelayMap()["D"]);
//		//--------- pipelining

//		//--------- pipelining
//		nextCycle(true);
//		//--------- pipelining

		//compute the final result
		REPORT(DEBUG, "compute the final result");
		BitHeap *bitheap = new BitHeap(
										this,											// parent operator
										msbW-lsbW+1,									// maximum weight
										false, 											// enable supertiles
//										join("Bitheap_"+name.str()+"_", getNewUId())	// bitheap name
										""
										);
		//add the digits of the intermediate computations
		REPORT(DEBUG, "add the digits of the intermediate computations");
		for(int i=(nbIter-1); i>0; i--)
		{
			/*
			REPORT(DEBUG, "adding D_" << i << "_" << 0 << " at weight " << (nbIter-1-i)*ceil(log2(radix)));
			bitheap->addSignedBitVector(
										(nbIter-1-i)*ceil(log2(radix)),			//weight
										join("D_", i, "_", 0),					//input signal name
										msbD-lsbD+1								//size
										);
			*/

			REPORT(DEBUG, "adding D_" << i << "_" << 0 << " at weight " << (nbIter+g-1-i)*ceil(log2(radix)));
			bitheap->addSignedBitVector(
										(nbIter+g-1-i)*ceil(log2(radix)),		//weight
										join("D_", i, "_", 0),					//input signal name
										msbD-lsbD+1								//size
										);

		}
		//add the rounding bit
		REPORT(DEBUG, "add the rounding bit");
		//bitheap->addConstantOneBit(g-1);
		//compress the bitheap
		REPORT(DEBUG, "compress the bitheap");
		bitheap->generateCompressorVHDL();

//		//--------- pipelining
//		syncCycleFromSignal(bitheap->getSumName(), true);
//		//--------- pipelining

		//retrieve the bits we want from the bit heap
		REPORT(DEBUG, "retrieve the bits from the bit heap");
		vhdl << tab << declareFixPoint("sum", true, msbW, lsbW) << " <= signed(" <<
				bitheap->getSumName() << range(msbW-lsbW, 0) << ");" << endl;

		//write the result to the output
		REPORT(DEBUG, "write the result to the output, only the bits that we want");
		vhdl << tab << "Y <= sum" << range(msbInOut-lsbInOut+g, g) << ";" << endl;

		REPORT(DEBUG, "constructor completed");
	}


	FixEMethodEvaluatorTrunc::~FixEMethodEvaluatorTrunc()
	{
		for(size_t i=0; i<maxDegree; i++)
		{
			mpfr_clear(mpCoeffsP[i]);
			mpfr_clear(mpCoeffsQ[i]);
		}

		mpfr_clears(mpDelta, mpAlpha, mpXi, mpInputScaleFactor, mpXLimit, mpWPathError, (mpfr_ptr)nullptr);
	}


	void FixEMethodEvaluatorTrunc::copyVectors()
	{
		size_t iterLimit;

		iterLimit = coeffsP.size();
		//copy the coefficients of P
		for(size_t i=0; i<iterLimit; i++)
		{
			//create a copy as MPFR
			mpfr_init2(mpCoeffsP[i], LARGEPREC);
			//	parse the constant using Sollya
			sollya_obj_t node;
			node = sollya_lib_parse_string(coeffsP[i].c_str());
			/* If  parse error throw an exception */
			if (sollya_lib_obj_is_error(node))
			{
				THROWERROR("emulate: Unable to parse string "<< coeffsP[i] << " as a numeric constant");
			}
			sollya_lib_get_constant(mpCoeffsP[i], node);
			free(node);
		}
		//fill with zeros, if necessary
		for(size_t i=iterLimit; i<maxDegree; i++)
		{
			//add a zero as string
			coeffsP.push_back(string("0"));

			//create a copy as MPFR
			mpfr_init2(mpCoeffsP[i], LARGEPREC);
			mpfr_set_zero(mpCoeffsP[i], 0);
		}

		iterLimit = coeffsQ.size();
		//copy the coefficients of Q
		for(size_t i=0; i<iterLimit; i++)
		{
			//create a copy as MPFR
			mpfr_init2(mpCoeffsQ[i], LARGEPREC);
			//	parse the constant using Sollya
			sollya_obj_t node;
			node = sollya_lib_parse_string(coeffsQ[i].c_str());
			/* If  parse error throw an exception */
			if (sollya_lib_obj_is_error(node))
			{
				THROWERROR("emulate: Unable to parse string "<< coeffsQ[i] << " as a numeric constant");
			}
			sollya_lib_get_constant(mpCoeffsQ[i], node);
			free(node);
		}
		//fill with zeros, if necessary
		for(size_t i=iterLimit; i<maxDegree; i++)
		{
			//add a zero as string
			coeffsQ.push_back(string("0"));

			//create a copy as MPFR
			mpfr_init2(mpCoeffsQ[i], LARGEPREC);
			mpfr_set_zero(mpCoeffsQ[i], 0);
		}
	}


	void FixEMethodEvaluatorTrunc::setAlgorithmParameters()
	{
		mpfr_t mpTmp;

		mpfr_inits2(LARGEPREC, mpTmp, mpDelta, mpAlpha, mpXi, mpInputScaleFactor, (mpfr_ptr)nullptr);

		//set delta
		mpfr_set_d(mpDelta, delta, GMP_RNDN);
		//set xi
		//	generic case
		xi    = 0.5  * (1.0+delta);
		//	multi-precision version
		mpfr_set(mpXi, mpDelta, GMP_RNDN);
		mpfr_add_ui(mpXi, mpXi, 1, GMP_RNDN);
		mpfr_div_ui(mpXi, mpXi, 2, GMP_RNDN);
		//set alpha
		//	generic case
		alpha = (1.0-delta) / (2*radix);
		//	multi-precision version
		mpfr_set_ui(mpAlpha, 1, GMP_RNDN);
		mpfr_sub(mpAlpha, mpAlpha, mpDelta, GMP_RNDN);
		mpfr_div_ui(mpAlpha, mpAlpha, 2, GMP_RNDN);
		mpfr_div_ui(mpAlpha, mpAlpha, radix, GMP_RNDN);
		//set the scale factor - compute the value of the scale factor in two cases:
		//	1) when scaling is required, and it hasn't been set by the user
		//	2) when scaling isn't required, but the scale factor is needed for the analysis of the parameters
		if(((scaleInput == true) && (inputScaleFactor == -1)) || (scaleInput == false))
		{
			//if the scale factor needs to be computed,
			//	then set if to the maximum admissible limit
			//	we need to first determine the maximum value of the coefficients of Q,
			//	and then subtract it from alpha
			mpfr_set_zero(mpInputScaleFactor, 0);
			for(size_t i=1; i<m; i++)
			{
				mpfr_abs(mpTmp, mpCoeffsQ[i], GMP_RNDN);
				if(mpfr_cmp(mpTmp, mpInputScaleFactor) > 0)
					mpfr_set(mpInputScaleFactor, mpTmp, GMP_RNDN);
			}

			mpfr_sub(mpInputScaleFactor, mpAlpha, mpInputScaleFactor, GMP_RNDN);
			inputScaleFactor = mpfr_get_d(mpInputScaleFactor, GMP_RNDN);
		}

		mpfr_clear(mpTmp);
	}


	void FixEMethodEvaluatorTrunc::checkPCoeffs()
	{
		//check that the coefficients of P are smaller than xi
		for(size_t i=0; i<maxDegree; i++)
		{
			if(mpfr_cmp(mpCoeffsP[i], mpXi) > 0)
				THROWERROR("checkPCoeff: coefficient coeffsP[" << i << "]=" << coeffsP[i]
					<< " does not satisfy the constraints");
		}
	}


	void FixEMethodEvaluatorTrunc::checkQCoeffs()
	{
		mpfr_t mpLimit, mpTmp;

		mpfr_inits2(LARGEPREC, mpLimit, mpTmp, (mpfr_ptr)nullptr);
		//set the maximum value allowed for the coefficients
		mpfr_sub(mpLimit, mpAlpha, mpInputScaleFactor, GMP_RNDN);

		//check that coeffsQ[0]=1
		if(mpfr_cmp_ui(mpCoeffsQ[0], 1) != 0)
			THROWERROR("checkQCoeff: coefficient coeffsQ[0]=" << coeffsQ[0]
				<< " should be 1");
		//check that the coefficients of Q are smaller than the limit
		for(size_t i=1; i<maxDegree; i++)
		{
			mpfr_abs(mpTmp, mpCoeffsQ[i], GMP_RNDN);
			if(mpfr_cmp(mpTmp, mpLimit) > 0)
				THROWERROR("checkQCoeff: coefficient coeffsQ[" << i << "]=" << coeffsQ[i]
					<< " does not satisfy the constraints");
		}

		mpfr_clears(mpLimit, mpTmp, (mpfr_ptr)nullptr);
	}


	void FixEMethodEvaluatorTrunc::checkX()
	{
		mpfr_t mpX, mpTmp;

		mpfr_inits2(LARGEPREC, mpXLimit, mpX, mpTmp, (mpfr_ptr)nullptr);

		//compute the maximum value of Q's coefficients
		mpfr_set_zero(mpXLimit, 0);
		for(size_t i=1; i<m; i++)
		{
			mpfr_abs(mpTmp, mpCoeffsQ[i], GMP_RNDN);
			if(mpfr_cmp(mpTmp, mpXLimit) > 0)
				mpfr_set(mpXLimit, mpTmp, GMP_RNDN);
		}
		mpfr_sub(mpXLimit, mpAlpha, mpXLimit, GMP_RNDN);

		//check that the largest value that X can take is smaller than the limit
		//	largest value X can take
		mpfr_set_ui(mpX, 1, GMP_RNDN);
		mpfr_mul_2si(mpX, mpX, msbInOut, GMP_RNDN);
		//	need to subtract 1 ulp
		mpfr_set_ui(mpTmp, 1, GMP_RNDN);
		mpfr_mul_2si(mpTmp, mpTmp, lsbInOut, GMP_RNDN);
		//	get the actual largest value X can take
		mpfr_sub(mpX, mpX, mpTmp, GMP_RNDN);
		//scale the input
		mpfr_mul(mpX, mpX, mpInputScaleFactor, GMP_RNDN);

		//now perform the test
		if(mpfr_cmp(mpX, mpXLimit) > 0)
			THROWERROR("checkX: input format for X, with msb=" << msbInOut
					<< " and lsb=" << lsbInOut << " does not satisfy the constraints");

		mpfr_clears(mpX, mpTmp, (mpfr_ptr)nullptr);
	}


	void FixEMethodEvaluatorTrunc::setWDatapathLengths()
	{
		mpfr_t mpTmp, mpErr;
		int errorBound;
		double errorBoundD;
		double dTmp;

		//int gW = 0;
		int gW = 21;
		//int gW = 21;
		//int gW = 30;

		//initialize the working variables
		mpfr_inits2(LARGEPREC, mpTmp, mpErr, (mpfr_ptr)nullptr);

		//for each of the maxDegree columns, compute the slack and then create
		//	a new signal which contains the specific details for each of the datapath lengths

		//column 0
		mpfr_set_zero(mpErr, 0);
		mpfr_set_zero(mpTmp, 0);
		//	compute the slack
		//		slack_i = 1/2 - 2^k(bounded by delta) - radix*x_limit
		mpfr_set_d(mpErr, 0.5, GMP_RNDN);
		dTmp = mpfr_get_d(mpErr, GMP_RNDN);
		mpfr_sub(mpErr, mpErr, mpDelta, GMP_RNDN);
		dTmp = mpfr_get_d(mpDelta, GMP_RNDN);
		dTmp = mpfr_get_d(mpErr, GMP_RNDN);
		//		tmp = radix*x_limit
		mpfr_mul_ui(mpTmp, mpXLimit, radix, GMP_RNDN);
		dTmp = mpfr_get_d(mpTmp, GMP_RNDN);
		mpfr_sub(mpErr, mpErr, mpTmp, GMP_RNDN);
		dTmp = mpfr_get_d(mpErr, GMP_RNDN);
		//	the errorBound_i is ceil(log2(slack))
		if(mpfr_sgn(mpErr) > 0)
		{
			mpfr_log2(mpTmp, mpErr, GMP_RNDN);
			errorBoundD = mpfr_get_d(mpTmp, GMP_RNDN);
			errorBound = mpfr_get_si(mpTmp, GMP_RNDN);
		}else{
			errorBound = lsbW;
		}
		//	create the w_0 dummy signal
		//dWTrunc.push_back(new Signal("dW_0", Signal::wire, true, msbW, maxInt(2, errorBound, lsbW)));
		dWTrunc.push_back(new Signal("dW_0", Signal::wire, true, msbW, maxInt(2, errorBound-gW, lsbW)));
		//dWTrunc.push_back(new Signal("dW_0", Signal::wire, true, msbW, lsbW));

		//columns 1 to n-1
		for(size_t i=1; i<maxDegree-1; i++)
		{
			mpfr_set_zero(mpErr, 0);
			mpfr_set_zero(mpTmp, 0);

			//compute the slack
			//	slack_i = 1/2 - 2^k(bounded by delta) - radix*q_i - radix*x_limit
			mpfr_set_d(mpErr, 0.5, GMP_RNDN);
			mpfr_sub(mpErr, mpErr, mpDelta, GMP_RNDN);
			//	tmp = radix*q_i
			mpfr_mul_ui(mpTmp, mpCoeffsQ[i], radix, GMP_RNDN);
			mpfr_abs(mpTmp, mpTmp, GMP_RNDN);
			mpfr_sub(mpErr, mpErr, mpTmp, GMP_RNDN);
			//	tmp = radix*x_limit
			mpfr_mul_ui(mpTmp, mpXLimit, radix, GMP_RNDN);
			mpfr_sub(mpErr, mpErr, mpTmp, GMP_RNDN);

			//the errorBound_i is ceil(log2(slack))
			if(mpfr_sgn(mpErr) > 0)
			{
				mpfr_log2(mpTmp, mpErr, GMP_RNDN);
				errorBound = mpfr_get_si(mpTmp, GMP_RNDN);
			}else{
				errorBound = lsbW;
			}

			//create the w_i dummy signal
			//dWTrunc.push_back(new Signal(join("dW_", i), Signal::wire, true, msbW, maxInt(2, errorBound, lsbW)));
			dWTrunc.push_back(new Signal(join("dW_", i), Signal::wire, true, msbW, maxInt(2, errorBound-gW, lsbW)));
			//dWTrunc.push_back(new Signal(join("dW_", i), Signal::wire, true, msbW, lsbW));
		}

		//column n
		mpfr_set_zero(mpErr, 0);
		mpfr_set_zero(mpTmp, 0);
		//	compute the slack
		//		slack_i = 1/2 - 2^k(bounded by delta) - radix*q_i
		mpfr_set_d(mpErr, 0.5, GMP_RNDN);
		mpfr_sub(mpErr, mpErr, mpDelta, GMP_RNDN);
		//		tmp = radix*q_n
		mpfr_mul_ui(mpTmp, mpCoeffsQ[maxDegree-1], radix, GMP_RNDN);
		mpfr_abs(mpTmp, mpTmp, GMP_RNDN);
		mpfr_sub(mpErr, mpErr, mpTmp, GMP_RNDN);
		//	the errorBound_i is ceil(log2(slack))
		if(mpfr_sgn(mpErr) > 0)
		{
			mpfr_log2(mpTmp, mpErr, GMP_RNDN);
			errorBound = mpfr_get_si(mpTmp, GMP_RNDU);
		}else{
			errorBound = lsbW;
		}
		//	create the w_n dummy signal
		//dWTrunc.push_back(new Signal("dW_n", Signal::wire, true, msbW, maxInt(2, errorBound, lsbW)));
		dWTrunc.push_back(new Signal("dW_n", Signal::wire, true, msbW, maxInt(2, errorBound-gW, lsbW)));
		//dWTrunc.push_back(new Signal("dW_n", Signal::wire, true, msbW, lsbW));

		//cleanup
		mpfr_clears(mpTmp, mpErr, (mpfr_ptr)nullptr);
	}


	int FixEMethodEvaluatorTrunc::simulateWDatapathLengths(int nbTests)
	{
		bool simPass, iterPass;
		int guardBits;
		mpz_class a, svY;
		//manage signed digits
		mpz_class big1X  = (mpz_class(1) << (msbInOut-lsbInOut+1));
		mpz_class big1Xp = (mpz_class(1) << (msbInOut-lsbInOut));
		//manage a multi-precision input
		mpfr_t mpX, mpP, mpQ, mpY, mpTmp;

		//initialize the internal state of the simulation variables
		for(size_t i=0; i<maxDegree; i++)
		{
			mpfr_inits2(LARGEPREC, wiSim[i], diSim[i], (mpfr_ptr)nullptr);
		}
		mpfr_inits2(LARGEPREC, xSim, ySim, (mpfr_ptr)nullptr);

		//initialize the MPFR variables
		mpfr_inits2(LARGEPREC, mpX, mpP, mpQ, mpY, mpTmp, (mpfr_ptr)nullptr);

		//initialize the simulation parameters
		simPass = false;
		guardBits = 0;
		//start the iterations
		while(!simPass)
		{
			//set the flag
			iterPass = true;

			//create nbTests testcases to check if the current number of guard
			//	bits is sufficient
			for(size_t i=0; i<nbTests; i++)
			{
				//initialize the input to the iterations
				a = getLargeRandom(dX->width());
				//manage signed inputs
				if(a >= big1Xp)
					a -= big1X;
				mpfr_init2(mpX, LARGEPREC);
				//set the input value
				mpfr_set_z(mpX, a.get_mpz_t(), GMP_RNDN);
				//scale X appropriately, by the amount given by lsbInOut
				mpfr_mul_2si(mpX, mpX, lsbInOut, GMP_RNDN);
				//scale the input, if needed
				if(scaleInput == true)
				{
					mpfr_mul_d(mpX, mpX, inputScaleFactor, GMP_RNDN);
				}

				//initialize the internal state
				for(size_t i=0; i<maxDegree; i++)
				{
					mpfr_set_zero(diSim[i], GMP_RNDN);
					mpfr_set(wiSim[i], mpCoeffsP[i], GMP_RNDN);
				}
				mpfr_set_zero(ySim, GMP_RNDN);

				//simulate nbTests iterations of the E-method algorithm
				for(size_t j=0; j<nbIter; j++)
				{
					simulateIteration(guardBits);
				}

				//create the output
				mpfr_set_zero(ySim, GMP_RNDN);
				for(size_t j=0; j<nbIter; j++)
				{
					mpfr_mul_2si(mpTmp, diSim[j], -j, GMP_RNDN);
					mpfr_add(ySim, ySim, mpTmp, GMP_RNDN);
				}

				//extract the useful bits
				mpfr_mul_2si(ySim, ySim, -lsbInOut+msbInOut, GMP_RNDN);
				//	round the result
				mpfr_get_z(svY.get_mpz_t(), ySim, GMP_RNDD);
				//	store the result back
				mpfr_set_z(ySim, svY.get_mpz_t(), GMP_RNDN);
				//	scale the result back
				mpfr_div_2si(ySim, ySim, -lsbInOut+msbInOut, GMP_RNDN);

				//create the golden reference
				//	initialize P and Q
				mpfr_set_zero(mpP, 0);
				mpfr_set_zero(mpQ, 0);
				//	compute P
				for(int j=0; j<(int)n; j++)
				{
					//compute X^i
					mpfr_pow_si(mpTmp, mpX, j, GMP_RNDN);
					//multiply by coeffsP[i]
					mpfr_mul(mpTmp, mpTmp, mpCoeffsP[j], GMP_RNDN);
					//add the new term to the sum
					mpfr_add(mpP, mpP, mpTmp, GMP_RNDN);
				}
				//	compute Q
				for(int j=0; j<(int)m; j++)
				{
					//compute X^i
					mpfr_pow_si(mpTmp, mpX, j, GMP_RNDN);
					//multiply by coeffsQ[i]
					mpfr_mul(mpTmp, mpTmp, mpCoeffsQ[j], GMP_RNDN);
					//add the new term to the sum
					mpfr_add(mpQ, mpQ, mpTmp, GMP_RNDN);
				}
				//	compute Y = P/Q
				mpfr_div(mpY, mpP, mpQ, GMP_RNDN);

				//extract the useful bits
				mpfr_mul_2si(mpY, mpY, -lsbInOut+msbInOut, GMP_RNDN);
				//	round the result
				mpfr_get_z(svY.get_mpz_t(), mpY, GMP_RNDD);
				//	store the result back
				mpfr_set_z(mpY, svY.get_mpz_t(), GMP_RNDN);
				//	scale the result back
				mpfr_div_2si(mpY, mpY, -lsbInOut+msbInOut, GMP_RNDN);

				//compare the results
				if(mpfr_cmp(ySim, mpY) != 0)
				{
					iterPass = false;
					break;
				}
			}

			//check if all testcases passed
			if(iterPass == true)
			{
				simPass = true;
			}else
			{
				guardBits++;
			}
		}

		//MPFR cleanup
		mpfr_clears(mpX, mpP, mpQ, mpY, mpTmp, (mpfr_ptr)nullptr);
		for(size_t i=0; i<maxDegree; i++)
		{
			mpfr_clears(wiSim[i], diSim[i], (mpfr_ptr)nullptr);
		}
		mpfr_clears(xSim, ySim, (mpfr_ptr)nullptr);


		return guardBits;
	}


	void FixEMethodEvaluatorTrunc::simulateIteration(int guardBits)
	{
		mpfr_t mpTmp, mpResult, d0;
		mpz_class svTmp, svResult;

		//initialize the internal variables
		mpfr_inits2(LARGEPREC, mpTmp, mpResult, d0, (mpfr_ptr)nullptr);

		//save the value of d[0]^{j-1}, as it will be overwritten in the following iterations
		mpfr_set(d0, diSim[0], GMP_RNDN);

		//component 0
		//	initialize the working variables
		mpfr_set_zero(mpTmp, 0);
		mpfr_set_zero(mpResult, 0);
		//	create w[0]^{j} = r * [ w[0]^{j-1} - d[0]^{j-1} + d[1]^{j-1}*x ]
		mpfr_set(mpResult, wiSim[0], GMP_RNDN);
		mpfr_sub(mpResult, mpResult, d0, GMP_RNDN);
		mpfr_mul(mpTmp, diSim[1], xSim, GMP_RNDN);
		mpfr_add(mpResult, mpResult, mpTmp, GMP_RNDN);
		mpfr_mul_ui(mpResult, mpResult, radix, GMP_RNDN);
		//	save the result in wiSim[0]
		mpfr_set(wiSim[0], mpResult, GMP_RNDN);
		//	create d[0]^j = select( w[0]^{j} )
		mpfr_get_z(svResult.get_mpz_t(), wiSim[0], GMP_RNDN);
		mpfr_set_z(diSim[0], svResult.get_mpz_t(), GMP_RNDN);

		//component 1 to n-2
		for(size_t i=0; i<maxDegree; i++)
		{

		}

		//component n-1
		//	initialize the working variables
		mpfr_set_zero(mpTmp, 0);
		mpfr_set_zero(mpResult, 0);
		//	create w[n-1]^{j} = r * [ w[n-1]^{j-1} - d[n-1]^{j-1} - q[n-1]^{j-1}*d[0]^{j-1} ]
		mpfr_set(mpResult, wiSim[maxDegree-1], GMP_RNDN);
		mpfr_sub(mpResult, mpResult, diSim[maxDegree-1], GMP_RNDN);
		mpfr_mul(mpTmp, mpCoeffsQ[maxDegree-1], d0, GMP_RNDN);
		mpfr_sub(mpResult, mpResult, mpTmp, GMP_RNDN);
		mpfr_mul_ui(mpResult, mpResult, radix, GMP_RNDN);
		//	save the result in wiSim[maxDegree-1]
		mpfr_set(wiSim[maxDegree-1], mpResult, GMP_RNDN);
		//	create d[maxDegree-1]^j = select( w[maxDegree-1]^{j} )
		mpfr_get_z(svResult.get_mpz_t(), wiSim[maxDegree-1], GMP_RNDN);
		mpfr_set_z(diSim[maxDegree-1], svResult.get_mpz_t(), GMP_RNDN);

		//MPFR cleanup
		mpfr_inits2(LARGEPREC, mpTmp, mpResult, d0, (mpfr_ptr)nullptr);
	}


	void FixEMethodEvaluatorTrunc::emulate(TestCase * tc)
	{
		double dTmp;

		//get the inputs from the TestCase
		mpz_class svX   = tc->getInputValue("X");

		//manage signed digits
		mpz_class big1X      = (mpz_class(1) << (msbInOut-lsbInOut+1));
		mpz_class big1Xp     = (mpz_class(1) << (msbInOut-lsbInOut));

		//handle the signed inputs
		if(svX >= big1Xp)
			svX -= big1X;

		// compute the multiple-precision output
		mpz_class svYd, svYu;
		mpfr_t mpX, mpP, mpQ, mpTmp, mpY;

		//initialize the variables
		mpfr_inits2(LARGEPREC, mpX, mpP, mpQ, mpTmp, mpY, (mpfr_ptr)nullptr);

		//initialize P and Q
		mpfr_set_zero(mpP, 0);
		mpfr_set_zero(mpQ, 0);

		//initialize X
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN);
		dTmp = mpfr_get_d(mpX, GMP_RNDN);
		//	scale X appropriately, by the amount given by lsbInOut
		mpfr_mul_2si(mpX, mpX, lsbInOut, GMP_RNDN);
		dTmp = mpfr_get_d(mpX, GMP_RNDN);

		//if required, scale the input
		if(scaleInput == true)
		{
			mpfr_mul_d(mpX, mpX, inputScaleFactor, GMP_RNDN);
			dTmp = mpfr_get_d(mpX, GMP_RNDN);
		}

		//compute P
		for(int i=0; i<(int)n; i++)
		{
			//compute X^i
			mpfr_pow_si(mpTmp, mpX, i, GMP_RNDN);
			//multiply by coeffsP[i]
			mpfr_mul(mpTmp, mpTmp, mpCoeffsP[i], GMP_RNDN);

			//add the new term to the sum
			mpfr_add(mpP, mpP, mpTmp, GMP_RNDN);
 		}
		dTmp = mpfr_get_d(mpP, GMP_RNDN);

		//compute Q
		for(int i=0; i<(int)m; i++)
		{
			//compute X^i
			mpfr_pow_si(mpTmp, mpX, i, GMP_RNDN);
			//multiply by coeffsQ[i]
			mpfr_mul(mpTmp, mpTmp, mpCoeffsQ[i], GMP_RNDN);

			//add the new term to the sum
			mpfr_add(mpQ, mpQ, mpTmp, GMP_RNDN);
		}
		dTmp = mpfr_get_d(mpQ, GMP_RNDN);

		//compute Y = P/Q
		mpfr_div(mpY, mpP, mpQ, GMP_RNDN);
		dTmp = mpfr_get_d(mpY, GMP_RNDN);

		//scale the result back to an integer
		mpfr_mul_2si(mpY, mpY, -lsbInOut+msbInOut, GMP_RNDN);

		//round the result
		mpfr_get_z(svYd.get_mpz_t(), mpY, GMP_RNDD);
		mpfr_get_z(svYu.get_mpz_t(), mpY, GMP_RNDU);

		//handle the signed outputs
		if(svYd < 0)
			svYd += big1X;
		if(svYu < 0)
			svYu += big1X;

		//only use the required bits
		svYd &= (big1X-1);
		svYu &= (big1X-1);

		//add this expected output to the TestCase
		tc->addExpectedOutput("Y", svYd);
		tc->addExpectedOutput("Y", svYu);

		//cleanup
		mpfr_clears(mpX, mpP, mpQ, mpTmp, mpY, (mpfr_ptr)nullptr);
	}

	OperatorPtr FixEMethodEvaluatorTrunc::parseArguments(Target *target, std::vector<std::string> &args) {
		int radix;
		int maxDigit;
		int msbIn;
		int lsbIn;
		vector<string> coeffsP;
		vector<string> coeffsQ;
		double delta;
		bool scaleInput;
		double inputScaleFactor;
		string in, in2;

		UserInterface::parseStrictlyPositiveInt(args, "radix", &radix);
		UserInterface::parseStrictlyPositiveInt(args, "maxDigit", &maxDigit);
		UserInterface::parseInt(args, "msbIn", &msbIn);
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseString(args, "coeffsP", &in);
		UserInterface::parseString(args, "coeffsQ", &in2);
		UserInterface::parseFloat(args, "delta", &delta);
		UserInterface::parseBoolean(args, "scaleInput", &scaleInput);
		UserInterface::parseFloat(args, "inputScaleFactor", &inputScaleFactor);

		stringstream ss(in);
		string substr;
		while(std::getline(ss, substr, ':'))
		{
			coeffsP.insert(coeffsP.begin(), std::string(substr));
			//coeffsP.push_back(std::string(substr));
		}

		stringstream ss2(in2);
		while(std::getline(ss2, substr, ':'))
		{
			coeffsQ.insert(coeffsQ.begin(), std::string(substr));
			//coeffsQ.push_back(std::string(substr));
		}

		OperatorPtr result = new FixEMethodEvaluatorTrunc(target, radix, maxDigit, msbIn, lsbIn,
				coeffsP, coeffsQ, delta, scaleInput, inputScaleFactor);

		return result;
	}

	void FixEMethodEvaluatorTrunc::registerFactory(){
		UserInterface::add("FixEMethodEvaluatorTrunc", // name
				"A truncated hardware implementation of the E-method for the evaluation of polynomials and rational polynomials.", //description
				"FunctionApproximation", // category
				"",
				"radix(int): the radix of the digit set being used;\
				 maxDigit(int): the maximum digit in the redundant digit set;\
				 msbIn(int): MSB of the input;\
				 lsbIn(int): LSB of the input;\
				 coeffsP(string): colon-separated list of real coefficients of polynomial P, using Sollya syntax. Example: coeff=\"1.234567890123:sin(3*pi/8)\";\
				 coeffsQ(string): colon-separated list of real coefficients of polynomial Q, using Sollya syntax. Example: coeff=\"1.234567890123:sin(3*pi/8)\";\
				 delta(real)=0.5: the value for the delta parameter in the E-method algorithm;\
				 scaleInput(bool)=false: flag showing if the input is to be scaled by the factor delta;\
				 inputScaleFactor(real)=-1: the factor by which the input is scaled"
				"",
				"",
				FixEMethodEvaluatorTrunc::parseArguments,
				FixEMethodEvaluatorTrunc::unitTest
		) ;

	}

	TestList FixEMethodEvaluatorTrunc::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;



		return testStateList;
	}

} /* namespace flopoco */
