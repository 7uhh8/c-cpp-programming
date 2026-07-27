#include "return_codes.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define SIGN_SHIFT(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 31 : 15)
#define MANTISSA_LENGTH(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 23 : 10)
#define MANTISSA_FULL_LENGTH(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 24 : 11)
#define MANTISSA_MASK(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 0x7FFFFF : 0x3FF)
#define EXPONENT_MASK(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 0xFF : 0x1F)
#define EXPONENT_BIAS(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 127 : 15)
#define IMPLICIT_BIT(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 0x800000 : 0x400)

#define TO_0 0
#define TO_EVEN 1
#define TO_POSITIVE_INF 2
#define TO_NEGATIVE_INF 3

#define NAN(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 0x7fc00000 : 0x7e00)
#define POS_INFTY(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 0x7f800000 : 0x7c00)
#define NEG_INFTY(NUMBER_FORMAT) (((NUMBER_FORMAT) == 'f') ? 0xff800000 : 0xfc00)
#define POS_ZERO 0x0

#define DIGITS_TO_PRINT(NUMBER_FORMAT) (NUMBER_FORMAT == 'f' ? 6 : 3)
#define BUFFER_SIZE 64

char format;
uint8_t rounding_type;

uint8_t highest_bit(uint64_t number)
{
	if (number == 0)
		return 0;
	uint8_t position = 0;
	while (number != 0)
	{
		number >>= 1;
		position++;
	}
	return position;
}

void round_result(uint8_t round_from, uint8_t round_to, uint64_t *mantissa, uint8_t *exponent, uint8_t *sign)
{
	uint64_t curr_mantissa = *mantissa;
	uint8_t curr_exponent = *exponent;
	uint8_t curr_sign = *sign;

	if (curr_exponent != EXPONENT_MASK(format) && (curr_exponent || curr_mantissa))
	{
		uint64_t tail;
		int8_t shift = round_from - round_to;
		uint8_t to_even_adjust = 0;
		uint8_t to_inf_adjust = 0;
		uint8_t guard_bit = 0;

		if (shift <= 0)
			curr_mantissa <<= -shift;
		else
		{
			tail = curr_mantissa & (~((MANTISSA_MASK(format) + IMPLICIT_BIT(format)) << shift));
			curr_mantissa >>= shift;
			guard_bit = tail >> shift - 1;
			to_even_adjust =
				rounding_type == TO_EVEN && (guard_bit && ((curr_mantissa & 1) || (tail - (guard_bit << shift - 1))));
			to_inf_adjust =
				tail && ((rounding_type == TO_NEGATIVE_INF && curr_sign) || (rounding_type == TO_POSITIVE_INF && !curr_sign));
			if (to_even_adjust || to_inf_adjust)
				++curr_mantissa;
			if (highest_bit(curr_mantissa) > round_to)
			{
				curr_mantissa >>= 1;
				++curr_exponent;
			}
			if (highest_bit(curr_mantissa) == MANTISSA_FULL_LENGTH(format) && !curr_exponent)
				curr_exponent = 1;
		}
	}
	*mantissa = curr_mantissa;
	*exponent = curr_exponent;
	*sign = curr_sign;
}

void print(uint32_t number)
{
	uint8_t sign = number >> SIGN_SHIFT(format);
	uint8_t exponent = (number >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa = number & MANTISSA_MASK(format);

	int16_t exp;

	if (sign && !(exponent == EXPONENT_MASK(format) && mantissa))
		printf("-");
	if (exponent == EXPONENT_MASK(format))
	{
		if (mantissa)
		{
			printf("nan\n");
			return;
		}
		printf("inf\n");
		return;
	}

	if (!exponent && !mantissa)
	{
		printf("0x0.%0*dp+0\n", DIGITS_TO_PRINT(format), 0);
		return;
	}

	exp = exponent;
	if (!exponent)
	{
		exp = highest_bit(mantissa) - MANTISSA_LENGTH(format);
		mantissa <<= -exp + 1;
		mantissa -= IMPLICIT_BIT(format);
	}
	exp -= EXPONENT_BIAS(format);
	mantissa <<= 1 + (format == 'h');
	printf("0x1.%0*" PRIx64 "p%+" PRId16 "\n", DIGITS_TO_PRINT(format), mantissa, exp);
}

void add(uint32_t a, uint32_t b)
{
	uint8_t sign_a = a >> SIGN_SHIFT(format);
	uint8_t exponent_a = (a >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_a = a & MANTISSA_MASK(format);

	uint8_t sign_b = b >> SIGN_SHIFT(format);
	uint8_t exponent_b = (b >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_b = b & MANTISSA_MASK(format);

	uint8_t sign_rez;
	uint8_t exponent_rez;
	uint64_t mantissa_rez;

	int16_t shift;
	/*
	If any NaN or (a and b both INF with different sign) => NaN
	If a or b (or both) INF => INF with sign of INF number
	If a or both ZERO => b
	If b ZERO => a
	If a == b but with different sign => ZERO with sign of b
	Else a and b are both normalized or denormalized numbers
	*/
	if ((exponent_a == EXPONENT_MASK(format) && mantissa_a) || (exponent_b == EXPONENT_MASK(format) && mantissa_b) ||
		(exponent_a == EXPONENT_MASK(format) && exponent_b == EXPONENT_MASK(format) && sign_a != sign_b))
	{
		print(NAN(format));
		return;
	}
	if (a == POS_INFTY(format) || b == POS_INFTY(format))
	{
		print(POS_INFTY(format));
		return;
	}
	if (a == NEG_INFTY(format) || b == NEG_INFTY(format))
	{
		print(NEG_INFTY(format));
		return;
	}
	if (!exponent_a && !mantissa_a)
	{
		print(b);
		return;
	}
	if (!exponent_b && !mantissa_b)
	{
		print(a);
		return;
	}
	if (exponent_a == exponent_b && mantissa_a == mantissa_b && sign_a != sign_b)
	{
		print(POS_ZERO + (sign_b << SIGN_SHIFT(format)));
		return;
	}
	// If normalized => Add Implicit bit
	mantissa_a += exponent_a ? IMPLICIT_BIT(format) : 0;
	mantissa_b += exponent_b ? IMPLICIT_BIT(format) : 0;
	// if |a| < |b| => swap
	if (exponent_a < exponent_b || (exponent_a == exponent_b && mantissa_a < mantissa_b))
	{
		sign_a = sign_b;
		exponent_a = exponent_b;
		mantissa_a = mantissa_b;

		sign_b = a >> SIGN_SHIFT(format);
		exponent_b = (a >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
		mantissa_b = a & MANTISSA_MASK(format);
		mantissa_b += exponent_b ? IMPLICIT_BIT(format) : 0;
	}
	/*
	Result sign = sign of greatest value
	Result exponent = exponent of greatest value
	*/
	sign_rez = sign_a;
	exponent_rez = exponent_a;
	/*
	shift greatest value to a preset like 0**********...
	and lowest value to a preset like 0---***, where - count equals to difference in exponents
	After that combine them based on signs
	*/
	shift = BUFFER_SIZE - highest_bit(mantissa_a) - 1;
	mantissa_a <<= shift;

	int16_t shift_length = shift + exponent_b - exponent_a;
	mantissa_b = (shift_length > 0) ? (mantissa_b << shift_length) : (mantissa_b >> -shift_length);

	mantissa_rez = sign_a == sign_b ? mantissa_a + mantissa_b : mantissa_a - mantissa_b;

	/*	Normalization	*/
	if (highest_bit(mantissa_a) < highest_bit(mantissa_rez))
	{
		mantissa_rez >>= 1;
		++exponent_rez;
	}
	if (highest_bit(mantissa_a) > highest_bit(mantissa_rez) && exponent_rez > highest_bit(mantissa_a) - highest_bit(mantissa_rez))
	{
		exponent_rez -= highest_bit(mantissa_a) - highest_bit(mantissa_rez);
		mantissa_rez <<= highest_bit(mantissa_a) - highest_bit(mantissa_rez);
	}
	else if (highest_bit(mantissa_a) > highest_bit(mantissa_rez) && exponent_rez)
	{
		// In case of underflow
		mantissa_rez <<= highest_bit(mantissa_a) - highest_bit(mantissa_rez) % exponent_rez;
		exponent_rez = 0;
	}
	if (exponent_rez == 0 && highest_bit(mantissa_rez) == MANTISSA_FULL_LENGTH(format))
		exponent_rez = 1;
	// In case of overflow
	if (exponent_rez == EXPONENT_MASK(format))
		mantissa_rez = 0;
	round_result(highest_bit(mantissa_a), highest_bit(mantissa_a) - shift, &mantissa_rez, &exponent_rez, &sign_rez);
	print((sign_rez << SIGN_SHIFT(format)) + (exponent_rez << MANTISSA_LENGTH(format)) + (mantissa_rez & MANTISSA_MASK(format)));
}

void subtract(uint32_t a, uint32_t b)
{
	uint8_t sign_a = a >> SIGN_SHIFT(format);
	uint8_t exponent_a = (a >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_a = a & MANTISSA_MASK(format);

	uint8_t sign_b = b >> SIGN_SHIFT(format);
	uint8_t exponent_b = (b >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_b = b & MANTISSA_MASK(format);

	uint8_t sign_rez;
	uint8_t exponent_rez;
	uint64_t mantissa_rez;
	int16_t shift;
	/*
	If any NaN or (a and b both INF with same sign) => NaN
	If a or b (or both) INF => INF with apropriate sign of INF number
	If a or both ZERO => -b
	If b ZERO => a
	If a == b with same sign => ZERO with sign of -b
	Else a and b are both normalized or denormalized numbers
	*/
	if ((exponent_a == EXPONENT_MASK(format) && mantissa_a) || (exponent_b == EXPONENT_MASK(format) && mantissa_b) ||
		(exponent_a == EXPONENT_MASK(format) && exponent_b == EXPONENT_MASK(format) && sign_a == sign_b))
	{
		print(NAN(format));
		return;
	}
	if (a == POS_INFTY(format) || b == NEG_INFTY(format))
	{
		print(POS_INFTY(format));
		return;
	}
	if (a == NEG_INFTY(format) || b == POS_INFTY(format))
	{
		print(NEG_INFTY(format));
		return;
	}
	if (!exponent_a && !mantissa_a)
	{
		print(b + (1 << SIGN_SHIFT(format)));
		return;
	}
	if (!exponent_b && !mantissa_b)
	{
		print(a);
		return;
	}
	if (a == b)
	{
		print(POS_ZERO + (!sign_b << SIGN_SHIFT(format)));
		return;
	}
	// The rest is almost the same as Add

	// If normalized => Add Implicit bit
	mantissa_a += exponent_a ? IMPLICIT_BIT(format) : 0;
	mantissa_b += exponent_b ? IMPLICIT_BIT(format) : 0;
	// if |a| < |b| => swap but change signs
	if (exponent_a < exponent_b || (exponent_a == exponent_b && mantissa_a < mantissa_b))
	{
		sign_a = !sign_b;
		exponent_a = exponent_b;
		mantissa_a = mantissa_b;

		sign_b = !(a >> SIGN_SHIFT(format));
		exponent_b = (a >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
		mantissa_b = a & MANTISSA_MASK(format);
		mantissa_b += exponent_b ? IMPLICIT_BIT(format) : 0;
	}
	/*
	Result sign = sign of greatest value
	Result exponent = exponent of greatest value
	*/
	sign_rez = sign_a;
	exponent_rez = exponent_a;
	/*
	shift the greatest value to a preset like 0**********...
	and lowest value to a preset like 0---***, where - count equals to difference in exponents
	After that combine them based on signs
	*/
	shift = BUFFER_SIZE - highest_bit(mantissa_a) - 1;
	mantissa_a <<= shift;

	int16_t shift_length = shift + exponent_b - exponent_a;
	mantissa_b = (shift_length > 0) ? (mantissa_b << shift_length) : (mantissa_b >> -shift_length);

	mantissa_rez = sign_a != sign_b ? mantissa_a + mantissa_b : mantissa_a - mantissa_b;

	/*	Normalization	*/
	if (highest_bit(mantissa_a) < highest_bit(mantissa_rez))
	{
		mantissa_rez >>= 1;
		++exponent_rez;
	}
	// In case of underflow
	if (highest_bit(mantissa_a) > highest_bit(mantissa_rez) && exponent_rez > highest_bit(mantissa_a) - highest_bit(mantissa_rez))
	{
		exponent_rez -= highest_bit(mantissa_a) - highest_bit(mantissa_rez);
		mantissa_rez <<= highest_bit(mantissa_a) - highest_bit(mantissa_rez);
	}
	else if (highest_bit(mantissa_a) > highest_bit(mantissa_rez) && exponent_rez)
	{
		// In case of underflow
		mantissa_rez <<= highest_bit(mantissa_a) - highest_bit(mantissa_rez) % exponent_rez;
		exponent_rez = 0;
	}
	if (exponent_rez == 0 && highest_bit(mantissa_rez) == MANTISSA_FULL_LENGTH(format))
		exponent_rez = 1;
	// In case of overflow
	if (exponent_rez == EXPONENT_MASK(format))
		mantissa_rez = 0;
	round_result(highest_bit(mantissa_a), highest_bit(mantissa_a) - shift, &mantissa_rez, &exponent_rez, &sign_rez);
	print((sign_rez << SIGN_SHIFT(format)) + (exponent_rez << MANTISSA_LENGTH(format)) + (mantissa_rez & MANTISSA_MASK(format)));
}

void multiply(uint32_t a, uint32_t b)
{
	uint8_t sign_a = a >> SIGN_SHIFT(format);
	uint8_t exponent_a = (a >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_a = a & MANTISSA_MASK(format);

	uint8_t sign_b = b >> SIGN_SHIFT(format);
	uint8_t exponent_b = (b >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_b = b & MANTISSA_MASK(format);

	uint8_t sign_rez = sign_a ^ sign_b;
	uint8_t exponent_rez;
	uint64_t mantissa_rez;

	int16_t shift = 0;
	int16_t pivot;
	/*
	If any NaN or (a INF, b ZERO) or (b INF, a ZERO) => NaN
	If a or b (or both) INF => INF with result sign
	If a or b (or both) ZERO => ZERO with result sign
	Else a and b are both normalized or denormalized numbers
	*/
	if ((exponent_a == EXPONENT_MASK(format) && mantissa_a) || (exponent_b == EXPONENT_MASK(format) && mantissa_b) ||
		(exponent_a == EXPONENT_MASK(format) && (!exponent_b && !mantissa_b)) ||
		(exponent_b == EXPONENT_MASK(format) && (!exponent_a && !mantissa_a)))
	{
		print(NAN(format));
		return;
	}
	if (exponent_a == EXPONENT_MASK(format) || exponent_b == EXPONENT_MASK(format))
	{
		print(POS_INFTY(format) + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	if ((!exponent_a && !mantissa_a) || (!exponent_b && !mantissa_b))
	{
		print(POS_ZERO + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	// If normalized => Add Implicit bit else substract from shift denormalized exponent
	mantissa_a += exponent_a ? IMPLICIT_BIT(format) : 0;
	mantissa_b += exponent_b ? IMPLICIT_BIT(format) : 0;
	shift -= (exponent_a ? 0 : MANTISSA_FULL_LENGTH(format) - highest_bit(mantissa_a)) +
			 (exponent_b ? 0 : MANTISSA_FULL_LENGTH(format) - highest_bit(mantissa_b));
	// Shift them all to right, to leave as many guard bits as posible
	while (!(mantissa_a & 1))
		mantissa_a >>= 1;
	while (!(mantissa_b & 1))
		mantissa_b >>= 1;
	/*
	shift here is a variable, which is used to check for over/underflow
	pivot - location of imp bit, used for normalization
	*/
	shift += exponent_a + exponent_b - EXPONENT_BIAS(format);
	mantissa_rez = mantissa_a * mantissa_b;
	pivot = highest_bit(mantissa_a) + highest_bit(mantissa_b) - 1;

	/*	Normalization */
	int16_t difference = highest_bit(mantissa_rez) - pivot;
	shift += difference;
	mantissa_rez = (difference > 0) ? (mantissa_rez >> difference) : (mantissa_rez << -difference);

	// In case of overflow/underflow/denormalization
	if (shift >= EXPONENT_MASK(format) - 1)	   // overflow
	{
		print(POS_INFTY(format) + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	if (shift <= -MANTISSA_LENGTH(format))	  // underflow
	{
		print(POS_ZERO + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	if (shift <= 0)	   // denormalized
	{
		exponent_rez = 0;
		round_result(pivot, MANTISSA_LENGTH(format) + shift, &mantissa_rez, &exponent_rez, &sign_rez);
		print((sign_rez << SIGN_SHIFT(format)) + (exponent_rez << MANTISSA_LENGTH(format)) + (mantissa_rez & MANTISSA_MASK(format)));
		return;
	}
	// default response
	exponent_rez = shift;
	round_result(pivot, MANTISSA_FULL_LENGTH(format), &mantissa_rez, &exponent_rez, &sign_rez);
	print((sign_rez << SIGN_SHIFT(format)) + (exponent_rez << MANTISSA_LENGTH(format)) + (mantissa_rez & MANTISSA_MASK(format)));
}

void divide(uint32_t a, uint32_t b)
{
	uint8_t sign_a = a >> SIGN_SHIFT(format);
	uint8_t exponent_a = (a >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_a = a & MANTISSA_MASK(format);

	uint8_t sign_b = b >> SIGN_SHIFT(format);
	uint8_t exponent_b = (b >> MANTISSA_LENGTH(format)) & EXPONENT_MASK(format);
	uint64_t mantissa_b = b & MANTISSA_MASK(format);

	uint8_t sign_rez = sign_a ^ sign_b;
	uint8_t exponent_rez;
	uint64_t mantissa_rez;

	int16_t shift = 0;
	int16_t pivot;
	/*
	If any NaN or (a and b INF) or (a and b ZERO) => NaN
	If a INF or b ZERO => INF with result sign
	If a ZERO or b INF => ZERO with result sign
	Else a and b are both normalized or denormalized numbers
	*/
	if ((exponent_a == EXPONENT_MASK(format) && mantissa_a) || (exponent_b == EXPONENT_MASK(format) && mantissa_b) ||
		(exponent_a == EXPONENT_MASK(format) && exponent_b == EXPONENT_MASK(format)) ||
		((!exponent_a && !mantissa_a) && (!exponent_b && !mantissa_b)))
	{
		print(NAN(format));
		return;
	}
	if (exponent_a == EXPONENT_MASK(format) || (!exponent_b && !mantissa_b))
	{
		print(POS_INFTY(format) + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	if ((!exponent_a && !mantissa_a) || exponent_b == EXPONENT_MASK(format))
	{
		print(POS_ZERO + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	// If normalized => Add Implicit bit else subtract from shift denormalized exponent
	mantissa_a += exponent_a ? IMPLICIT_BIT(format) : 0;
	mantissa_b += exponent_b ? IMPLICIT_BIT(format) : 0;
	shift -= (exponent_a ? 0 : MANTISSA_FULL_LENGTH(format) - highest_bit(mantissa_a)) -
			 (exponent_b ? 0 : MANTISSA_FULL_LENGTH(format) - highest_bit(mantissa_b));
	// Shift a to left and b to right, leaving as many guard bits as possible
	mantissa_a <<= BUFFER_SIZE - highest_bit(mantissa_a);
	while (!(mantissa_b & 1))
		mantissa_b >>= 1;
	/*
	shift here is a variable, which is used to check for over/underflow
	pivot - location of implicit bit, used for normalization
	*/
	shift += exponent_a - exponent_b + EXPONENT_BIAS(format);
	mantissa_rez = mantissa_a / mantissa_b;
	pivot = highest_bit(mantissa_a) - highest_bit(mantissa_b) + 1;
	/*	Normalization	*/

	int16_t difference = highest_bit(mantissa_rez) - pivot;
	shift += difference;
	mantissa_rez = (difference > 0) ? (mantissa_rez >> difference) : (mantissa_rez << -difference);

	// In case of overflow/underflow/denormalization
	if (shift >= EXPONENT_MASK(format) - 1)	   // overflow
	{
		print(POS_INFTY(format) + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	if (shift <= -MANTISSA_LENGTH(format))	  // underflow
	{
		print(POS_ZERO + (sign_rez << SIGN_SHIFT(format)));
		return;
	}
	if (shift <= 0)	   // denormalized
	{
		exponent_rez = 0;
		round_result(pivot, MANTISSA_LENGTH(format) + shift, &mantissa_rez, &exponent_rez, &sign_rez);
		print((sign_rez << SIGN_SHIFT(format)) + (exponent_rez << MANTISSA_LENGTH(format)) + (mantissa_rez & MANTISSA_MASK(format)));
		return;
	}
	// default response
	exponent_rez = shift;
	round_result(pivot, MANTISSA_FULL_LENGTH(format), &mantissa_rez, &exponent_rez, &sign_rez);
	print((sign_rez << SIGN_SHIFT(format)) + (exponent_rez << MANTISSA_LENGTH(format)) + (mantissa_rez & MANTISSA_MASK(format)));
}

int main(int argc, char *argv[])
{
	uint32_t first_number;
	uint32_t second_number;
	char operation;
	// Parsing first 3 arguments
	if (argc != 4 && argc != 6)
	{
		fprintf(stderr, "Invalid number of arguments\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (argv[1][0] == '\0' || argv[1][1] != '\0')
	{
		fprintf(stderr, "Format type must be a single character\n");
		return ERROR_ARGUMENTS_INVALID;
	}

	format = argv[1][0];
	if (format != 'f' && format != 'h')
	{
		fprintf(stderr, "Unknown format: %c\n", format);
		return ERROR_ARGUMENTS_INVALID;
	}
	if (sscanf(argv[2], "%" SCNu8, &rounding_type) + sscanf(argv[3], "%" SCNx32, &first_number) != 2)
	{
		fprintf(stderr, "Error parsing rounding type or numbers\n");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (rounding_type > 3)
	{
		fprintf(stderr, "Invalid rounding type: %u\n", rounding_type);
		return ERROR_ARGUMENTS_INVALID;
	}
	if (argc == 4)
		print(first_number);
	else
	{
		// Parsing last 2 arguments
		if (argv[4][0] == '\0' || argv[4][1] != '\0')
		{
			fprintf(stderr, "Operation must be a single character\n");
			return ERROR_ARGUMENTS_INVALID;
		}
		operation = argv[4][0];
		if (sscanf(argv[5], "%" SCNx32, &second_number) != 1)
		{
			fprintf(stderr, "Error parsing second number\n");
			return ERROR_ARGUMENTS_INVALID;
		}
		switch (operation)
		{
		case '+':
			add(first_number, second_number);
			break;
		case '-':
			subtract(first_number, second_number);
			break;
		case '*':
			multiply(first_number, second_number);
			break;
		case '/':
			divide(first_number, second_number);
			break;
		default:
			fprintf(stderr, "Unsupported operation: %c\n", operation);
			return ERROR_ARGUMENTS_INVALID;
		}
	}
	return SUCCESS;
}
