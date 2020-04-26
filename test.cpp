#include "include/charconv.hpp"

#include <iomanip>
#include <iostream>

//template<typename T, uint64_t Len>
//T test(const char str[Len], std::chars_format format = std::chars_format::general) {
//	T value = -1.0;
//	std::from_chars_result result = std::from_chars(str, str + Len, value, format);
//	std::cout << "input: " << str << std::endl;
//	std::cout << "match length: " << result.ptr - str << std::endl;
//	std::cout << "ec: ";
//	if (result.ec == std::errc()) {
//		std::cout << "ok";
//	}
//	else if (result.ec == std::errc::invalid_argument) {
//		std::cout << "invalid_argument";
//	}
//	else if (result.ec == std::errc::result_out_of_range) {
//		std::cout << "result_out_of_range";
//	}
//	else {
//		std::cout << "unknown";
//	}
//	std::cout << std::endl;
//	std::cout << "result: " << value << "\n\n";
//	return value;
//}

int testNumber = 0;

void print(std::errc er, std::ostream& out) {
	if (er == std::errc()) {
		out << "ok";
	}
	else if (er == std::errc::result_out_of_range) {
		out << "result_out_of_range";
	}
	else if (er == std::errc::invalid_argument) {
		out << "invalid_argument";
	}
	else {
		out << "unknown";
	}
}

constexpr const long double UNINITIALIZED = std::numeric_limits<long double>::quiet_NaN();

template<typename T>
int test(
	const char *first, const char *last, std::chars_format fmt, T valueOut, const char *convEnd,
	std::errc ec
) {
	std::cout << "[Test Number " << ++testNumber << ": \"" + std::string(first, last - first) << "\"]\n";
	T value = UNINITIALIZED;
	std::from_chars_result result = std::from_chars(first, last, value, fmt);
	int ret = 0;
	if (
		value != valueOut && !std::isnan(value)
		&& valueOut != std::nextafter(value, -value) && valueOut != std::nextafter(value, 2*value)
	) {
		std::cerr << std::fixed;
		std::cerr << "Expected value: " << std::setprecision(20) << valueOut << " Actual value: ";
		std::cerr << std::setprecision(20) << value << std::endl;
		ret = 1;
	}
	if (convEnd != result.ptr) {
		std::cerr << "Expected ptr: " << (void*)convEnd << " Actual ptr: " << (void*)result.ptr;
		std::cerr << std::endl;
		ret = 1;
	}
	if (ec != result.ec) {
		std::cerr << "Expected ec: ";
		print(ec, std::cerr);
		std::cerr << " Actual ec: ";
		print(result.ec, std::cerr);
		std::cerr << std::endl;
		ret = 1;
	}
	std::cout << "[Test Number " << testNumber << " ";
	if (ret == 0) {
		std::cout << "PASSED";
	}
	else {
		std::cout << "FAILED";
	}
	std::cout << "]\n";
	return ret;
}

int main() {
	const char *a = "5";
	const char *b = "0x123";
	const char *c = "+1";
	const char *d = "4E3";
	const char *e = " \t3.1";
	const char *f = "-2";
	const char *g = "15e-5";
	const char *h = "-51.23P23";
	const char *i = "f.4p+2a";
	const char *j = "-InF";
	const char *k = "INfInItY";
	const char *l = "Nan";
	const char *m = "NAN(slk38_klj)";
	const char *n = "nan(*)";
	const char *o = "51E200";
	const char *p = "";
	const char *q = "hello";
	const char *r = "4.5";
	const char *s = "5E";
	const char *t = "3E+";
	int failures = 0;

	failures += test<double>(a, a + 1, std::chars_format::general, 5, a + 1, std::errc());
	failures += test<long double>(b, b + 5, std::chars_format::general, 0.0, b + 1, std::errc());
	failures += test<float>(c, c + 2, std::chars_format::general, UNINITIALIZED, c, std::errc::invalid_argument);
	failures += test<double>(a, a + 1, std::chars_format::scientific, UNINITIALIZED, a, std::errc::invalid_argument);
	failures += test<float>(d, d + 3, std::chars_format::fixed, 4.0, d + 1, std::errc());
	failures += test<long double>(d, d + 3, std::chars_format::general, 4000, d + 3, std::errc());
	failures += test<double>(e, e + 5, std::chars_format::general, 3.1, e + 5, std::errc());
	failures += test<float>(f, f + 2, std::chars_format::general, -2, f + 2, std::errc());
	failures += test<double>(g, g + 5, std::chars_format::general, 15e-5, g + 5, std::errc());
	failures += test<float>(h, h + 9, std::chars_format::hex, -680624128, h + 9, std::errc());
	failures += test<double>(i, i + 7, std::chars_format::hex, 61, i + 6, std::errc());
	failures += test<double>(j, j + 4, std::chars_format::general, -std::numeric_limits<double>::infinity(), j + 4, std::errc());
	failures += test<float>(k, k + 8, std::chars_format::general, std::numeric_limits<float>::infinity(), k + 8, std::errc());
	failures += test<float>(l, l + 3, std::chars_format::general, std::numeric_limits<float>::quiet_NaN(), l + 3, std::errc());
	failures += test<float>(m, m + 14, std::chars_format::general, std::numeric_limits<float>::quiet_NaN(), m + 14, std::errc());
	failures += test<float>(n, n + 6, std::chars_format::general, std::numeric_limits<float>::quiet_NaN(), n + 3, std::errc());
	failures += test<float>(o, o + 6, std::chars_format::general, UNINITIALIZED, o + 6, std::errc::result_out_of_range);
	failures += test<double>(p, p, std::chars_format::general, UNINITIALIZED, p, std::errc::invalid_argument);
	failures += test<float>(q, q + 5, std::chars_format::general, UNINITIALIZED, q, std::errc::invalid_argument);
	failures += test<float>(r, r + 3, std::chars_format::general, 4.5, r + 3, std::errc());
	failures += test<float>(s, s + 2, std::chars_format::general, 5, s + 1, std::errc());
	failures += test<float>(s, s + 2, std::chars_format::scientific, UNINITIALIZED, s, std::errc::invalid_argument);
	failures += test<float>(t, t + 3, std::chars_format::general, 3, t + 1, std::errc());
	failures += test<float>(t, t + 3, std::chars_format::scientific, UNINITIALIZED, t, std::errc::invalid_argument);

	if (failures == 0) {
		std::cout << "SUCCESS: All tests passed!\n";
	}
	else {
		std::cout << "FAILURE: " << failures << " tests failed!\n";
	}

//	test<double, 1>("5");
//	test<float, 3>("4.2");
//	test<long double, 2>("-5");
//	test<float, 3>("2E5");
//	test<double, 4>("5e-2");
}
