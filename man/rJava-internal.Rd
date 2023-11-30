\name{rJava-internal}
\alias{rJava-internal}
\alias{.jaddLibrary}
\alias{.jclass}
\alias{.jclass.boolean}
\alias{.jclass.double}
\alias{.jclass.float}
\alias{.jclass.int}
\alias{.jclass.void}
\alias{.jclassClass}
\alias{.jclassObject}
\alias{.jclassRef}
\alias{.jclassString}
\alias{.jfindClass}
\alias{.jfirst}
\alias{.jidenticalRef}
\alias{.jinherits}
\alias{.jinit.merge.error}
\alias{.jmergeClassPath}
\alias{.jmkref}
\alias{.jniInitialized}
\alias{.joptions}
\alias{.jproperty}
\alias{.jrcall}
\alias{.jrmLibrary}
\alias{.jsetJConvertor}
\alias{.jsetRConvertor}
\alias{.jstrVal}
\alias{.jzeroRef}
\alias{.r2j}
\alias{.rJava.base.path}
\title{
  Internal functions and constants
}
\description{
  The following functions are either internal or are not officially part
  of the API and therefore may changes in the future.
}
\usage{
.jaddLibrary(name, path, class.loader = .rJava.class.loader) 
.jclass(o, true = TRUE) 
.jclassRef(x, silent = FALSE) 
.jfindClass(cl, silent = FALSE, class.loader = .rJava.class.loader) 
.jfirst(libname, pkgname) 
.jidenticalRef(a, b) 
.jinherits(o, cl, class.loader = .rJava.class.loader) 
.jmergeClassPath(cp) 
.jmkref(jobj, jclass = "java/lang/Object") 
.joptions(...) 
.jproperty(key) 
.jrcall(o, method, ..., simplify = TRUE, class.loader = .rJava.class.loader) 
.jrmLibrary(name) 
.jsetJConvertor(java.class, fn) 
.jsetRConvertor(r.class, fn) 
.jstrVal(obj) 
.r2j(x, engine = NULL, convert = TRUE) 
}
\arguments{
  \item{name}{string, name of the library}
  \item{path}{string, path}
  \item{class.loader}{class loader object}
  \item{o}{Java object}
  \item{x}{Java object}
  \item{cl}{string, class name (or Java class name object)}
  \item{libname}{string, library location}
  \item{pkgname}{string, package name}
  \item{a}{Java object}
  \item{b}{Java object}
  \item{cp}{string, class path}
  \item{jobj}{Java object}
  \item{simplify}{logical}
  \item{java.class}{string, class name}
  \item{fn}{convertor function}
  \item{r.convertor}{string, R class}
  \item{obj}{Java object}
  \item{engine}{Java engine object, if \code{NULL} uses the main engine}
  \item{convert}{logical}
  \item{\dots}{additional parameters}
}
\keyword{internal}
