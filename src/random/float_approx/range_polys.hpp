#ifndef flopoco_random_float_approx_range_polys_hpp
#define flopoco_random_float_approx_range_polys_hpp

#include "range.hpp"

#include <boost/smart_ptr.hpp>
#include <boost/any.hpp>
#include <boost/format.hpp>
#include <boost/utility.hpp>

#include "mpfr_vec.hpp"

#include "FixedPointFunctions/PolynomialEvaluator.hpp"

#include "static_quantiser.hpp"


namespace flopoco
{
namespace random
{
namespace float_approx
{

MPFRVec getPolyCoeffs(sollya_node_t poly)
{
	unsigned degree=getDegree(poly);
	MPFRVec res(degree+1, getToolPrecision());
	for(unsigned i=0;i<=degree;i++){
		sollya_node_t c=getIthCoefficient(poly, i);
		evaluateConstantExpression(res[i], c, getToolPrecision());
		free_memory(c);
	}
	return res;
}

class RangePolys
{
public:	
	typedef Range::segment_it_t segment_it_t;
private:
	
	Range &m_range;
	unsigned m_degree;

	void infNorm(mpfr_t error, sollya_node_t func, sollya_node_t poly, mpfr_t a, mpfr_t b, unsigned points)
	{
		sollya_node_t diff=makeSub(copyTree(func), copyTree(poly));
		
		uncertifiedInfnorm(error, diff, a, b, points, getToolPrecision()); 
		free_memory(diff);
	}

	void update_segment_minimax(segment_it_t curr)
	{
		if(curr->has_property("minimax") && curr->has_property("minimax_error"))
			return;
		
		sollya_node_ptr_t poly=make_shared_sollya(curr->minimax(m_degree));
		sollya_node_t func=curr->get_scaled_flat_function(); // doesn't need to be freed
		
		MPFRVec error(1, getToolPrecision());
		infNorm(error[0], func, poly.get(), curr->domainStartFrac, curr->domainFinishFrac, 71);
		
		curr->properties["minimax"]=poly;
		curr->properties["minimax_error"]=error;
	}
public:
	RangePolys(Range &range, unsigned degree)
		: m_range(range)
		, m_degree(degree)
	{
		segment_it_t it=m_range.m_segments.begin();
		while(it!=m_range.m_segments.end()){
			update_segment_minimax(it);
			++it;
		}
	}
		
	void split_to_error(double error)
	{
		mpfr_t tol;
		mpfr_init2(tol, getToolPrecision());
		mpfr_set_d(tol, error, MPFR_RNDN);
		
		segment_it_t it=m_range.m_segments.begin();
		while(it!=m_range.m_segments.end()){
			update_segment_minimax(it);
			
			MPFRVec serror=boost::any_cast<MPFRVec>(it->properties["minimax_error"]);
			if( mpfr_greaterequal_p(serror[0],tol)){
				mpfr_fprintf(stderr, "Splitting [%Rf,%Rf]",
					it->domainStart, it->domainFinish
				);
				mpfr_fprintf(stderr, " error=%Rg\n", serror[0]);
				m_range.split_segment(it);
				update_segment_minimax(it);
			}			
			assert(it->has_property("minimax_error"));
			
			serror=boost::any_cast<MPFRVec>(it->properties["minimax_error"]);
			if(mpfr_less_p(serror[0],tol))
				++it;
		}
		mpfr_clear(tol);
	}
	
	// Find fixed-point polynomial coefficients such that the result is faithful (?) on rangeWF+guard bits
	void calc_faithful_fixed_point(segment_it_t curr, int guard=0)
	{
		std::string pnCoeffs=boost::str(boost::format("minimax_fixed%1%_coeffs")%guard);
		
		if(curr->has_property(pnCoeffs))
			return;
		
		update_segment_minimax(curr);
		
		mpfr_t tol;
		mpfr_init2(tol, getToolPrecision());
		mpfr_set_d(tol, pow(2.0, -m_range.m_rangeWF-guard-1), MPFR_RNDN);
		
		MPFRVec serror=boost::any_cast<MPFRVec>(curr->properties["minimax_error"]);
		if(mpfr_greater_p(serror[0],tol))
			throw std::runtime_error("calc_faithful_fixed_point - poly is not accurate enough to be faithful (?).");
		
		// Ok, let's try to do this properly. Heuristic from ASAP 2010 is that where:
		// k = leading zeros. Equivalent to index bits in a table-poly over [0,1). Here k=1, as we always have a leading zero due to range over [0,1)
		// j = Coefficient degree
		// p = target precision in bits (e.g. precision is 2^-p)
		// then we need
		//   lsb(c[i]) = -p + k*j
		// In our case that simplifies to
		//   lsb(c[i]) = -p + j
		std::vector<int> lsbs(m_degree+1), fmts(m_degree+1);
		for(unsigned j=0;j<=m_degree;j++){
			lsbs[j]= -(m_range.m_rangeWF+guard+1) + j -1;	// TODO : I seem to need one more LSB than the paper says, otherwise PolynomialEvaluator doesn't converge
			fmts[j]=-lsbs[j]; // format is negative of lsb
		}
		sollya_node_t fp=curr->fpminimax(fmts, boost::any_cast<sollya_node_ptr_t>(curr->properties["minimax"]).get() );
		
		MPFRVec error(1, getToolPrecision());
		infNorm(error[0], curr->get_scaled_flat_function(), fp, curr->domainStartFrac, curr->domainFinishFrac, 71);
		mpfr_abs(error[0],error[0],MPFR_RNDN);
		MPFRVec coeffs=getPolyCoeffs(fp);
		free_memory(fp);
		
		if(mpfr_greater_p(error[0],tol))
			throw std::runtime_error("calc_faithful_fixed_point - fixed-point poly was not faithful.");
		
		std::string pnLsbs=boost::str(boost::format("minimax_fixed%1%_lsbs")%guard);
		std::string pnError=boost::str(boost::format("minimax_fixed%1%_error")%guard);
		
		curr->properties[pnCoeffs]=coeffs;
		curr->properties[pnLsbs]=lsbs;
		curr->properties[pnError]=error;
		
		mpfr_clear(tol);
	}
	
	void calc_faithful_fixed_point(int guard=0)
	{
		// TODO : We split it so that the minimax is twice as accurate as needed. This means we use too
		// many segments, and should be optimised. Yes, I suck at maths.
		split_to_error(pow(2.0, -m_range.m_rangeWF-guard-2));
		
		segment_it_t curr=m_range.m_segments.begin();
		while(curr!=m_range.m_segments.end()){
			calc_faithful_fixed_point(curr, guard);
			++curr;
		}
	}
	
	MPFRVec m_concretePartition;
	std::vector<int> m_concreteExp;
	std::vector<int> m_concreteCoeffMsbs, m_concreteCoeffLsbs;
	std::vector<mpf_class> m_concreteTableRamContents;
	MPFRVec m_concreteApproxError;	// worst polynomial error
	
	// Signed number with bits:  sign,  msb, msb-1, ... lsb+1, lsb
	// s, -1, -2, -3, -4 -> width=5=1+(-1- -4)=1+(msb-lsb)
	mpz_class ToTwosComplement(mpz_class x, int msb, int lsb)
	{
		int w=1+msb-lsb;
		mpfr_fprintf(stderr, "   x=%Zd=%Zx, msb=%d, lsb=%d, width=%d\n", x.get_mpz_t(), x.get_mpz_t(), msb, lsb, w);
		
		mpz_class ret;
		if(x>=0){
			ret=x;
		}else{
			for(int i=w-1;i>=0;i--){
				ret=(ret<<1) + mpz_tstbit(x.get_mpz_t(), i);	// manually read out two's complements bits. Heh.
			}
		}
		mpfr_fprintf(stderr, "  ret=%Zd = %Zx\n", ret.get_mpz_t(), ret.get_mpz_t());
		mpz_class tmp;
		mpz_ui_pow_ui(tmp.get_mpz_t(), 2, w);
		assert(ret>=0);
		assert(ret<tmp);
		return ret;
	}
	
	void build_concrete(int guard=0)
	{
		m_concreteApproxError=MPFRVec(1,getToolPrecision());
		mpfr_set_d(m_concreteApproxError[0], 0.0, MPFR_RNDN);
		
		mpfr_t tmp;
		mpfr_init2(tmp, getToolPrecision());
		
		std::string pnCoeffs=boost::str(boost::format("minimax_fixed%1%_coeffs")%guard);
		std::string pnLsbs=boost::str(boost::format("minimax_fixed%1%_lsbs")%guard);
		std::string pnError=boost::str(boost::format("minimax_fixed%1%_error")%guard);
		
		m_concretePartition.resize(m_range.m_segments.size(), m_range.m_domainWF);
		m_concreteExp.resize(0);
		
		std::vector<int> coeffLsbs(m_degree+1, INT_MAX);
		std::vector<int> coeffMsbs(m_degree+1, -INT_MAX);
		
		// First walk over all the coefficients, and find the min lsb and max msb of each coefficient
		segment_it_t curr=m_range.m_segments.begin();
		for(unsigned i=0;i<m_range.m_segments.size();i++){
			assert(curr->has_property(pnCoeffs));
			MPFRVec coeffs=boost::any_cast<MPFRVec>(curr->properties[pnCoeffs]);
			std::vector<int> lsbs=boost::any_cast<std::vector<int> >(curr->properties[pnLsbs]);
			MPFRVec error=boost::any_cast<MPFRVec>(curr->properties[pnError]);
			
			for(unsigned j=0;j<=m_degree;j++){
				coeffLsbs[j]=std::min(coeffLsbs[j], lsbs[j]);
				coeffMsbs[j]=std::max(coeffMsbs[j], (int)mpfr_get_exp(coeffs[j])-1);	// 0.5*2^0=0.5 -> mpfr_get_exp(0.5)==0,  0.5*2^1=1 -> mpfr_get_exp(1)==1
				mpfr_fprintf(stderr, "coeff = %Rg, msb=%d, lsb=%d\n", coeffs[j], lsbs[j], mpfr_get_exp(coeffs[j])-1);
			}
			
			mpfr_set(m_concretePartition[i], curr->domainFinish, MPFR_RNDN);
			m_concreteExp.push_back(mpfr_get_exp(curr->rangeStart));
			
			if(mpfr_greater_p(error[0], m_concreteApproxError[0]))
				m_concreteApproxError=error;
			
			++curr;
		}
		m_concreteCoeffMsbs=coeffMsbs;
		m_concreteCoeffLsbs=coeffLsbs;
		
		fprintf(stderr, "  Building table.\n");
		
		// Now we know the precise types of the coefficients - they are all signed with
		// bits in the range coeffMsbs..coeffLsbs. Let's build the table first
		curr=m_range.m_segments.begin();
		while(curr!=m_range.m_segments.end()){
			MPFRVec coeffs=boost::any_cast<MPFRVec>(curr->properties[pnCoeffs]);
			
			mpz_class acc, local;
			for(int i=m_degree;i>=0;i--){
				mpfr_set(tmp, coeffs[i], MPFR_RNDN);
				mpfr_mul_2si(tmp, tmp, -coeffLsbs[i], MPFR_RNDN);
				mpfr_get_z(local.get_mpz_t(), tmp, MPFR_RNDN);
				
				acc=(acc<<(1+coeffMsbs[i]-coeffLsbs[i])) + ToTwosComplement(local, coeffMsbs[i], coeffLsbs[i]);
			}
			mpfr_fprintf(stderr, "%Zx\n", acc.get_mpz_t());
			m_concreteTableRamContents.push_back(acc);
			
			++curr;
		}
		
		fprintf(stderr, "Done build concrete.\n");
		
		mpfr_clear(tmp);
	}
	
	PolynomialEvaluator *make_polynomial_evaluator(Target *target, map<string, double> inputDelays = map<string, double>())
	{
		std::vector<FixedPointCoefficient*> coeffs(m_degree+1);
		for(unsigned i=0;i<=m_degree;i++){
			/*
			 * The input coefficients have the format: 
			 *           size   = #of bits at the right of the .
			 *           weight = the weight of the MSB
			 * for example 0.000CCCCCC would be size=9 and weight=-3		// DT10 : <- do this format
			 * and will get converted to        size=9-3=6 (#of C) weight = -3  
			*/
			coeffs[i]=new FixedPointCoefficient(/*size*/ -m_concreteCoeffLsbs[i], /*weight*/ m_concreteCoeffMsbs[i]);
		}
		
		// Input : msb is -1, lsb is -m_range.m_domainWF
		YVar y( m_range.m_domainWF, -1);
		
		mpfr_t approxError;
		mpfr_init2(approxError, getToolPrecision());
		mpfr_set(approxError, m_concreteApproxError[0], MPFR_RNDN);
		
		flopoco::PolynomialEvaluator *res=new PolynomialEvaluator(target, coeffs, &y, /*targetPrec*/ m_range.m_domainWF, &approxError, inputDelays);
		
		for(unsigned i=0;i<=m_degree;i++){
			delete coeffs[i];
		}
		
		return res;
	}
	
	mpz_class ToPositiveConstant(std::pair<int,int> format, mpfr_t x)
	{
		if(mpfr_sgn(x)<=0)
			throw std::invalid_argument("ToPositiveConstant - non positive numbers not supported.");
		if(!mpfr_regular_p(x))
			throw std::invalid_argument("ToPositiveConstant - only regular numbers are supported.");
		
		std::string s=fp2bin(x, format.first, format.second);
		mpz_class acc;
		for(unsigned i=0;i<s.size();i++){
			acc = (acc<<1) + ((s[i]=='1')?1:0);
		}
		return acc;
	}
	
	StaticQuantiser *make_static_quantiser(Target *target, map<string, double> inputDelays = map<string, double>())
	{
		std::pair<int,int> rep=m_range.GetFloatTypeEnclosingDomain();
		
		std::vector<mpz_class> boundaries;
		
		segment_it_t curr=m_range.m_segments.begin();
		while(curr!=m_range.m_segments.end()){
			boundaries.push_back(ToPositiveConstant(rep, curr->domainStart));
			++curr;
		}
		boundaries.push_back(ToPositiveConstant(rep, boost::prior(curr)->domainFinish)+1);	// This won't actually be used by the quantiser
		
		StaticQuantiser *sq=new StaticQuantiser(target, (2+rep.first+rep.second), boundaries, inputDelays);
		
		return sq;
	}
	
	/*
	unsigned eval_concrete(mpfr_t res, mpfr_t x)
	{
		int i;
		for(i=0;i<m_concretePartition.size();i++){
			if(mpfr_lessequal_p(x, m_concretePartition[i]))
				break;
		}
		if(i==m_concretePartition.size())
			throw std::invalid_argument("eval_concrete - out of range.");
		
		throw std::invalid_argument("eval_concrete - Not implemented for now.");
	}
	
	void dump_concrete(FILE *dst)
	{
		for(int i=0;i<m_concretePolys.size();i++){
			m_concretePolys[i].dump(dst, "  ");
		}
		fprintf(stderr, "Total polys=%u\n", m_concretePolys.size());
	}
	*/
	
	int ulps(mpfr_t got, mpfr_t want)
	{
		if(mpfr_equal_p(got, want))
			return 0;
		
		int dir=mpfr_greater_p(want, got) ? +1 : -1;
		
		mpfr_t tmp;
		mpfr_init_copy(tmp, got);
		int res=0;
		
		while(true){
			++res;
			if(dir==+1){
				mpfr_nextabove(tmp);
				if(mpfr_lessequal_p(want, tmp))
					break;
			}else{
				mpfr_nextbelow(tmp);
				if(mpfr_greaterequal_p(want, tmp))
					break;
			}
		}
			
		mpfr_clear(tmp);
		
		return res;
	}
	
	/*
	void exhaust_concrete()
	{
		mpfr_t x, got, want, actual, err, tol;
		
		mpfr_init2(got, m_range.m_rangeWF);
		mpfr_init2(want, m_range.m_rangeWF);
		mpfr_init2(actual, getToolPrecision());
		mpfr_init2(err, getToolPrecision());
		mpfr_init2(tol, getToolPrecision());
		mpfr_init_copy(x, m_range.m_segments.begin()->domainStart);
		
		mpfr_set_si_2exp(tol, 1, -m_range.m_rangeWF, MPFR_RNDN);
		
		int start=time(NULL);
		int delta_print=2;
		int next_print=start+delta_print;
		
		while(mpfr_lessequal_p(x, boost::prior(m_range.m_segments.end())->domainFinish)){
			unsigned seg=eval_concrete(got, x);
			m_range.m_function.eval(actual, x);
			mpfr_set(want, actual, MPFR_RNDN);
			
			mpfr_sub(err, got, actual, MPFR_RNDN);
			mpfr_div(err, err, actual, MPFR_RNDN);
			mpfr_abs(err, err, MPFR_RNDN);
			
			int u=ulps(got, want);
			
			if(u>1){
				mpfr_fprintf(stdout, "%u, %Rg, %Rg, %Rg, %Rg, %Rg, %s, %d\n", seg, x, got, want, actual, err, mpfr_greater_p(err, tol) ? "FAIL" : "pass", u);
			}
			
			mpfr_nextabove(x);
			
			if(time(NULL) > next_print){
				mpfr_fprintf(stderr, "  curr=%Rg, range=[%Rg,%Rg]\n", x, m_range.m_segments.begin()->domainStart, boost::prior(m_range.m_segments.end())->domainFinish);
				delta_print=std::min(60, delta_print*2);
				next_print=time(NULL)+delta_print;
			}
		}
		
		mpfr_clears(x, got, want, (mpfr_ptr)0);
	}
	*/
};

}; // float_approx
};	// random
}; // flopco

#endif