/*
 *    ▄███████▄  ▄██████▄   ▄█        ▄█  ███▄▄▄▄      ▄████████ ███▄▄▄▄    ▄█    ▄▄▄▄███▄▄▄▄      ▄████████
 *   ███    ███ ███    ███ ███       ███  ███▀▀▀██▄   ███    ███ ███▀▀▀██▄ ███  ▄██▀▀▀███▀▀▀██▄   ███    ███
 *   ███    ███ ███    ███ ███       ███▌ ███   ███   ███    ███ ███   ███ ███▌ ███   ███   ███   ███    █▀
 *   ███    ███ ███    ███ ███       ███▌ ███   ███   ███    ███ ███   ███ ███▌ ███   ███   ███  ▄███▄▄▄
 * ▀█████████▀  ███    ███ ███       ███▌ ███   ███ ▀███████████ ███   ███ ███▌ ███   ███   ███ ▀▀███▀▀▀
 *   ███        ███    ███ ███       ███  ███   ███   ███    ███ ███   ███ ███  ███   ███   ███   ███    █▄
 *   ███        ███    ███ ███▌    ▄ ███  ███   ███   ███    ███ ███   ███ ███  ███   ███   ███   ███    ███
 *  ▄████▀       ▀██████▀  █████▄▄██ █▀    ▀█   █▀    ███    █▀   ▀█   █▀  █▀    ▀█   ███   █▀    ██████████
 */

#include "return_codes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	uint32_t implicitBit;
	uint32_t mantissaMask;
	uint8_t mantissaShift;
	int16_t maxExp;
	uint8_t shift;
	uint16_t maskExp;
	uint32_t inf;
	uint32_t negateInf;
	uint32_t nan;
	uint32_t negateNull;
} FormatConstants;

void initFormatConstants(char format, FormatConstants *constants)
{
	constants->implicitBit = format == 'f' ? 0x00800000 : 0x0400;
	constants->mantissaMask = format == 'f' ? 0x007FFFFF : 0x3FF;
	constants->mantissaShift = format == 'f' ? 23 : 10;
	constants->maxExp = format == 'f' ? 127 : 15;
	constants->shift = format == 'f' ? 31 : 15;
	constants->maskExp = format == 'f' ? 0xFF : 0x1F;
	constants->inf = format == 'f' ? 0x7f800000 : 0x7c00;
	constants->negateInf = format == 'f' ? 0xff800000 : 0xfc00;
	constants->nan = format == 'f' ? 0x7fc00000 : 0xfc01;
	constants->negateNull = format == 'f' ? 0x80000000 : 0x8000;
}


/*
██╗░░░██╗░██████╗███████╗███████╗██╗░░░██╗██╗░░░░░
██║░░░██║██╔════╝██╔════╝██╔════╝██║░░░██║██║░░░░░
██║░░░██║╚█████╗░█████╗░░█████╗░░██║░░░██║██║░░░░░
██║░░░██║░╚═══██╗██╔══╝░░██╔══╝░░██║░░░██║██║░░░░░
╚██████╔╝██████╔╝███████╗██║░░░░░╚██████╔╝███████╗
░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░░╚═════╝░╚══════╝
 */

#define GET_SIGN(hex, constants) ((hex) >> (constants)->shift)
#define GET_EXP(hex, constants)                                                                                        \
	((int16_t)(((hex) >> (constants)->mantissaShift) & (constants)->maskExp) - (constants)->maxExp)
#define GET_MANTISSA(hex, constants) ((hex) & (constants)->mantissaMask)

void getParts(uint32_t hex, uint8_t *sign, uint32_t *mantissa, int16_t *exp, FormatConstants *constants)
{
	*sign = GET_SIGN(hex, constants);
	*exp = GET_EXP(hex, constants);
	*mantissa = GET_MANTISSA(hex, constants);
}

uint32_t collectNumber(uint8_t sign, uint32_t mantissa, int32_t exp, FormatConstants *constants)
{
	return (sign << constants->shift) | ((uint32_t)(exp + constants->maxExp) << constants->mantissaShift) |
		   (mantissa & constants->mantissaMask);
}

void normalizeNumber(uint32_t *mantissa, int16_t *exp, FormatConstants *constants)
{
	uint32_t localMantissa = *mantissa;
	int16_t localExp = *exp;
	while (localMantissa >= (1 << (constants->mantissaShift + 1)))
	{
		localMantissa >>= 1;
		localExp++;
	}
	while (localMantissa < (1ULL << constants->mantissaShift) && localExp >= -constants->maxExp)
	{
		localMantissa <<= 1;
		localExp--;
	}
	*mantissa = localMantissa;
	*exp = localExp;
}

void normalizeNumberToFormat(uint32_t *mantissa, int16_t *exp, FormatConstants *constants)
{
	uint32_t localMantissa = *mantissa;
	int16_t localExp = (int16_t)(*exp + 1);
	while ((localMantissa != 0) && ((localMantissa & constants->implicitBit) == 0))
	{
		localMantissa <<= 1;
		localExp -= 1;
	}
	*mantissa = localMantissa & constants->mantissaMask;
	*exp = localExp;
}


/*
    ██████╗░░█████╗░██╗░░░██╗███╗░░██╗██████╗░██╗███╗░░██╗░██████╗░      ███████╗████████╗░█████╗░
    ██╔══██╗██╔══██╗██║░░░██║████╗░██║██╔══██╗██║████╗░██║██╔════╝░      ██╔════╝╚══██╔══╝██╔══██╗
    ██████╔╝██║░░██║██║░░░██║██╔██╗██║██║░░██║██║██╔██╗██║██║░░██╗░      █████╗░░░░░██║░░░██║░░╚═╝
    ██╔══██╗██║░░██║██║░░░██║██║╚████║██║░░██║██║██║╚████║██║░░╚██╗      ██╔══╝░░░░░██║░░░██║░░██╗
    ██║░░██║╚█████╔╝╚██████╔╝██║░╚███║██████╔╝██║██║░╚███║╚██████╔╝      ███████╗░░░██║░░░╚█████╔╝
    ╚═╝░░╚═╝░╚════╝░░╚═════╝░╚═╝░░╚══╝╚═════╝░╚═╝╚═╝░░╚══╝░╚═════╝░      ╚══════╝░░░╚═╝░░░░╚════╝░
 */


uint32_t rounding(uint8_t sign, uint32_t mantissa, int16_t exp, uint8_t round, FormatConstants *constants, char op, uint32_t gRsBits)
{
	int16_t maxExp = constants->maxExp;
	if (exp < -maxExp - 1)
		return sign == 1 ? constants->negateNull : 0;
	if (abs(exp) > (maxExp + 1))
		return sign == 1 ? constants->negateInf : constants->inf;
	uint8_t g = (gRsBits & 0x4) >> 2;
	uint8_t r = (gRsBits & 0x2) >> 1;
	uint8_t s = gRsBits & 0x1;

	switch (round)
	{
	case 1:	   // toward_nearest_even
		if (op == '+' || op == '*' || op == '/')
		{
			if ((g && (s || r)) || (gRsBits == 0x4 && (mantissa & (1U << constants->mantissaShift))) ||
				(op == '*' && gRsBits == 0x0 && (mantissa & 1) == 0))
				mantissa++;
		}
		break;
	case 2:	   // toward_pos_infinity
		if (op == '+' && !sign && (g || r || s))
			mantissa++;
		else if (op == '*' || op == '/')
		{
			mantissa &= constants->mantissaMask;
			if (!sign && mantissa > 0)
				mantissa++;
		}
		break;
	case 3:	   // toward_neg_infinity
		if (op == '+')
		{
			if (sign && (g || r || s))
				mantissa++;
		}
		else if (op == '*' || op == '/')
		{
			mantissa &= constants->mantissaMask;
			if (sign && mantissa > 0)
				mantissa++;
		}
		break;
	default:
		mantissa &= constants->mantissaMask;
		break;
	}
	return collectNumber(sign, mantissa, exp, constants);
}


/*
00010110011 ░█████╗░██████╗░██╗████████╗██╗░░██╗███╗░░░███╗███████╗████████╗██╗░█████╗░ 11010101010
10011010010 ██╔══██╗██╔══██╗██║╚══██╔══╝██║░░██║████╗░████║██╔════╝╚══██╔══╝██║██╔══██╗ 00010110011
01000101110 ███████║██████╔╝██║░░░██║░░░███████║██╔████╔██║█████╗░░░░░██║░░░██║██║░░╚═╝ 11100011010
11100101010 ██╔══██║██╔══██╗██║░░░██║░░░██╔══██║██║╚██╔╝██║██╔══╝░░░░░██║░░░██║██║░░██╗ 01101010101
10011000010 ██║░░██║██║░░██║██║░░░██║░░░██║░░██║██║░╚═╝░██║███████╗░░░██║░░░██║╚█████╔╝ 11010101001
00111100010 ╚═╝░░╚═╝╚═╝░░╚═╝╚═╝░░░╚═╝░░░╚═╝░░╚═╝╚═╝░░░░░╚═╝╚══════╝░░░╚═╝░░░╚═╝░╚════╝░ 00111101011
 */


uint32_t add(uint32_t a, uint32_t b, FormatConstants *constants, uint8_t round)
{
	uint32_t inf = constants->inf;
	uint32_t negateInf = constants->negateInf;
	uint32_t nan = constants->nan;
	uint32_t negateNull = constants->negateNull;
	if (a == inf && b == negateInf)
		return nan;
	if ((a == negateNull || a == 0x0) && (b == 0x0 || b == negateNull))
		return 0;
	if (a == 0x0 || a == negateNull || b == 0x0 || b == negateNull)
		return a == 0x0 || a == negateNull ? b : a;
	uint8_t aSign, bSign;
	uint32_t aMantissa, bMantissa;
	int16_t aExp, bExp;
	getParts(a, &aSign, &aMantissa, &aExp, constants);
	getParts(b, &bSign, &bMantissa, &bExp, constants);
	aMantissa |= constants->implicitBit;
	bMantissa |= constants->implicitBit;
	int32_t expDiff = aExp - bExp;
	if (abs(expDiff) > 31)
		return expDiff > 31 ? a : b;

	uint8_t shiftAmount = abs(expDiff);
	uint32_t mask = (1U << shiftAmount) - 1;
	uint32_t gRsBits = ((shiftAmount > 0 ? aMantissa : bMantissa) & mask) >> (shiftAmount - 3);
	int16_t finalExp;
	if (aExp > bExp)
	{
		finalExp = aExp;
		bMantissa >>= shiftAmount;
	}
	else
	{
		finalExp = bExp;
		aMantissa >>= shiftAmount;
	}
	uint32_t resultMantissa;
	if (aSign == bSign)
		resultMantissa = aMantissa + bMantissa;
	else
	{
		if (aMantissa < bMantissa)
		{
			resultMantissa = bMantissa - aMantissa;
			aSign = bSign;
		}
		else
			resultMantissa = aMantissa - bMantissa;
	}

	normalizeNumber(&resultMantissa, &finalExp, constants);
	return rounding(aSign, resultMantissa, finalExp, round, constants, '+', gRsBits);
}

uint32_t multiply(uint64_t a, uint64_t b, FormatConstants *constants, uint8_t round)
{
	uint32_t negateInf = constants->negateInf;
	uint32_t inf = constants->inf;
	uint32_t nan = constants->nan;
	uint16_t shift = constants->shift;
	uint32_t negateNull = constants->negateNull;
	if (b == negateInf)
		return negateInf;
	if (b == inf && (a >> shift) == 0)
		return inf;
	if (a == nan || b == nan || ((a == inf || a == negateInf) && (b == 0x0 || b == negateNull)))
		return nan;
	if (b == negateNull)
		return (1) << shift;
	if (a == 0x0 || b == 0x0)
		return 0;

	uint8_t aSign, bSign;
	uint32_t aMantissa, bMantissa;
	int16_t aExp, bExp;
	getParts(a, &aSign, &aMantissa, &aExp, constants);
	getParts(b, &bSign, &bMantissa, &bExp, constants);
	aMantissa |= constants->implicitBit;
	bMantissa |= constants->implicitBit;
	uint8_t resultSign = aSign ^ bSign;
	uint32_t resultMantissa = ((uint64_t)aMantissa * (uint64_t)bMantissa) >> constants->mantissaShift;
	int16_t resultExp = (int16_t)(aExp + bExp);
	uint32_t extraBits = (resultMantissa >> constants->mantissaShift) & (((1U << constants->mantissaShift)));
	uint32_t gRsBits = (extraBits >> (constants->mantissaShift - 3));
	normalizeNumber(&resultMantissa, &resultExp, constants);
	return rounding(resultSign, resultMantissa, resultExp, round, constants, '*', gRsBits);
}

uint32_t divide(uint64_t a, uint64_t b, FormatConstants *constants, uint8_t round)
{
	uint32_t negateInf = constants->negateInf;
	uint32_t inf = constants->inf;
	uint32_t nan = constants->nan;
	uint8_t shift = constants->shift;
	if (a == constants->negateNull && b == negateInf)
		return 0;
	if (((a == negateInf || a == inf) && b == inf != 0 || (a == nan)) || (a == 0x0 && b == 0x0) ||
		((a == inf || a == negateInf) && b == negateInf))
		return nan;
	if (b == negateInf)
		return negateInf;
	if (a == 0x0)
		return (b >> shift) << shift;
	if (a == constants->negateNull)
		return (a >> shift) << shift;
	if (a == inf || a == negateInf)
		return ((b >> shift) == 1 ? negateInf : inf) ^ (a == negateInf ? inf ^ negateInf : 0);
	if (b == 0x0)
		return (a >> shift) == 1 ? negateInf : inf;
	uint8_t aSign, bSign;
	uint32_t aMantissa, bMantissa;
	int16_t aExp, bExp;
	getParts(a, &aSign, &aMantissa, &aExp, constants);
	getParts(b, &bSign, &bMantissa, &bExp, constants);
	aMantissa |= constants->implicitBit;
	bMantissa |= constants->implicitBit;
	uint8_t resultSign = aSign ^ bSign;
	uint32_t resultMantissa = ((uint64_t)aMantissa << constants->mantissaShift) / bMantissa;
	uint32_t extraBits = resultMantissa & ((1U << constants->mantissaShift) - 1);
	uint32_t gRsBits =
		(extraBits > 0) ? 0x4 | ((extraBits >> (constants->mantissaShift - 2)) & 0x3) : (extraBits >> (constants->mantissaShift - 2)) & 0x7;
	int16_t resultExp = (int16_t)(aExp - bExp);
	normalizeNumber(&resultMantissa, &resultExp, constants);
	return rounding(resultSign, resultMantissa, resultExp, round, constants, '/', gRsBits);
}


/*

██╗███╗░░██╗████████╗███████╗██████╗░███████╗░█████╗░░█████╗░███████╗      ░█████╗░███╗░░██╗██████╗░
██║████╗░██║╚══██╔══╝██╔════╝██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔════╝      ██╔══██╗████╗░██║██╔══██╗
██║██╔██╗██║░░░██║░░░█████╗░░██████╔╝█████╗░░███████║██║░░╚═╝█████╗░░      ███████║██╔██╗██║██║░░██║
██║██║╚████║░░░██║░░░██╔══╝░░██╔══██╗██╔══╝░░██╔══██║██║░░██╗██╔══╝░░      ██╔══██║██║╚████║██║░░██║
██║██║░╚███║░░░██║░░░███████╗██║░░██║██║░░░░░██║░░██║╚█████╔╝███████╗      ██║░░██║██║░╚███║██████╔╝
╚═╝╚═╝░░╚══╝░░░╚═╝░░░╚══════╝╚═╝░░╚═╝╚═╝░░░░░╚═╝░░╚═╝░╚════╝░╚══════╝      ╚═╝░░╚═╝╚═╝░░╚══╝╚═════╝░

 */

uint64_t performOperation(uint64_t a, uint64_t b, uint8_t round, FormatConstants *constants, char format, char op)
{
	switch (op)
	{
	case '+':
		return add(a, b, constants, round);
	case '-':
		if (format == 'f')
			b ^= 0x80000000;
		else
			b ^= 0x8000;
		return add(a, b, constants, round);
	case '*':
		return multiply(a, b, constants, round);
	case '/':
		return divide(a, b, constants, round);
	default:
		return constants->nan;
	}
}

void printFormatted(uint32_t hex, char format, FormatConstants *constants)
{
	uint8_t sign;
	uint32_t mantissa;
	int16_t exp;
	getParts(hex, &sign, &mantissa, &exp, constants);
	if (exp == -constants->maxExp && mantissa == 0)
	{
		printf("%s0x0.%sp+0\n", sign == 1 ? "-" : "", format == 'f' ? "000000" : "000");
		return;
	}
	if (exp == -constants->maxExp && mantissa != 0)
		normalizeNumberToFormat(&mantissa, (int16_t *)&exp, constants);
	if ((exp == constants->maxExp + 1 || exp == -constants->maxExp - 1) && mantissa == 0)
	{
		printf("%sinf", sign == 1 ? "-" : "");
		return;
	}
	if ((exp == -constants->maxExp - 1 || exp == constants->maxExp + 1) && mantissa != 0)
	{
		printf("nan\n");
		return;
	}
	if (sign)
		printf("-");
	if (format == 'f')
		printf("0x1.%.6xp%+i\n", mantissa << 1, exp);
	else
		printf("0x1.%.3xp%+i\n", mantissa << 2, exp);
}

int main(int argc, char *argv[])
{
	char *end;
	if ((argc != 6 && argc != 4) || ('\0' != argv[1][1]) || !((argv[1][0] == 'f') || (argv[1][0] == 'h')) ||
		(strtoull(argv[2], &end, 10) > 3))
	{
		fprintf(stderr, "Invalid arguments");
		return ERROR_ARGUMENTS_INVALID;
	}
	char format = argv[1][0];
	FormatConstants constants;
	initFormatConstants(format, &constants);
	uint8_t round = strtoull(argv[2], &end, 10);
	if (3 < round || round < 0)
	{
		fprintf(stderr, "incorrect rounding ");
		return ERROR_ARGUMENTS_INVALID;
	}
	uint32_t result;
	uint32_t num1 = strtoull(argv[3], &end, 16);

	if (argc > 4)
	{
		if (argv[4][0] != '+' && argv[4][0] != '*' && argv[4][0] != '-' && argv[4][0] != '/')
		{
			fprintf(stderr, "Incorrect format");
			return ERROR_ARGUMENTS_INVALID;
		}
		uint32_t num2 = strtoull(argv[5], &end, 16);
		result = performOperation(num1, num2, round, &constants, format, argv[4][0]);
	}
	else
		result = num1;

	printFormatted(result, format, &constants);
	return SUCCESS;
}
//
