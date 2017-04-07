// 
// 
// 

#include "qcExtras.h"




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