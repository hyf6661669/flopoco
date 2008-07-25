/*
 * A model of FPGA that works well enough for Virtex IV chips. 
 *
 * Author : Florent de Dinechin
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#ifndef VIRTEXIV_HPP
#define VIRTEXIV_HPP
#include "../Target.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

/** Class for representing an VirtexIV target */
class VirtexIV : public Target
{
public:
	/** The default constructor. */  
	VirtexIV() : Target()	{
		// all these values are set more or less randomly, to match  virtex 4 more or less
		fastcarryDelay_ = 3.4e-11; //s   
		elemWireDelay_  = 0.3e-11;
		lut2lutDelay_   = 1.5e-10;
		lutDelay_       = 1.5e-9; 
		multXInputs_    = 18;
		multYInputs_    = 18;
	}

	/** The destructor */
	virtual ~VirtexIV() {}

	/** overloading the virtual functions of Target
	 * @see the target class for more details 
	 */
	double carryPropagateDelay();
	double adderDelay(int size);
	double localWireDelay();
	double lutDelay();
	double distantWireDelay(int n);
	bool   suggestSubmultSize(int &x, int &y, int wInX, int wInY);
	bool   suggestSubaddSize(int &x, int wIn);

private:
	double fastcarryDelay_; /**< The delay of the fast carry chain */
	double lut2lutDelay_;   /**< The delay between two LUTs */
	double elemWireDelay_;  /**< The elementary wire dealy (for computing the distant wire delay) */
	double lutDelay_;       /**< The LUT delay (in seconds)*/
};

#endif
