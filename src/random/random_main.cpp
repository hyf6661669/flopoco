#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <mpfr.h>
#include <cstdlib>

#include "urng/lut_sr_rng.hpp"

#include "UtilSollya.hh"

#include "transforms/blocks/HadamardTransform.hpp"
#include "transforms/blocks/CLTTransform.hpp"
#include "transforms/blocks/TableTransform.hpp"
#include "transforms/blocks/GRNGTableTransform.hpp"
#include "transforms/blocks/OutputShuffle.hpp"
#include "transforms/blocks/OutputShuffleTransform.hpp"
#include "float_approx/static_quantiser.hpp"

// TODO : Bring this back in - HOTBM has changed significantly
/*
#include "fixed_point_exp/func_approx_exp_stage.hpp"
#include "fixed_point_exp/fixed_point_exp_stage.hpp"
#include "fixed_point_exp/fixed_point_exp_tester.hpp"
#include "fixed_point_exp/table_exp_stage.hpp"
#include "fixed_point_exp/close_table_exp_stage.hpp"
#include "fixed_point_exp/multiplier_exp_stage.hpp"
#include "fixed_point_exp/chained_exp_stage.hpp"
*/

#include "utils/operator_factory.hpp"

#include "utils/comparable_float_type.hpp"

#include "float_approx/FloatApprox.hpp"

//#include "FloPoCo.hpp"


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

extern void addOperator(Operator *op);

std::vector<boost::shared_ptr<Operator> > g_pinOperator;

void pinOperator(boost::shared_ptr<Operator> op)
{
	g_pinOperator.push_back(op);
}

namespace flopoco{ namespace random{
	void PolynomialEvaluator_registerFactory();
	void TransformStats_registerFactory();
	void OutputCombiner_registerFactory();
	void CLTCorrectedTransform_registerFactory();
	void FixedPointPolynomialEvaluator_registerFactory();
	void MWCRng_registerFactory();
	void MWCTransform_registerFactory();
}; }; 

void random_register_factories()
{
	flopoco::random::TableTransform::registerFactory();
	flopoco::random::CLTTransform::registerFactory();
	flopoco::random::GRNGTableTransform_registerFactory();
	flopoco::random::TransformStats_registerFactory();
	flopoco::random::CLTCorrectedTransform_registerFactory();
	flopoco::random::OutputShuffle::registerFactory();
	flopoco::random::OutputShuffleTransform::registerFactory();
	
	flopoco::random::StaticQuantiser_registerFactory();	
	flopoco::random::FloatApprox_registerFactory();
	flopoco::random::OutputCombiner_registerFactory();
	flopoco::random::PolynomialEvaluator_registerFactory();
	flopoco::random::FixedPointPolynomialEvaluator_registerFactory();
	flopoco::random::MWCRng_registerFactory();
	flopoco::random::MWCTransform_registerFactory();
}
	
void random_usage(char *name, string opName = ""){
	flopoco::random::OperatorFactory::classic_usage(name, opName);
	
	bool full = (opName=="");
	
	if(full || opName=="Exit"){
		OP("Exit", "");
		cerr<<"    Immediately exit flopoco without writing VHDL (useful\n";
		cerr<<"    if you are calculating stats or something).\n";
	}

	if( full || opName=="lut_sr_rng"){
		OP("lut_sr_rng", "r t k");
		cerr << "       uniform RNG using LUTs and Shift Registers\n";
		cerr << "	r - width of output random number\n";
		cerr << "	t - XOR gate input count\n";
		cerr << "	k - Maximum Shift Register length\n";
	}
	if( opName=="clt_hadamard_transform"){
		OP("clt_hadamard_transform", "log2n baseWidth");
		cerr << "       Generates vectors of Gaussian Random numbers\n";
		cerr << "	log2n - Number of output variates (n=2^log2n)\n";
		cerr << "	baseWidth - How many bits per base generator.\n";
	}
	if( opName=="table_hadamard_transform"){
		OP("table_hadamard_transform", "[-extra_pipeline] log2n log2k stddev fb correction quantisation");
		cerr << "       Generates vectors of Gaussian Random numbers using table generators and a hadamard matrix\n";
		cerr << "	log2n - Number of output variates (n=2^log2n)\n";
		cerr << "	log2k - How many table entries per base generator.\n";
		cerr << "	stddev - Target standard deviation.\n";
		cerr << "	fb - Number of fractional bits of output.\n";
		cerr << "	correction - How to correct the table .\n";
		cerr << "	quantisation - How to quantise the table .\n";
	}
	/*
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
	if(opName=="ComparableFloatEncoder"){
		OP("ComparableFloatEncoder", "wE wF");
		std::cerr<<" Converts a flopoco FP number into an alternate form that can be directly compared as bit patterns.";
	}
	*/
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
	std::string opname, int &i
){
	if(flopoco::random::OperatorFactory::classic_parseCommandLine(argc, argv, target, opname, i))
		return true;
	
	if(opname=="Exit"){
		std::cerr<<"Exit command - quiting flopoco without writing operator.\n";
		exit(0);
	}
	
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
		int t = checkPositiveOrNull(argv[i++], argv[0]);
		int k = checkPositiveOrNull(argv[i++], argv[0]);


		cerr << "> lut_sr_rng: r=" << tr << "	t= " << t << "	k= " << k <<endl;
		addOperator(new flopoco::random::LutSrRng(target, tr, t, k));
		return true;
	}
	
	else
	if (opname == "clt_rng")
	{
		int nargs = 2;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int wBase = checkStrictlyPositive(argv[i++], argv[0]);
		int k = checkStrictlyPositive(argv[i++], argv[0]);

		cerr << "> clt_rng: wBase="<<wBase<<", k="<<k<<endl;
		if(k!=2)
			throw std::string("clt_transform - Only k==2 is supported at the moment.");
		std::stringstream acc;
		acc<<"CLTRng_w"<<wBase<<"_k"<<k;
		
		flopoco::random::RngTransformOperator *base=new flopoco::random::CLTTransform(target, wBase);
		addOperator(flopoco::random::LutSrRng::DriveTransform(acc.str(), base));
		return true;
	}
	
	else
	if (opname == "clt_hadamard_transform")
	{
		int nargs = 2;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int log2n = checkStrictlyPositive(argv[i++], argv[0]);
		int wBase = checkStrictlyPositive(argv[i++], argv[0]);

		cerr << "> clt_hadamard_transform: log2n=" << log2n <<", wBase="<<wBase<<endl;
		
		flopoco::random::RngTransformOperator *base=new flopoco::random::CLTTransform(target, wBase);
		assert(base);
		addOperator(new flopoco::random::HadamardTransform(target, log2n, base));
		return true;
	}
	
	else
	if (opname == "table_hadamard_transform")
	{
		bool extraPipeline=false;
		
		if(i<argc){
			if(!strcmp("-extra_pipeline", argv[i])){
				i++;
				extraPipeline=true;
			}
		}
		
		int nargs = 6;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int log2n = checkStrictlyPositive(argv[i++], argv[0]);
		int log2k = checkStrictlyPositive(argv[i++], argv[0]);
		mpfr::mpreal stddev(0,getToolPrecision());
		parseSollyaConstant(stddev.mpfr_ptr(), argv[i++]);
		int fb=checkStrictlyPositive(argv[i++], argv[0]);
		std::string corr=argv[i++];
		std::string quant=argv[i++];

		cerr << "> clt_hadamard_transform: log2n=" << log2n <<", log2k="<<log2k<<", stddev="<<stddev<<", fb="<<fb<<", extraPipeline="<<extraPipeline<<"\n";
		
		mpfr::mpreal partStddev=sqrt(stddev*stddev*pow(2.0, -log2n));
		
		flopoco::random::RngTransformOperator * base=flopoco::random::MakeGRNGTable(target, log2k, fb, partStddev, corr, quant);
		
		assert(base);
		addOperator(new flopoco::random::HadamardTransform(target, log2n, base, extraPipeline));
		cerr << "> clt_hadamard_transform: done\n";
		
		return true;
	}
	
	else
	if (opname == "clt_hadamard_rng")
	{
		int nargs = 2;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int log2n = checkStrictlyPositive(argv[i++], argv[0]);
		int wBase = checkStrictlyPositive(argv[i++], argv[0]);

		cerr << "> clt_hadamard_rng: log2n=" << log2n <<", wBase="<<wBase<<endl;
		std::stringstream acc;
		acc<<"CLTHadamardRng_n"<<(1<<log2n)<<"_wb"<<wBase;
		
		flopoco::random::RngTransformOperator *base=new flopoco::random::CLTTransform(target, wBase);
		
		flopoco::random::RngTransformOperator *hadamard=new flopoco::random::HadamardTransform(target, log2n, base);
		
		addOperator(flopoco::random::LutSrRng::DriveTransform(acc.str(), hadamard));
		return true;
	}
	
	else if(opname=="connect_rng_transform_to_urng"){
		vector<Operator*> * globalOpListRef=target->getGlobalOpListRef();
		if(globalOpListRef->size()==0)
			throw std::string("No existing op to connect to urng.");
		flopoco::Operator *baseOp=globalOpListRef->back();
		
		flopoco::random::RngTransformOperator *baseTransform=dynamic_cast<flopoco::random::RngTransformOperator*>(baseOp);
		if(baseTransform==NULL)
			throw std::string("Previous op is not an RngTransformOperator.");
		
		addOperator(flopoco::random::LutSrRng::DriveTransform(baseTransform->getName()+"_rng", baseTransform));
		
		return true;
	}
	/*
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
		addOperator(table.get());
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
		addOperator(table.get());
		
		boost::shared_ptr<flopoco::random::MultiplierExpStage> stage(
			new flopoco::random::MultiplierExpStage(target,
			table,
			inputResult,
			outputResultWidth
		));
		pinOperator(stage);
		addOperator(stage.get());
		
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
			addOperator(table.get());
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
				addOperator(m.get());
				stages.push_back(m);
			}
		}
		
		boost::shared_ptr<flopoco::random::ChainedExpStage> chain(
			new flopoco::random::ChainedExpStage(target,
				stages
			)
		);
		pinOperator(chain);
		addOperator(chain.get());
		
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
		addOperator(table.get());
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
		addOperator(table.get());
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
			addOperator(table.get());
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
				addOperator(m.get());
				stages.push_back(m);
			}
		}
		
		boost::shared_ptr<flopoco::random::ChainedExpStage> chain(
			new flopoco::random::ChainedExpStage(target,
				stages
			)
		);
		pinOperator(chain);
		addOperator(chain.get());
		
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
		addOperator(chain.get());
		
		return true;
	}
	*/
	else
	if (opname == "ComparableFloatEncoder")
	{
		int nargs = 2;
		if (i+nargs > argc)
			usage(argv[0], opname); // and exit
		int wE = checkStrictlyPositive(argv[i++], argv[0]);
		int wF = checkStrictlyPositive(argv[i++], argv[0]);

		flopoco::random::ComparableFloatType type(wE, wF);
		addOperator(type.MakeEncoder(target));
		return true;
	}
	
	else
	{
		return false;
	}
}



