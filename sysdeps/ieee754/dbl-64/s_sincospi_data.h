/* Common tables for the binary64 sinpi/cospi implementations.

Copyright (c) 2023-2025 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/sinpi/sinpi.c, revision cd0b9d90).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _SINCOSPI_DATA_H
#define _SINCOSPI_DATA_H

/* Put in *SH + *SL and *CH + *CL the sine and cosine of (pi/2) * S/1024.
   sincosn is accurate to slightly more than double precision, sincosn2 to
   double-double precision.  */
extern void __sincospi_sincosn (int __s, double *__sh, double *__sl,
				double *__ch, double *__cl) attribute_hidden;
extern void __sincospi_sincosn2 (int __s, double *__sh, double *__sl,
				 double *__ch, double *__cl) attribute_hidden;

#endif
