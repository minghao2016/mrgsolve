// This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
// To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/ or send a letter to
// Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

#include "RcppInclude.h"
#include "mrgsolve.h"
#include <vector>
#include <string>
#include "boost/tokenizer.hpp"

/**
 * Limit a number to a specific number of significant digits.
 * 
 * @param a the number to limit
 * @param b the number of digits
 * 
 */
double digits(const double& a, const double& b) {
  return std::floor(a*b)/b;
}

/** Find the position of a string in a character vector.
 * 
 * @param what the string to look for
 * @param table where to look for the string
 * @return the position of the string with 0-based indexing if the string is found;
 * -1 otherwise
 * 
 */
int find_position(const Rcpp::CharacterVector& what, const Rcpp::CharacterVector& table) {
  Rcpp::IntegerVector ma = Rcpp::match(what,table);
  if(Rcpp::IntegerVector::is_na(ma[0])) return(-1);
  return(ma[0]-1);
}

void neg_istate(int istate) {
  Rcpp::Rcout << std::endl << "mrgsolve: DLSODA returned with istate " << istate << std::endl;
  /*
   ISTATE = 2  if DLSODA was successful, negative otherwise.
   -1 means excess work done on this call (perhaps wrong JT).
   -2 means excess accuracy requested (tolerances too small).
   -3 means illegal input detected (see printed message).
   -4 means repeated error test failures (check all inputs).
   -5 means repeated convergence failures (perhaps bad Jacobian
   supplied or wrong choice of JT or tolerances).
   -6 means error weight became zero during problem. (Solution
   component i vanished, and ATOL or ATOL(i) = 0.)
   -7 means work space insufficient to finish (see messages).
   */
  
  switch (istate) {
  case -1:
    Rcpp::Rcout << "  excess work done on this call; check the model or increase maxsteps." << std::endl << std::endl;
    break;
  case -2:
    Rcpp::Rcout << "  excess accuracy requested; reduce atol and/or rtol." << std::endl  << std::endl;
    break;
  case -3:
    Rcpp::Rcout << "  illegal input detected (see printed message)." << std::endl  << std::endl;
    break;
  case -4:
    Rcpp::Rcout << "  repeated error test failures (check all inputs)." << std::endl  << std::endl;
    break;
  case -5:
    Rcpp::Rcout << "  means repeated convergence failures "<< std::endl;
    Rcpp::Rcout << "  (perhaps bad Jacobian supplied or wrong choice of JT or tolerances)." << std::endl  << std::endl;
    break;
  case -6:
    Rcpp::Rcout << "  error weight became zero during problem." << std::endl  << std::endl;
    Rcpp::Rcout << "  (Solution component i vanished, and ATOL or ATOL(i) = 0.)" << std::endl  << std::endl;
    break;
    //   case -7:
    // Rcpp::Rcout << "  work space insufficient to finish (see messages)." << std::endl;
    // break;
  default:
    break;
  }
}




/** 
 * Simulate from a multivariate normal distribution with mean 0.
 * 
 * @param OMEGA_ the covariance matrix
 * @param n the number of variates to simulate
 * @return matrix of simulated variates
 * 
 */
// [[Rcpp::export]]
arma::mat MVGAUSS(Rcpp::NumericMatrix& OMEGA_, int n) {
  
  arma::mat OMEGA(OMEGA_.begin(), OMEGA_.nrow(), OMEGA_.ncol(), false );
  
  return MVGAUSS(OMEGA,n);
  
}

arma::mat MVGAUSS(arma::mat& OMEGA, int n) {
  
  arma::vec eigval;
  arma::mat eigvec;
  arma::eig_sym(eigval,eigvec, OMEGA);
  
  int ncol = OMEGA.n_cols;
  
  arma::mat X = arma::randn<arma::mat>(n,ncol);
  
  eigval = arma::sqrt(eigval);
  
  arma::mat Z = arma::diagmat(eigval);
  
  X = eigvec * Z * X.t();
  
  return X.t();
}



//[[Rcpp::export]]
void decorr(Rcpp::NumericMatrix& x) {
  int i = 1, j = 1, n = x.nrow();
  if(n != x.ncol()) Rcpp::stop("matrix is not square");
  for(i=0; i < n; ++i) {
    for(j=0; j < n; ++j) {
      if(j!=i) x(i,j) = x(i,j)*sqrt(x(i,i)*x(j,j));
    }
  }
}


//[[Rcpp::export]]
Rcpp::NumericMatrix ZERO(Rcpp::NumericMatrix& x) {
  int i=0, j=0;
  for(i=0; i < x.ncol(); ++i) {
    for(j=0; j < x.nrow(); ++j) {
      x(i,j) = 0;
    }
  }
  return(x);
}


//[[Rcpp::export]]
Rcpp::NumericMatrix SUPERMATRIX(const Rcpp::List& a, bool keep_names) {
  
  int j,k;
  Rcpp::NumericMatrix mat;
  
  int tot=0;
  
  Rcpp::CharacterVector rnam;
  Rcpp::CharacterVector cnam;
  
  Rcpp::CharacterVector this_nam;
  Rcpp::List dnames(2);
  
  for(int i=0, n = a.size(); i < n; ++i) {
    mat = Rcpp::as<Rcpp::NumericMatrix>(a[i]);
    if(mat.nrow() ==0) continue;
    if(mat.nrow() != mat.ncol()) Rcpp::stop("Not all matrices are square");
    
    tot = tot + mat.nrow();
    
    if(!keep_names) continue;
    
    dnames = mat.attr("dimnames");
    
    if(dnames.size()==0) {
      for(j=0; j < mat.nrow(); ++j) {
        rnam.push_back(".");
        cnam.push_back(".");
      }
      continue;
    }
    
    if(!Rf_isNull(dnames[0])) {
      this_nam = dnames[0];
      for(int j=0, n=this_nam.size(); j < n; ++j) rnam.push_back(this_nam[j]);
    } else {
      for(j=0; j < mat.nrow(); ++j) rnam.push_back(".");
    }
    if(!Rf_isNull(dnames[1])) {
      this_nam = dnames[1];
      for(int j=0, n=this_nam.size(); j < n; ++j) cnam.push_back(this_nam[j]);
    } else {
      for(j=0; j < mat.ncol(); ++j) cnam.push_back(".");
    }
  }
  
  
  int totrow = 0;
  int totcol = 0;
  
  Rcpp::NumericMatrix ret(tot,tot);
  for(int i=0, n=a.size(); i < n; ++i) {
    mat = Rcpp::as<Rcpp::NumericMatrix>(a[i]);
    
    for(j=0; j < mat.nrow(); ++j) {
      for(k=0; k < mat.ncol(); ++k) {
        ret(totrow+j,totcol+k) = mat(j,k);
      }
    }
    totrow = totrow + mat.nrow();
    totcol = totcol + mat.ncol();
  }
  
  if(keep_names) {
    Rcpp::List dn = Rcpp::List::create(rnam,cnam);
    ret.attr("dimnames") = dn;
  }
  return(ret);
}


//[[Rcpp::export]]
Rcpp::List get_tokens(const Rcpp::CharacterVector& code) {
  
  Rcpp::List ret(code.size());
  
  for(int i = 0; i < code.size(); ++i) {
    Rcpp::CharacterVector tokens;
    std::string s = Rcpp::as<std::string>(code[i]);
    boost::tokenizer<> tok(s);
    for(boost::tokenizer<>::iterator beg=tok.begin(); beg!=tok.end();++beg){
      tokens.push_back(*beg);
    }
    ret[i] = tokens;
  }
  
  
  Rcpp::List ans;
  
  ans["tokens"] = ret;
  
  return ans;
}


//[[Rcpp::export]]
void from_to(const Rcpp::CharacterVector& a, 
             const Rcpp::CharacterVector& b,
             Rcpp::IntegerVector& ai,
             Rcpp::IntegerVector& bi) {
  
  ai = Rcpp::match(b,a)-1;
  bi = Rcpp::match(a,b)-1;
  
  ai = na_omit(ai);
  bi = na_omit(bi);
  
  std::sort(bi.begin(), bi.end());
  
}


// [[Rcpp::export]]
Rcpp::NumericMatrix EXPAND_EVENTS(const Rcpp::IntegerVector& idcol_,
                                  const Rcpp::NumericMatrix& events,
                                  const Rcpp::NumericVector& id) {
  
  int i,j,k;
  int crow = 0;
  
  int idcol = idcol_[0]-1;
  int ncol_new = events.ncol();
  
  Rcpp::List dimnames = events.attr("dimnames");
  Rcpp::CharacterVector names = dimnames[1];
  
  if(idcol < 0) {
    ncol_new = events.ncol() + 1;  
    names.push_back("ID");
    idcol = ncol_new-1;
    dimnames[1] = names;
  } 
  
  Rcpp::NumericMatrix ans(events.nrow()*id.size(),ncol_new);
  
  for(i=0; i < id.size(); ++i) {
    for(j=0; j < events.nrow(); ++j) {
      for(k=0; k < events.ncol(); ++k) {
        ans(crow,k) = events(j,k);
      }
      ans(crow,idcol) = id[i];
      ++crow;
    }
  }
  dimnames[0]  = Rcpp::CharacterVector(0);
  ans.attr("dimnames") = dimnames;
  return(ans);
}


// 
// Rcpp::NumericMatrix recdata(const Rcpp::NumericMatrix& dose,
//                             const Rcpp::NumericMatrix& obs,
//                             const Rcpp::IntegerVector& cols,
//                             const int n_out_col,const int n_out_row,
//                             const Rcpp::NumericVector& addl_,
//                             const Rcpp::NumericVector& ii_,
//                             const int nid, const int ntime, 
//                             const int namt, const int nevid, 
//                             const int ncmt, const int nrate) {
//   
//   Rcpp::NumericMatrix mat(n_out_row,n_out_col);
//   
//   int crow = 0;
//   double lastid  = dose(0,nid);
//   int ii = 0;
//   int addl = 0;
//   double time = 0;
//   double rate = 0;
//   int i = 0; int j = 0;
//   bool has_addl = addl_.size() > 0;
//   bool has_ii = ii_.size() > 0;
//   
//   for(int r = 0; r < dose.nrow(); ++r) {
//     
//     for(i = 0; i < cols.size(); ++i) {
//       mat(crow,i) = dose(r,i);
//     }
//     ++crow;
//     
//     time = dose(r,ntime);
//     
//     if(has_addl) {
//       ii  = ii_(r);
//       addl = addl_(r);      
//     } else {
//       ii = 0;
//       addl = 0;
//     }
//     
//     rate = dose(r,nrate);
//     
//     if(rate > 0) {
//       for(i = 0; i < cols.size(); ++i) {
//         mat(crow,i)  = dose(r,i);
//       }
//       mat(crow,ntime) = dose(r,ntime) + dose(r,namt)/rate;
//       mat(crow,namt) = -1.0*mat(crow,namt);
//       ++crow;
//     }
//     
//     if(addl > 0) {
//       for(i = 1; i <= addl; ++i) {
//         for(int k = 0; k < cols.size(); ++k) {
//           mat(crow,k) = dose(r,k);
//         }
//         
//         mat(crow,ntime) = dose(r,ntime) + i*ii;
//         ++crow;
//         
//         if(rate > 0) {
//           for(int k = 0; k < cols.size(); ++k) {
//             mat(crow,k) = dose(r,k);
//           }
//           mat(crow,ntime) = dose(r,ntime) + i*ii + dose(r,namt)/rate;
//           mat(crow,namt) = -1.0*mat(crow,namt);
//           ++crow;
//         }
//       }
//     }
//     
//     bool last_dose = r==dose.nrow()-1;
//     
//     bool next_diff = false;
//     if(r < dose.nrow()-1) {
//       next_diff = dose(r+1,nid) != dose(r,nid);
//     }
//     
//     if(last_dose || next_diff) {
//       for(i = 0; i < obs.nrow(); ++i) {
//         for(j = 0; j < obs.ncol(); ++j) {
//           mat(crow,j) = obs(i,j);
//         }
//         mat(crow,nid) = dose(r,nid);
//         ++crow;
//       } 
//     }
//     lastid = dose(r,nid);
//   }
//   return mat;
// }
// 
// 

