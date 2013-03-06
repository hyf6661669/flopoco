/*
  Euclidean division by 3
   
  The input is given as x_{n}x_{n-1}...x_{0}, but the dividend is actually
  x_{n}00x_{n-1}00...x_{1}00x_{0}

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Matei Istoan

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2013.
  All rights reserved.

*/

// TODOs: remove from d its powers of two .

#include <iostream>
#include <fstream>
#include <sstream>

#include "IntConstDiv3.hpp"


using namespace std;


namespace flopoco{

	IntConstDiv3::EuclideanDiv3Table::EuclideanDiv3Table(Target* target, int d_, int alpha_, int gamma_, int delta_) :
		//Table(target, alpha_+gamma_, alpha_+gamma_, 0, -1, 1), d(d_), alpha(alpha_), gamma(gamma_)
		/* input on alpha+gamma bits: alpha from the number, gamma from the pervious remainder */
		/* computations on alpha+gamma+nbZeros bits */
		/* output on alpha+gamma+nbZeros-1 bits (the -1 should be right for all the sizes that we are working with -- could also be -intlog2(d_)+1 )*/
		Table(target, alpha_+gamma_, alpha_+gamma_+delta_-1+gamma_, 0, -1, 1), d(d_), alpha(alpha_), gamma(gamma_), delta(delta_)
	{
		ostringstream name;
		
		srcFileName = "IntConstDiv3::EuclideanDiv3Table";
		name << "EuclideanDiv3Table_" << d << "_" << alpha << "_" << delta << "zeros" ;
		setName(name.str());
	};


	mpz_class IntConstDiv3::EuclideanDiv3Table::function(int x)
	{
		// machine integer arithmetic should be safe here
		//	getting r_{1}r_{2}x_{i} as input, actually will work with r_{1}r_{2}x_{i}00
		if((x < 0) || (x >= (1<<(1+gamma+2))))
		{
			ostringstream e;
			e << "ERROR in IntConstDiv3::EuclideanDiv3Table::function, argument out of range" <<endl;
			throw e.str();
		}
		
		// shift the input two positions to the left, to get the actual input
		int q = (x << delta) / d;
		int r = (x << delta) - q*d;
		
		return mpz_class((q<<gamma) + r);
	};
		
	

	int IntConstDiv3::quotientSize()
	{
		return qSize;
	};

	int IntConstDiv3::remainderSize()
	{
		return gamma;
	};



	IntConstDiv3::IntConstDiv3(Target* target, int wIn_, int d_, int alpha_, int nbZeros_,  bool remainderOnly_, map<string, double> inputDelays)
		: Operator(target), wIn(wIn_), d(d_), alpha(alpha_), nbZeros(nbZeros_), remainderOnly(remainderOnly_)
	{
		setCopyrightString("F. de Dinechin, Matei Istoan (2013)");
		srcFileName="IntConstDiv3";

		//set gamma to the size of the remainder
		gamma = intlog2(d-1);
		
		//check alpha, and set it properly if necessary
		if(alpha == -1)
		{
			int count;
			
			alpha = target->lutInputs() - gamma;
			count = 0;
			while(alpha > 0)
			{
				alpha -= (1+nbZeros);
				count++;
			}
			if(alpha != 0)
				count--;
			alpha = count;
						
			if (alpha<1)
			{
				REPORT(LIST, "WARNING: This value of d is too large for the LUTs of this FPGA (alpha=" << alpha << ").");
				REPORT(LIST, " Building an architecture nevertheless, but it may be very large and inefficient.");
				alpha=1;
			}
		}
		REPORT(INFO, "alpha=" << alpha);

		// Generate a unique name
		std::ostringstream o;
		if(remainderOnly)
			o << "IntConstRem3_";
		else
			o << "IntConstDiv3_";
		o << d << "_" << vhdlize(wIn) << "_"  << vhdlize(alpha) << "_" << vhdlize(nbZeros) << "zeros_";
		if(target->isPipelined()) 
			o << vhdlize(target->frequencyMHz());
		else
			o << "comb";
		uniqueName_ = o.str();

		//set the quotient size
		qSize = (nbZeros + 1)*wIn - intlog2(d) + 1;

		//create the input
		addInput("X", wIn);

		//create the output for the quotient (if necessary) and the remainder
		if(!remainderOnly)
			addOutput("Q", qSize);
		addOutput("R", gamma);

		int k = wIn/alpha;
		int rem = (nbZeros + 1)*wIn - k*(alpha + nbZeros);
		if(rem != 0)
			k++;

		REPORT(INFO, "Architecture consists of k=" << k  <<  " levels.");
		REPORT(DEBUG, "  d=" << d << "  wIn=" << wIn << "  alpha=" << alpha << "  gamma=" << gamma <<  "  k=" << k  <<  "  qSize=" << qSize);
		
		EuclideanDiv3Table* table;
		table = new EuclideanDiv3Table(target, d, alpha, gamma, nbZeros);
		useSoftRAM(table);
		oplist.push_back(table);
		double tableDelay = table->getOutputDelay("Y");

		string ri, xi, ini, outi, qi;
		
		ri = join("r", k);
		vhdl << tab << declare(ri, gamma) << " <= " << zg(gamma, 0) << ";" << endl;

		setCriticalPath(getMaxInputDelays(inputDelays));

		for (int i=k-1; i>=0; i--)
		{
			manageCriticalPath(tableDelay);

			xi = join("x", i);
			vhdl << tab << declare(xi, alpha, true) << " <= " << "X" << (alpha==1 ? of(i) : range((i+1)*alpha-1, i*alpha)) << ";" << endl; 
			
			ini = join("in", i);
			// This ri is r_{i+1}
			vhdl << tab << declare(ini, alpha+gamma) << " <= " << ri << " & " << xi << ";" << endl;
			
			outi = join("out", i);
			
			inPortMap(table, "X", ini);
			outPortMap(table, "Y", outi);
			vhdl << instance(table, join("table",i));
			
			ri = join("r", i);
			qi = join("q", i);
			vhdl << tab << declare(qi, alpha+gamma+nbZeros-1, true) << " <= " << outi << range(alpha+2*gamma+nbZeros-1, gamma) << ";" << endl;
			vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl;
		}


		if(!remainderOnly)
		{
			// build the quotient output
			vhdl << tab << declare("tempQ", k*alpha) << " <= ";
			for (unsigned int i=k-1; i>=1; i--) 
				vhdl << "q" << i << " & ";
			vhdl << "q0 ;" << endl;
				vhdl << tab << "Q <= tempQ" << range(qSize-1, 0)  << ";" << endl;
		}

		// This ri is r_0
		vhdl << tab << "R <= " << ri << ";" << endl;
	}	

	IntConstDiv3::~IntConstDiv3()
	{
	}



	void IntConstDiv3::emulate(TestCase * tc)
	{
		// Get I/O values
		mpz_class X = tc->getInputValue("X");
		
		// Recreate the true input value (x_{n}00x_{n-1}00...x_{1}00x_{0})
		int shiftSize = 0;
		mpz_class copyX = X, trueX = 0, digit, shiftedDigit;
		
		cout << "computing the actual value of X=" << X << endl;
		
		while(copyX > 0)
		{
			cout << "iteration " << shiftSize << " X=" << X << " trueX=" << trueX << " copyX=" << copyX;
			
			digit = copyX & 1;
			shiftedDigit = digit<<((int)intpow2(shiftSize)-1);
			
			cout << " digit=" << digit << " shifted digit=" << shiftedDigit << endl;
			
			trueX = trueX + digit<<(3*shiftSize-1);		//continue here -> make the new number in the correct format!!!
			
			copyX = copyX >> 1;
			shiftSize++;
		}
		
		// Compute correct value
		mpz_class Q = trueX / d;
		mpz_class R = trueX - Q*d;
		
		// Add the output values
		if(!remainderOnly)
			tc->addExpectedOutput("Q", Q);
		tc->addExpectedOutput("R", R);
	}
 

}