\name{packing}
\alias{packRaw}
\alias{unpackRaw}
\alias{encodeObject}
\alias{decodeObject}
\title{Converting raw vectors and R objects to and from sane strings}
\description{
R can easily serialize objects to raw vectors, but they are hard to be send using triggr because of "0" chars (which are prohibited by R)
and possible accidental generation of RNRNs (triggr terminal sequences).

To this end, functions \code{packRaw} and \code{unpackRaw} are provided; they perform fast encoding of raw vectors into character vectors acceptable for R and 
safe for transmission via triggr protocol.

\code{encodeObject} and \code{decodeObject} are their wrappers which directly accept and return R objects, hiding operations on raw connections from the user.
}
\usage{
packRaw(xRaw)
unpackRaw(pRaw)
encodeObject(x)
decodeObject(cx)
}
\arguments{
  \item{xRaw}{Raw vector to be encoded}
  \item{pRaw}{Encoded raw vector}
  \item{x}{R object to be serialized and encoded}
  \item{cx}{Encoded raw vector to be decoded and unserialized}
  
}

\details{
The encoding is based on a simple escaping; details can be found in \code{rawpack.c} source file. The generated string contains a small header composed of the size of the raw vector (expressed in decimal) and ":" sign.

Under the hood, \code{encodeObject} and \code{decodeObject} are using R's built-in \code{(un)serialize} mechanism. 

Due to R's limitations, raw vectors cannot store more than 1GB of data.
}

\examples{
# Make a raw vector and transcode it
exRaw<-charToRaw('ab\r\nc');
print(packRaw(exRaw))
print(identical(unpackRaw(packRaw(exRaw)),exRaw))

# Transcode some R object
exO<-list(theAnswer=42,iris=iris,runifs=runif(100))
identical(decodeObject(encodeObject(exRaw)),exRaw)
}


\author{
Miron B. Kursa <M.Kursa@icm.edu.pl>
}


