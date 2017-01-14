% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/qsim.R
\name{qsim}
\alias{qsim}
\title{A quick simulation function.}
\usage{
qsim(x, e, idata, req = NULL, tgrid = NULL)
}
\arguments{
\item{x}{model object}

\item{e}{event object}

\item{idata}{individual data set}

\item{req}{compartments to request}

\item{tgrid}{\code{tgrid} object; used if \code{e} is an \code{ev} object}
}
\description{
A quick simulation function.
}
\examples{

mod <- mrgsolve:::house()

des <- tgrid(0,2400,1)

data <- recmatrix(ev(amt=1000, ii=24, addl=100),des)

out <- mod \%>\% qsim(data)

}
