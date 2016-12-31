/*
QControl PID controller for BBQ

Copyright (c) 2016 David Paiva (david@nailbuster.com). All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "qGlobals.h"
#include <stdarg.h>



void fBuf(char buf[], int bufsize, const __FlashStringHelper *fmt, ...) {   //my function to convert passed format to bluemsg array
	va_list args;
	va_start(args, fmt);
#ifdef __AVR__
	vsnprintf_P(buf, bufsize, (const char *)fmt, args); // progmem for AVR
#else
	vsnprintf(buf, bufsize, (const char *)fmt, args); // progmem for AVR
	//vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world	
#endif
	va_end(args);
}



String getValue(const String& data,  int index , char separator ) {
	int stringData = 0;        //variable to count data part nr 
	String dataPart = "";      //variable to hole the return text

	for (int i = 0; i<=data.length() - 1; i++) {    //Walk through the text one letter at a time

		if (data[i] == separator) {
			//Count the number of times separator character appears in the text
			stringData++;

		}
		else if (stringData == index) {
			//get the text when separator is the rignt one
			dataPart.concat(data[i]);

		}
		else if (stringData>index) {
			//return text and stop if the next separator appears - to save CPU-time
			return dataPart;
			break;

		}

	}
	//return text if this is the last part
	return dataPart;
}



//
// davekw7x
// December, 2010
//
// Build a C-style "string" for a floating point variable in a static array.
// The optional "digits" parameter tells how many decimal digits to store
// after the decimal point.  If no "digits" argument is given in the calling
// function a value of 2 is used.
//
// Utility functions to raise 10 to an unsigned int power and to print
// the hex bytes of a floating point variable are also included here.
//

void printBytes(float f)
{
	unsigned char *chpt = (unsigned char *)&f;
	char buffer[5]; // Big enough to hold each printed byte 0x..\0
					//
					// It's little-endian: Get bytes from memory in reverse order
					// so that they show in "register order."
					//
	for (int i = sizeof(f) - 1; i >= 0; i--) {
		sprintf(buffer, "%02x ", (unsigned char)chpt[i]);
		Serial.print(buffer);
	}
	Serial.println();
}

int round5(int a)
{
	return a >= 0 ? (a + 2) / 5 * 5 : (a - 2) / 5 * 5;
}

//
// Raise 10 to an unsigned integer power,
// It's used in this program for powers
// up to 6, so it must have a long return
// type, since in avr-gcc, an int can't hold
// 10 to the power 6.
//
// Since it's an integer function, negative
// powers wouldn't make much sense.
//
// If you want a more general function for raising
// an integer to an integer power, you could make 
// "base" a parameter.
unsigned long ipow10(unsigned power)
{
	const unsigned base = 10;
	unsigned long retval = 1;

	for (int i = 0; i < power; i++) {
		retval *= base;
	}
	return retval;
}

char *float2s(float f, char buf[], unsigned int digits)
{
	// Buffer to build string representation
	int index = 0;       // Position in buf to copy stuff to

						 // For debugging: Uncomment the following line to see what the
						 // function is working on.
						 //Serial.print("In float2s: bytes of f are: ");printBytes(f);

						 // Handle the sign here:
	if (f < 0.0) {
		buf[index++] = '-';
		f = -f;
	}
	// From here on, it's magnitude

	// Handle infinities 
	if (isinf(f)) {
		strcpy(buf + index, "INF");
		return buf;
	}

	// Handle NaNs
	if (isnan(f)) {
		strcpy(buf + index, "NAN");
		return buf;
	}

	//
	// Handle numbers.
	//

	// Six or seven significant decimal digits will have no more than
	// six digits after the decimal point.
	//
	if (digits > 6) {
		digits = 6;
	}

	// "Normalize" into integer part and fractional part
	int exponent = 0;
	if (f >= 10) {
		while (f >= 10) {
			f /= 10;
			++exponent;
		}
	}
	else if ((f > 0) && (f < 1)) {
		while (f < 1) {
			f *= 10;
			--exponent;
		}
	}

	//
	// Now add 0.5 in to the least significant digit that will
	// be printed.

	//float rounder = 0.5/pow(10, digits);
	// Use special power-of-integer function instead of the
	// floating point library function.
	float rounder = 0.5 / ipow10(digits);
	f += rounder;

	//
	// Get the whole number part and the fractional part into integer
	// data variables.
	//
	unsigned intpart = (unsigned)f;
	unsigned long fracpart = (unsigned long)((f - intpart) * 1.0e7);

	//
	// Divide by a power of 10 that zeros out the lower digits
	// so that the "%0.lu" format will give exactly the required number
	// of digits.
	//
	fracpart /= ipow10(6 - digits + 1);

	//
	// Create the format string and use it with sprintf to form
	// the print string.
	//
	char format[16];
	// If digits > 0, print
	//    int part decimal point fraction part and exponent.

	if (digits) {

		sprintf(format, "%%u.%%0%dlue%%+d", digits);
		//
		// To make sure the format is what it is supposed to be, uncomment
		// the following line.
		//Serial.print("format: ");Serial.println(format);
		sprintf(buf + index, format, intpart, fracpart, exponent);
	}
	else { // digits == 0; just print the intpart and the exponent
		sprintf(format, "%%ue%%+d");
		sprintf(buf + index, format, intpart, exponent);
	}

	return buf;
}
//
// Handy function to print hex values
// of the bytes of a float.  Sometimes
// helps you see why things don't
// get rounded to the values that you
// might think they should.
//
// You can print the actual byte values
// and compare with the floating point
// representation that is shown in a a
// reference like
//    [urlhttp://en.wikipedia.org/wiki/Floating_point[/url]
//




QControllerClass curQ;
QLinkClass curLink;
QFanClass curFan;
QPIDClass curPID;
QProbeClass curProbes[MaxProbes];
QGUIClass QGUI;

