//
//  genomic_element_type.cpp
//  SLiM
//
//  Created by Ben Haller on 12/13/14.
//  Copyright (c) 2014 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of SLiM.
//
//	SLiM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	SLiM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with SLiM.  If not, see <http://www.gnu.org/licenses/>.


#include "genomic_element_type.h"


genomic_element_type::genomic_element_type(std::vector<int> M, std::vector<double> G)
{
	m = M;
	g = G;  
	
	if (m.size() != g.size()) { exit(1); }
	double A[m.size()]; for (int i=0; i<m.size(); i++) { A[i] = g[i]; }
	LT = gsl_ran_discrete_preproc(G.size(),A);
}

int genomic_element_type::draw_mutation_type()
{
	return m[gsl_ran_discrete(g_rng,LT)];
}
