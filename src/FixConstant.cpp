#include <iostream>
#include <sstream>
#include "FixConstant.hpp"
#include "utils.hpp"

using namespace std;

namespace flopoco{


	FixConstant::FixConstant(const int MSB_, const int LSB_, const bool isSigned_, const mpfr_t val_) : 
		MSB(MSB_), LSB(LSB_), width(MSB_-LSB_+1), isSigned(isSigned_) {
		mpfr_init2(fpValue, width);
		mpfr_set(fpValue, val_, GMP_RNDN); // TODO check no error?
	}




#if 0 // Warning the following code is unfinished and untested
	FixConstant::FixConstant(const bool signed_, const mpfr_t val_) : 
		isSigned(signed_) {

		bool sign;
		mpz_class zval;
		LSB =  mpfr_get_z_exp (zval.get_mpz_t(),val_);

		// conversion to (sign, absval)
		if(isSigned) {
			if(zval<0) {
				sign=true; 
				zval=-zval;
			}
			else 
				sign=false;
		}
		else {// unsigned
			if(zval<0) {
				throw("Signed input to FixConstant(unsigned)");
			}
		}

		// Normalisation?
		cout << "Normalisation";
		while(zval & mpz_class(1) == 0) {
			cout << ".";
			zval = zval>>1;
			LSB++;
		}
		MSB=0;
		while(zval!=0) {
			cout << "*";
			zval = zval>>1;
			MSB++;			
		}

		if(isSigned)
			MSB++;

		mpfr_init2(fpValue, width);
		mpfr_set(fpValue, val_, GMP_RNDN); // TODO check no error?
	}
#endif



	FixConstant::~FixConstant(){
		mpfr_clear(fpValue);
	} 
	


	std::string FixConstant::getBitVector(int margins){
		//		cout <<  "in FixConstant::getBitVector, fpValue=" << printMPFR(fpValue) << endl;
		// use the function in utils.hpp
		if(isSigned)
			return signedFixPointNumber(fpValue, MSB, LSB);
		else
			return unsignedFixPointNumber(fpValue, MSB, LSB);
	}

	bool FixConstant::isZero(){
		return mpfr_zero_p(fpValue);
	}

	void FixConstant::changeMSB(int newMSB){
		MSB=newMSB;
		if(newMSB>=MSB){
			// Nothing to do! the new size includes the old one
		}
		else{
			// TODO: check that the number fits its new size, and bork otherwise. 
		}
	}

	void FixConstant::changeLSB(int newLSB){
		throw("FixConstant::changeLSB: TODO");
	}

	void  FixConstant::addRoundBit(int weight){
		if(weight<LSB) {
			ostringstream e;
			e << "in FixConstant::addRoundBit, weight of the round bit is "<< weight << ", lower than LSB=" << LSB;
			throw e.str();
		}
		mpfr_t b,s;
		mpfr_init2(b,16);
		mpfr_init2(s,width+1);
		mpfr_set_ui_2exp(b, 1, weight, GMP_RNDN); //exact
		mpfr_add(s, fpValue, b, GMP_RNDN);
		if(mpfr_get_exp(s) != mpfr_get_exp(fpValue)) {
			//cerr << "FixConstant::addRoundBit has increased MSB";
			MSB++;
			width++;
			mpfr_set_prec(fpValue, width);
		}
		mpfr_set(fpValue, s,  GMP_RNDN); //exact
		mpfr_clears(s,b, NULL);
	}

	std::string FixConstant::report(){
		ostringstream s;
		s << "(MSB=" << MSB << ", LSB=" << LSB << ")   "<< printMPFR(fpValue);
		return s.str();
	}

	
}
