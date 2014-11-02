#ifndef random_systems_cubic_system_hpp
#define random_systems_cubic_system_hpp

#include <utility>
#include <iterator>
#include <math.h>

namespace flopoco
{
namespace random
{
namespace systems
 {

template<class T,class TACC=T>
class CubicSystem{
private:
  T ss[13];
  T tmom[5];
  static T square(const T &x) { return x*x; }
public:
  typedef T real_t;

  int arity() const { return 4; };

  CubicSystem(  const T *_ss ,   const T *_tmom )
  {
    std::copy(_ss, _ss+13, ss);
    std::copy(_tmom, _tmom+5, tmom);
  }

  void DefaultGuess(T *params) const{
    params[0]=0; // c0
    params[1]=1; // c1
    params[2]=0; // c2
    params[3]=0; // c3
  }

T metric(const T *parameters) const {
  T c0p1 = parameters[0];
  T c1p1 = parameters[1];
  T c2p1 = parameters[2];
  T c3p1 = parameters[3];
  TACC root_0 = c1p1*ss[1];
  root_0 += c2p1*ss[2];
  root_0 += c3p1*ss[3];
  root_0 += c0p1;
  root_0 += -tmom[1];
  T c0p2 = c0p1*c0p1;
  T c1p2 = c1p1*c1p1;
  T c2p2 = c2p1*c2p1;
  T c3p2 = c3p1*c3p1;
  TACC root_1 = 2*c0p1*c1p1*ss[1];
  root_1 += 2*c0p1*c2p1*ss[2];
  root_1 += 2*c0p1*c3p1*ss[3];
  root_1 += 2*c1p1*c2p1*ss[3];
  root_1 += 2*c1p1*c3p1*ss[4];
  root_1 += 2*c2p1*c3p1*ss[5];
  root_1 += c1p2*ss[2];
  root_1 += c2p2*ss[4];
  root_1 += c3p2*ss[6];
  root_1 += c0p2;
  root_1 += -tmom[2];
  T c0p3 = c0p2*c0p1;
  T c1p3 = c1p2*c1p1;
  T c2p3 = c2p2*c2p1;
  T c3p3 = c3p2*c3p1;
  TACC root_2 = 6*c0p1*c1p1*c2p1*ss[3];
  root_2 += 6*c0p1*c1p1*c3p1*ss[4];
  root_2 += 6*c0p1*c2p1*c3p1*ss[5];
  root_2 += 6*c1p1*c2p1*c3p1*ss[6];
  root_2 += 3*c0p1*c1p2*ss[2];
  root_2 += 3*c0p1*c2p2*ss[4];
  root_2 += 3*c0p1*c3p2*ss[6];
  root_2 += 3*c0p2*c1p1*ss[1];
  root_2 += 3*c0p2*c2p1*ss[2];
  root_2 += 3*c0p2*c3p1*ss[3];
  root_2 += 3*c1p1*c2p2*ss[5];
  root_2 += 3*c1p1*c3p2*ss[7];
  root_2 += 3*c1p2*c2p1*ss[4];
  root_2 += 3*c1p2*c3p1*ss[5];
  root_2 += 3*c2p1*c3p2*ss[8];
  root_2 += 3*c2p2*c3p1*ss[7];
  root_2 += c1p3*ss[3];
  root_2 += c2p3*ss[6];
  root_2 += c3p3*ss[9];
  root_2 += c0p3;
  root_2 += -tmom[3];
  T c0p4 = c0p3*c0p1;
  T c1p4 = c1p3*c1p1;
  T c2p4 = c2p3*c2p1;
  T c3p4 = c3p3*c3p1;
  TACC root_3 = 24*c0p1*c1p1*c2p1*c3p1*ss[6];
  root_3 += 12*c0p1*c1p1*c2p2*ss[5];
  root_3 += 12*c0p1*c1p1*c3p2*ss[7];
  root_3 += 12*c0p1*c1p2*c2p1*ss[4];
  root_3 += 12*c0p1*c1p2*c3p1*ss[5];
  root_3 += 12*c0p1*c2p1*c3p2*ss[8];
  root_3 += 12*c0p1*c2p2*c3p1*ss[7];
  root_3 += 12*c0p2*c1p1*c2p1*ss[3];
  root_3 += 12*c0p2*c1p1*c3p1*ss[4];
  root_3 += 12*c0p2*c2p1*c3p1*ss[5];
  root_3 += 12*c1p1*c2p1*c3p2*ss[9];
  root_3 += 12*c1p1*c2p2*c3p1*ss[8];
  root_3 += 12*c1p2*c2p1*c3p1*ss[7];
  root_3 += 4*c0p1*c1p3*ss[3];
  root_3 += 4*c0p1*c2p3*ss[6];
  root_3 += 4*c0p1*c3p3*ss[9];
  root_3 += 6*c0p2*c1p2*ss[2];
  root_3 += 6*c0p2*c2p2*ss[4];
  root_3 += 6*c0p2*c3p2*ss[6];
  root_3 += 4*c0p3*c1p1*ss[1];
  root_3 += 4*c0p3*c2p1*ss[2];
  root_3 += 4*c0p3*c3p1*ss[3];
  root_3 += 4*c1p1*c2p3*ss[7];
  root_3 += 4*c1p1*c3p3*ss[10];
  root_3 += 6*c1p2*c2p2*ss[6];
  root_3 += 6*c1p2*c3p2*ss[8];
  root_3 += 4*c1p3*c2p1*ss[5];
  root_3 += 4*c1p3*c3p1*ss[6];
  root_3 += 4*c2p1*c3p3*ss[11];
  root_3 += 6*c2p2*c3p2*ss[10];
  root_3 += 4*c2p3*c3p1*ss[9];
  root_3 += c1p4*ss[4];
  root_3 += c2p4*ss[8];
  root_3 += c3p4*ss[12];
  root_3 += c0p4;
  root_3 += -tmom[4];
  return sqrt(root_0*root_0+root_1*root_1+root_2*root_2+root_3*root_3);
}
T metric_and_gradient(const T *parameters, T *gradient) const {
  T c0p1 = parameters[0];
  T c1p1 = parameters[1];
  T c2p1 = parameters[2];
  T c3p1 = parameters[3];
  TACC sys_part_0 = c1p1*ss[1];
  sys_part_0 += c2p1*ss[2];
  sys_part_0 += c3p1*ss[3];
  sys_part_0 += c0p1;
  sys_part_0 += -tmom[1];
  T c0p2 = c0p1*c0p1;
  T c1p2 = c1p1*c1p1;
  T c2p2 = c2p1*c2p1;
  T c3p2 = c3p1*c3p1;
  TACC sys_part_1 = 2*c0p1*c1p1*ss[1];
  sys_part_1 += 2*c0p1*c2p1*ss[2];
  sys_part_1 += 2*c0p1*c3p1*ss[3];
  sys_part_1 += 2*c1p1*c2p1*ss[3];
  sys_part_1 += 2*c1p1*c3p1*ss[4];
  sys_part_1 += 2*c2p1*c3p1*ss[5];
  sys_part_1 += c1p2*ss[2];
  sys_part_1 += c2p2*ss[4];
  sys_part_1 += c3p2*ss[6];
  sys_part_1 += c0p2;
  sys_part_1 += -tmom[2];
  T c0p3 = c0p2*c0p1;
  T c1p3 = c1p2*c1p1;
  T c2p3 = c2p2*c2p1;
  T c3p3 = c3p2*c3p1;
  TACC sys_part_2 = 6*c0p1*c1p1*c2p1*ss[3];
  sys_part_2 += 6*c0p1*c1p1*c3p1*ss[4];
  sys_part_2 += 6*c0p1*c2p1*c3p1*ss[5];
  sys_part_2 += 6*c1p1*c2p1*c3p1*ss[6];
  sys_part_2 += 3*c0p1*c1p2*ss[2];
  sys_part_2 += 3*c0p1*c2p2*ss[4];
  sys_part_2 += 3*c0p1*c3p2*ss[6];
  sys_part_2 += 3*c0p2*c1p1*ss[1];
  sys_part_2 += 3*c0p2*c2p1*ss[2];
  sys_part_2 += 3*c0p2*c3p1*ss[3];
  sys_part_2 += 3*c1p1*c2p2*ss[5];
  sys_part_2 += 3*c1p1*c3p2*ss[7];
  sys_part_2 += 3*c1p2*c2p1*ss[4];
  sys_part_2 += 3*c1p2*c3p1*ss[5];
  sys_part_2 += 3*c2p1*c3p2*ss[8];
  sys_part_2 += 3*c2p2*c3p1*ss[7];
  sys_part_2 += c1p3*ss[3];
  sys_part_2 += c2p3*ss[6];
  sys_part_2 += c3p3*ss[9];
  sys_part_2 += c0p3;
  sys_part_2 += -tmom[3];
  T c0p4 = c0p3*c0p1;
  T c1p4 = c1p3*c1p1;
  T c2p4 = c2p3*c2p1;
  T c3p4 = c3p3*c3p1;
  TACC sys_part_3 = 24*c0p1*c1p1*c2p1*c3p1*ss[6];
  sys_part_3 += 12*c0p1*c1p1*c2p2*ss[5];
  sys_part_3 += 12*c0p1*c1p1*c3p2*ss[7];
  sys_part_3 += 12*c0p1*c1p2*c2p1*ss[4];
  sys_part_3 += 12*c0p1*c1p2*c3p1*ss[5];
  sys_part_3 += 12*c0p1*c2p1*c3p2*ss[8];
  sys_part_3 += 12*c0p1*c2p2*c3p1*ss[7];
  sys_part_3 += 12*c0p2*c1p1*c2p1*ss[3];
  sys_part_3 += 12*c0p2*c1p1*c3p1*ss[4];
  sys_part_3 += 12*c0p2*c2p1*c3p1*ss[5];
  sys_part_3 += 12*c1p1*c2p1*c3p2*ss[9];
  sys_part_3 += 12*c1p1*c2p2*c3p1*ss[8];
  sys_part_3 += 12*c1p2*c2p1*c3p1*ss[7];
  sys_part_3 += 4*c0p1*c1p3*ss[3];
  sys_part_3 += 4*c0p1*c2p3*ss[6];
  sys_part_3 += 4*c0p1*c3p3*ss[9];
  sys_part_3 += 6*c0p2*c1p2*ss[2];
  sys_part_3 += 6*c0p2*c2p2*ss[4];
  sys_part_3 += 6*c0p2*c3p2*ss[6];
  sys_part_3 += 4*c0p3*c1p1*ss[1];
  sys_part_3 += 4*c0p3*c2p1*ss[2];
  sys_part_3 += 4*c0p3*c3p1*ss[3];
  sys_part_3 += 4*c1p1*c2p3*ss[7];
  sys_part_3 += 4*c1p1*c3p3*ss[10];
  sys_part_3 += 6*c1p2*c2p2*ss[6];
  sys_part_3 += 6*c1p2*c3p2*ss[8];
  sys_part_3 += 4*c1p3*c2p1*ss[5];
  sys_part_3 += 4*c1p3*c3p1*ss[6];
  sys_part_3 += 4*c2p1*c3p3*ss[11];
  sys_part_3 += 6*c2p2*c3p2*ss[10];
  sys_part_3 += 4*c2p3*c3p1*ss[9];
  sys_part_3 += c1p4*ss[4];
  sys_part_3 += c2p4*ss[8];
  sys_part_3 += c3p4*ss[12];
  sys_part_3 += c0p4;
  sys_part_3 += -tmom[4];
  TACC grad_0_num = 2*(c1p1*ss[1] + c2p1*ss[2] + c3p1*ss[3] + c0p1)*sys_part_1;
  grad_0_num += 3*(2*c0p1*c1p1*ss[1] + 2*c0p1*c2p1*ss[2] + 2*c0p1*c3p1*ss[3] + 2*c1p1*c2p1*ss[3] + 2*c1p1*c3p1*ss[4] + 2*c2p1*c3p1*ss[5] + c1p2*ss[2] + c2p2*ss[4] + c3p2*ss[6] + c0p2)*sys_part_2;
  grad_0_num += 4*(6*c0p1*c1p1*c2p1*ss[3] + 6*c0p1*c1p1*c3p1*ss[4] + 6*c0p1*c2p1*c3p1*ss[5] + 6*c1p1*c2p1*c3p1*ss[6] + 3*c0p1*c1p2*ss[2] + 3*c0p1*c2p2*ss[4] + 3*c0p1*c3p2*ss[6] + 3*c0p2*c1p1*ss[1] + 3*c0p2*c2p1*ss[2] + 3*c0p2*c3p1*ss[3] + 3*c1p1*c2p2*ss[5] + 3*c1p1*c3p2*ss[7] + 3*c1p2*c2p1*ss[4] + 3*c1p2*c3p1*ss[5] + 3*c2p1*c3p2*ss[8] + 3*c2p2*c3p1*ss[7] + c1p3*ss[3] + c2p3*ss[6] + c3p3*ss[9] + c0p3)*sys_part_3;
  grad_0_num += c1p1*ss[1];
  grad_0_num += c2p1*ss[2];
  grad_0_num += c3p1*ss[3];
  grad_0_num += c0p1;
  grad_0_num += -tmom[1];
  T grad_0_den = sqrt(square(sys_part_0) + square(sys_part_1) + square(sys_part_2) + square(sys_part_3));
  TACC grad_1_num = 2*(c0p1*ss[1] + c1p1*ss[2] + c2p1*ss[3] + c3p1*ss[4])*sys_part_1;
  grad_1_num += 3*(2*c0p1*c1p1*ss[2] + 2*c0p1*c2p1*ss[3] + 2*c0p1*c3p1*ss[4] + 2*c1p1*c2p1*ss[4] + 2*c1p1*c3p1*ss[5] + 2*c2p1*c3p1*ss[6] + c0p2*ss[1] + c1p2*ss[3] + c2p2*ss[5] + c3p2*ss[7])*sys_part_2;
  grad_1_num += 4*(6*c0p1*c1p1*c2p1*ss[4] + 6*c0p1*c1p1*c3p1*ss[5] + 6*c0p1*c2p1*c3p1*ss[6] + 6*c1p1*c2p1*c3p1*ss[7] + 3*c0p1*c1p2*ss[3] + 3*c0p1*c2p2*ss[5] + 3*c0p1*c3p2*ss[7] + 3*c0p2*c1p1*ss[2] + 3*c0p2*c2p1*ss[3] + 3*c0p2*c3p1*ss[4] + 3*c1p1*c2p2*ss[6] + 3*c1p1*c3p2*ss[8] + 3*c1p2*c2p1*ss[5] + 3*c1p2*c3p1*ss[6] + 3*c2p1*c3p2*ss[9] + 3*c2p2*c3p1*ss[8] + c0p3*ss[1] + c1p3*ss[4] + c2p3*ss[7] + c3p3*ss[10])*sys_part_3;
  grad_1_num += ss[1]*sys_part_0;
  T grad_1_den = sqrt(square(sys_part_0) + square(sys_part_1) + square(sys_part_2) + square(sys_part_3));
  TACC grad_2_num = 2*(c0p1*ss[2] + c1p1*ss[3] + c2p1*ss[4] + c3p1*ss[5])*sys_part_1;
  grad_2_num += 3*(2*c0p1*c1p1*ss[3] + 2*c0p1*c2p1*ss[4] + 2*c0p1*c3p1*ss[5] + 2*c1p1*c2p1*ss[5] + 2*c1p1*c3p1*ss[6] + 2*c2p1*c3p1*ss[7] + c0p2*ss[2] + c1p2*ss[4] + c2p2*ss[6] + c3p2*ss[8])*sys_part_2;
  grad_2_num += 4*(6*c0p1*c1p1*c2p1*ss[5] + 6*c0p1*c1p1*c3p1*ss[6] + 6*c0p1*c2p1*c3p1*ss[7] + 6*c1p1*c2p1*c3p1*ss[8] + 3*c0p1*c1p2*ss[4] + 3*c0p1*c2p2*ss[6] + 3*c0p1*c3p2*ss[8] + 3*c0p2*c1p1*ss[3] + 3*c0p2*c2p1*ss[4] + 3*c0p2*c3p1*ss[5] + 3*c1p1*c2p2*ss[7] + 3*c1p1*c3p2*ss[9] + 3*c1p2*c2p1*ss[6] + 3*c1p2*c3p1*ss[7] + 3*c2p1*c3p2*ss[10] + 3*c2p2*c3p1*ss[9] + c0p3*ss[2] + c1p3*ss[5] + c2p3*ss[8] + c3p3*ss[11])*sys_part_3;
  grad_2_num += ss[2]*sys_part_0;
  T grad_2_den = sqrt(square(sys_part_0) + square(sys_part_1) + square(sys_part_2) + square(sys_part_3));
  TACC grad_3_num = 2*(c0p1*ss[3] + c1p1*ss[4] + c2p1*ss[5] + c3p1*ss[6])*sys_part_1;
  grad_3_num += 3*(2*c0p1*c1p1*ss[4] + 2*c0p1*c2p1*ss[5] + 2*c0p1*c3p1*ss[6] + 2*c1p1*c2p1*ss[6] + 2*c1p1*c3p1*ss[7] + 2*c2p1*c3p1*ss[8] + c0p2*ss[3] + c1p2*ss[5] + c2p2*ss[7] + c3p2*ss[9])*sys_part_2;
  grad_3_num += 4*(6*c0p1*c1p1*c2p1*ss[6] + 6*c0p1*c1p1*c3p1*ss[7] + 6*c0p1*c2p1*c3p1*ss[8] + 6*c1p1*c2p1*c3p1*ss[9] + 3*c0p1*c1p2*ss[5] + 3*c0p1*c2p2*ss[7] + 3*c0p1*c3p2*ss[9] + 3*c0p2*c1p1*ss[4] + 3*c0p2*c2p1*ss[5] + 3*c0p2*c3p1*ss[6] + 3*c1p1*c2p2*ss[8] + 3*c1p1*c3p2*ss[10] + 3*c1p2*c2p1*ss[7] + 3*c1p2*c3p1*ss[8] + 3*c2p1*c3p2*ss[11] + 3*c2p2*c3p1*ss[10] + c0p3*ss[3] + c1p3*ss[6] + c2p3*ss[9] + c3p3*ss[12])*sys_part_3;
  grad_3_num += ss[3]*sys_part_0;
  T grad_3_den = sqrt(square(sys_part_0) + square(sys_part_1) + square(sys_part_2) + square(sys_part_3));
  TACC metric_sqr = square(sys_part_0);
  metric_sqr += square(sys_part_1);
  metric_sqr += square(sys_part_2);
  metric_sqr += square(sys_part_3);
  gradient[0]=grad_0_num / grad_0_den;
  gradient[1]=grad_1_num / grad_1_den;
  gradient[2]=grad_2_num / grad_2_den;
  gradient[3]=grad_3_num / grad_3_den;
  return sqrt(metric_sqr);
}
void roots(const T *parameters, T *roots) const {
  T c0p1 = parameters[0];
  T c1p1 = parameters[1];
  T c2p1 = parameters[2];
  T c3p1 = parameters[3];
  TACC root_0 = c1p1*ss[1];
  root_0 += c2p1*ss[2];
  root_0 += c3p1*ss[3];
  root_0 += c0p1;
  root_0 += -tmom[1];
  T c0p2 = c0p1*c0p1;
  T c1p2 = c1p1*c1p1;
  T c2p2 = c2p1*c2p1;
  T c3p2 = c3p1*c3p1;
  TACC root_1 = 2*c0p1*c1p1*ss[1];
  root_1 += 2*c0p1*c2p1*ss[2];
  root_1 += 2*c0p1*c3p1*ss[3];
  root_1 += 2*c1p1*c2p1*ss[3];
  root_1 += 2*c1p1*c3p1*ss[4];
  root_1 += 2*c2p1*c3p1*ss[5];
  root_1 += c1p2*ss[2];
  root_1 += c2p2*ss[4];
  root_1 += c3p2*ss[6];
  root_1 += c0p2;
  root_1 += -tmom[2];
  T c0p3 = c0p2*c0p1;
  T c1p3 = c1p2*c1p1;
  T c2p3 = c2p2*c2p1;
  T c3p3 = c3p2*c3p1;
  TACC root_2 = 6*c0p1*c1p1*c2p1*ss[3];
  root_2 += 6*c0p1*c1p1*c3p1*ss[4];
  root_2 += 6*c0p1*c2p1*c3p1*ss[5];
  root_2 += 6*c1p1*c2p1*c3p1*ss[6];
  root_2 += 3*c0p1*c1p2*ss[2];
  root_2 += 3*c0p1*c2p2*ss[4];
  root_2 += 3*c0p1*c3p2*ss[6];
  root_2 += 3*c0p2*c1p1*ss[1];
  root_2 += 3*c0p2*c2p1*ss[2];
  root_2 += 3*c0p2*c3p1*ss[3];
  root_2 += 3*c1p1*c2p2*ss[5];
  root_2 += 3*c1p1*c3p2*ss[7];
  root_2 += 3*c1p2*c2p1*ss[4];
  root_2 += 3*c1p2*c3p1*ss[5];
  root_2 += 3*c2p1*c3p2*ss[8];
  root_2 += 3*c2p2*c3p1*ss[7];
  root_2 += c1p3*ss[3];
  root_2 += c2p3*ss[6];
  root_2 += c3p3*ss[9];
  root_2 += c0p3;
  root_2 += -tmom[3];
  T c0p4 = c0p3*c0p1;
  T c1p4 = c1p3*c1p1;
  T c2p4 = c2p3*c2p1;
  T c3p4 = c3p3*c3p1;
  TACC root_3 = 24*c0p1*c1p1*c2p1*c3p1*ss[6];
  root_3 += 12*c0p1*c1p1*c2p2*ss[5];
  root_3 += 12*c0p1*c1p1*c3p2*ss[7];
  root_3 += 12*c0p1*c1p2*c2p1*ss[4];
  root_3 += 12*c0p1*c1p2*c3p1*ss[5];
  root_3 += 12*c0p1*c2p1*c3p2*ss[8];
  root_3 += 12*c0p1*c2p2*c3p1*ss[7];
  root_3 += 12*c0p2*c1p1*c2p1*ss[3];
  root_3 += 12*c0p2*c1p1*c3p1*ss[4];
  root_3 += 12*c0p2*c2p1*c3p1*ss[5];
  root_3 += 12*c1p1*c2p1*c3p2*ss[9];
  root_3 += 12*c1p1*c2p2*c3p1*ss[8];
  root_3 += 12*c1p2*c2p1*c3p1*ss[7];
  root_3 += 4*c0p1*c1p3*ss[3];
  root_3 += 4*c0p1*c2p3*ss[6];
  root_3 += 4*c0p1*c3p3*ss[9];
  root_3 += 6*c0p2*c1p2*ss[2];
  root_3 += 6*c0p2*c2p2*ss[4];
  root_3 += 6*c0p2*c3p2*ss[6];
  root_3 += 4*c0p3*c1p1*ss[1];
  root_3 += 4*c0p3*c2p1*ss[2];
  root_3 += 4*c0p3*c3p1*ss[3];
  root_3 += 4*c1p1*c2p3*ss[7];
  root_3 += 4*c1p1*c3p3*ss[10];
  root_3 += 6*c1p2*c2p2*ss[6];
  root_3 += 6*c1p2*c3p2*ss[8];
  root_3 += 4*c1p3*c2p1*ss[5];
  root_3 += 4*c1p3*c3p1*ss[6];
  root_3 += 4*c2p1*c3p3*ss[11];
  root_3 += 6*c2p2*c3p2*ss[10];
  root_3 += 4*c2p3*c3p1*ss[9];
  root_3 += c1p4*ss[4];
  root_3 += c2p4*ss[8];
  root_3 += c3p4*ss[12];
  root_3 += c0p4;
  root_3 += -tmom[4];
  roots[0]=root_0;
  roots[1]=root_1;
  roots[2]=root_2;
  roots[3]=root_3;
}
void roots_and_jacobian(const T *parameters, T *roots, T *jacobian) const {
  T c0p1 = parameters[0];
  T c1p1 = parameters[1];
  T c2p1 = parameters[2];
  T c3p1 = parameters[3];
  TACC root_0 = c1p1*ss[1];
  root_0 += c2p1*ss[2];
  root_0 += c3p1*ss[3];
  root_0 += c0p1;
  root_0 += -tmom[1];
  T c0p2 = c0p1*c0p1;
  T c1p2 = c1p1*c1p1;
  T c2p2 = c2p1*c2p1;
  T c3p2 = c3p1*c3p1;
  TACC root_1 = 2*c0p1*c1p1*ss[1];
  root_1 += 2*c0p1*c2p1*ss[2];
  root_1 += 2*c0p1*c3p1*ss[3];
  root_1 += 2*c1p1*c2p1*ss[3];
  root_1 += 2*c1p1*c3p1*ss[4];
  root_1 += 2*c2p1*c3p1*ss[5];
  root_1 += c1p2*ss[2];
  root_1 += c2p2*ss[4];
  root_1 += c3p2*ss[6];
  root_1 += c0p2;
  root_1 += -tmom[2];
  T c0p3 = c0p2*c0p1;
  T c1p3 = c1p2*c1p1;
  T c2p3 = c2p2*c2p1;
  T c3p3 = c3p2*c3p1;
  TACC root_2 = 6*c0p1*c1p1*c2p1*ss[3];
  root_2 += 6*c0p1*c1p1*c3p1*ss[4];
  root_2 += 6*c0p1*c2p1*c3p1*ss[5];
  root_2 += 6*c1p1*c2p1*c3p1*ss[6];
  root_2 += 3*c0p1*c1p2*ss[2];
  root_2 += 3*c0p1*c2p2*ss[4];
  root_2 += 3*c0p1*c3p2*ss[6];
  root_2 += 3*c0p2*c1p1*ss[1];
  root_2 += 3*c0p2*c2p1*ss[2];
  root_2 += 3*c0p2*c3p1*ss[3];
  root_2 += 3*c1p1*c2p2*ss[5];
  root_2 += 3*c1p1*c3p2*ss[7];
  root_2 += 3*c1p2*c2p1*ss[4];
  root_2 += 3*c1p2*c3p1*ss[5];
  root_2 += 3*c2p1*c3p2*ss[8];
  root_2 += 3*c2p2*c3p1*ss[7];
  root_2 += c1p3*ss[3];
  root_2 += c2p3*ss[6];
  root_2 += c3p3*ss[9];
  root_2 += c0p3;
  root_2 += -tmom[3];
  T c0p4 = c0p3*c0p1;
  T c1p4 = c1p3*c1p1;
  T c2p4 = c2p3*c2p1;
  T c3p4 = c3p3*c3p1;
  TACC root_3 = 24*c0p1*c1p1*c2p1*c3p1*ss[6];
  root_3 += 12*c0p1*c1p1*c2p2*ss[5];
  root_3 += 12*c0p1*c1p1*c3p2*ss[7];
  root_3 += 12*c0p1*c1p2*c2p1*ss[4];
  root_3 += 12*c0p1*c1p2*c3p1*ss[5];
  root_3 += 12*c0p1*c2p1*c3p2*ss[8];
  root_3 += 12*c0p1*c2p2*c3p1*ss[7];
  root_3 += 12*c0p2*c1p1*c2p1*ss[3];
  root_3 += 12*c0p2*c1p1*c3p1*ss[4];
  root_3 += 12*c0p2*c2p1*c3p1*ss[5];
  root_3 += 12*c1p1*c2p1*c3p2*ss[9];
  root_3 += 12*c1p1*c2p2*c3p1*ss[8];
  root_3 += 12*c1p2*c2p1*c3p1*ss[7];
  root_3 += 4*c0p1*c1p3*ss[3];
  root_3 += 4*c0p1*c2p3*ss[6];
  root_3 += 4*c0p1*c3p3*ss[9];
  root_3 += 6*c0p2*c1p2*ss[2];
  root_3 += 6*c0p2*c2p2*ss[4];
  root_3 += 6*c0p2*c3p2*ss[6];
  root_3 += 4*c0p3*c1p1*ss[1];
  root_3 += 4*c0p3*c2p1*ss[2];
  root_3 += 4*c0p3*c3p1*ss[3];
  root_3 += 4*c1p1*c2p3*ss[7];
  root_3 += 4*c1p1*c3p3*ss[10];
  root_3 += 6*c1p2*c2p2*ss[6];
  root_3 += 6*c1p2*c3p2*ss[8];
  root_3 += 4*c1p3*c2p1*ss[5];
  root_3 += 4*c1p3*c3p1*ss[6];
  root_3 += 4*c2p1*c3p3*ss[11];
  root_3 += 6*c2p2*c3p2*ss[10];
  root_3 += 4*c2p3*c3p1*ss[9];
  root_3 += c1p4*ss[4];
  root_3 += c2p4*ss[8];
  root_3 += c3p4*ss[12];
  root_3 += c0p4;
  root_3 += -tmom[4];
  T jac_0_0 = 1;
  T jac_0_1 = ss[1];
  T jac_0_2 = ss[2];
  T jac_0_3 = ss[3];
  TACC jac_1_0 = 2*c1p1*ss[1];
  jac_1_0 += 2*c2p1*ss[2];
  jac_1_0 += 2*c3p1*ss[3];
  jac_1_0 += 2*c0p1;
  TACC jac_1_1 = 2*c0p1*ss[1];
  jac_1_1 += 2*c1p1*ss[2];
  jac_1_1 += 2*c2p1*ss[3];
  jac_1_1 += 2*c3p1*ss[4];
  TACC jac_1_2 = 2*c0p1*ss[2];
  jac_1_2 += 2*c1p1*ss[3];
  jac_1_2 += 2*c2p1*ss[4];
  jac_1_2 += 2*c3p1*ss[5];
  TACC jac_1_3 = 2*c0p1*ss[3];
  jac_1_3 += 2*c1p1*ss[4];
  jac_1_3 += 2*c2p1*ss[5];
  jac_1_3 += 2*c3p1*ss[6];
  TACC jac_2_0 = 6*c0p1*c1p1*ss[1];
  jac_2_0 += 6*c0p1*c2p1*ss[2];
  jac_2_0 += 6*c0p1*c3p1*ss[3];
  jac_2_0 += 6*c1p1*c2p1*ss[3];
  jac_2_0 += 6*c1p1*c3p1*ss[4];
  jac_2_0 += 6*c2p1*c3p1*ss[5];
  jac_2_0 += 3*c1p2*ss[2];
  jac_2_0 += 3*c2p2*ss[4];
  jac_2_0 += 3*c3p2*ss[6];
  jac_2_0 += 3*c0p2;
  TACC jac_2_1 = 6*c0p1*c1p1*ss[2];
  jac_2_1 += 6*c0p1*c2p1*ss[3];
  jac_2_1 += 6*c0p1*c3p1*ss[4];
  jac_2_1 += 6*c1p1*c2p1*ss[4];
  jac_2_1 += 6*c1p1*c3p1*ss[5];
  jac_2_1 += 6*c2p1*c3p1*ss[6];
  jac_2_1 += 3*c0p2*ss[1];
  jac_2_1 += 3*c1p2*ss[3];
  jac_2_1 += 3*c2p2*ss[5];
  jac_2_1 += 3*c3p2*ss[7];
  TACC jac_2_2 = 6*c0p1*c1p1*ss[3];
  jac_2_2 += 6*c0p1*c2p1*ss[4];
  jac_2_2 += 6*c0p1*c3p1*ss[5];
  jac_2_2 += 6*c1p1*c2p1*ss[5];
  jac_2_2 += 6*c1p1*c3p1*ss[6];
  jac_2_2 += 6*c2p1*c3p1*ss[7];
  jac_2_2 += 3*c0p2*ss[2];
  jac_2_2 += 3*c1p2*ss[4];
  jac_2_2 += 3*c2p2*ss[6];
  jac_2_2 += 3*c3p2*ss[8];
  TACC jac_2_3 = 6*c0p1*c1p1*ss[4];
  jac_2_3 += 6*c0p1*c2p1*ss[5];
  jac_2_3 += 6*c0p1*c3p1*ss[6];
  jac_2_3 += 6*c1p1*c2p1*ss[6];
  jac_2_3 += 6*c1p1*c3p1*ss[7];
  jac_2_3 += 6*c2p1*c3p1*ss[8];
  jac_2_3 += 3*c0p2*ss[3];
  jac_2_3 += 3*c1p2*ss[5];
  jac_2_3 += 3*c2p2*ss[7];
  jac_2_3 += 3*c3p2*ss[9];
  TACC jac_3_0 = 24*c0p1*c1p1*c2p1*ss[3];
  jac_3_0 += 24*c0p1*c1p1*c3p1*ss[4];
  jac_3_0 += 24*c0p1*c2p1*c3p1*ss[5];
  jac_3_0 += 24*c1p1*c2p1*c3p1*ss[6];
  jac_3_0 += 12*c0p1*c1p2*ss[2];
  jac_3_0 += 12*c0p1*c2p2*ss[4];
  jac_3_0 += 12*c0p1*c3p2*ss[6];
  jac_3_0 += 12*c0p2*c1p1*ss[1];
  jac_3_0 += 12*c0p2*c2p1*ss[2];
  jac_3_0 += 12*c0p2*c3p1*ss[3];
  jac_3_0 += 12*c1p1*c2p2*ss[5];
  jac_3_0 += 12*c1p1*c3p2*ss[7];
  jac_3_0 += 12*c1p2*c2p1*ss[4];
  jac_3_0 += 12*c1p2*c3p1*ss[5];
  jac_3_0 += 12*c2p1*c3p2*ss[8];
  jac_3_0 += 12*c2p2*c3p1*ss[7];
  jac_3_0 += 4*c1p3*ss[3];
  jac_3_0 += 4*c2p3*ss[6];
  jac_3_0 += 4*c3p3*ss[9];
  jac_3_0 += 4*c0p3;
  TACC jac_3_1 = 24*c0p1*c1p1*c2p1*ss[4];
  jac_3_1 += 24*c0p1*c1p1*c3p1*ss[5];
  jac_3_1 += 24*c0p1*c2p1*c3p1*ss[6];
  jac_3_1 += 24*c1p1*c2p1*c3p1*ss[7];
  jac_3_1 += 12*c0p1*c1p2*ss[3];
  jac_3_1 += 12*c0p1*c2p2*ss[5];
  jac_3_1 += 12*c0p1*c3p2*ss[7];
  jac_3_1 += 12*c0p2*c1p1*ss[2];
  jac_3_1 += 12*c0p2*c2p1*ss[3];
  jac_3_1 += 12*c0p2*c3p1*ss[4];
  jac_3_1 += 12*c1p1*c2p2*ss[6];
  jac_3_1 += 12*c1p1*c3p2*ss[8];
  jac_3_1 += 12*c1p2*c2p1*ss[5];
  jac_3_1 += 12*c1p2*c3p1*ss[6];
  jac_3_1 += 12*c2p1*c3p2*ss[9];
  jac_3_1 += 12*c2p2*c3p1*ss[8];
  jac_3_1 += 4*c0p3*ss[1];
  jac_3_1 += 4*c1p3*ss[4];
  jac_3_1 += 4*c2p3*ss[7];
  jac_3_1 += 4*c3p3*ss[10];
  TACC jac_3_2 = 24*c0p1*c1p1*c2p1*ss[5];
  jac_3_2 += 24*c0p1*c1p1*c3p1*ss[6];
  jac_3_2 += 24*c0p1*c2p1*c3p1*ss[7];
  jac_3_2 += 24*c1p1*c2p1*c3p1*ss[8];
  jac_3_2 += 12*c0p1*c1p2*ss[4];
  jac_3_2 += 12*c0p1*c2p2*ss[6];
  jac_3_2 += 12*c0p1*c3p2*ss[8];
  jac_3_2 += 12*c0p2*c1p1*ss[3];
  jac_3_2 += 12*c0p2*c2p1*ss[4];
  jac_3_2 += 12*c0p2*c3p1*ss[5];
  jac_3_2 += 12*c1p1*c2p2*ss[7];
  jac_3_2 += 12*c1p1*c3p2*ss[9];
  jac_3_2 += 12*c1p2*c2p1*ss[6];
  jac_3_2 += 12*c1p2*c3p1*ss[7];
  jac_3_2 += 12*c2p1*c3p2*ss[10];
  jac_3_2 += 12*c2p2*c3p1*ss[9];
  jac_3_2 += 4*c0p3*ss[2];
  jac_3_2 += 4*c1p3*ss[5];
  jac_3_2 += 4*c2p3*ss[8];
  jac_3_2 += 4*c3p3*ss[11];
  TACC jac_3_3 = 24*c0p1*c1p1*c2p1*ss[6];
  jac_3_3 += 24*c0p1*c1p1*c3p1*ss[7];
  jac_3_3 += 24*c0p1*c2p1*c3p1*ss[8];
  jac_3_3 += 24*c1p1*c2p1*c3p1*ss[9];
  jac_3_3 += 12*c0p1*c1p2*ss[5];
  jac_3_3 += 12*c0p1*c2p2*ss[7];
  jac_3_3 += 12*c0p1*c3p2*ss[9];
  jac_3_3 += 12*c0p2*c1p1*ss[4];
  jac_3_3 += 12*c0p2*c2p1*ss[5];
  jac_3_3 += 12*c0p2*c3p1*ss[6];
  jac_3_3 += 12*c1p1*c2p2*ss[8];
  jac_3_3 += 12*c1p1*c3p2*ss[10];
  jac_3_3 += 12*c1p2*c2p1*ss[7];
  jac_3_3 += 12*c1p2*c3p1*ss[8];
  jac_3_3 += 12*c2p1*c3p2*ss[11];
  jac_3_3 += 12*c2p2*c3p1*ss[10];
  jac_3_3 += 4*c0p3*ss[3];
  jac_3_3 += 4*c1p3*ss[6];
  jac_3_3 += 4*c2p3*ss[9];
  jac_3_3 += 4*c3p3*ss[12];
  roots[0]=root_0;
  roots[1]=root_1;
  roots[2]=root_2;
  roots[3]=root_3;
  jacobian[0] = jac_0_0;
  jacobian[1] = jac_0_1;
  jacobian[2] = jac_0_2;
  jacobian[3] = jac_0_3;
  jacobian[4] = jac_1_0;
  jacobian[5] = jac_1_1;
  jacobian[6] = jac_1_2;
  jacobian[7] = jac_1_3;
  jacobian[8] = jac_2_0;
  jacobian[9] = jac_2_1;
  jacobian[10] = jac_2_2;
  jacobian[11] = jac_2_3;
  jacobian[12] = jac_3_0;
  jacobian[13] = jac_3_1;
  jacobian[14] = jac_3_2;
  jacobian[15] = jac_3_3;
}
}; // CubicSystem

}; // systems
}; // random
}; // flopoco

#endif
