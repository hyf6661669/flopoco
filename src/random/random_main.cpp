#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <mpfr.h>
#include <cstdlib>

#include "urng/lut_sr_rng.hpp"

#include "fixed_point_exp/func_approx_exp_stage.hpp"
#include "fixed_point_exp/fixed_point_exp_stage.hpp"
#include "fixed_point_exp/fixed_point_exp_tester.hpp"
#include "fixed_point_exp/table_exp_stage.hpp"
#include "fixed_point_exp/close_table_exp_stage.hpp"
#include "fixed_point_exp/multiplier_exp_stage.hpp"
#include "fixed_point_exp/chained_exp_stage.hpp"

#include "FloPoCo.hpp"


#define BRIGHT 1
#define RED 31
#define OPER 32
#define NEWOPER 32
#define PARAM 34
#define OP(op,paramList)             {cerr << "    "; printf("%c[%d;%dm",27,1,OPER); cerr <<  op; printf("%c[%dm",27,0); cerr<< " "; printf("%c[%d;%dm",27,1,PARAM); cerr << paramList; printf("%c[%dm\n",27,0); } 
#define NEWOP(op,paramList)          {cerr << "    "; printf("%c[%d;%dm",27,1,NEWOPER); cerr <<  op; printf("%c[%dm",27,0); cerr<< " "; printf("%c[%d;%dm",27,1,PARAM); cerr << paramList; printf("%c[%dm\n",27,0); } 


using namespace std;
using namespace flopoco;

// Global variables, useful in this main to avoid parameter passing


	//string filename="flopoco.vhdl";
	//string cl_name=""; // used for the -name option
	//Target* target;
	
extern void usage(char *name, string opName);
extern int checkStrictlyPositive(char* s, char* cmd);
extern int checkPositiveOrNull(char* s, char* cmd);
extern bool checkBoolean(char* s, char* cmd);
extern int checkSign(char* s, char* cmd);
extern void addOperator(vector<Operator*> &oplist, Operator *op);

std::vector<boost::shared_ptr<Operator> > g_pinOperator;

void pinOperator(boost::shared_ptr<Operator> op)
{
	g_pinOperator.push_back(op);
}
	
void random_usage(char *name, string opName = ""){
	bool full = (opName=="");

	if( full || opName=="lut_sr_rng"){
		OP("lut_sr_rng", "r t k");
		cerr << "       uniform RNG using LUTs and Shift Registers\n";
		cerr << "	r - width of output random number\n";
		cerr << "	t - XOR gate input count\n";
		cerr << "	k - Maximum Shift Register length\n";
	}
	if(opName=="TableExpStage"){
		OP("TableExpStage", "msb lsb addrW resultW");
	}
	if(opName=="MultiplierTableExp"){
		OP("MultiplierTableExp", "msb lsb addrW tableW   resultInW resultInMinExp resultInMaxExp   resultOutW");
		std::cerr<<" Creates an exp approximation stage using a table and a multiplier.";
	}
	if(opName=="ChainedMultiplierTableExp"){
		OP("ChainedMultiplierTableExp", "msb lsb  addrW resultTableW resultCalcW  resultOutW");
		std::cerr<<" Creates an exp approximation using (non-exact) tables of the same width and multipliers.";
	}
	/*
	//6.20 bitwise architecture Junfei Yan
	if (full || opName=="bitwise"){
		OP("bitwise", "MSB LSB m lambda");
		cerr << "	exponential distribution bit-wise generator\n";
		cerr << "	MSB, LSB defines the output fixed point random number\n";
		cerr << "	m is the width of each comparator\n";
	}
	*/
}

bool random_parseCommandLine(
	int argc, char* argv[], Target *target,
	std::string opname, int &i,
	vector<Operator*> &oplist
){
	
	/*
	if (opname == "bitwise")
	{
		int nargs = 4;
		if (i+nargs > argc)
			usage(argv[0], opName); // and exit
		int MSB = checkStrictlyPositive(argv[i++], argv[0]);
		int LSB = checkStrictlyPositive(argv[i++], argv[0]);
		string m = argv[i++];
		double lambda   = checkStrictlyPositive(argv[i++], argv[0]);

		cerr << "> bitwise: MSB=" << MSB << " LSB=" << LSB << " m=" << m << " lambda=" << lambda << endl;
		op = new bitwise(target, MSB, LSB, m, lambda);
		addOperator(oplist, op);
		return true;
	}
	else */
	if (opname == "lut_sr_rng")
	{
		int nargs = 3;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int tr = checkStrictlyPositive(argv[i++], argv[0]);
		int t = checkStrictlyPositive(argv[i++], argv[0]);
		int k = checkStrictlyPositive(argv[i++], argv[0]);


		cerr << "> lut_sr_rng: r=" << tr << "	t= " << t << "	k= " << k <<endl;
		addOperator(oplist, new flopoco::random::LutSrRng(target, tr, t, k));
		return true;
	}
	else
	if (opname == "TableExpStage")
	{
		int nargs = 4;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		
		int residualMsb = atoi(argv[i++]);
		int residualLsb = atoi(argv[i++]);
		int addressWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int fracWidth = checkStrictlyPositive(argv[i++], argv[0]);
		
		flopoco::random::residual_type<double> inputResidual(true, residualMsb-residualLsb+1, residualMsb);
		cerr<<"input type = "<<inputResidual<<"\n";
		
		double mu=0.0;
		double sigma=1.0;
		
		flopoco::random::FixedPointExpStagePtr table(
			new flopoco::random::TableExpStage(target, mu, sigma,
				inputResidual, 
				addressWidth,
				fracWidth
			)
		);
		addOperator(oplist, table.get());
		pinOperator(table);
		
		return true;
	}
	else if (opname == "MultiplierTableExp")
	{
		int nargs = 8;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		
		int residualMsb = atoi(argv[i++]);
		int residualLsb = atoi(argv[i++]);
		int addressWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int tableWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int inputResultWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int inputMinExp = atoi(argv[i++]);
		int inputMaxExp = atoi(argv[i++]);
		int outputResultWidth = checkStrictlyPositive(argv[i++], argv[0]);
		
		flopoco::random::residual_type<double> inputResidual(true, residualMsb-residualLsb+1, residualMsb);
		flopoco::random::result_type<double> inputResult(inputResultWidth);
		inputResult.expMin=inputMinExp;
		inputResult.expMax=inputMaxExp;
		cerr<<"input type = "<<inputResidual<<"\n";
		cerr<<"output type = "<<inputResult<<"\n";
		
		double mu=0.0;
		double sigma=1.0;
		
		flopoco::random::FixedPointExpStagePtr table(new flopoco::random::TableExpStage(target, mu, sigma,
			inputResidual, 
			addressWidth,
			tableWidth
		));
		pinOperator(table);
		addOperator(oplist, table.get());
		
		boost::shared_ptr<flopoco::random::MultiplierExpStage> stage(
			new flopoco::random::MultiplierExpStage(target,
			table,
			inputResult,
			outputResultWidth
		));
		pinOperator(stage);
		addOperator(oplist, stage.get());
		
		return true;
	}
	else if (opname == "ChainedMultiplierTableExp")
	{
		int nargs = 6;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		
		int residualMsb = atoi(argv[i++]);
		int residualLsb = atoi(argv[i++]);
		int addressWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int tableResultWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int calcResultWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int outputResultWidth = checkStrictlyPositive(argv[i++], argv[0]);
		
		flopoco::random::residual_type<double> inputResidual(true, residualMsb-residualLsb+1, residualMsb);
		
		double mu=0.0;
		double sigma=1.0;
		
		std::vector<flopoco::random::FixedPointExpStagePtr> stages;
		while(inputResidual.Width()>0){
			flopoco::random::FixedPointExpStagePtr table(new flopoco::random::TableExpStage(target,
				stages.size()==0 ? mu : 0.0, sigma,
				inputResidual, 
				std::min(addressWidth, inputResidual.Width()),
				tableResultWidth
			));
			addOperator(oplist, table.get());
			inputResidual=table->OutputResidualType();
			
			if(stages.size()==0){
				stages.push_back(table);
			}else{
				boost::shared_ptr<flopoco::random::MultiplierExpStage> m(
					new flopoco::random::MultiplierExpStage(target,
						table,
						stages.back()->OutputResultType(),
						inputResidual.Width()==0 ? outputResultWidth : calcResultWidth
					)
				);
				addOperator(oplist, m.get());
				stages.push_back(m);
			}
		}
		
		boost::shared_ptr<flopoco::random::ChainedExpStage> chain(
			new flopoco::random::ChainedExpStage(target,
				stages
			)
		);
		pinOperator(chain);
		addOperator(oplist, chain.get());
		
		return true;
	}
	else
	if (opname == "CloseTableExpStage")
	{
		int nargs = 5;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		
		int residualMsb = atoi(argv[i++]);
		int residualLsb = atoi(argv[i++]);
		int addressWidth = checkStrictlyPositive(argv[i++], argv[0]);
		double errMin=boost::lexical_cast<double>(argv[i++]);
		double errMax=boost::lexical_cast<double>(argv[i++]);
		
		if((errMin>0) || (errMax<0)){
			std::cerr<<"Need minErr <= 0 <= maxErr";
			exit(1);
		}
		
		flopoco::random::residual_type<double> inputResidual(true, residualMsb-residualLsb+1, residualMsb);
		cerr<<"input type = "<<inputResidual<<"\n";
		
		double mu=0.0;
		double sigma=1.0;
		
		flopoco::random::FixedPointExpStagePtr table(
			new flopoco::random::CloseTableExpStage(target, mu, sigma,
				inputResidual, 
				addressWidth,
				errMin, errMax
			)
		);
		pinOperator(table);
		addOperator(oplist, table.get());
		return true;
	}
	if (opname == "FuncApproxExpStage")
	{
		int nargs = 3;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		
		int residualMsb = atoi(argv[i++]);
		int residualLsb = atoi(argv[i++]);
		int outWidth = atoi(argv[i++]);
		
		flopoco::random::residual_type<double> inputResidual(false, residualMsb-residualLsb+1, residualMsb);
		cerr<<"input type = "<<inputResidual<<"\n";
		
		double mu=0.0;
		double sigma=1.0;
		
		flopoco::random::FixedPointExpStagePtr table(
			new flopoco::random::FuncApproxExpStage(target, mu, sigma,
				inputResidual, 
				outWidth
			)
		);
		pinOperator(table);
		addOperator(oplist, table.get());
		return true;
	}
	else if (opname == "ChainedMultiplierCloseTableExp")
	{
		int nargs = 5;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		
		int residualMsb = atoi(argv[i++]);
		int residualLsb = atoi(argv[i++]);
		int addressWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int outputResultWidth = checkStrictlyPositive(argv[i++], argv[0]);
		int crossOver=atoi(argv[i++]);
		
		flopoco::random::residual_type<double> inputResidual(true, residualMsb-residualLsb+1, residualMsb);
		
		double mu=0.0;
		double sigma=1.0;
		
		// we want (1+stageErr)^k < (1+outputErr)
		// Let's assume (1+stageErr)^k < 1 + (k+1)*stageErr <= (1+outputErr), and quietly walk away
		
		double outputErr=pow(2.0, -outputResultWidth-1);
		int stageCount=(int)ceil((inputResidual.Width()-1.0)/(addressWidth-1.0));
		
		double stageErr=outputErr / (stageCount+1);
		
		// Again, apologies...
		int calcResultWidth=outputResultWidth+stageCount+2;
		
		std::cerr<<"stageErr = 2^"<<log(stageErr)/log(2.0)<<"\n";
		
		std::vector<flopoco::random::FixedPointExpStagePtr> stages;
		while(inputResidual.Width()>0){
			flopoco::random::FixedPointExpStagePtr table;
			
			if(inputResidual.MsbPower() < crossOver){
				table.reset(new flopoco::random::FuncApproxExpStage(target,
					stages.size()==0 ? mu : 0.0, sigma,
					inputResidual, 
					outputResultWidth+3
				));
			}else{
				table.reset(new flopoco::random::CloseTableExpStage(target,
					stages.size()==0 ? mu : 0.0, sigma,
					inputResidual, 
					std::min(addressWidth, inputResidual.Width()),
					-stageErr, stageErr
				));
			}
			addOperator(oplist, table.get());
			inputResidual=table->OutputResidualType();
			
			if(stages.size()==0){
				stages.push_back(table);
			}else{
				boost::shared_ptr<flopoco::random::MultiplierExpStage> m(
					new flopoco::random::MultiplierExpStage(target,
						table,
						stages.back()->OutputResultType(),
						inputResidual.Width()==0 ? outputResultWidth : std::min((unsigned)calcResultWidth, stages.back()->OutputResultType().FracWidth()+table->OutputResultType().FracWidth())
					)
				);
				addOperator(oplist, m.get());
				stages.push_back(m);
			}
		}
		
		boost::shared_ptr<flopoco::random::ChainedExpStage> chain(
			new flopoco::random::ChainedExpStage(target,
				stages
			)
		);
		pinOperator(chain);
		addOperator(oplist, chain.get());
		
		return true;
	}else if (opname == "ExpStageTester"){
		if(g_pinOperator.size()==0)
			throw std::logic_error("ExpStageTester must follow a FixExp operator.");
		
		flopoco::random::FixedPointExpStagePtr back=boost::dynamic_pointer_cast<flopoco::random::FixedPointExpStage>(g_pinOperator.back());
		if(back==0)
			throw std::logic_error("ExpStageTester must follow a FixExp operator.");
		
		boost::shared_ptr<flopoco::random::FixedPointExpTester> chain(
			new flopoco::random::FixedPointExpTester(target,
				back
			)
		);
		pinOperator(chain);
		addOperator(oplist, chain.get());
		
		return true;
	}
	else
	{
		return false;
	}
}



