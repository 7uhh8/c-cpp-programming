#include <stdint.h>
#include <stdio.h>

uint8_t numberLength(uint32_t n)
{
	uint8_t result = 1;
	while (n >= 10)
	{
		n /= 10;
		result++;
	}
	return result;
}

uint16_t max(uint16_t a, uint16_t b)
{
	return a > b ? a : b;
}

#define M31 (2147483647)	// 2^31 - 1
uint32_t factorial(uint16_t n)
{
	uint32_t result = 1;
	for (uint32_t i = 1; i <= n; i++)
	{
		result = ((uint64_t)result * i) % M31;
	}
	return result;
}

uint8_t factMaxLength(uint16_t n_start, uint16_t n_end)
{
	uint8_t result = 0;
	if (n_start > n_end)
	{
		for (uint16_t n = n_start; n <= UINT16_MAX && result < 10; n++)	   // max length is 10
		{
			result = max(result, numberLength(factorial(n)));
		}
		for (uint16_t n = 0; n <= n_end && result < 10; n++)
		{
			result = max(result, numberLength(factorial(n)));
		}
	}
	else
	{
		for (uint16_t n = n_start; n <= n_end && result < 10; n++)
		{
			result = max(result, numberLength(factorial(n)));
		}
	}
	return result;
}

void printWhitespaces(uint8_t amount)
{
	for (uint8_t i = 0; i < amount; i++)
	{
		printf(" ");
	}
}

void printEdge(uint8_t n_len, uint8_t fact_len)
{
	printf("+");
	for (uint8_t i = 0; i < n_len + 2; ++i)
	{
		printf("-");
	}
	printf("+");
	for (uint8_t i = 0; i < max(fact_len, 2) + 2; ++i)
	{
		printf("-");
	}
	printf("+\n");
}

void printHeader(uint8_t n_len, uint8_t fact_len, int8_t align)
{
	switch (align)
	{
	case -1:
		printf("| n");
		printWhitespaces(n_len);
		printf("| n!");
		printWhitespaces(max(fact_len, 2) - 1);
		printf("|\n");
		break;
	case 1:
		printf("|");
		printWhitespaces(n_len);
		printf("n |");
		printWhitespaces(max(fact_len, 2) - 1);
		printf("n! |\n");
		break;
	default:
		printf("|");
		printWhitespaces(n_len / 2 + 1);
		printf("n");
		printWhitespaces((n_len - 1) / 2 + 1);
		printf("|");
		printWhitespaces((max(fact_len, 2) + 1) / 2);
		printf("n!");
		printWhitespaces((max(fact_len, 2)) / 2);
		printf("|\n");
		break;
	}
}

void printRow(uint16_t n, uint32_t n_fact, uint8_t n_len, uint8_t fact_len, int8_t align)
{
	uint8_t n_spaces = n_len - numberLength(n);
	uint8_t fact_spaces = max(fact_len, 2) - numberLength(n_fact);

	switch (align)
	{
	case -1:
		printf("| %d ", n);
		printWhitespaces(n_spaces);
		printf("| %u ", n_fact);
		printWhitespaces(fact_spaces);
		printf("|\n");
		break;
	case 1:
		printf("| ");
		printWhitespaces(n_spaces);
		printf("%d | ", n);
		printWhitespaces(fact_spaces);
		printf("%u |\n", n_fact);
		break;
	default:
		printf("|");
		printWhitespaces((n_spaces + 3) / 2);
		printf("%d", n);
		printWhitespaces(n_spaces / 2 + 1);
		printf("|");
		printWhitespaces((fact_spaces + 3) / 2);
		printf("%u", n_fact);
		printWhitespaces(fact_spaces / 2 + 1);
		printf("|\n");
		break;
	}
}

void printTable(uint16_t n_start, uint16_t n_end, int8_t align)
{
	uint8_t n_len = numberLength(max(n_start, n_end));
	uint8_t fact_len = factMaxLength(n_start, n_end);

	printEdge(n_len, fact_len);
	printHeader(n_len, fact_len, align);
	printEdge(n_len, fact_len);

	if (n_end < n_start)
	{
		for (uint16_t n = n_start;; n++)
		{
			printRow(n, factorial(n), n_len, fact_len, align);
			if (n == UINT16_MAX)
			{
				break;
			}
		}

		for (uint16_t n = 0; n <= n_end; n++)
		{
			printRow(n, factorial(n), n_len, fact_len, align);
		}
	}
	else
	{
		for (uint16_t n = n_start; n <= n_end; n++)
		{
			printRow(n, factorial(n), n_len, fact_len, align);
		}
	}

	printEdge(n_len, fact_len);
}

int main()
{
	int32_t n_start_temp, n_end_temp;
	int8_t align;
	if (scanf("%d %d %hhd", &n_start_temp, &n_end_temp, &align) < 3)
	{
		fprintf(stderr, "Incorrect input values. Please enter three integers.\n");
		return 1;
	}

	if (n_start_temp < 0 || n_end_temp < 0 || align > 1 || align < -1)
	{
		fprintf(stderr, "Incorrect input values. Please try again.\n");
		return 1;
	}

	uint16_t n_start = (uint16_t)n_start_temp;
	uint16_t n_end = (uint16_t)n_end_temp;

	printTable(n_start, n_end, align);

	return 0;
}
