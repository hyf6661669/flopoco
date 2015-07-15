#include <gmpxx.h>
#include "SelFunctionTable.hpp"
#include <math.h>

using namespace std;
namespace flopoco
{
	SelFunctionTable::SelFunctionTable(Target* target, float dmin, float dmax, int nbd, int nbw, int digit, int base, int wIn, int wOut) : Table(target, wIn, wOut),
																														Dmin(dmin),
																														Dmax(dmax),
																														nbBitD(nbd),
																														nbBitW(nbw),
																														digitSet(digit),
																														radix(base)
	{
		ro = ((float)digitSet)/(radix-1);
		ostringstream name;
		srcFileName="SelFunctionTable";
		name << "SelFunctionTable";
		setName(name.str());
	}

	SelFunctionTable::~SelFunctionTable() {}

	mpz_class SelFunctionTable::function(int x)
	{
		mpz_class result;

		//TODO : Parameterized the table

        int w = (x>>nbBitD);	 //separating w and d (together in x)
        int d = x - (w<<nbBitD);
        int wneg = (w >> (nbBitW-1)) << (nbBitW-1); //getting the sign bit
        w -= 2*wneg; //switching from two's complement to decimal representation, more convenient for upcoming computation

		int decimalResult;
		int nbBitK = ceil(log2(digitSet)+1); //Nb of bit for the entire part of w
		float realw = w/pow(2, nbBitW-nbBitK);

		float hPitch = pow(2, -nbBitD)*(Dmax-Dmin);
		float vPitch = pow(2, -nbBitW+nbBitK);

		float uSlope, lSlope;
		int uCorrecter, lCorrecter;


		for(int k = -digitSet; k <= digitSet; k++)
		{
			uSlope = k+ro;
			lSlope = k-ro;

			uCorrecter = (uSlope<0 ? 1 : 0);
			lCorrecter = (lSlope<0 ? 0 : 1);

			float wMax = ((d+uCorrecter)*hPitch + Dmin) * uSlope;
			float wMin = ((d+lCorrecter)*hPitch + Dmin) * lSlope;

			if((realw+vPitch <= wMax && realw >= wMin) || (k == digitSet && realw >= wMin) || (k == -digitSet && realw+vPitch <= wMax))
			{
				decimalResult = k;
				break;
			}
		}

		int qa = 0;
		if(qa % 2 == 1)
			qa = 1;
		if(decimalResult == 6 || decimalResult == 10)
			qa = 2;
		if((decimalResult < 0 && decimalResult != -7) || decimalResult == 7)
			qa += 4;

		int qb = 0;
		if(decimalResult == 2 || decimalResult == 3 || decimalResult == -2 || decimalResult == -3)
			qb = 1;
		else if((decimalResult >= 4 && decimalResult == 6) || (decimalResult <= -4 && decimalResult >= -6))
			qb = 2;
		else if(decimalResult >= 7 || decimalResult <= -7)
			qb = 3;
		if(decimalResult < 0)
			qb += 4;

		result = mpz_class((qa << 3) + qb);


//		if(decimalResult < 0)
//		{
//			decimalResult+=(pow(2, nbBitK)); //switch to two's complement
//			if(decimalResult != 9 && radix > 4)
//				decimalResult+=pow(2, nbBitK); //qa is negative
//		}
//		else if(decimalResult == 7)
//		{
//			decimalResult+=pow(2, nbBitK); //qa is negative
//		}
//
//		result = mpz_class(decimalResult);
		return result;
	}
}
