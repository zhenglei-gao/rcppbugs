// -*- mode: C++; c-indent-level: 2; c-basic-offset: 2; tab-width: 8 -*-
///////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012  Whit Armstrong                                    //
//                                                                       //
// This program is free software: you can redistribute it and/or modify  //
// it under the terms of the GNU General Public License as published by  //
// the Free Software Foundation, either version 3 of the License, or     //
// (at your option) any later version.                                   //
//                                                                       //
// This program is distributed in the hope that it will be useful,       //
// but WITHOUT ANY WARRANTY; without even the implied warranty of        //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
// GNU General Public License for more details.                          //
//                                                                       //
// You should have received a copy of the GNU General Public License     //
// along with this program.  If not, see <http://www.gnu.org/licenses/>. //
///////////////////////////////////////////////////////////////////////////


#ifndef HELPERS_H
#define HELPERS_H

#include <map>
#include <string>
#include <vector>
#include <Rinternals.h>
#include "distribution.types.h"

typedef std::map<std::string, distT> distMapT;

std::string getAttr(SEXP x, const char* attr_name);
std::vector<R_len_t> getDims(SEXP x);
distT matchDistibution(const std::string distibution);
distMapT initDistributions();
SEXP forceEval(SEXP x_, SEXP rho_, const int limit);

#endif // HELPERS_H
