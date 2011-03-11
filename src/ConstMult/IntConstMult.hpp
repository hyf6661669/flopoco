#ifndef INTCONSTMULT_HPP
#define INTCONSTMULT_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "../Operator.hpp"
#include "ShiftAddOp.hpp"
#include "ShiftAddDag.hpp"
#include "../IntAdder.hpp"

/**
	Integer constant multiplication.

	See also ShiftAddOp, ShiftAddDag
	ShiftAddOp defines a shift-and-add operation for IntConstMult.

	ShiftAddDag deals with the implementation of an IntConstMult as a
	vector of ShiftAddOp. It defines the intermediate variables with
	their bit sizes and provide methods for evaluating the cost of an
	implementation.

*/


namespace flopoco{

	class IntConstMult : public Operator
	{
	public:
		IntConstMult(Target* target, int xsize, mpz_class n);
		~IntConstMult();

		mpz_class n;  /**< The constant */ 
		int xsize;   
		int rsize;   
		ShiftAddDag* implementation;

#if 0
		int* bits;
		int* BoothCode;
		int nonZeroInBoothCode;
#endif



		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);
		void buildStandardTestCases(TestCaseList* tcl);

		/** Recodes input n; returns the number of non-zero bits */
		int recodeBooth(mpz_class n, int* BoothCode);

		// void buildMultBooth();      /**< Build a rectangular (low area, long latency) implementation */
		ShiftAddOp* buildMultBoothTree(mpz_class n);  /**< Build a balanced tree implementation as per the ASAP 2008 paper */ 

		/** Build an optimal tree for rational constants
		 Parameters are such that n = headerSize + (2^i + 2^j)periodSize */ 
		void buildTreeForRational(mpz_class header, mpz_class period, int headerSize, int periodSize, int i, int j);  

	private:
		void build_pipeline(ShiftAddOp* sao, double& delay);
		string printBoothCode(int* BoothCode, int size);
		void showShiftAddDag();
		void optimizeLefevre(const vector<mpz_class>& constants);

	};
}
#endif
