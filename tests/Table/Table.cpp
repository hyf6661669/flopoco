#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TableTest

#include <iostream>
#include <vector>

#include <gmpxx.h>
#include <boost/test/unit_test.hpp>

#include "Table.hpp"

using std::vector;

BOOST_AUTO_TEST_CASE(TestDifferentialCompressionLarge)
{
	constexpr size_t TABLE_WIN = 8;
	constexpr size_t TABLE_SIZE = 1 << TABLE_WIN;
	constexpr size_t TABLE_WOUT = 13;
	constexpr size_t TABLE_FOLD_WIDTH = 3;
	constexpr size_t FOLD_SIZE = 1 << TABLE_FOLD_WIDTH;
	vector<mpz_class> val(TABLE_SIZE);
	mpz_class offset{1 << (TABLE_WOUT - 1)};

	for (size_t i = 0 ; i < TABLE_SIZE ; i += FOLD_SIZE) {
		for (size_t j = 0 ; j < FOLD_SIZE ; j++) {
			val[i+j] = offset + j;
		}
	}

	BOOST_TEST_CHECKPOINT("Calling find_differential_compression");
	auto diff_compress = Table::find_differential_compression(val, TABLE_WIN, TABLE_WOUT);

	BOOST_TEST_CHECKPOINT("find_differential_compression returned");
	auto reconstructedTable = Table::reconstructTable(diff_compress);
	for (size_t i = 0 ; i < (1 << (TABLE_WIN)) ; ++i) {
		BOOST_REQUIRE_MESSAGE(reconstructedTable[i] == val[i], "Error with reconstitution of table value " <<i << ": got " << reconstructedTable[i] << " instead of " << val[i]);
	}
}
