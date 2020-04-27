#ifndef CHARCONV_HPP
#define CHARCONV_HPP


#include <cmath>
#include <limits>
#include <system_error>


namespace std {

	struct from_chars_result {
		const char *ptr;
		errc ec;
	};

	enum class chars_format {
		scientific = 1,
		fixed = 2,
		hex = 4,
		general = fixed | scientific
	};


	namespace __internal {

		// Make sure the result isn't out of bounds, assign to the value variable, and return
		template<typename Num, enable_if_t<is_floating_point<Num>::value, int> = 0>
		constexpr from_chars_result __exit_from_chars(const char *curr, long double result, bool negative, Num *value) {
			if (result > numeric_limits<Num>::max()) {
				return {curr, errc::result_out_of_range};
			}
			*value = negative ? -result : result;
			return {curr, errc()};
		}

		// Compare the string [first1, last1) to [first2, first2 + last1-first1) in a (mostly)
		// case-insensitive fashion. The case of [first1, last1) doesn't matter, but the string
		// starting at first2 must be all lower case.
		bool __case_insensitive_compare(const char *first1, const char *last1, const char *first2) {
			return std::equal(first1, last1, first2, [](char lhs, char rhs) -> bool {
				return tolower(lhs) == rhs;
			});
		}

		// This function does all the real work for floating point implementation of from_chars
		template<int (*IsDigit)(int), int(*DigitToNumber)(char), int Base, int ExpChar, typename Num, int ExponentBase = Base, enable_if_t<is_floating_point<Num>::value, int> = 0>
		constexpr from_chars_result __from_chars_helper(const char *first, const char *last, Num& value, chars_format fmt) {

			// Skip white space
			const char *curr = first;
			while (isspace(*curr) && curr != last) {
				++curr;
			}

			// Return invalid_argument if string is empty or all white space
			if (curr == last) {
				return {first, errc::invalid_argument};
			}

			// Check sign
			bool negative = *curr == '-';
			if (negative) {
				++curr;
			}
			if (curr == last) {
				return {first, errc::invalid_argument};
			}

			// Handle NaN and infinity
			if (last - curr >= 3) {
				const char *wordEnd = curr + 3;
				if (__case_insensitive_compare(curr, wordEnd, "inf")) {
					value = std::numeric_limits<Num>::infinity();
					if (negative) {
						value = -value;
					}
					if (last - curr >= 8 && __case_insensitive_compare(curr, curr + 8, "infinity")) {
						return {curr + 8, errc()};
					}
					else {
						return {wordEnd, errc()};
					}
				}
				else if (__case_insensitive_compare(curr, wordEnd, "nan")) {
					value = std::numeric_limits<Num>::quiet_NaN();
					if (negative) {
						value = -value;
					}
					curr = wordEnd;
					if (*curr == '(') {
						for (++curr; curr != last && (isalnum(*curr) || *curr == '_'); ++curr) {}
						if (curr != last && *curr == ')') {
							return {curr + 1, errc()};
						}
					}
					return {wordEnd, errc()};
				}
			}

			// Handle part before the decimal point
			long double result = 0.0;
			char nextChar = *curr;
			if (IsDigit(nextChar)) {
				result = DigitToNumber(nextChar);
			}
			else {
				return {first, errc::invalid_argument};
			}
			while (++curr != last && IsDigit(nextChar = *curr)) {
				result = result*Base + DigitToNumber(nextChar);
			}

			// Handle fraction part
			if (nextChar == '.') {
				constexpr long double OneOverBase = 1.0 / Base;
				long double digitMultiplier = OneOverBase;
				while (++curr != last && IsDigit(nextChar = *curr)) {
					result += DigitToNumber(nextChar) * digitMultiplier;
					digitMultiplier *= OneOverBase;
				}
			}

			// Check whether scientific notation is allowed/permitted
			bool fixed = int(fmt) & int(chars_format::fixed);
			bool scientific = int(fmt) & int(chars_format::scientific);
			bool exponentRequired = scientific && !fixed;
			bool exponentAllowed = scientific || !fixed;
			if (curr == last) {
				if (exponentRequired) {
					return {first, errc::invalid_argument};
				}
				return __exit_from_chars(curr, result, negative, &value);
			}

			// Handle exponent
			if (exponentAllowed && (nextChar == ExpChar || nextChar == tolower(ExpChar))) {
				const char *significandEnd = curr;
				if (++curr == last) {
					if (!exponentRequired) {
						return __exit_from_chars(significandEnd, result, negative, &value);
					}
					else {
						return {first, errc::invalid_argument};
					}
				}
				nextChar = *curr;
				bool exponentNegative = nextChar == '-';
				if (exponentNegative || nextChar == '+') {
					++curr;
					if (curr == last) {
						if (!exponentRequired) {
							return __exit_from_chars(significandEnd, result, negative, &value);
						}
						else {
							return {first, errc::invalid_argument};
						}
					}
				}
				nextChar = *curr;
				if (!isdigit(nextChar)) {
					if (!exponentRequired) {
						return __exit_from_chars(significandEnd, result, negative, &value);
					}
					else {
						return {first, errc::invalid_argument};
					}
				}
				long long exponent = nextChar - '0';
				while (++curr != last && isdigit(nextChar = *curr)) {
					exponent = exponent*10 + nextChar - '0';
				}
				if (exponentNegative) {
					exponent = -exponent;
				}
				result *= pow(ExponentBase, exponent);
			}
			else if (exponentRequired) {
				return {first, errc::invalid_argument};
			}

			return __exit_from_chars(curr, result, negative, &value);
		}

		// Convert decimal digit to number
		constexpr int __decimal_to_number(char ch) {
			return ch - '0';
		}

		// Convert hexadecimal digit to number
		constexpr int __hex_to_number(char ch) {
			if (isdigit(ch)) {
				return ch - '0';
			}
			else {
				return tolower(ch) - ('a' - 10);
			}
		}

		// templated implementation of from_chars for floating point
		template<typename Num, enable_if_t<is_floating_point<Num>::value, int> = 0>
		constexpr from_chars_result __from_chars(const char *first, const char *last, Num& value, chars_format fmt) {
			// Call __from_chars_helper templated for the proper base
			if (int(fmt) & int(chars_format::hex)) {
				return __from_chars_helper<&isxdigit, &__hex_to_number, 16, 'P', Num, 2>(
					first, last, value, fmt
				);
			}
			else {
				return __from_chars_helper<&isdigit, &__decimal_to_number, 10, 'E'>(
					first, last, value, fmt
				);
			}
		}
	}

	// from_chars for float
	from_chars_result from_chars(const char *first, const char *last, float& value, chars_format fmt = chars_format::general) {
		return __internal::__from_chars(first, last, value, fmt);
	}

	// from_chars for double
	from_chars_result from_chars(const char *first, const char *last, double& value, chars_format fmt = chars_format::general) {
		return __internal::__from_chars(first, last, value, fmt);
	}

	// from_chars for long double
	from_chars_result from_chars(const char *first, const char *last, long double& value, chars_format fmt = chars_format::general) {
		return __internal::__from_chars(first, last, value, fmt);
	}
}


#endif

