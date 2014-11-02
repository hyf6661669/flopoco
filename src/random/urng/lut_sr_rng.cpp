// general c++ library for manipulating streams
#include <iostream>
#include <sstream>
#include <math.h>	// for NaN
#include <fstream>

/* header of libraries to manipulate multiprecision numbers
  There will be used in the emulate function to manipulate arbitraly large
  entries */
#include "gmp.h"
//#include "mpfr.h"
//#include "FPNumber.hpp"

// include the header of the Operator
#include "lut_sr_rng.hpp"
#include "../utils/chain_operator.hpp"

#include <boost/math/common_factor.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

namespace flopoco
{
	extern std::vector<Operator *> oplist;
	
namespace random
{


// personalized parameter
string LutSrRng::operatorInfo = "LutSrRng info r t k";



struct rng_para
{
	int n,r,t,k;
	uint32_t s;
};

rng_para table[]={
	// LUT-Opt tables
// HACK ! Stop very low period generators getting used
  /*{4, 4, 3, 0,  0x560b0d15},
  {4, 4, 4, 0,  0xaebfe64},
  {4, 4, 5, 0,  0x43a98de4},
  {4, 4, 6, 0,  0x450f0dca},
  {5, 5, 3, 0,  0x7a51235b},
  {5, 5, 4, 0,  0xd7f90c9},
  {5, 5, 5, 0,  0x46308c3a},
  {5, 5, 6, 0,  0x45149433},
  {6, 6, 3, 0,  0x37ed0c51},
  {6, 6, 4, 0,  0x5ae965c1},
  {6, 6, 5, 0,  0x87477a9},
  {6, 6, 6, 0,  0x479d8e01},
  {7, 7, 3, 0,  0x207e9d1d},
  {7, 7, 4, 0,  0x5273be02},
  {7, 7, 5, 0,  0x594a4040},
  {7, 7, 6, 0,  0x421b1b5e},
  {8, 8, 3, 0,  0x55306015},
  {8, 8, 4, 0,  0x71e973fb},
  {8, 8, 5, 0,  0x7939e6d8},
  {8, 8, 6, 0,  0x457670c},
  {9, 9, 3, 0,  0x4b2afae8},
  {9, 9, 4, 0,  0x233eacea},
  {9, 9, 5, 0,  0x214b78a2},
  {9, 9, 6, 0,  0x72b2c9ad},
  {10, 10, 3, 0,  0x39e9c88},
  {10, 10, 4, 0,  0x5e20ddfa},
  {10, 10, 5, 0,  0x2afbdf73},
  {10, 10, 6, 0,  0x7449c3e7},
  {11, 11, 3, 0,  0x7d2b4b2},
  {11, 11, 4, 0,  0x2fb8b2a6},
  {11, 11, 5, 0,  0x272adc87},
  {11, 11, 6, 0,  0x52ca4274},
  {12, 12, 3, 0,  0x24743c33},
  {12, 12, 4, 0,  0x2243fcab},
  {12, 12, 5, 0,  0x2451c4},
  {12, 12, 6, 0,  0x48d25711},
  {13, 13, 3, 0,  0xec6376c},
  {13, 13, 4, 0,  0x729859fe},
  {13, 13, 5, 0,  0x6d74efe4},
  {13, 13, 6, 0,  0x5cf0481b},
  {14, 14, 3, 0,  0x1a158327},
  {14, 14, 4, 0,  0x34ada20},
  {14, 14, 5, 0,  0x8960418},
  {14, 14, 6, 0,  0x796b461c},
  {15, 15, 3, 0,  0x3a9edfae},
  {15, 15, 4, 0,  0x417ba8c5},
  {15, 15, 5, 0,  0x19a1f170},
  {15, 15, 6, 0,  0x1b20e41f},
  {16, 16, 3, 0,  0xedb4a8},
  {16, 16, 4, 0,  0x2aa93a},
  {16, 16, 5, 0,  0x15930c7},
  {16, 16, 6, 0,  0x24283065},
  {17, 17, 3, 0,  0x44eba2e8},
  {17, 17, 4, 0,  0x17b26269},
  {17, 17, 5, 0,  0x33944689},
  {17, 17, 6, 0,  0x772a4437},
  {18, 18, 3, 0,  0x39a1af3e},
  {18, 18, 4, 0,  0x5da1f9c9},
  {18, 18, 5, 0,  0x23ecaea4},
  {18, 18, 6, 0,  0x56a9fe15},
  {19, 19, 3, 0,  0x145efbc1},
  {19, 19, 4, 0,  0x2d13b93e},
  {19, 19, 5, 0,  0x1f54479},
  {19, 19, 6, 0,  0x106fd3b3},
  {20, 20, 3, 0,  0x1a461c32},
  {20, 20, 4, 0,  0x5f1aa8fa},
  {20, 20, 5, 0,  0x5aba5337},
  {20, 20, 6, 0,  0x7e741053},
  {21, 21, 3, 0,  0xb3c8cb3},
  {21, 21, 4, 0,  0x70d4bdb4},
  {21, 21, 5, 0,  0x2f8932ac},
  {21, 21, 6, 0,  0x3110f2cf},
  {22, 22, 3, 0,  0x54e6c7e},
  {22, 22, 4, 0,  0x1821ed1d},
  {22, 22, 5, 0,  0x25bb0341},
  {22, 22, 6, 0,  0x779b36af},
  {23, 23, 3, 0,  0x5852f380},
  {23, 23, 4, 0,  0x42fb3351},
  {23, 23, 5, 0,  0x3b261151},
  {23, 23, 6, 0,  0x59bdb506},
  {24, 24, 3, 0,  0x7258df48},
  {24, 24, 4, 0,  0x34b8c1c1},
  {24, 24, 5, 0,  0x49b777e4},
  {24, 24, 6, 0,  0x56a59750},
  {25, 25, 3, 0,  0x7617a526},
  {25, 25, 4, 0,  0x6c68a975},
  {25, 25, 5, 0,  0x39afca52},
  {25, 25, 6, 0,  0x7d210261},
  {26, 26, 3, 0,  0x31572866},
  {26, 26, 4, 0,  0x52a9c70c},
  {26, 26, 5, 0,  0x3a9e9cd},
  {26, 26, 6, 0,  0x73686130},
  {27, 27, 3, 0,  0x3971fcbf},
  {27, 27, 4, 0,  0x7cbdc497},
  {27, 27, 5, 0,  0x19490b4d},
  {27, 27, 6, 0,  0x78f42a74},
  {28, 28, 3, 0,  0x7190c7f4},
  {28, 28, 4, 0,  0x5b561a81},
  {28, 28, 5, 0,  0x5eff274b},
  {28, 28, 6, 0,  0x37edb9e7},
  {29, 29, 3, 0,  0x74e560fa},
  {29, 29, 4, 0,  0x4b223475},
  {29, 29, 5, 0,  0x62e1022c},
  {29, 29, 6, 0,  0x245adddf},
  {30, 30, 3, 0,  0x5cfb48e3},
  {30, 30, 4, 0,  0xe3d3ec3},
  {30, 30, 5, 0,  0x68f9ca1f},
  {30, 30, 6, 0,  0x41aa8a12},
  {31, 31, 3, 0,  0x674fd4ed},
  {31, 31, 4, 0,  0x26a76ba3},
  {31, 31, 5, 0,  0x38798329},
  {31, 31, 6, 0,  0x25616556},
  {32, 32, 3, 0,  0x42ed0981},
  {32, 32, 4, 0,  0x6d426836},
  {32, 32, 5, 0,  0x76f13c9a},
  {32, 32, 6, 0,  0x60b4f976},
  {33, 33, 3, 0,  0x71928812},
  {33, 33, 4, 0,  0xc31ad58},
  {33, 33, 5, 0,  0x7cb7ec9a},
  {33, 33, 6, 0,  0x10724f2a},
  {34, 34, 3, 0,  0x37db2ab5},
  {34, 34, 4, 0,  0x20f71667},
  {34, 34, 5, 0,  0x2cb45721},
  {34, 34, 6, 0,  0x2812536f},
  {35, 35, 3, 0,  0x1f171aea},
  {35, 35, 4, 0,  0x2f599f93},
  {35, 35, 5, 0,  0x2935d8d9},
  {35, 35, 6, 0,  0x5e122afb},
  {36, 36, 3, 0,  0x729c1d99},
  {36, 36, 4, 0,  0x19c6d40f},
  {36, 36, 5, 0,  0x1e1a36ea},
  {36, 36, 6, 0,  0x2a029328},
  {37, 37, 3, 0,  0x53867795},
  {37, 37, 4, 0,  0x6cc75490},
  {37, 37, 5, 0,  0x6303b02d},
  {37, 37, 6, 0,  0x5b57bd33},
  {38, 38, 3, 0,  0x744c273b},
  {38, 38, 4, 0,  0x39049d96},
  {38, 38, 5, 0,  0x9b2f84f},
  {38, 38, 6, 0,  0x7f290f10},
  {39, 39, 3, 0,  0x670ca0ab},
  {39, 39, 4, 0,  0x64248574},
  {39, 39, 5, 0,  0x18889ba6},
  {39, 39, 6, 0,  0x3cc89ac4},
  {40, 40, 3, 0,  0x51ae568b},
  {40, 40, 4, 0,  0x7bf542e1},
  {40, 40, 5, 0,  0x38dd4bdb},
  {40, 40, 6, 0,  0x47420ad},
  {41, 41, 3, 0,  0x62d3a420},
  {41, 41, 4, 0,  0x38095919},
  {41, 41, 5, 0,  0x4c09fafd},
  {41, 41, 6, 0,  0x4dbb6433},
  {42, 42, 3, 0,  0x5bea3355},
  {42, 42, 4, 0,  0x1fb77fee},
  {42, 42, 5, 0,  0x11b3d9e3},
  {42, 42, 6, 0,  0x34d095ce},
  {43, 43, 3, 0,  0x66160f80},
  {43, 43, 4, 0,  0x44f64211},
  {43, 43, 5, 0,  0x7e30ed34},
  {43, 43, 6, 0,  0x3f5c3cff},
  {44, 44, 3, 0,  0x562a6b8b},
  {44, 44, 4, 0,  0x9c82000},
  {44, 44, 5, 0,  0x4259307e},
  {44, 44, 6, 0,  0x619a584b},
  {45, 45, 3, 0,  0x78d89cdd},
  {45, 45, 4, 0,  0x1c876812},
  {45, 45, 5, 0,  0x780c9626},
  {45, 45, 6, 0,  0x11817e43},
  {46, 46, 3, 0,  0x25cf7cf3},
  {46, 46, 4, 0,  0x35f9448a},
  {46, 46, 5, 0,  0x5662c34d},
  {46, 46, 6, 0,  0xeb6c45d},
  {47, 47, 3, 0,  0x4c680e30},
  {47, 47, 4, 0,  0x483dd06a},
  {47, 47, 5, 0,  0x76d9a546},
  {47, 47, 6, 0,  0x4c366ee8},*/
  {48, 48, 3, 0,  0x6d55079d},
  {48, 48, 4, 0,  0x7099a1e5},
  {48, 48, 5, 0,  0x30348ef6},
  {48, 48, 6, 0,  0x7addd578},
  {49, 49, 3, 0,  0x35f1446e},
  {49, 49, 4, 0,  0x7328c17a},
  {49, 49, 5, 0,  0x6c0648e8},
  {49, 49, 6, 0,  0x2cb7e651},
  {50, 50, 3, 0,  0x6f35e795},
  {50, 50, 4, 0,  0x6471d9b5},
  {50, 50, 5, 0,  0x4fbb5e0c},
  {50, 50, 6, 0,  0x87c7ca},
  {51, 51, 3, 0,  0x23e594e6},
  {51, 51, 4, 0,  0x3cf89cfc},
  {51, 51, 5, 0,  0x1f0965bb},
  {51, 51, 6, 0,  0x70bedfef},
  {52, 52, 3, 0,  0x2a5f7cc},
  {52, 52, 4, 0,  0x50af790},
  {52, 52, 5, 0,  0x2d157d7},
  {52, 52, 6, 0,  0xa4fd291},
  {53, 53, 3, 0,  0x507b539a},
  {53, 53, 4, 0,  0x79b22968},
  {53, 53, 5, 0,  0x1a70575b},
  {53, 53, 6, 0,  0x690010ea},
  {54, 54, 3, 0,  0x66b0b3e9},
  {54, 54, 4, 0,  0x464e305b},
  {54, 54, 5, 0,  0x1219dc1a},
  {54, 54, 6, 0,  0x7269d5c3},
  {55, 55, 3, 0,  0x228658a8},
  {55, 55, 4, 0,  0x7e5336e},
  {55, 55, 5, 0,  0x762389e9},
  {55, 55, 6, 0,  0x5c0e363b},
  {56, 56, 3, 0,  0x42a95a22},
  {56, 56, 4, 0,  0x4ec75064},
  {56, 56, 5, 0,  0x16104e5b},
  {56, 56, 6, 0,  0x518a7fe6},
  {57, 57, 3, 0,  0x4439731b},
  {57, 57, 4, 0,  0x67a4483b},
  {57, 57, 5, 0,  0x37113aa7},
  {57, 57, 6, 0,  0x7715dc8f},
  {58, 58, 3, 0,  0x9219d56},
  {58, 58, 4, 0,  0x243ee967},
  {58, 58, 5, 0,  0x520b5c16},
  {58, 58, 6, 0,  0x36ac8bdb},
  {59, 59, 3, 0,  0x28e35d42},
  {59, 59, 4, 0,  0xd0d7df6},
  {59, 59, 5, 0,  0x3e119ef9},
  {59, 59, 6, 0,  0x583c7888},
  {60, 60, 3, 0,  0x5460af4f},
  {60, 60, 4, 0,  0x6d11c996},
  {60, 60, 5, 0,  0x4bdd8821},
  {60, 60, 6, 0,  0x2d6bed64},
  {61, 61, 3, 0,  0x133224b1},
  {61, 61, 4, 0,  0x47e4cbac},
  {61, 61, 5, 0,  0x5642f444},
  {61, 61, 6, 0,  0x2df63af2},
  {62, 62, 3, 0,  0x59d61362},
  {62, 62, 4, 0,  0x33026ea1},
  {62, 62, 5, 0,  0x3217c74a},
  {62, 62, 6, 0,  0x65ea3bcf},
  {63, 63, 3, 0,  0x7e576dbe},
  {63, 63, 4, 0,  0x9fbfcb7},
  {63, 63, 5, 0,  0xdcda315},
  {63, 63, 6, 0,  0x2e74e3eb},
  {64, 64, 3, 0,  0x43e73941},
  {64, 64, 4, 0,  0x4c1b5358},
  {64, 64, 5, 0,  0x6aaef98f},
  {64, 64, 6, 0,  0x6883be9d},
  {66, 66, 3, 0,  0xe538957},
  {66, 66, 4, 0,  0x2b288381},
  {66, 66, 5, 0,  0x411ebe6c},
  {66, 66, 6, 0,  0x41c0574d},
  {68, 68, 3, 0,  0x69cb2002},
  {68, 68, 4, 0,  0x8aa75a1},
  {68, 68, 5, 0,  0x9e82a32},
  {68, 68, 6, 0,  0x2cb3aa6d},
  {70, 70, 3, 0,  0x78e88fe3},
  {70, 70, 4, 0,  0x798a8e3c},
  {70, 70, 5, 0,  0x28ddb1cb},
  {70, 70, 6, 0,  0x8c58c8b},
  {72, 72, 3, 0,  0x781f3b63},
  {72, 72, 4, 0,  0x3649046c},
  {72, 72, 5, 0,  0x3223ae4f},
  {72, 72, 6, 0,  0x3bf80105},
  {74, 74, 3, 0,  0x51c613c9},
  {74, 74, 4, 0,  0x5392c28d},
  {74, 74, 5, 0,  0x6d1cde8e},
  {74, 74, 6, 0,  0x24522cc5},
  {76, 76, 3, 0,  0x2fbf2f10},
  {76, 76, 4, 0,  0x638f7a60},
  {76, 76, 5, 0,  0x6d0b33c1},
  {76, 76, 6, 0,  0x268878c2},
  {78, 78, 3, 0,  0x2e6efd6e},
  {78, 78, 4, 0,  0x5a58971e},
  {78, 78, 5, 0,  0x757a10f4},
  {78, 78, 6, 0,  0x48c34176},
  {80, 80, 3, 0,  0x5a1190a6},
  {80, 80, 4, 0,  0x16936f8e},
  {80, 80, 5, 0,  0x33428e27},
  {80, 80, 6, 0,  0x3d7321fc},
  {82, 82, 3, 0,  0xf8bd964},
  {82, 82, 4, 0,  0x70a9a5b5},
  {82, 82, 5, 0,  0x6ca39893},
  {82, 82, 6, 0,  0x7ec6033},
  {84, 84, 3, 0,  0x4ab1f537},
  {84, 84, 4, 0,  0x5d01bf60},
  {84, 84, 5, 0,  0x388894d9},
  {84, 84, 6, 0,  0x44b72180},
  {86, 86, 3, 0,  0x519228cf},
  {86, 86, 4, 0,  0x42db7a49},
  {86, 86, 5, 0,  0x6659c89b},
  {86, 86, 6, 0,  0x1adfcaf2},
  {88, 88, 3, 0,  0x5bbd7091},
  {88, 88, 4, 0,  0x16c50ca8},
  {88, 88, 5, 0,  0x13c53e31},
  {88, 88, 6, 0,  0x588314d4},
  {90, 90, 3, 0,  0x27a962ca},
  {90, 90, 4, 0,  0x54df406c},
  {90, 90, 5, 0,  0x2859a8f4},
  {90, 90, 6, 0,  0x41dccd85},
  {92, 92, 3, 0,  0x5684b52a},
  {92, 92, 4, 0,  0x8fa1ad0},
  {92, 92, 5, 0,  0x7dee5960},
  {92, 92, 6, 0,  0x5e6eca40},
  {94, 94, 3, 0,  0x1303c4d7},
  {94, 94, 4, 0,  0x1e2899fc},
  {94, 94, 5, 0,  0x32aa6e4b},
  {94, 94, 6, 0,  0x324f7bd6},
  {96, 96, 3, 0,  0x88f07d1},
  {96, 96, 4, 0,  0x2d681808},
  {96, 96, 5, 0,  0x625167ec},
  {96, 96, 6, 0,  0x4b6920fb},
  {98, 98, 3, 0,  0x2b0d782e},
  {98, 98, 4, 0,  0x2ed73da2},
  {98, 98, 5, 0,  0x527569db},
  {98, 98, 6, 0,  0x2025308a},
  {100, 100, 3, 0,  0x5c9017d6},
  {100, 100, 4, 0,  0x11c8f1d0},
  {100, 100, 5, 0,  0x6065dd4e},
  {100, 100, 6, 0,  0x7ef60559},
  {102, 102, 3, 0,  0x221aee8b},
  {102, 102, 4, 0,  0x86e758b},
  {102, 102, 5, 0,  0x3e0e66fa},
  {102, 102, 6, 0,  0x7f79db1a},
  {104, 104, 3, 0,  0x3048f8fd},
  {104, 104, 4, 0,  0x41cd970b},
  {104, 104, 5, 0,  0xec3f84f},
  {104, 104, 6, 0,  0x17328b9d},
  {106, 106, 3, 0,  0x4a43809b},
  {106, 106, 4, 0,  0x4bccfe2b},
  {106, 106, 5, 0,  0x77c7955},
  {106, 106, 6, 0,  0x2432351e},
  {108, 108, 3, 0,  0x701f2e7d},
  {108, 108, 4, 0,  0x4936e28d},
  {108, 108, 5, 0,  0x5af67cf8},
  {108, 108, 6, 0,  0x1dc114fc},
  {110, 110, 3, 0,  0x730aec44},
  {110, 110, 4, 0,  0x17e7feea},
  {110, 110, 5, 0,  0x16ea686f},
  {110, 110, 6, 0,  0x4c8baa9c},
  {112, 112, 3, 0,  0x32e232a8},
  {112, 112, 4, 0,  0x46518943},
  {112, 112, 5, 0,  0x4eb71ccd},
  {112, 112, 6, 0,  0x7bd7e209},
  {114, 114, 3, 0,  0x2b0f7882},
  {114, 114, 4, 0,  0x4f69b63},
  {114, 114, 5, 0,  0x51de37a0},
  {114, 114, 6, 0,  0x7446f3a8},
  {116, 116, 3, 0,  0x5c6fa663},
  {116, 116, 4, 0,  0x489514fd},
  {116, 116, 5, 0,  0xbabb07d},
  {116, 116, 6, 0,  0xa557933},
  {118, 118, 3, 0,  0x66d20228},
  {118, 118, 4, 0,  0x11b969bb},
  {118, 118, 5, 0,  0x3f7421a9},
  {118, 118, 6, 0,  0x72868dfd},
  {120, 120, 3, 0,  0x4660438f},
  {120, 120, 4, 0,  0x1cfa444},
  {120, 120, 5, 0,  0x17093fff},
  {120, 120, 6, 0,  0x167b8430},
  {122, 122, 3, 0,  0x77eb3a00},
  {122, 122, 4, 0,  0x7126c40e},
  {122, 122, 5, 0,  0x21dcc675},
  {122, 122, 6, 0,  0x4f8a3863},
  {124, 124, 3, 0,  0x580cd473},
  {124, 124, 4, 0,  0x6518bf},
  {124, 124, 5, 0,  0x1e4842d8},
  {124, 124, 6, 0,  0x55d4961f},
  {126, 126, 3, 0,  0x3170ba96},
  {126, 126, 4, 0,  0x2be58789},
  {126, 126, 5, 0,  0x357fe738},
  {126, 126, 6, 0,  0x381337a1},
  {128, 128, 3, 0,  0x9a2c527},
  {128, 128, 4, 0,  0x12dbca3f},
  {128, 128, 5, 0,  0x18a00443},
  {128, 128, 6, 0,  0x78a46f6d},
  {132, 132, 3, 0,  0x74d3313},
  {132, 132, 4, 0,  0x2b80ebe7},
  {132, 132, 5, 0,  0x32b66059},
  {132, 132, 6, 0,  0xed1a17},
  {136, 136, 3, 0,  0x20091ed0},
  {136, 136, 4, 0,  0x5e4b50b3},
  {136, 136, 5, 0,  0x5b539abe},
  {136, 136, 6, 0,  0x4310b0ba},
  {140, 140, 3, 0,  0x3c7a2925},
  {140, 140, 4, 0,  0x403088d6},
  {140, 140, 5, 0,  0x55fe9e72},
  {140, 140, 6, 0,  0x309879dc},
  {144, 144, 3, 0,  0xbf17e35},
  {144, 144, 4, 0,  0x4c9f5e8a},
  {144, 144, 5, 0,  0x31019be6},
  {144, 144, 6, 0,  0x43b625b2},
  {148, 148, 3, 0,  0xc79a5bc},
  {148, 148, 4, 0,  0x6bd2477f},
  {148, 148, 5, 0,  0x1b320a80},
  {148, 148, 6, 0,  0x68709106},
  {152, 152, 3, 0,  0x466a19c3},
  {152, 152, 4, 0,  0x6dc0a6f},
  {152, 152, 5, 0,  0x74067780},
  {152, 152, 6, 0,  0x4c6549cb},
  {156, 156, 3, 0,  0x118509c4},
  {156, 156, 4, 0,  0x7ba5cb72},
  {156, 156, 5, 0,  0x4b3fa2eb},
  {156, 156, 6, 0,  0x282a8947},
  {160, 160, 3, 0,  0x42278068},
  {160, 160, 4, 0,  0x3604b081},
  {160, 160, 5, 0,  0x552a706},
  {160, 160, 6, 0,  0x7edbe216},
  {164, 164, 3, 0,  0x56e30eac},
  {164, 164, 4, 0,  0x68951c1d},
  {164, 164, 5, 0,  0x74664076},
  {164, 164, 6, 0,  0x2571561},
  {168, 168, 3, 0,  0x3258dbf7},
  {168, 168, 4, 0,  0x30b39f50},
  {168, 168, 5, 0,  0x4279845d},
  {168, 168, 6, 0,  0x3e4a4363},
  {172, 172, 3, 0,  0xfefbcb7},
  {172, 172, 4, 0,  0x5799923a},
  {172, 172, 5, 0,  0x1e272d25},
  {172, 172, 6, 0,  0x4bf2e29},
  {176, 176, 3, 0,  0x11f22bf},
  {176, 176, 4, 0,  0xc0bed96},
  {176, 176, 5, 0,  0x2893991b},
  {176, 176, 6, 0,  0x4d098400},
  {180, 180, 3, 0,  0xa64539d},
  {180, 180, 4, 0,  0x789cc211},
  {180, 180, 5, 0,  0x230560c4},
  {180, 180, 6, 0,  0x7fbe2b95},
  {184, 184, 3, 0,  0x63a4388f},
  {184, 184, 4, 0,  0x6863eb27},
  {184, 184, 5, 0,  0x2d46157e},
  {184, 184, 6, 0,  0x31223e7f},
  {188, 188, 3, 0,  0x6ad0fe73},
  {188, 188, 4, 0,  0x3fde3e62},
  {188, 188, 5, 0,  0x4a4030dc},
  {188, 188, 6, 0,  0x704f3e1},
  {192, 192, 3, 0,  0x40ba0328},
  {192, 192, 4, 0,  0x3cf733f1},
  {192, 192, 5, 0,  0x233662e3},
  {192, 192, 6, 0,  0x37914840},
  {196, 196, 3, 0,  0x2db74d9},
  {196, 196, 4, 0,  0x1730ec80},
  {196, 196, 5, 0,  0x6c7e01b},
  {196, 196, 6, 0,  0x22dcc00b},
  {200, 200, 3, 0,  0x27309be},
  {200, 200, 4, 0,  0x23b3632a},
  {200, 200, 5, 0,  0x1e4091bb},
  {200, 200, 6, 0,  0x415dd85e},
  {204, 204, 3, 0,  0x18d326f9},
  {204, 204, 4, 0,  0x76f58fe3},
  {204, 204, 5, 0,  0x6892339e},
  {204, 204, 6, 0,  0x42002d81},
  {208, 208, 3, 0,  0x283eeb7},
  {208, 208, 4, 0,  0x39348bf1},
  {208, 208, 5, 0,  0x50455b21},
  {208, 208, 6, 0,  0x5afc99c2},
  {212, 212, 3, 0,  0x4f7e1723},
  {212, 212, 4, 0,  0x21224aeb},
  {212, 212, 5, 0,  0x1297ae81},
  {212, 212, 6, 0,  0x4ec2a2f3},
  {216, 216, 3, 0,  0x8d60843},
  {216, 216, 4, 0,  0x1efb676e},
  {216, 216, 5, 0,  0x45ed1d2},
  {216, 216, 6, 0,  0x77086a3b},
  {220, 220, 3, 0,  0x276b6c29},
  {220, 220, 4, 0,  0x1b50e585},
  {220, 220, 5, 0,  0x5ea77db0},
  {220, 220, 6, 0,  0x3c672232},
  {224, 224, 3, 0,  0x64fb27ed},
  {224, 224, 4, 0,  0x3b7425f},
  {224, 224, 5, 0,  0x185ea9db},
  {224, 224, 6, 0,  0x795daf34},
  {228, 228, 3, 0,  0x2b2c35bf},
  {228, 228, 4, 0,  0x710d1569},
  {228, 228, 5, 0,  0x2d432766},
  {228, 228, 6, 0,  0x5827229c},
  {232, 232, 3, 0,  0x53296d39},
  {232, 232, 4, 0,  0x3eff1f9a},
  {232, 232, 5, 0,  0x2df5cf91},
  {232, 232, 6, 0,  0x3bb1e960},
  {236, 236, 3, 0,  0x684356c5},
  {236, 236, 4, 0,  0x13a0347a},
  {236, 236, 5, 0,  0x763d5f84},
  {236, 236, 6, 0,  0xe5154c},
  {240, 240, 3, 0,  0x64c7eab1},
  {240, 240, 4, 0,  0x18b77b8a},
  {240, 240, 5, 0,  0x484d3a4},
  {240, 240, 6, 0,  0x1b339d52},
  {244, 244, 3, 0,  0x4783958e},
  {244, 244, 4, 0,  0x158393d7},
  {244, 244, 5, 0,  0x60e581c6},
  {244, 244, 6, 0,  0x32e31607},
  {248, 248, 3, 0,  0x65989fcf},
  {248, 248, 4, 0,  0x54787e32},
  {248, 248, 5, 0,  0x14c3a010},
  {248, 248, 6, 0,  0x71e8287b},
  {252, 252, 3, 0,  0x47f9c1ba},
  {252, 252, 4, 0,  0x6b1986ac},
  {252, 252, 5, 0,  0xd31f1f8},
  {252, 252, 6, 0,  0x74541dff},
  {256, 256, 3, 0,  0x4164aa3b},
  {256, 256, 4, 0,  0x18afec8c},
  {256, 256, 5, 0,  0x25a5e41d},
  {256, 256, 6, 0,  0x40ad3323},
  {260, 260, 3, 0,  0x1db96366},
  {260, 260, 4, 0,  0x1f79f206},
  {260, 260, 5, 0,  0x75c4c659},
  {260, 260, 6, 0,  0x11e55e5a},
  {264, 264, 3, 0,  0x44ba8c3f},
  {264, 264, 4, 0,  0x67b26965},
  {264, 264, 5, 0,  0x5480f34f},
  {264, 264, 6, 0,  0xc22ff3c},
  {268, 268, 3, 0,  0x5eb04727},
  {268, 268, 4, 0,  0x2528fb94},
  {268, 268, 5, 0,  0x79f0b782},
  {268, 268, 6, 0,  0x1d6d3774},
  {272, 272, 3, 0,  0x15575119},
  {272, 272, 4, 0,  0x43c4b88b},
  {272, 272, 5, 0,  0x6f829427},
  {272, 272, 6, 0,  0x1f9ffd5d},
  {276, 276, 3, 0,  0x7be5a6dd},
  {276, 276, 4, 0,  0x1e808ab3},
  {276, 276, 5, 0,  0x435527c7},
  {276, 276, 6, 0,  0x3c69de20},
  {280, 280, 3, 0,  0x5104a268},
  {280, 280, 4, 0,  0x1adfe333},
  {280, 280, 5, 0,  0x7818dcfe},
  {280, 280, 6, 0,  0x592922cd},
  {284, 284, 3, 0,  0x5c19e347},
  {284, 284, 4, 0,  0x433e22c7},
  {284, 284, 5, 0,  0x47cf938a},
  {284, 284, 6, 0,  0x1fdbb926},
  {288, 288, 3, 0,  0x61c8e5c8},
  {288, 288, 4, 0,  0x70f35c4c},
  {288, 288, 5, 0,  0x7175c21e},
  {288, 288, 6, 0,  0x33c78919},
  {292, 292, 3, 0,  0x790e0a99},
  {292, 292, 4, 0,  0x6793b6ca},
  {292, 292, 5, 0,  0x450952eb},
  {292, 292, 6, 0,  0x251637ec},
  {296, 296, 3, 0,  0x6d6683d2},
  {296, 296, 4, 0,  0x70c7ff0c},
  {296, 296, 5, 0,  0x7c0a598},
  {296, 296, 6, 0,  0x3d862f0c},
  {300, 300, 3, 0,  0x23cafa8a},
  {300, 300, 4, 0,  0x1f6f2287},
  {300, 300, 5, 0,  0x39e4f6f2},
  {300, 300, 6, 0,  0x30f62709},
  {304, 304, 3, 0,  0x18b907c0},
  {304, 304, 4, 0,  0x7f1ea66},
  {304, 304, 5, 0,  0x1a0dfb2b},
  {304, 304, 6, 0,  0x6e64ff5b},
  {308, 308, 3, 0,  0x32abbeba},
  {308, 308, 4, 0,  0x65bcd086},
  {308, 308, 5, 0,  0x1dcf8f32},
  {308, 308, 6, 0,  0x38f4b8df},
  {312, 312, 3, 0,  0x3dd9f50a},
  {312, 312, 4, 0,  0x291c55a9},
  {312, 312, 5, 0,  0x58d163bf},
  {312, 312, 6, 0,  0x73196053},
  {316, 316, 3, 0,  0x4c10af44},
  {316, 316, 4, 0,  0x65c05bc1},
  {316, 316, 5, 0,  0x63b349d2},
  {316, 316, 6, 0,  0x2b5cbbe7},
  {320, 320, 3, 0,  0x3273f6c},
  {320, 320, 4, 0,  0x760b7f52},
  {320, 320, 5, 0,  0x69920e6a},
  {320, 320, 6, 0,  0x6fb0da99},
  {324, 324, 3, 0,  0x1e3dc838},
  {324, 324, 4, 0,  0x41821111},
  {324, 324, 5, 0,  0x6faa463},
  {324, 324, 6, 0,  0x78967fdc},
  {328, 328, 3, 0,  0x2bd30e92},
  {328, 328, 4, 0,  0x39e03579},
  {328, 328, 5, 0,  0x1632fa11},
  {328, 328, 6, 0,  0xffadc73},
  {332, 332, 3, 0,  0xfd155f0},
  {332, 332, 4, 0,  0x23ca48f5},
  {332, 332, 5, 0,  0x3a0ef546},
  {332, 332, 6, 0,  0x72632252},
  {336, 336, 3, 0,  0x72ef3402},
  {336, 336, 4, 0,  0x4013ffcb},
  {336, 336, 5, 0,  0x680544ae},
  {336, 336, 6, 0,  0x554b48cc},
  {340, 340, 3, 0,  0x3043a116},
  {340, 340, 4, 0,  0x75819874},
  {340, 340, 5, 0,  0x51cfb63a},
  {340, 340, 6, 0,  0x43eb4747},
  {344, 344, 3, 0,  0x256166c3},
  {344, 344, 4, 0,  0x61182ce1},
  {344, 344, 5, 0,  0x729eaf25},
  {344, 344, 6, 0,  0x1df7e4ce},
  {348, 348, 3, 0,  0x35f3e92c},
  {348, 348, 4, 0,  0x26467656},
  {348, 348, 5, 0,  0x6c546f52},
  {348, 348, 6, 0,  0x1fff72d7},
  {352, 352, 3, 0,  0x494e0bb4},
  {352, 352, 4, 0,  0x1e2d0699},
  {352, 352, 5, 0,  0x666f0be1},
  {352, 352, 6, 0,  0x5cf716b6},
  {356, 356, 3, 0,  0x3d2d4a42},
  {356, 356, 4, 0,  0xa0c8388},
  {356, 356, 5, 0,  0x74ba8abb},
  {356, 356, 6, 0,  0x3b5af944},
  {360, 360, 3, 0,  0x78b04769},
  {360, 360, 4, 0,  0x2c2123d5},
  {360, 360, 5, 0,  0x3d47a0d7},
  {360, 360, 6, 0,  0x380af2ba},
  {364, 364, 3, 0,  0x52c5acc4},
  {364, 364, 4, 0,  0x9ed446e},
  {364, 364, 5, 0,  0x1e27568d},
  {364, 364, 6, 0,  0xe0164c1},
  {368, 368, 3, 0,  0x548d0b10},
  {368, 368, 4, 0,  0x6ed81f54},
  {368, 368, 5, 0,  0x5ae279b1},
  {368, 368, 6, 0,  0xabe3f02},
  {372, 372, 3, 0,  0x6b2a186b},
  {372, 372, 4, 0,  0x47039848},
  {372, 372, 5, 0,  0x5c3fd6a},
  {372, 372, 6, 0,  0x290b69cf},
  {376, 376, 3, 0,  0x6da896b1},
  {376, 376, 4, 0,  0x506f6d23},
  {376, 376, 5, 0,  0x75017e7e},
  {376, 376, 6, 0,  0x21adbf4d},
  {380, 380, 3, 0,  0x79a9e3a6},
  {380, 380, 4, 0,  0x6f7bd15c},
  {380, 380, 5, 0,  0x5a84a8bb},
  {380, 380, 6, 0,  0x3f1c107e},
  {384, 384, 3, 0,  0x27494abd},
  {384, 384, 4, 0,  0x306c4478},
  {384, 384, 5, 0,  0x37fd3c29},
  {384, 384, 6, 0,  0x3ea440b9},
  {388, 388, 3, 0,  0x4b939b2f},
  {388, 388, 4, 0,  0x6f7212aa},
  {388, 388, 5, 0,  0x5ece5338},
  {388, 388, 6, 0,  0x280d46c8},
  {392, 392, 3, 0,  0x141c780b},
  {392, 392, 4, 0,  0x6c0245c8},
  {392, 392, 5, 0,  0x39ac5e8e},
  {392, 392, 6, 0,  0x566832aa},
  {396, 396, 3, 0,  0x55aca591},
  {396, 396, 4, 0,  0x16c63af9},
  {396, 396, 5, 0,  0x53a35020},
  {396, 396, 6, 0,  0x43295726},
  {400, 400, 3, 0,  0x50cf21a6},
  {400, 400, 4, 0,  0x6b42fc27},
  {400, 400, 5, 0,  0x1eeba7fd},
  {400, 400, 6, 0,  0x57719cb6},
  {404, 404, 3, 0,  0x667330b2},
  {404, 404, 4, 0,  0x419c059e},
  {404, 404, 5, 0,  0x6453f6f5},
  {404, 404, 6, 0,  0x127851e},
  {408, 408, 3, 0,  0x2dd3a9c5},
  {408, 408, 4, 0,  0x26d0e0bc},
  {408, 408, 5, 0,  0x3a1dbfdb},
  {408, 408, 6, 0,  0x63773f67},
  {412, 412, 3, 0,  0x3a888bc8},
  {412, 412, 4, 0,  0x1b4d6471},
  {412, 412, 5, 0,  0xcfb5f71},
  {412, 412, 6, 0,  0x1a9efb53},
  {416, 416, 3, 0,  0x7729b4b1},
  {416, 416, 4, 0,  0x4693cae2},
  {416, 416, 5, 0,  0xe5b3a23},
  {416, 416, 6, 0,  0x2adda7b4},
  {420, 420, 3, 0,  0x7eaa8601},
  {420, 420, 4, 0,  0xb9f408e},
  {420, 420, 5, 0,  0x6f0aa8e4},
  {420, 420, 6, 0,  0x5382c36f},
  {424, 424, 3, 0,  0x6153e12b},
  {424, 424, 4, 0,  0x167cabb3},
  {424, 424, 5, 0,  0x42e75b5f},
  {424, 424, 6, 0,  0x2a28f4dd},
  {428, 428, 3, 0,  0x5a6a661},
  {428, 428, 4, 0,  0x15012065},
  {428, 428, 5, 0,  0xccf141},
  {428, 428, 6, 0,  0x294e919},
  {432, 432, 3, 0,  0x712dd166},
  {432, 432, 4, 0,  0x15461450},
  {432, 432, 5, 0,  0x69ffab90},
  {432, 432, 6, 0,  0x47e7a6b9},
  {436, 436, 3, 0,  0x599f39a9},
  {436, 436, 4, 0,  0x1c3f04cf},
  {436, 436, 5, 0,  0x66199689},
  {436, 436, 6, 0,  0x429a6358},
  {440, 440, 3, 0,  0x4e640491},
  {440, 440, 4, 0,  0x5bf15137},
  {440, 440, 5, 0,  0x2b39605e},
  {440, 440, 6, 0,  0x2e3af696},
  {444, 444, 3, 0,  0x22ac3fd2},
  {444, 444, 4, 0,  0x56005e26},
  {444, 444, 5, 0,  0x283e94b},
  {444, 444, 6, 0,  0x7389dae7},
  {448, 448, 3, 0,  0x13745fcb},
  {448, 448, 4, 0,  0x77fbedf5},
  {448, 448, 5, 0,  0x734747fa},
  {448, 448, 6, 0,  0x77ed1370},
  {452, 452, 3, 0,  0x463f7736},
  {452, 452, 4, 0,  0x3ea74e84},
  {452, 452, 5, 0,  0x25519def},
  {452, 452, 6, 0,  0x63cb54d2},
  {456, 456, 3, 0,  0x4c8474d0},
  {456, 456, 4, 0,  0x55423986},
  {456, 456, 5, 0,  0x10a6550e},
  {456, 456, 6, 0,  0x66beaaae},
  {460, 460, 3, 0,  0x4b411b19},
  {460, 460, 4, 0,  0x5169b88d},
  {460, 460, 5, 0,  0x25524b78},
  {460, 460, 6, 0,  0x62dff7e0},
  {464, 464, 3, 0,  0x7de9e305},
  {464, 464, 4, 0,  0x819e8f1},
  {464, 464, 5, 0,  0x3ef3aab0},
  {464, 464, 6, 0,  0x2e3228f4},
  {468, 468, 3, 0,  0x73e45e09},
  {468, 468, 4, 0,  0x2f346e13},
  {468, 468, 5, 0,  0x74f4594d},
  {468, 468, 6, 0,  0x19d93b1a},
  {472, 472, 3, 0,  0x50598df1},
  {472, 472, 4, 0,  0x5703c09a},
  {472, 472, 5, 0,  0x225f607a},
  {472, 472, 6, 0,  0x42f228c6},
  {476, 476, 3, 0,  0x9695022},
  {476, 476, 4, 0,  0x20875702},
  {476, 476, 5, 0,  0x5583f755},
  {476, 476, 6, 0,  0x7fbb4100},
  {480, 480, 3, 0,  0x813647},
  {480, 480, 4, 0,  0x40108757},
  {480, 480, 5, 0,  0x613f79ab},
  {480, 480, 6, 0,  0x36d574f1},
  {484, 484, 3, 0,  0x52036b07},
  {484, 484, 4, 0,  0x50e79f87},
  {484, 484, 5, 0,  0x3092869c},
  {484, 484, 6, 0,  0x42083d7},
  {488, 488, 3, 0,  0x4b3a2150},
  {488, 488, 4, 0,  0x17172d2b},
  {488, 488, 5, 0,  0xd587056},
  {488, 488, 6, 0,  0x648cd579},
  {492, 492, 3, 0,  0x5f0736e5},
  {492, 492, 4, 0,  0x40bd3358},
  {492, 492, 5, 0,  0x3ce559c},
  {492, 492, 6, 0,  0x47b119e5},
  {496, 496, 3, 0,  0xdb4babd},
  {496, 496, 4, 0,  0x56619c47},
  {496, 496, 5, 0,  0x3c68e5a7},
  {496, 496, 6, 0,  0x70f909fb},
  {500, 500, 3, 0,  0x4703f37b},
  {500, 500, 4, 0,  0x3f86af07},
  {500, 500, 5, 0,  0x53f49033},
  {500, 500, 6, 0,  0xb76d6ab},
  {504, 504, 3, 0,  0x1fcd853c},
  {504, 504, 4, 0,  0x6fadbbc1},
  {504, 504, 5, 0,  0x6d742118},
  {504, 504, 6, 0,  0x26b6b7c0},
  {508, 508, 3, 0,  0x441a9bf8},
  {508, 508, 4, 0,  0x640b7145},
  {508, 508, 5, 0,  0x1ff08cb9},
  {508, 508, 6, 0,  0x1a8e164c},
  {512, 512, 3, 0,  0x6e20d77d},
  {512, 512, 4, 0,  0x2cb8872b},
  {512, 512, 5, 0,  0x4d9a0a7c},
  {512, 512, 6, 0,  0x1fd5bf39},

	// LUT-SR tables
	{1024 , 32 , 3 , 32 ,  0x1a5eb},
	{1024 , 32 , 4 , 32 ,  0x1562cd6},
	{1024 , 32 , 5 , 32 ,  0x1c48},
	{1024 , 32 , 6 , 32 ,  0x2999b26},
	{1280 , 40 , 3 , 32 ,  0xc51b5},
	{1280 , 40 , 4 , 32 ,  0x4ffa6a},
	{1280 , 40 , 5 , 32 ,  0x3453f},
	{1280 , 40 , 6 , 32 ,  0x171013},
	{1536 , 48 , 3 , 32 ,  0x76010},
	{1536 , 48 , 4 , 32 ,  0xc2dc4a},
	{1536 , 48 , 5 , 32 ,  0x4b2be0},
	{1536 , 48 , 6 , 32 ,  0x811a15},
	{1788 , 56 , 3 , 32 ,  0xa2aae},
	{1788 , 56 , 4 , 32 ,  0x23f5fd},
	{1788 , 56 , 5 , 32 ,  0x1dde4b},
	{1788 , 56 , 6 , 32 ,  0x129b8},
	{2048 , 64 , 3 , 32 ,  0x5f81cb},
	{2048 , 64 , 4 , 32 ,  0x456881},
	{2048 , 64 , 5 , 32 ,  0xbfbaac},
	{2048 , 64 , 6 , 32 ,  0x21955e},
	{2556 , 80 , 3 , 32 ,  0x276868},
	{2556 , 80 , 4 , 32 ,  0x2695b0},
	{2556 , 80 , 5 , 32 ,  0x2d51a0},
	{2556 , 80 , 6 , 32 ,  0x4450c5},
	{3060 , 96 , 3 , 32 ,  0x79e56},
	{3060 , 96 , 4 , 32 ,  0x9a7cd},
	{3060 , 96 , 5 , 32 ,  0x41a62},
	{3060 , 96 , 6 , 32 ,  0x1603e},
	{3540 , 112 , 3 , 32 ,  0x29108e},
	{3540 , 112 , 4 , 32 ,  0x27ec7c},
	{3540 , 112 , 5 , 32 ,  0x2e1e55},
	{3540 , 112 , 6 , 32 ,  0x3dac0a},
	{3900 , 128 , 3 , 32 ,  0x10023},
	{3900 , 128 , 4 , 32 ,  0x197bf8},
	{3900 , 128 , 5 , 32 ,  0xcc71},
	{3900 , 128 , 6 , 32 ,  0x14959e},
	{5064 , 160 , 3 , 32 ,  0x1aedee},
	{5064 , 160 , 4 , 32 ,  0x1a23b0},
	{5064 , 160 , 5 , 32 ,  0x1aaf88},
	{5064 , 160 , 6 , 32 ,  0x1f6302},
	{5064 , 192 , 3 , 32 ,  0x48a92},
	{5064 , 192 , 4 , 32 ,  0x439d3},
	{5064 , 192 , 5 , 32 ,  0x4637},
	{5064 , 192 , 6 , 32 ,  0x577ce},
	{6120 , 224 , 3 , 32 ,  0x23585f},		//Q1 not match the table in paper
	{6120 , 224 , 4 , 32 ,  0x25e3a1},
	{6120 , 224 , 5 , 32 ,  0x270f3f},
	{6120 , 224 , 6 , 32 ,  0x259047},
	{8033 , 256 , 3 , 32 ,  0x437c26},
	{8033 , 256 , 4 , 32 ,  0x439995},
	{8033 , 256 , 5 , 32 ,  0x43664f},
	{8033 , 256 , 6 , 32 ,  0x427ba2},
	{11213 , 384 , 3 , 32 ,  0x11d4d},		// not match the table in paper
	{11213 , 384 , 4 , 32 ,  0x23dd1},
	{11213 , 384 , 5 , 32 ,  0x257a8},
	{11213 , 384 , 6 , 32 ,  0x17bd8},
	{19937 , 624 , 3 , 32 ,  0xda8},
	{19937 , 624 , 4 , 32 ,  0xb433},
	{19937 , 624 , 5 , 32 ,  0x2fffb},
	{19937 , 624 , 6 , 32 ,  0x25c7d},
	{19937 , 750 , 3 , 32 ,  0xb433},
	{19937 , 750 , 4 , 32 ,  0xb433},
	{19937 , 750 , 5 , 32 ,  0xb433},
	{19937 , 750 , 6 , 32 ,  0xb433}
};

//no of elements in table
unsigned  int no_tuple=sizeof(table)/sizeof(table[0]);


// Simple LCG RNG
	static int LCG(uint32_t &s) 
	{ return (s=1664525UL*s+1013904223UL)>>16; }		

//permute from last element of vector
	static void Permute(uint32_t &s, std::vector<int> &p)		
	{ for(int j=p.size();j>1;j--) swap(p[j-1],p[LCG(s)%j]); }


	LutSrRng::LutSrRng(Target* target, int want_r, int want_t, int want_k, int want_n)
	: Operator(target), seedtap(0), want_r(want_r)
	{
		if(want_t==0){
			want_t=std::max(3,target->lutInputs()-1);
		}
		
  		// Copyright 
  		setCopyrightString("Junfei Yan and David Thomas 2011");
		
		// We have recurrences of one cycle, anything more is an error
		setHasDelay1Feedbacks();

//============================================================================================================================
// 0: select suitable parameters n, s according to r and t; k=32
		
		REPORT(DETAILED, "LutSrRng : Looking for parameters with r>="<<want_r<<", t>="<<want_t<<", k>="<<want_k);

        n=0;
		for(unsigned i=0;i<no_tuple;i++){
			if ((table[i].t>=want_t) && (table[i].r >= want_r) && (table[i].k>=want_k)){
				if(want_n==0 || table[i].n==want_n){
					n=table[i].n;
					r=table[i].r;
					t=table[i].t;
					k=table[i].k;
					s=table[i].s;
					break;
				}
			}
		}
		if(n==0){
			throw std::string("LutSrRng : No valid parameter set found - the set of tables may not cover this bitwidth.");
		}
		
		REPORT(DETAILED, "LutSrRng : Found parameters n="<<n<<", r="<<r<<", t="<<t<<", k="<<k<<", s=0x"<<std::hex<<s<<std::dec);
		
		// definition of the name of the operator
  		ostringstream name;
  		name << "lut_sr_rng_"<<n<<"_" << r << "_" << t << "_" << k << "_"<<std::hex<<s<<std::dec;
  		setName(name.str());

	//define the size of taps and cycle

		taps.resize(n);
		cycle.resize(n);
		perm.resize(r); 

 	        cs.assign(n,0);

		std::vector<int> outputs(r), len(r,0);  //zero array
		int bit;

		//printf("m=%d	n= %d	r=%d	k=%d\n", m, n, r, k);

//============================================================================================================================
//Matrix expansion

	uint32_t working_s=s;

// 1: Create cycle through bits for seed loading
		for(int i=0; i<r; i++)
		{	cycle[i]=perm[i]=(i+1)%r;	}

		outputs=perm;			//set current output of each fifo

// 2: Extend bit-wide FIFOs
		for(int i=r;i<n;i++)
		{ 
			do{ bit=LCG(working_s)%r; 

			//printf("bit=%d	len[%d]= %d\n", bit, bit, len[bit]);

			  }		//randomly selects one bit has a SR length smaller than k
			while(len[bit]>=k);

		//std::cerr<<">jump out of loop\n";		
		//printf("i=%d	n= %d	r=%d	k=%d\n", i, n, r, k);
			cycle[i]=i;       
			swap(cycle[i], cycle[bit]);
			outputs[bit]=i;    len[bit]++;
		}
		
// 3: Loading connections
		for(int i=0;i<n;i++) 
		{	taps[i].insert(cycle[i]);}


// 4: XOR connections		
		for(int j=1;j<t;j++)
		{ 
			Permute(s, outputs);
			for(int i=0;i<r;i++)
			{
				taps[i].insert(outputs[i]);		
				if(taps[i].size()<taps[seedtap].size())		
					seedtap=i;
			}	
		}

// 5: Output permutation
		Permute(working_s, perm); 


//============================================================================================================================
//output taps

	/*
	ofstream fout;
	fout.open("taps.txt");
	for(int i=0; i<n; i++)
	{
		fout << i << "	->	";
		set<int>::iterator it=taps[i].begin();
		while(it!=taps[i].end())
		{
			fout << *it++ << "	";
		}
			fout << "\n";
	}

	fout<<flush;
	fout.close();
	*/
//============================================================================================================================
		addInput("m");			//mode-> m=1 load; m=0 RNG
		addInput("Sin");		//serial load input in load mode
		addOutput("RNG", want_r, 1, true);
		addOutput("Sout");

	for (int i=0; i<n; i++)
	{
		declare(join("SR_",i),1,false, Signal::registeredWithZeroInitialiser);
	}

// 6: output FIFO connections in VHDL
	for (int i=0; i<n; i++)
	{
		//register SR
		vhdl << tab << use(join("SR_",i)) << "<=" << join("state_",i) << ";" << endl;
		nextCycle();
		set<int>:: iterator it=taps[i].begin();	//set it points to the first element of tap[i]
	// seedtap case
		if (i==seedtap)			
 		{
			vhdl << tab << declare(join("state_",i))<< " <=  Sin WHEN m='1' ELSE ";
			while (it!= taps[i].end()) 
			{ vhdl << use(join("SR_",*it++)) <<" XOR ";}

			vhdl << "'0';\n" << endl;
		}

	//non-seedtap cases
		else 
		{	
			if(taps[i].size()==1){
				vhdl << tab << declare(join("state_",i)) << " <= " << use(join("SR_",cycle[i])) << ";\n";
			}else{
				vhdl << tab << declare(join("state_",i)) << " <= " << use(join("SR_",cycle[i])) << " WHEN m='1' ELSE ";
				set<int>:: iterator it=taps[i].begin();
				while (it!= taps[i].end()) 
				{ vhdl <<use(join("SR_",*it++)) <<" XOR ";}

				vhdl << "'0';\n" << endl;
			}

		}

		setCycleFromSignal("SR_0");

	}
		nextCycle();

// 7: r XOR connections for outputs

	// TODO : This was originally pulling out of state_i. Make sure it still lines up with emulate
	for (int i=0; i<want_r; i++)
		{vhdl << tab << "RNG" << of(i) << " <= " << use(join("SR_", perm[i])) << ";" << endl;}

	vhdl << tab << "Sout <= " << use(join("SR_",cycle[seedtap])) << ";" << endl;


}

//============================================================================================================================

LutSrRng::~LutSrRng(){	
}

//============================================================================================================================
void LutSrRng::emulate(TestCase * tc) {
	//std::cerr<<"LutSrRng::emulate\n";
	
	throw std::string("LutSrRng::emulate - DT10 - Check that fix to register output hasn't broken test-case.");
	
  mpz_class smode = tc->getInputValue("m");
  mpz_class ssin= tc->getInputValue("Sin");
  std::vector<int> rout(want_r);	
  
  mpz_class sr=0;
  mpz_class temp=0;

  /* then manipulate our bit vectors in order to get the correct output*/

	 // Advance state cs[0:n-1] using inputs (m,s_in)

	ns.assign(n,0);

		//printf("seedtap1->%d\n", seedtap);	
		for( int i=0;i<n;i++)
		{ // Do XOR tree and FIFOs
			if(smode==0)
				{ // RNG mode
					std::set<int>::iterator it=taps[i].begin();
					while(it!=taps[i].end()) 
					{	
						ns[i] ^= cs[*it++];	

					}
				}
				else
				{ // load mode 
					ns[i]= (i==seedtap) ? mpz_get_ui(ssin.get_mpz_t()) : cs[cycle[i]];	

				}  

		}

		//std::cerr<<">all ns\n\n\n";
		
		// capture permuted output signals
		mpz_class s_out=cs[cycle[seedtap]]; // output of load chain
		
		cs = ns;	// simulate "clock-edge", so FFs toggle

		for( int i=0;i<want_r;i++) 
		{		//std::cerr<<">in loop 3\n";
		//temp=0;

		rout[i]=cs[perm[i]];	

		mpz_set_ui(temp.get_mpz_t(),rout[i]);

		mpz_mul_2exp(temp.get_mpz_t(), temp.get_mpz_t(), i);			//MSB rout[0]; LSB rout[r-1]

		sr+=temp;			//find value for r outputs 

		//printf("rout[%d]->%d ", i,rout[i]);
		//gmp_printf("temp[%d]->%Zd	sr->%Zd\n", i, temp.get_mpz_t(),sr.get_mpz_t());

		}


  /* at the end, we indicate to the TestCase object what is the expected
    output corresponding to the inputs */
	tc->addExpectedOutput("RNG",sr);
	tc->addExpectedOutput("Sout",s_out);

	//std::cerr<<">over	RNG\n";

}

//============================================================================================================================
void LutSrRng::buildStandardTestCases(TestCaseList* tcl)
{
	TestCase *tc;
	mpz_class x;
	mpz_class mode;


//Test.1:	TEST_INIT_STATE
	
	for( int i=0;i<n;i++)
	{
		
		tc = new TestCase(this);
		tc->setSetupCycle(true);
		
	mpz_class x = getLargeRandom(1);

	//gmp_printf("x= %Zd\n", x.get_mpz_t());

		mpz_class mode = 1;

		tc->addInput("Sin", x);
		tc->addInput("m", mode);

	emulate(tc);
	tcl->add(tc);

	//printf("i= %d	n=%d\n", i,n);

	}


//Test.2:	TEST_OUT_DATA
	
	for(int i=0;i<n*2;i++)			// Q7 2*n?
	{
	tc = new TestCase(this);
		mpz_class x = 0;
		mpz_class mode = 0;

		tc->addInput("Sin", x);
		tc->addInput("m", mode);

	emulate(tc);
	tcl->add(tc);
	}
	

//Test.3:	TEST_REF_READBACK

	for( int i=0;i<n;i++)
	{

	tc = new TestCase(this); 
		mpz_class x = 1;
		mpz_class mode = 1;

		tc->addInput("Sin", x);
		tc->addInput("m", mode);
	emulate(tc);
	tcl->add(tc);
	}

}


/*! If we look at  for 2<= i < j <=1024 and gcd(i,j)<=p, then  max(gcd(2^i-1,2^j-1)) <= 2^p-1
  for pairs I looked at. This may be strictly true, I'm too tired to check right now. So we only
  lose a little pair-wise period if we are careful.
*/
struct CompositeLutSrRng : Operator
{
	int m_r;
  int m_p;
  std::vector<rng_para> m_generators; // LUT-SRs we have generators for
  
  int GCD(int n1, int n2) const
  {
	// may eventually make into true gcd(2^n1-1,2^n2-1), or into cached version
	return boost::math::gcd(n1,n2);
  }
  
  // Try to extend upwards from each point. If we find any valid combination, we always take
  // the first one, as this will have the largest seed.
  bool Extend(std::vector<int> &curr, int sum, std::vector<std::vector<int> > &best) const
  {
	  if(::flopoco::verbose>=DEBUG){
		  std::stringstream aa;
		  aa<<"[";
		  for(int i=0;i<(int)curr.size();i++){
			  if(i!=0)
				  aa<<",";
			  aa<<m_generators.at(curr.at(i)).n;
		  }
		  aa<<"]";
		REPORT(DEBUG, "Curr="<<aa.str());
	  }
	  
    if(sum>=m_r){
		int extra=sum-m_r;
		REPORT(INFO, " Solution : extra="<<extra);
		if((int)best.size()<=extra)
			best.resize(extra+1);
		if(best.at(extra).size()==0)
			best[extra]=curr;
      if(extra<8)
        return true;
	  
    }else{
      int i=curr.back()+1;
      while(i<(int)m_generators.size()){
		if(sum + m_generators[i].r < best.size()-1){
			int n1=m_generators.at(i).n;
			
			bool ok=true;
			for(int j=0;j<(int)curr.size();j++){
				int g=GCD(n1, m_generators.at(curr.at(j)).n);
				//REPORT(DEBUG, "GCD("<<n1<<","<<m_generators[curr[j]].n<<") = "<<g);
			  if(g> m_p){
				ok=false;
				break;
			  }
			}
			if(ok){
			  curr.push_back(i);
			  bool found=Extend(curr, sum+m_generators.at(i).r, best);
			  curr.pop_back();
				if(found)
					return true;
			}
		}
        i++;
      }
    }
    return false;
  }
  
  // Try to create decomposition from seed. Work through seeds in descending order
  std::vector<int> Search()
  {
	  std::vector<std::vector<int> > best;
	  
    for(int i=m_generators.size()-1;i>=0;i--){
		REPORT(DEBUG, "Trying seed="<<m_generators[i].n);
      std::vector<int> curr(1, i);
      if(Extend(curr, m_generators[i].r, best))
        break;
    }
    
    for(int i=0;i<(int)best.size();i++){
      if(best[i].size()!=0){
		return best[i];
	  }
    }
	
	throw std::string("CompositeLutSr::Search - No valid decomposition into sub-generators found.");
  }
  
  CompositeLutSrRng(Target *target, int r)
    : Operator(target)
	, m_r(r)
  {
    int min_r=128, max_r=512;
	m_p=16;
	// for min_r=128 and m_p=7, all individual generators have period at least 2^128-1,
	// and all pairs of bits have period at least 2^256 / 2^7 ~ 2^249.
	// The overall period of the generator will be at least  2^(floor(r/128)*(128-7))
	  
	ostringstream name;
	name << "composite_lut_sr_"<<r;
	setName(name.str());
	  
	  int t=std::min(4, std::max(3,target->lutInputs()-1));
    
    // Build a table of all the pairs that are useful
	int total_r=0;
    for(int i=0;i<(int)no_tuple;i++){
      if(table[i].n==table[i].r && table[i].n>=min_r && table[i].n <= max_r && table[i].t==t){
        m_generators.push_back(table[i]);
		  total_r+=table[i].r;
      }
    }
	
	if(total_r<r)
		throw std::string("CompositeLutSrRng - Number of bits ")+boost::lexical_cast<std::string>(r)+" is so large that this method won't work.";
	
	REPORT(INFO, "Got "<<m_generators.size()<<" candidate generators.");
    
    std::vector<int> sol=Search();
	std::vector<rng_para> choices;
	for(int i=0;i<(int)sol.size();i++){
		choices.push_back(m_generators[sol[i]]);
	}
	
	
	// Get them ordered from long to short, that way if we have to throw any bits
	// away they'll go from the lowest period generator
	std::reverse(choices.begin(), choices.end());
	
	addInput("m");			//mode-> m=1 load; m=0 RNG
	addInput("Sin");		//serial load input in load mode
	addOutput("RNG", r, 1, true);
	addOutput("Sout");
	
	// Pipeline the m part, as they will have a large fanout
	syncCycleFromSignal("m");
	nextCycle();
	
	mpz_class got_period=1, max_period=1;
	
	int acc_r=0;
	for(int i=0;i<(int)choices.size();i++){
		Operator *op=new LutSrRng(target, choices[i].r, choices[i].t, choices[i].k, choices[i].n);
		oplist.push_back(op);
		
		inPortMap(op, "m", "m");
		inPortMap(op, "Sin", "Sin");
		outPortMap(op, "RNG", join("RNG_",i));
		
		vhdl << instance(op, join("inst_",i,"_r_",choices[i].r));
		
		int taken=std::min(r-acc_r, choices[i].r);
		vhdl<<"RNG"<<range(taken+acc_r-1, acc_r)<<" <= "<<join("RNG_",i)<<range(taken-1,0)<<";\n";
		
		mpz_class pp=(mpz_class(1)<<choices[i].n)-1;
		mpz_lcm(got_period.get_mpz_t(), got_period.get_mpz_t(), pp.get_mpz_t());
		max_period=(max_period<<choices[i].n)-max_period;
		
		acc_r += choices[i].r;
	}
	
	REPORT(INFO, "MinPeriod=2^"<<choices.back().n<<"-1");
	REPORT(INFO, "EnsemblePeriod=~2^"<<mpz_sizeinbase(got_period.get_mpz_t(),2));
	REPORT(INFO, "LostPeriod=~2^"<<mpz_sizeinbase(mpz_class(max_period/got_period).get_mpz_t(),2));
  }
};

Operator *MakeLutOptGenerator(Target *target, int r)
{
	if(r<=256){
    return new LutSrRng(target, r, 0, 0);
  }else{
    return new CompositeLutSrRng(target, r);
  }
}


//============================================================================================================================

Operator *LutSrRng::DriveTransform(std::string name, RngTransformOperator *base)
{	
	Operator *urng=MakeLutOptGenerator(base->getTarget(), base->uniformInputBits());
	
	ChainOperator::mapping_list_t mapping;
	mapping.push_back(std::pair<std::string,std::string>("RNG", base->uniformInputName()));
	mapping.push_back(std::pair<std::string,std::string>("Sout", "-"));	// Get rid of the shift out
		
	return ChainOperator::Create(name, urng, mapping, base);
}


}; // random
}; // flopoco
