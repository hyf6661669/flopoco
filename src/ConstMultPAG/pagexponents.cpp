#include "pagexponents.hpp"
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <sstream>

#include "fundamental_extended.hpp"
#include "rpag/log2_64.h"
#include "rpag/compute_successor_set.h"

using namespace std;

#if 0

bool getExponents(int_t a,int_t b, int_t c, int *eA, int *eB, int *signA, int *signB)
{
  cout << " getExponents(" << a << "," << b << "," << c << ")"; flush(cout);
  bool exponentsFound=false;
  int e; //exponent

  //check left shift solutions:
  if(fundamental(c-a,&e) == b)
  {
    //c = a + 2^eB b => fundamental(c-a) == b
    *eA=0; *eB=e; *signA=1; *signB=1; exponentsFound=true;
  }
  if(fundamental(c+a,&e) == b)
  {
    //c = -a + 2^eB b => fundamental(c-a) == b
    *eA=0; *eB=e; *signA=-1; *signB=1; exponentsFound=true;
  }
  else if(-fundamental(c-a,&e) == b)
  {
    //c = a - 2^eB b => -fundamental(c-a) == b
    *eA=0; *eB=e; *signA=1; *signB=-1; exponentsFound=true;
  }
  else if(fundamental(c-b,&e) == a)
  {
    //c = 2^eA a + b => fundamental(c-b) == a
    *eA=e; *eB=0; *signA=1; *signB=1; exponentsFound=true;
  }
  else if(fundamental(b-c,&e) == a)
  {
    //c = -2^eA a + b => fundamental(c-b) == a
    *eA=e; *eB=0; *signA=-1; *signB=1; exponentsFound=true;
  }
  else if(fundamental(c+b,&e) == a)
  {
    //c = 2^eA a - b => fundamental(c+b) == a
    *eA=e; *eB=0; *signA=1; *signB=-1; exponentsFound=true;
  }
  //check right shift solutions:
  else if(fundamental(a+b) == c)
  {
    //c = 2^-eA a + 2^-eB b (with eA==eB)
    *eA = -log2c_64((a+b)/c); *eB = *eA; *signA = 1; *signB = 1;
    exponentsFound=true;
  }
  else if(fundamental(abs(a-b)) == c)
  {
    //|c| = |2^-eA a - 2^-eB b| (with eA==eB)
    *eA = -log2c_64(abs(a-b)/c); *eB = *eA;

    if(a>b)
    {
      *signA = 1;
      *signB = -1;
    }
    else
    {
      *signA = -1;
      *signB = 1;
    }
    exponentsFound=true;
  }

  if(exponentsFound)
  {
     cout << " : eA=" << *eA << ", eB=" << *eB << ", signA=" << *signA << ", signB=" << *signB << endl; flush(cout);


    //check results:
    if(*eA >= 0)
    {
      if(!((*signA * (a << (*eA))) + *signB * (b << (*eB)) == c))
      {
        cout << "Error: Exponens invalid: (" << *signA << ")*" << a << "*2^" << (*eA) << " + (" << *signB << ")*" << b << "*2^" << (*eB) << "=" << (*signA * (a << (*eA))) + *signB * (b << (*eB)) << " != " << c << endl;
        exit(-1);
      }
//      assert((*signA * (a << (*eA))) + *signB * (b << (*eB)) == c);
      return true;
    }
    else
    {
      if(!(((*signA * a + *signB * b) >> abs(*eA)) == c))
      {
        cout << "Error: Exponens invalid: " << ((*signA * a + *signB * b) >> abs(*eA)) << " != " << c << endl;
        exit(-1);
      }
//      assert(((*signA * a + *signB * b) >> abs(*eA)) == c);
      return true;
    }
  }
  else
  {
//    cout << "no solution found for " << a << "," << b << "," << c << "!" << endl;
//    exit(-1);
    return false;
  }
}

int compute_k_max(int_t &r1, int_t &r2, int &c_max)
{
  return log2c_64((int) ceil(((double)c_max+r1)/((double)r2)));
}

int compute_k_max(int_t &r1, int_t &r2, int_t &r3, int &c_max)
{
  int_t r1_ = fundamental(r1);
  int_t r2_ = fundamental(r2);
  int_t r3_ = fundamental(r3);
  //set r1 to the maximum value
  { //to make r1 > r2 > r3
    int tmp;
    if(r3_ > r2_)
    {
        tmp = r2_;
        r2_ = r3_;
        r3_ = tmp;
    }
    if(r2_ > r1_)
    {
        tmp = r1_;
        r1_ = r2_;
        r2_ = tmp;
    }
    if(r3_ > r2_)
    {
        tmp = r2_;
        r2_ = r3_;
        r3_ = tmp;
    }
  }
  if(r1_ > c_max) return 0;

  int k_max = compute_k_max(r1_,r3_,c_max);
  if(k_max < 0)
  {
    std::cout << "ERROR: k_max < 0 " << std::endl;
    exit(-1);
    return 0;
  }
  else
  {
    return k_max;
  }
}

bool getExponents(int_t &a, int_t &b, int_t &c, int_t &w, int *eA, int *eB, int *eC, int *signA, int *signB, int *signC, int c_max)
{
  cout << " getExponents(" << a << "," << b << "," << c << "," << w << ")"; flush(cout);
  int_t Z;
  int k_max  = compute_k_max(a,b,c,c_max);
  bool found = false;
  bool neg   = false;
  int permut = 0;

  for(int r_shift = 0; r_shift <= k_max; ++r_shift)
  {
    for(permut = 0; permut <= 2; ++permut)// right shifts can only be detected between a and b. If there is one between c and another thy have to be switched.
    {
      switch (permut)
      {
        case 0: break;//initial set. test for right shift beetween the original a and b.
        case 1: swap(a,c); swap(*eA,*eC); swap(*signA,*signC); break; // test for right shift beetween the original c and b.
        case 2: swap(b,c); swap(*eB,*eC); swap(*signB,*signC); break; // test for right shift beetween the original c and a.
        default: std::cout << "error in getExponents permut is not 0,1 or 2" << endl; exit(-1);
      }

      for ((*eC)=0; (*eC) <= k_max; ++(*eC))
      {
        for(int sign_loop=0; sign_loop <= 1; ++sign_loop)
        {
          switch (sign_loop)
          {
            case 0: (*signC) =  1; break;
            case 1: (*signC) = -1; break;
            default: std::cout << "error in getExponents sign_loop is not 0 or 1" << endl; exit(-1);
          }

          //Z = w - ((c << (*eC)) * (*signC));
          Z = (w << (r_shift)) - ((c << (*eC)) * (*signC));

          if (Z < 0)// this have to be different for the rpag_vector
          {
            Z = -Z;//the getExponent function does not support negative values.
            neg = true;
          }
          else
          {
            neg = false;
          }

          //std::cout << "Z= " << Z << "= " << d << " - " << (*signC) << " * " << c << " * 2^" << (*eC) << " " << neg << endl;
          if(Z % 2 == 0)// if Z is even
          {
            int_t Z_odd;
            int eZ_odd;
            Z_odd = fundamental_count(Z, eZ_odd);
            found = getExponents(a,b,Z_odd,eA,eB,signA,signB);
            cout << "found1=" << found << endl;

            if(neg)//the getExponent function does not support negativ values.
            {
              (*signA) = -(*signA);
              (*signB) = -(*signB);
            }
            (*eA) = (*eA) + eZ_odd;
            (*eB) = (*eB) + eZ_odd;
          }
          else // if Z is odd
          {
            found = getExponents(a,b,Z,eA,eB,signA,signB);
            cout << "found2=" << found << endl;

            if(neg)//the getExponent function does not support negativ values.
            {
              (*signA) = -(*signA);
              (*signB) = -(*signB);
            }
          }

          if(((*eA) > k_max) || ((*eB) > k_max) || ((*eC) > k_max)){found = false;}
          if(found == true)
          {
              cout << "a=" << a << ", b=" << b  << ", c=" << c << ", Z=" << Z << ", r_shift=" << r_shift << ", eA=" << (*eA) << ", eB=" << (*eB) << ", eC=" << (*eC) << ", signA=" << (*signA) << ", signB=" << (*signB) << ", signC=" << (*signC) << endl; flush(cout);

              (*eA) -= r_shift;
              (*eB) -= r_shift;
              (*eC) -= r_shift;
              switch (permut)
              {
                case 2: swap(b,c); swap(*eB,*eC); swap(*signB,*signC); // switch b&c and a&c to create the original set -> break necessary??
                case 1: swap(a,c); swap(*eA,*eC); swap(*signA,*signC); // switch         a&c to create the original set -> break necessary??
                case 0: break;                                         // initial set.                     no switching
                default: std::cout << "error in getExponents permut is not 0,1 or 2" << endl; exit(-1);
              }
              cout << " : eA=" << *eA << ", eB=" << *eB << ", eC=" << *eC << ", signA=" << *signA << ", signB=" << *signB << ", signC=" << *signC << endl; flush(cout);
              return true;
          }
        }
      }
    }
  }

  //the variable's has to be in the correct order.
  swap(b,c); swap(*eB,*eC); swap(*signB,*signC);
  swap(a,c); swap(*eA,*eC); swap(*signA,*signC);

  return false;
}



#else

int compute_k_max(int_t &r1, int_t &r2, int_t &c_max)
{
  return log2c_64((int_t) ceil(((double)c_max+r1)/((double)r2)));
}
int compute_k_max(int_t &r1, int_t &r2, int_t &r3, int_t &c_max)
{
  int_t r1_ = fundamental(r1);
  int_t r2_ = fundamental(r2);
  int_t r3_ = fundamental(r3);
  //set r1 to the maximum value
  { //to make r1 > r2 > r3
    int_t tmp;
    if(r3_ > r2_)
    {
        tmp = r2_;
        r2_ = r3_;
        r3_ = tmp;
    }
    if(r2_ > r1_)
    {
        tmp = r1_;
        r1_ = r2_;
        r2_ = tmp;
    }
    if(r3_ > r2_)
    {
        tmp = r2_;
        r2_ = r3_;
        r3_ = tmp;
    }
  }
  if(r1_ > c_max) return 0;

  int k_max;
  if(r3_ == 0)
    k_max = compute_k_max(r1_,r2_,c_max);
  else
    k_max = compute_k_max(r1_,r3_,c_max);

  if(k_max < 0)
  {
    std::cout << "ERROR: k_max < 0 " << std::endl;
    exit(-1);
    return 0;
  }
  else
  {
    return k_max;
  }
}

//template <class T>
//bool getExponents(T &a, T &b, T &w, int *eA, int *eB, int *signA, int *signB)
//bool getExponents(int_t &a, int_t &b, int_t &w, int *eA, int *eB, int *signA, int *signB)
bool getExponents(int_t a, int_t b, int_t w, int *eA, int *eB, int *signA, int *signB)
{
  bool exponentsFound=false;
  int e; //exponent

  //check left shift solutions:
  if(fundamental_count(w-a,e) == b)
  {
//    IF_VERBOSE(8)  cout << "case 01" << endl;
    //c = a + 2^eB b => fundamental(c-a) == b
    *eA=0; *eB=e; *signA=1; *signB=1; exponentsFound=true;
  }
  else if(fundamental_count(w-b,e) == a)
  {
//    IF_VERBOSE(8)  cout << "case 02" << endl;
    //c = 2^eA a + b => fundamental(c-b) == a
    *eA=e; *eB=0; *signA=1; *signB=1; exponentsFound=true;
  }
  else if(fundamental_count(w+a,e) == b)
  {
//    IF_VERBOSE(8)  cout << "case 03" << endl;
    //c = -a + 2^eB b => fundamental(c+a) == b
    *eA=0; *eB=e; *signA=-1; *signB=1; exponentsFound=true;
  }
  else if(-fundamental_count(w-a,e) == b)
  {
//    IF_VERBOSE(8)  cout << "case 04" << endl;
    //c = a - 2^eB b => -fundamental(c-a) == b
    *eA=0; *eB=e; *signA=1; *signB=-1; exponentsFound=true;
  }
  else if(fundamental_count(b-w,e) == a)
  {
//    IF_VERBOSE(8)  cout << "case 05" << endl;
    //c = -2^eA a + b => fundamental(b-c) == a
    *eA=e; *eB=0; *signA=-1; *signB=1; exponentsFound=true;
  }
  else if(fundamental_count(w+b,e) == a)
  {
//    IF_VERBOSE(8)  cout << "case 06" << endl;
    //c = 2^eA a - b => fundamental(c+b) == a
    *eA=e; *eB=0; *signA=1; *signB=-1; exponentsFound=true;
  }
  //check right shift solutions:
  else if(fundamental_count(a+b,e) == w)
  {
//    IF_VERBOSE(8)  cout << "case 07" << endl;
    //c = 2^-eA a + 2^-eB b (with eA==eB)
    *eA = -e; *eB = *eA; *signA = 1; *signB = 1;
    exponentsFound=true;
  }
  else if(fundamental_count((a-b),e) == w)
  {
//    IF_VERBOSE(8)  cout << "case 08" << endl;
    //c = 2^-eA a - 2^-eB b (with eA==eB)
    *eA = -e; *eB = *eA;
    *signA = 1; *signB = -1;
    exponentsFound=true;
  }
  else if(fundamental_count((b-a),e) == w)
  {
//    IF_VERBOSE(8)  cout << "case 09" << endl;
    //c = -2^-eA a + 2^-eB b (with eA==eB)
    *eA = -e; *eB = *eA;
    *signA = -1; *signB = 1;
    exponentsFound=true;
  }

//  cout << " getExponents(" << a << "," << b << "," << w << ") : exponentsFound=" << exponentsFound << ", eA=" << *eA << ", eB=" << *eB << ", signA=" << *signA << ", signB=" << *signB << endl; flush(cout);
  if(exponentsFound)
  {
//    cout << " getExponents(" << a << "," << b << "," << w << ") : eA=" << *eA << ", eB=" << *eB << ", signA=" << *signA << ", signB=" << *signB << endl; flush(cout);

     //check results:
    if(*eA >= 0)
    {
      if(!((*signA * (a << (*eA))) + *signB * (b << (*eB)) == w))
      {
/*
          cout << "w=" << w << endl;
        cout << "(*signA * (a << (*eA))) + *signB * (b << (*eB))=" << (*signA * (a << (*eA))) + (*signB * (b << (*eB))) << endl;
        cout << "(a << (*eA))=" << (a << (*eA)) << endl;
        cout << "(*signA * (a << (*eA)))=" << (*signA * (a << (*eA))) << endl;
        cout << "(b << (*eB))==" << (b << (*eB)) << endl;
        cout << "(*signB * (b << (*eB)))=" << (*signB * (b << (*eB))) << endl;
*/
        cout << "Error: Exponens invalid: (" << *signA << ")*" << a << "*2^" << (*eA) << " + (" << *signB << ")*" << b << "*2^" << (*eB) << "=" << (*signA * (a << (*eA))) + *signB * (b << (*eB)) << " != " << w << endl;
        exit(-1);
      }
      return true;
    }
    else
    {
      if(!(((*signA * a + *signB * b) >> abs(*eA)) == w))
      {
        cout << "Error: Exponens invalid: " << ((*signA * a + *signB * b) >> abs(*eA)) << " != " << w << endl;
        exit(-1);
      }
      return true;
    }
  }
  else
  {
    // this case is possible until searching for a adder graph
    return false;
  }
}

//template<typename T>
//bool getExponents(T &a, T &b, T &c, T &w, int *eA, int *eB, int *eC, int *signA, int *signB, int *signC, int_t c_max, bool ternary_sign_filter)
bool getExponents(int_t &a, int_t &b, int_t &c, int_t &w, int *eA, int *eB, int *eC, int *signA, int *signB, int *signC, int_t c_max, bool ternary_sign_filter)
{
  int_t Z;
  int k_max  = compute_k_max(a,b,c,c_max);
  bool found = false;
  bool neg   = false;
  int permut = 0;

  for(int r_shift = 0; r_shift <= k_max; ++r_shift)
  {
    for(permut = 0; permut <= 2; ++permut)// right shifts can only be detected between a and b. If ther is one between c and another thy have to be switched.
    {
      switch (permut)
      {
        case 0: break;//initial set. test for right shift beetween the original a and b.
        case 1: swap(a,c); swap(*eA,*eC); swap(*signA,*signC); break; // test for right shift beetween the original c and b.
        case 2: swap(b,c); swap(*eB,*eC); swap(*signB,*signC); break; // test for right shift beetween the original c and a.
        default: std::cout << "error in getExponents permut is not 0,1 or 2" << endl; exit(-1);
      }

      for ((*eC)=0; (*eC) <= k_max; ++(*eC))
      {
        for(int sign_loop=0; sign_loop <= 1; ++sign_loop)
        {
          switch (sign_loop)
          {
            case 0: (*signC) =  1; break;
            case 1: (*signC) = -1; break;
            default: std::cout << "error in getExponents sign_loop is not 0 or 1" << endl; exit(-1);
          }

          //Z = w - ((c << (*eC)) * (*signC));
          Z = (w << (r_shift)) - ((c << (*eC)) * (*signC));
//          cout << "Z=" << Z << endl;

          if (Z < 0)// this have to be different for the rpag_vector
          {
            Z = -Z;//the getExponent function does not support negativ values.
            neg = true;
          }
          else
          {
            neg = false;
          }

          {
            //std::cout << "Z= " << Z << "= " << d << " - " << (*signC) << " * " << c << " * 2^" << (*eC) << " " << neg << endl;
            if(Z % 2 == 0)// if Z is evan
            {
              int_t Z_odd;
              int eZ_odd;
              Z_odd = fundamental_count(Z, eZ_odd);
//              cout << "Z_odd=" << Z_odd << endl;
              found = getExponents(a,b,Z_odd,eA,eB,signA,signB);

              if(neg)//the getExponent function does not support negativ values.
              {
                (*signA) = -(*signA);
                (*signB) = -(*signB);
              }
              (*eA) = (*eA) + eZ_odd;
              (*eB) = (*eB) + eZ_odd;
            }
            else // if Z is odd
            {
              found = getExponents(a,b,Z,eA,eB,signA,signB);
              if(neg)//the getExponent function does not support negativ values.
              {
                (*signA) = -(*signA);
                (*signB) = -(*signB);
              }
            }
          }
          if(((*eA) > k_max) || ((*eB) > k_max) || ((*eC) > k_max)){found = false;}
          if(found == true)
          {
              (*eA) -= r_shift;
              (*eB) -= r_shift;
              (*eC) -= r_shift;
              switch (permut)
              {
                case 2: swap(b,c); swap(*eB,*eC); swap(*signB,*signC); // switch b&c and a&c to create the original set
                case 1: swap(a,c); swap(*eA,*eC); swap(*signA,*signC); // switch         a&c to create the original set
                case 0: break;                                         // initial set.                     no switching
                default: std::cout << "error in getExponents permut is not 0,1 or 2" << endl; exit(-1);
              }
              return true;
          }
        }
      }
    }
  }

  //the variable's has to be in the correct order.
  swap(b,c); swap(*eB,*eC); swap(*signB,*signC);
  swap(a,c); swap(*eA,*eC); swap(*signA,*signC);

  return false;
}

#endif

bool computeAdderGraphTernary(set<int> &nof_set, set<int> &coefficient_set, string& adderGraphString)
{
    int eA, eB, eC;
    int signA, signB, signC;
	int c_max = 1LL<<23;

    bool expFound=false;
    int_t a, b, c;

/*
    //test:
    int_t coeffTest=2815;
    a=1;
    b=1;
    c=3;
    bool found = getExponents(a, b, c, coeffTest, &eA, &eB, &eC, &signA, &signB, &signC, c_max);
    cout << "test 1:" << found << " : " << coeffTest << " == " << signA << "*" << a << "*2^" << eA << " + " << signB << "*" << b << "*2^" << eB << " + " << signC << "*" << c << "*2^" << eC << endl;

    c=1;
    found = getExponents(a, b, c, coeffTest, &eA, &eB, &eC, &signA, &signB, &signC, c_max);
    cout << "test 2:" << found << " : " << coeffTest << " == " << signA << "*" << a << "*2^" << eA << " + " << signB << "*" << b << "*2^" << eB << " + " << signC << "*" << c << "*2^" << eC << endl;

    a=0;
    b=1;
    c=1;
    found = getExponents(a, b, c, coeffTest, &eA, &eB, &eC, &signA, &signB, &signC, c_max);
    cout << "test 3:" << found << " : " << coeffTest << " == " << signA << "*" << a << "*2^" << eA << " + " << signB << "*" << b << "*2^" << eB << " + " << signC << "*" << c << "*2^" << eC << endl;

    exit(-1);
*/

    stringstream solStr;

	set<int> realized_set;              //coefficients that are already realized
	set<int> new_realized_elements_set; //coefficients that are realized in one outer iteration
	realized_set.insert(0); //to cover two-input adders
    realized_set.insert(1);

    copy(coefficient_set.begin(), coefficient_set.end(), inserter(nof_set, nof_set.begin()));

/*
	cout << "nofs={";
	for(set<int>::iterator coeffIt = nof_set.begin(); coeffIt != nof_set.end(); ++coeffIt)
	{
		cout << *coeffIt << " ";
	}
	cout << "}" << endl;
*/

	do
	{
//		copy(new_realized_elements_set.begin(), new_realized_elements_set.end(), inserter(realized_set, realized_set.begin()));

		set<int>::iterator lastCoeffIt = nof_set.end();
		lastCoeffIt--;
		for(set<int>::iterator coeffIt = nof_set.begin(); coeffIt != nof_set.end(); ++coeffIt)
		{
			int_t coeff = *coeffIt;
			int_t coeffOdd = fundamental(coeff);
			for(set<int>::iterator nof1it = realized_set.begin(); nof1it != realized_set.end(); ++nof1it)
			{
				for(set<int>::iterator nof2it = nof1it; nof2it != realized_set.end(); ++nof2it)
				{
					if(*nof2it == 0) continue;
					for(set<int>::iterator nof3it = nof2it; nof3it != realized_set.end(); ++nof3it)
					{
						a = *nof1it;
						b = *nof2it;
						c = *nof3it;
						if(c == 0) continue;
//						cout << "trying " << coeff << " with " << a << "," << b << "," << c << endl; flush(cout);
						expFound = getExponents(a, b, c, coeffOdd, &eA, &eB, &eC, &signA, &signB, &signC, c_max);
						if(expFound) cout << coeffOdd << " == " << signA << "*" << a << "*2^" << eA << " + " << signB << "*" << b << "*2^" << eB << " + " << signC << "*" << c << "*2^" << eC << endl;
						if(expFound) break;
					}
					if(expFound) break;
				}
				if(expFound) break;
			}

			if(expFound)
			{
				realized_set.insert(coeffOdd);
				nof_set.erase(coeffIt);

				string signAchar = signA>0 ? "" : "-";
				string signBchar = signB>0 ? "" : "-";
				string signCchar = signC>0 ? "" : "-";

				int coeffStage=1;
				if((coefficient_set.find(coeffOdd) != coefficient_set.end()) && (coeffOdd == coeffOdd)) coeffStage=2; //All (even) output coefficients are defined to be in stage 2 (workaround until node type 'O' is supported)


				if(coeffOdd != signA*(a<<eA) + signB*(b<<eB) + signC*(c<<eC))
				{
					cerr << "Error: " << coeffOdd << " != " << signAchar << a << "*2^" << eA << " + " << signBchar << b << "*2^" << eB << " + " << signCchar << c << "*2^" << eC << endl;
					exit(-1);
				}

				solStr << "{" << coeffOdd << "," << coeffStage << ",";
				if(a != 0) solStr << signAchar << a << "," << (a == 1?0:1) << "," << eA << ",";
				if(b != 0) solStr << signBchar << b << "," << (b == 1?0:1) << "," << eB << ",";
				if(c != 0) solStr << signCchar << c << "," << (c == 1?0:1) << "," << eC;
				solStr << "}";

				if(!nof_set.empty()) solStr << ",";

				break;
			}
		}
	} while(expFound && !nof_set.empty());

	if(!nof_set.empty())
	{
		cerr << "Error in computeAdderGraphTernary(): No valid solution found" << endl;
		exit(-1);
	}


    adderGraphString = solStr.str();
}

