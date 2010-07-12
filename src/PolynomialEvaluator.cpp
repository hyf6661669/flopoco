/*
  A polynomial evaluator for FloPoCo
 
  Author : Bogdan Pasca
 
  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Authors:   Bogdan Pasca, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  CeCILL license, 2008-2010.
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"


#include "PolynomialEvaluator.hpp"

using namespace std;

namespace flopoco{

	extern vector<Operator*> oplist;
	
		PolynomialEvaluator::PolynomialEvaluator(Target* target, vector<FixedPointCoefficient*> coef, YVar* y, int targetPrec, mpfr_t* approxError,map<string, double> inputDelays):
		Operator(target, inputDelays), y_(y), targetPrec_(targetPrec){
		
		setCopyrightString("Bogdan Pasca (2010)");
		srcFileName = "PolynomialEvaluator";
		setName(join("PolynomialEvaluator_d",degree_));

		setApproximationError(approxError); /* set the approximation error budget we are allowed */
		setPolynomialDegree(coef.size()-1);

		/* both y and a[i], i in 0 ... d are described by two values: size and weight 
		for y, which is always positive, the two parameters are
		
		|<-- y_->getWeight() --->|
		.---------------------[0][y_{size-1} downto 0    ]
		                         |<--- y_getSize() ----->|
		
		for the coefficients
		|<-- coef[i]->getWeight() --->|
		.--------------------------[s][coef[i]->getSize()-1 downto 0 ]
		                              |<--- coef[i]_->getSize() ---->|        */
		
		updateCoefficients(coef);
		REPORT(DETAILED, "Polynomial to evaluate: " << printPolynomial(coef, y, 0));
		REPORT(DETAILED, "y size=" << y_->getSize() << " weight=" << y_->getWeight());
		
		/* I/O Signal Declarations; y and the coefficients*/
		addInput("Y", y_->getSize()); /* y is positive so we don't store the sign */

		for (uint32_t i=0; i <= unsigned(degree_); i++){
			addInput(join("a",i), coef_[i]->getSize()+1); /* the size does not contain the sign bit */
			REPORT(DETAILED, "a"<<i<<" size=" << coef_[i]->getSize() << " weight=" << coef_[i]->getWeight());
		}
		
		allocateErrorVectors();
		initializeErrorVectors();
		
		/* needed in the approximation error computation. We do it once as 
		this doesn't change during the iterations */
		setMaxYValue(y); 

		hornerGuardBitProfiler(); // sets the min values for pi truncations
				
		if (1){
		determineObjectiveStatesForTruncationsOnY();
		setNumberOfPossibleValuesForEachY();
		initializeExplorationVectors();

		mpfr_t u, *e;
		mpfr_init2(u, 100);

		int errExp;
		/* design space exploration */				
		if (degree_>1){
			
			while ((!sol) && (nextStateY())){
				reinitCoefficientGuardBits();
				/* run once the error estimation algo with these parameters */
				e = errorEstimator(yGuard_, aGuard_);
				/* test if by chance we met the error budget */
				mpfr_add( u, *approximationError, *e, GMP_RNDN);
				errExp = (mpfr_get_d(u, GMP_RNDZ)==0 ? 0 :mpfr_get_exp(u));
				if ((errExp <= -targetPrec-1)){
					/* if we do, then we set the solution true and that's it */
					sol = true;
					mpfr_clear(*e);
					free(e);
				}
			}
			/* at this point we have found a rough solution. We now try to find 
			smaller values for coefficient guard bits */
		
			sol = false; //reinit sol;
			reinitCoefficientGuardBits();
			nextStateA(); //
		
			while (((!sol) && nextStateA())){
				e = errorEstimator(yGuard_, aGuard_);
				mpfr_add( u, *approximationError, *e, GMP_RNDN);
				errExp = (mpfr_get_d(u, GMP_RNDZ)==0 ? 0 :mpfr_get_exp(u));
				if (errExp <= -targetPrec-1 ){
					sol = true;
					mpfr_clear(u);
					mpfr_clear(*e);
					free(e);
				}else{
					mpfr_clear(*e);
					free(e);
				}
			}
		
		/* at this pont we have a fine grain version of the solution */					

		}else{
			/* a 1st degree polynomial a1y+a0 doesn't need any guard bit exploration
			 for the coefficients */
			sol = false;
			while (!sol){
				while ((!sol) && (nextStateY())){
					mpfr_t* u;
					u = (mpfr_t*)malloc(sizeof(mpfr_t));
					mpfr_init2(*u, 100);
					mpfr_add( *u, *approximationError, *errorEstimator(yGuard_, aGuard_), GMP_RNDN);
					REPORT(DEBUG, " err = " << mpfr_get_exp(*u));
					int errExp = (mpfr_get_d(*u, GMP_RNDZ)==0 ? 0 :mpfr_get_exp(*u));
					if (errExp <= -targetPrec-1){
						sol = true;
					}
				}	
			}		
		}
	}

		ostringstream s1, s2;		
		s1 << "Guard bits for the sums: ";
		s2 << "Gard bits on the truncation of y : ";
		for (int j=0; j<=degree_; j++)
			s1 << "aG["<<j<<"]="<<aGuard_[j]<<" "; 

		for (int j=1; j<=degree_; j++)
			s2 << "yG["<<j<<"]="<<yGuard_[j]<<" "; 
		
		// Commented out by Florent whose laptop has a smaller screen
		//REPORT(INFO, "------------------------------------------------------------");
		REPORT(INFO, s1.str());
		REPORT(INFO, s2.str());
		//REPORT(INFO, "------------------------------------------------------------");
		setCriticalPath(getMaxInputDelays(inputDelays));

//		exit(-1);///////////////////////////////
		for (uint32_t i=0; i<=unsigned(degree_); i++){
			if (i==0){
				vhdl << tab << "-- weight of sigmaP"<<i<<" is="<<coef_[degree_-i]->getWeight()<<" size="<<1+coef_[degree_-i]->getSize()<<endl;
				vhdl << tab << declare( join("sigmaP",i), 1+coef_[degree_-i]->getSize()) << " <= a"<<degree_<<";"<<endl; 
			}else{
				vhdl << tab << "-- weight of yT"<<i<<" is="<<y_->getWeight()<<" size="<<1+y_->getSize()+yGuard_[i]<<endl;
				vhdl << tab << declare( join("yT",i) , 1+y_->getSize()+yGuard_[i]) << " <= \"0\" & Y"<<range(y_->getSize()-1, -yGuard_[i]) << ";" << endl;

				
//				cout << "sigmakPSize[i-1]( "<<i-1<<") is = " << sigmakPSize[i-1] << "yGuard_[i]="<<yGuard_[i]<< " y_->getSize()="<<y_->getSize()<<endl;
				vhdl << tab << "-- weight of piP"<<i<<" is="<<pikPWeight[i]<<" size="<<pikPSize[i]+2<<endl;

				SignedIntMultiplier* sm = new SignedIntMultiplier ( target, 1+y_->getSize()+yGuard_[i], sigmakPSize[i-1]+1, inDelayMap("X",getCriticalPath()));
				oplist.push_back(sm);
				
				inPortMap ( sm, "X", join("yT",i));
				inPortMap ( sm, "Y", join("sigmaP",i-1));
				outPortMap (sm, "R", join("piP",i));
				
				vhdl << instance ( sm, join("Product_",i) );
				syncCycleFromSignal(join("piP",i)); 
//				nextCycle();///////////////////////////////////////////////////
				
				setCriticalPath( sm->getOutputDelay("R") );
				vhdl << tab << "-- the delay at the output of the multiplier is : " << sm->getOutputDelay("R") << endl;
				
				if (i<unsigned(degree_)){
					vhdl << tab << "-- weight of piPT"<<i<<" is="<<pikPTWeight[i]<<" size="<<pikPTSize[i]+1<<endl;
					vhdl << tab << declare( join("piPT",i), pikPTSize[i]+1 ) << " <= " << join("piP",i)<<range(pikPSize[i], pikPSize[i] - pikPTSize[i] ) << ";" <<endl; // coef_[i]->getSize()+1+y_->getSize()+yGuard_[i]-1, coef_[i]->getSize()+1+y_->getSize()+yGuard_[i]-1 - pikPTSize[i]) << ";" << endl;   
					IntAdder* sa = new IntAdder (target, sigmakPSize[i]+1, inDelayMap("X",getCriticalPath()));
					oplist.push_back(sa);
				
				
					vhdl << tab << declare( join("op1_",i), sigmakPSize[i]+1 ) << " <= (" << rangeAssign(sigmakPWeight[i] - coef_[degree_-i]->getWeight()-1,0, join("a",degree_-i)+of(coef_[degree_-i]->getSize()))
						                                                               << " & " << join("a",degree_-i) << " & "<< zg(aGuard_[degree_-i],0) << ");"<<endl;
						                                                               
					vhdl << tab << declare( join("op2_",i), sigmakPSize[i]+1 ) << " <= (" << rangeAssign(sigmakPWeight[i]-pikPTWeight[i]-1,0, join("piPT",i)+of(pikPTSize[i])) 
						                                                               << " & " << join("piPT",i) << range(pikPTSize[i], pikPTSize[i] - pikPTWeight[i] - (coef_[degree_-i]->getSize()-coef_[degree_-i]->getWeight() + aGuard_[degree_ -i]))
						                                                               << " & "<< zg( - (pikPTSize[i] - pikPTWeight[i] - (coef_[degree_-i]->getSize()-coef_[degree_-i]->getWeight() + aGuard_[degree_ -i])) ,0)
						                                                               << ");" << endl;

					inPortMap ( sa, "X", join("op1_",i) );
					inPortMap ( sa, "Y", join("op2_",i) );
					inPortMapCst ( sa, "Cin", "'1'");
					outPortMap( sa, "R", join("sigmaP",i));
				
					vhdl << instance ( sa, join("Sum",i));
					syncCycleFromSignal( join("sigmaP",i) );
					setCriticalPath( sa->getOutputDelay("R") );
//					nextCycle();///////////////////////////////////////////////////                                                                   
				                                                                   
				}else{
					IntAdder* sa = new IntAdder (target, (coef_[0]->getSize()+3), inDelayMap("X",getCriticalPath()));
					oplist.push_back(sa);

					vhdl << tab << declare( join("op1_",i), sigmakPSize[i]+1 ) << " <= (" << rangeAssign(sigmakPWeight[i]-pikPWeight[i]-1,0, join("piP",i)+of(pikPSize[i]+1)) 
						                                                               << " & " << join("piP",i)<<range(pikPSize[i],0)  << " & \"0\");" << endl;

					vhdl << tab << declare( join("op2_",i), sigmakPSize[i]+1 ) << " <= (" << rangeAssign(sigmakPWeight[i]-coef_[degree_-i]->getWeight()-1,0, join("a",degree_-i)+of(coef_[degree_-i]->getSize()))
//						                                                               << " & " << join("a",degree_-i) << " & "<< zg(sigmakPSize[i]-sigmakPWeight[i]-(coef[degree_-i]->getSize()-coef[degree_-i]->getWeight())-1 ,0) << ");"<<endl;
					                                                                      << " & " << join("a",degree_-i) << " & "<< zg(sigmakPSize[i]+1 - (sigmakPWeight[i]-coef_[degree_-i]->getWeight()) - (coef_[degree_-i]->getSize()+1),0) << ");"<<endl;
					inPortMapCst ( sa, "X", join("op1_",i)+range(sigmakPSize[i], sigmakPSize[i]+1 - (coef_[0]->getSize()+3)) );
					inPortMapCst ( sa, "Y", join("op2_",i)+range(sigmakPSize[i], sigmakPSize[i]+1 - (coef_[0]->getSize()+3)) );
					inPortMapCst ( sa, "Cin", "'0'");
					outPortMap( sa, "R", join("sigmaP",i));
		
					vhdl << instance ( sa, join("Sum",i));
					syncCycleFromSignal( join("sigmaP",i) );

					wR = coef_[0]->getSize()+2; //sigmakPSize[i]+1;
					weightR = sigmakPWeight[i];
					addOutput("R", coef_[0]->getSize()+2 );//sigmakPSize[i]+1);
					setCriticalPath(sa->getOutputDelay("R"));
					vhdl << tab << "R <= " << join("sigmaP",i)<< range(coef_[0]->getSize()+2,1)<<";"<<endl; //<< << ";" << endl;
				}

				outDelayMap["R"]=getCriticalPath();			
			}		
		}
	}
	
	
	void PolynomialEvaluator::hornerGuardBitProfiler(){
		
		Sigma* sigma[degree_];
		Pi*    pi   [degree_];
		
		sigma[0] = new Sigma( coef_[degree_]->getSize(), coef_[degree_]->getWeight() , unsigned(0) , 0);

		
		for (int i=1; i<=degree_; i++){
			pi[i]     = new Pi    ( y_->getSize(), y_->getWeight(), sigma[i-1]->getSize(), sigma[i-1]->getWeight());
			sigma[i]  = new Sigma ( pi[i]->getSize(), pi[i]->getWeight(), coef_[degree_-i]->getSize(), coef_[degree_-i]->getWeight());
		}	

		for (int i=0; i<=degree_; i++){
			maxBoundA[i] = sigma[i]->getGuardBits();			
		}	
	}
	
	
	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	mpfr_t* PolynomialEvaluator::errorEstimator(vector<int> &yGuard, vector<int> &aGuard){
		ostringstream s1, s2, s3;		
		
		for (int j=0; j<=degree_; j++)
			s1 << "aG["<<j<<"]="<<aGuard[j]<<" "; 
		for (int j=1; j<=degree_; j++){
			s2 << "yG["<<j<<"]=" << yGuard[j] << " "; 
			s3 << "maxY["<<j<<"]=" << maxBoundY[j] << " "; 
		}

		REPORT(DETAILED, "------------------------------------------------------------");
		REPORT(DETAILED, s1.str());
		REPORT(DETAILED, s2.str());
		REPORT(DETAILED, s3.str());
		REPORT(DETAILED, "---------- HEADER ----------");
		
		vector<mpfr_t*> ykT_y; //the yk tild. The max absolute value of ykT
		ykT_y.push_back( NULL );

		for (uint32_t i=1; i<= unsigned(degree_); i++){
			mpfr_t *cykT;
			cykT = (mpfr_t*) malloc(sizeof(mpfr_t));
			mpfr_init2(*cykT, 100);
			if ( yGuard[i] < 0){
				mpfr_set_ui(*cykT, 2, GMP_RNDN);
				mpfr_pow_si(*cykT, *cykT, y_->getWeight()-(signed(y_->getSize())+yGuard[i]), GMP_RNDN); 
			}else
				mpfr_set_si(*cykT, 0, GMP_RNDN);
			ykT_y.push_back(cykT);
		}

		for (uint32_t i=1; i<= unsigned(degree_); i++)
			REPORT(DETAILED, "degree of |y"<<i<<"T - y|="<< (mpfr_get_d(*ykT_y[i],GMP_RNDN)!=0?mpfr_get_exp(*ykT_y[i]):0));

		vector<mpfr_t*> pikPT_pikP; //the pi k prime tild - p k prime ( trunc error)
		pikPT_pikP.push_back(NULL);
		for (uint32_t i=1; i< unsigned(degree_); i++){ //not needed of n
			mpfr_t *cpikPT_pikP;
			cpikPT_pikP = (mpfr_t*) malloc(sizeof(mpfr_t));
			mpfr_init2(*cpikPT_pikP, 100);
			
			mpfr_set_ui(*cpikPT_pikP, 2, GMP_RNDN);
			mpfr_pow_si(*cpikPT_pikP, *cpikPT_pikP, coef_[degree_-i]->getWeight()-(signed(coef_[degree_-i]->getSize())+aGuard[degree_-i]), GMP_RNDN); 
			pikPT_pikP.push_back(cpikPT_pikP);
		}
		pikPT_pikP.push_back(NULL);
		
		for (uint32_t i=1; i< unsigned(degree_); i++)
			REPORT(DETAILED, "|pi"<<i<<"TP - pi"<<i<<"T|="<< (mpfr_get_d(*pikPT_pikP[i],GMP_RNDN)!=0? mpfr_get_exp(*pikPT_pikP[i]):0));
		
		
		vector<mpfr_t*> sigmakP_sigmak;
		//initialize sigma0P_sigma0
		mpfr_t *sigmanP_sigman;
		sigmanP_sigman =(mpfr_t*) malloc( sizeof( mpfr_t));
		mpfr_init2 ( *sigmanP_sigman, 100);
		mpfr_set_ui( *sigmanP_sigman, 0, GMP_RNDN);
		sigmakP_sigmak.push_back(sigmanP_sigman);	//sigmakP_sigmak[0]

		vector<mpfr_t*> pikP_pik;
		vector<mpfr_t*> sigmakP;

		vector<mpfr_t*> a;

		mpfr_t *ak;
		for (uint32_t i=0; i<=unsigned(degree_);i++){
			ak =(mpfr_t*) malloc( sizeof( mpfr_t));
			mpfr_init2 ( *ak, 100);
			mpfr_set_ui( *ak, 2, GMP_RNDN);
			mpfr_pow_si( *ak, *ak, coef_[i]->getSize(), GMP_RNDN);
			mpfr_add_si( *ak, *ak , -1, GMP_RNDN);
			mpfr_set_exp( *ak, (mpfr_get_d(*ak, GMP_RNDZ)!=0?mpfr_get_exp(*ak):0) + coef_[i]->getWeight() - coef_[i]->getSize());
			a.push_back(ak);
		}

		for (uint32_t i=0; i< a.size(); i++)
			REPORT(DETAILED, "weight of a"<<i<<"="<< mpfr_get_exp(*a[i]));
		
		sigmakP.push_back(a[degree_]);

		mpfr_t *yy;
		yy =(mpfr_t*) malloc( sizeof( mpfr_t));
		mpfr_init2 ( *yy, 100);
		mpfr_set_ui( *yy, 2, GMP_RNDN);
		mpfr_pow_si( *yy, *yy, y_->getSize(), GMP_RNDN);
		mpfr_add_si( *yy, *yy , -1, GMP_RNDN);
		mpfr_set_exp( *yy, ( mpfr_get_d(*yy, GMP_RNDZ)!=0?mpfr_get_exp(*yy):0) + y_->getWeight() - y_->getSize());

		REPORT(DETAILED, "weight of y="<< mpfr_get_exp(*yy));

		vector<mpfr_t*> pikPT;

		sigmakPSize[0]   = coef_[degree_]->getSize();
		sigmakPWeight[0] = coef_[degree_]->getWeight();
		
		pikP_pik.push_back(NULL);
		pikPT.push_back(NULL);
		
		REPORT(DETAILED, "----------   END  ----------");
		for (uint32_t i=1; i<=unsigned(degree_); i++){
			mpfr_t *t; //the pi
			t = (mpfr_t *) malloc( sizeof( mpfr_t ));			
			mpfr_init2( *t, 100);
			mpfr_mul ( *t, *ykT_y[i], *sigmakP[i-1], GMP_RNDN);

			REPORT(DETAILED, "|(y"<<i<<"T - y)*sigma"<<i-1<<"P| weight=" << (mpfr_get_d(*t,GMP_RNDN)!=0?mpfr_get_exp(*t):0));

			pikPWeight[i]  = y_->getWeight() + sigmakPWeight[i-1]; 
			pikPSize[i]   = (y_->getSize()+yGuard[i]) + sigmakPSize[i-1];

			mpfr_t *t2;
			t2 = (mpfr_t *) malloc (sizeof (mpfr_t));
			mpfr_init2( *t2, 100);
			mpfr_mul ( *t2 , *yy , *sigmakP_sigmak[i-1], GMP_RNDN);
	
			REPORT(DETAILED, "|y*(sigma"<<i-1<<"P - sigma"<<i-1<<")| weight=" <<  (mpfr_get_d(*t2,GMP_RNDN)!=0?mpfr_get_exp(*t2):0));
			
			mpfr_add (*t, *t , *t2, GMP_RNDN);
			pikP_pik.push_back(t);
			mpfr_clear(*t2);
			free(t2);

			REPORT( DETAILED, "|pi"<<i<<"P - pi"<<i<<"| weight=" <<  (mpfr_get_d(*pikP_pik[i],GMP_RNDN)!=0?mpfr_get_exp(*pikP_pik[i]):0)); 

			
			pikPTWeight[i] =  y_->getWeight() + sigmakPWeight[i-1]; 
			pikPTSize[i] =    y_->getWeight() + sigmakPWeight[i-1]  + (coef_[degree_-i]->getSize()+aGuard[i]-coef_[degree_-i]->getWeight());
//			cerr << " +++  pikpt"<<i<< " size="<<  pikPTSize[i] << " weight = " << pikPTWeight[i] << endl;
	
//			REPORT( DEBUG, " pikPTSize="<< pikPTSize[i] << " pikPTWeight[i]="<<pikPTWeight[i]);
//			REPORT( DEBUG, "pikPTWeight[i]"<<pikPTWeight[i]<<" coef_[degree_-i]->getSize() "<< coef_[degree_-i]->getSize() << " coef_[degree_-i]->getWeight() "<<coef_[degree_-i]->getWeight() << " aGuard =" << aGuard[i] ); 

			//compute value of pi k P T
			mpfr_t *h;
			h = (mpfr_t *) malloc( sizeof(mpfr_t));
			mpfr_init2(*h, 100);
			mpfr_set_ui( *h, 2, GMP_RNDN);
			mpfr_pow_si( *h, *h, pikPTSize[i], GMP_RNDN);
			mpfr_add_si( *h, *h , -1, GMP_RNDN);
			mpfr_set_exp( *h, (mpfr_get_d(*h, GMP_RNDZ)!=0? mpfr_get_exp(*h):0) + pikPTWeight[i] - pikPTSize[i]);

			pikPT.push_back(h);
//			free(h);
			////////////////////////////////////////////////////////////////////

			mpfr_t *t3; //the sigma
			if (i < unsigned(degree_)){
				t3 = (mpfr_t *) malloc( sizeof( mpfr_t ));
				mpfr_init2( *t3, 100);
				mpfr_add( *t3, *pikPT_pikP[i], *pikP_pik[i],GMP_RNDN); 		   
			}else{
				t3 = (mpfr_t *) malloc( sizeof( mpfr_t ));
				mpfr_init2( *t3, 100);
				mpfr_set( *t3, *pikP_pik[i], GMP_RNDN); 		   
			}
			
			sigmakP_sigmak.push_back(t3);
//			free(t3);

			REPORT( DETAILED, "|sigma"<<i<<"P - sigma"<<i<<"| weight=" << (mpfr_get_d(*sigmakP_sigmak[i],GMP_RNDN)!=0?mpfr_get_exp(*sigmakP_sigmak[i]):0)); 

			if (i!=unsigned(degree_)){
				int maxMSB = max ( coef_[degree_-i]->getWeight(), pikPTWeight[i]);
				sigmakPSize[i]   = maxMSB + 1 - (coef_[degree_-i]->getWeight() - coef_[degree_-i]->getSize() - aGuard[degree_-i]);
				sigmakPWeight[i] = maxMSB + 1;
			
				REPORT( DEBUG, " sigma"<<i<<"PSize="<< sigmakPSize[i] << " sigma"<<i<<"PWeight[i]="<<sigmakPWeight[i]);
			}else{
				int maxMSB = max ( coef_[degree_-i]->getWeight(), pikPWeight[i]);
				int minLSB = min ( coef_[degree_-i]->getWeight()-coef_[degree_-i]->getSize(),pikPWeight[i]-pikPSize[i]); 
				sigmakPSize[i]   = 1 + maxMSB - minLSB + 1;//maxMSB + 1 - (coef_[degree_-i]->getWeight() - coef_[degree_-i]->getSize() - aGuard[degree_-i]);
				sigmakPWeight[i] = 1 + maxMSB;
			
				REPORT( DEBUG, " sigma"<<i<<"PSize="<< sigmakPSize[i] << " sigma"<<i<<"PWeight[i]="<<sigmakPWeight[i]);
			}
			
			//cerr << " +++  pikpt"<<i<< " size="<<  pikPTSize[i] << " weight = " << pikPTWeight[i] << endl;
			
			
			mpfr_t *r;
			r = (mpfr_t *) malloc( sizeof(mpfr_t));
			mpfr_init2( *r, 100);
			mpfr_add ( *r, *pikPT[i], *a[degree_-i], GMP_RNDN);			

			sigmakP.push_back(r);
//			sigmakP[i] = r;
			
			REPORT( DETAILED, "|sigma"<<i<<"P| weight=" << (mpfr_get_d(*r,GMP_RNDN)!=0?mpfr_get_exp(*r):0)); 
		}
		
		REPORT(DETAILED, "Error (order) for P(y)=" << mpfr_get_exp( *sigmakP_sigmak[degree_]));
		
//		/***** Clean up *********************/

		


		for (uint32_t i=1; i<= unsigned(degree_); i++){
			if ( pikPT_pikP[i] != NULL){
				if ( *pikPT_pikP[i]  != ((mpfr_ptr)0))
					mpfr_clear(*pikPT_pikP[i]);
				free(pikPT_pikP[i]);
			}
			
			if (pikP_pik[i] != NULL){
				mpfr_clear(*pikP_pik[i]);
				free(pikP_pik[i]);
			}
		}
		
		for (uint32_t i=0; i<= unsigned(degree_); i++){
			if (i < unsigned(degree_)){
				if (sigmakP_sigmak[i] != NULL){
					if (*sigmakP_sigmak[i] != ((mpfr_ptr)0))
					mpfr_clear(*sigmakP_sigmak[i]);
					free(sigmakP_sigmak[i]);
				}
			}
		
			if (sigmakP[i] != NULL){
				if ( *sigmakP[i] != ((mpfr_ptr)0))
					mpfr_clear(*sigmakP[i]);
					free(sigmakP[i]);
			}
		
			if ( i < unsigned(degree_)){
				if (a[i] != NULL){
					if ( (*a[i]) != NULL)
						mpfr_clear(*a[i]);
					free(a[i]);
				}
			}
		}
		
		
		for (uint32_t i=1; i<= unsigned(degree_); i++){
			if (ykT_y[i]!= NULL){
				if (*ykT_y[i] != ((mpfr_ptr)0))
					mpfr_clear(*ykT_y[i]);
				free( ykT_y[i] );
			}
			
			if (pikPT[i]!=NULL){
				mpfr_clear(*pikPT[i]);
				free(pikPT[i]);
			}
		}
		
		mpfr_clear(*yy);
		free(yy);
			 
		return sigmakP_sigmak[degree_];
	} 


	void PolynomialEvaluator::updateCoefficients(vector<FixedPointCoefficient*> coef){
		REPORT(DETAILED, "Coefficient manipulation ... ");
		for (uint32_t i=0; i< coef.size(); i++){
			REPORT(DEBUG, "Coefficient before; size="<<coef[i]->getSize()<<" weight="<<coef[i]->getWeight());
			FixedPointCoefficient *fp = new FixedPointCoefficient(coef[i]);
			/* update the coefficient size; see Doxygen in hpp for more details*/
			fp->setSize(coef[i]->getSize()+coef[i]->getWeight());
			coef_.push_back(fp);
			REPORT(DEBUG, "Coefficient after; size="<<coef_[i]->getSize()<<" weight="<<coef_[i]->getWeight()); 
		}
	}


	PolynomialEvaluator::~PolynomialEvaluator() {
	}



	void PolynomialEvaluator::initializeErrorVectors(){
		/* Gappa Style */
		
		yGuard_.reserve(20);
		aGuard_.reserve(20);
		yState_.reserve(20);
		maxBoundY.reserve(20);
		maxBoundA.reserve(20);

		
		/*init vectors */
		for (uint32_t i=0; i<=unsigned(degree_)+1; i++){
			yGuard_[i]  = 0; //maxBoundY;
			yState_[i] = 0;
			aGuard_[i] = 0;
			maxBoundY[i] = 0;
			maxBoundA[i] = 0;
		}
		
		maxBoundA[0] = 0; /* first coef doesn't need any guard bits */
//		maxBoundA[degree_] = 1; /*last addition is not truncated, so no guard bits are required for a */
	}
	
	void PolynomialEvaluator::resetCoefficientGuardBits(){
		for (uint32_t i=0; i<unsigned(degree_)+1; i++)
			aGuard_[i] = 0;
	}


	void PolynomialEvaluator::reinitCoefficientGuardBits(){
		bool done = false;
		for (uint32_t i=1; i<unsigned(degree_)+1; i++)
			if ((!done) && (maxBoundA[i]>0)){
				aGuard_[i] = maxBoundA[i] - 1;
				done = true;
			}else{
				aGuard_[i] = maxBoundA[i];
			}
	}

	void PolynomialEvaluator::reinitCoefficientGuardBitsLastIteration(){
		/* a solution was found with max values for guard bits. now we try to
		reduce */
		for (uint32_t i=1; i<unsigned(degree_)+1; i++)
			aGuard_[i] = maxBoundA[i];
	}

	
	void PolynomialEvaluator::initializeExplorationVectors(){
		/*init vectors */
		for (uint32_t i=1; i<=unsigned(degree_)+1; i++){
			yGuard_[i] = 0; //maxBoundY;
			yState_[i] = 0;
		}
		
		for (uint32_t i=0; i<unsigned(degree_)+1; i++)
			aGuard_[i] = maxBoundA[i];
			
		yState_[1]=-1;	
	}

	/* ----------------- ERROR RELATED ----------------------------------*/
	/* ------------------------------------------------------------------*/
	void PolynomialEvaluator::allocateErrorVectors(){
		sigmakPSize.reserve(20);
		pikPTSize.reserve(20);
		pikPSize.reserve(20);
		sigmakPWeight.reserve(20);
		pikPTWeight.reserve(20);
		pikPWeight.reserve(20);
	}

	void PolynomialEvaluator::setApproximationError( mpfr_t *p){
		approximationError = (mpfr_t*) malloc( sizeof(mpfr_t));
		mpfr_init2(*approximationError, 1000);
		mpfr_set( *approximationError, *p, GMP_RNDN);
		REPORT(DETAILED, "The approximation error budget is (represetned as double):" << mpfr_get_d(*approximationError,GMP_RNDN)); 
	}

	void PolynomialEvaluator::setMaxYValue(YVar* y){
		/* the abs of the maximal value of y */
		mpfr_init2 ( maxABSy, 100);
		mpfr_set_ui( maxABSy, 2, GMP_RNDN);
		mpfr_pow_si( maxABSy, maxABSy, y_->getSize(), GMP_RNDN);
		mpfr_add_si( maxABSy, maxABSy, -1, GMP_RNDN);
		mpfr_set_exp( maxABSy, mpfr_get_exp(maxABSy)+y_->getWeight()-y_->getSize());
		REPORT(DETAILED, "Abs max value of y is " << mpfr_get_d( maxABSy, GMP_RNDN)); 
	}


	/* ---------------- EXPLORATION RELATED -----------------------------*/
	/* ------------------------------------------------------------------*/
	void PolynomialEvaluator::determineObjectiveStatesForTruncationsOnY(){
		int Xd, Yd;
		target_->getDSPWidths(Xd,Yd);
		for (int i=0; i < getPolynomialDegree(); i++){
			if (( y_->getSize()> unsigned(Xd) ) && (y_->getSize() % unsigned(Xd) != 0)){
				int k=1;
				while (k*Xd < int(y_->getSize())){
						objectiveStatesY.insert(pair<int,int>(i+1, y_->getSize() - k*Xd));
					k++;					
				}
			}
			if (Yd!=Xd)
				if (( y_->getSize()> unsigned(Yd) ) && (y_->getSize() % unsigned(Yd) != 0)){
				int k=1;
				while (k*Yd < int(y_->getSize())){
					objectiveStatesY.insert(pair<int,int>(i+1, y_->getSize() - k*Yd));
					k++;					
				}
			}
			/* insert the "do no truncation" pair */
			objectiveStatesY.insert(pair<int,int>(i+1, 0)); 
		}
		for (multimap<int, int>::iterator it = objectiveStatesY.begin(); it != objectiveStatesY.end(); ++it){
			REPORT(DEBUG, "yGuardObjective[" << (*it).first << ", " << (*it).second << "]");
		}
	}
	
	void PolynomialEvaluator::setNumberOfPossibleValuesForEachY(){
		for (int i=1; i <= getPolynomialDegree(); i++){
			maxBoundY[i] = objectiveStatesY.count(i);
			REPORT(DEBUG, "MaxBoundY["<<i<<"]="<<maxBoundY[i]);
		}
	}

	void PolynomialEvaluator::setNumberOfPossibleValuesForEachA(){
		int Xd, Yd;
		target_->getDSPWidths(Xd,Yd);
		for (int i=1; i <= getPolynomialDegree(); i++){
			maxBoundA[i] = max(Xd-sigmakPSize[i]%Xd, Yd - sigmakPSize[i]%Yd);
			REPORT(DEBUG, "MaxBoundA["<<i<<"]="<<maxBoundA[i]);
		}
	}

	/* ------------------------------ EXTRA -----------------------------*/
	/* ------------------------------------------------------------------*/

	string PolynomialEvaluator::printPolynomial( vector<FixedPointCoefficient*> coef, YVar* y, int level){
		ostringstream horner;
		if (level == getPolynomialDegree()){
			horner << "a["<<level<<"]2^("<<coef[level]->getWeight()<<")";
			return horner.str();
		}else{
			horner << "y*2^("<<y_->getWeight()<<"){"<< printPolynomial(coef, y, level+1) << "} + " << "a["<<level<<"]2^("<<coef[level]->getWeight()<<")";
			return horner.str();
		}
	}


}



