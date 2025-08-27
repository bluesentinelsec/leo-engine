#include "leo/csv.h"
#include <catch2/catch_all.hpp>

#include <vector>
#include <string>
#include <cstring>

// -----------------------------
// Helpers
// -----------------------------
struct Table
{
	std::vector<std::vector<std::string> > rows;
};

static int collect_cell(void* user, const char* cell, size_t len, size_t row, size_t col)
{
	Table* t = static_cast<Table*>(user);
	if (row >= t->rows.size()) t->rows.resize(row + 1);
	auto& r = t->rows[row];
	if (col >= r.size()) r.resize(col + 1);
	r[col].assign(cell, cell + len);
	return 0; // continue
}

static leo_csv_opts defaults()
{
	leo_csv_opts o{};
	o.delimiter = ',';
	o.quote = '"';
	o.trim_ws = 1;
	o.allow_crlf = 1;
	return o;
}

// -----------------------------
// Tests
// -----------------------------

TEST_CASE("csv: basic parse with quotes and escapes")
{
	const char* src =
			"name,age,note\n"
			"\"Doe, John\",27,\"He said \"\"hi\"\".\"";

	Table t;
	auto o = defaults();
	leo_csv_result r = leo_csv_parse(src, std::strlen(src), &o, collect_cell, &t);
	REQUIRE(r == LEO_CSV_OK);

	REQUIRE(t.rows.size() == 2);
	REQUIRE(t.rows[0].size() == 3);
	REQUIRE(t.rows[1].size() == 3);

	CHECK(t.rows[0][0] == "name");
	CHECK(t.rows[0][1] == "age");
	CHECK(t.rows[0][2] == "note");

	CHECK(t.rows[1][0] == "Doe, John");
	CHECK(t.rows[1][1] == "27");
	CHECK(t.rows[1][2] == "He said \"hi\".");
}

TEST_CASE("csv: CRLF line endings and whitespace trimming")
{
	const char* src = "a , b , c\r\n 1,2 ,  3 \r\n";
	Table t;
	auto o = defaults();
	o.allow_crlf = 1;
	o.trim_ws = 1;

	leo_csv_result r = leo_csv_parse(src, std::strlen(src), &o, collect_cell, &t);
	REQUIRE(r == LEO_CSV_OK);

	REQUIRE(t.rows.size() == 2);
	REQUIRE(t.rows[0].size() == 3);
	REQUIRE(t.rows[1].size() == 3);

	CHECK(t.rows[0][0] == "a");
	CHECK(t.rows[0][1] == "b");
	CHECK(t.rows[0][2] == "c");

	CHECK(t.rows[1][0] == "1");
	CHECK(t.rows[1][1] == "2");
	CHECK(t.rows[1][2] == "3");
}

TEST_CASE("csv: custom delimiter without trimming")
{
	const char* src = "alpha; beta ;gamma\nx ; y; z";
	Table t;
	leo_csv_opts o{};
	o.delimiter = ';';
	o.quote = '"';
	o.trim_ws = 0;
	o.allow_crlf = 1;

	leo_csv_result r = leo_csv_parse(src, std::strlen(src), &o, collect_cell, &t);
	REQUIRE(r == LEO_CSV_OK);

	REQUIRE(t.rows.size() == 2);
	REQUIRE(t.rows[0].size() == 3);

	// No trimming: spaces are preserved
	CHECK(t.rows[0][0] == "alpha");
	CHECK(t.rows[0][1] == " beta ");
	CHECK(t.rows[0][2] == "gamma");

	CHECK(t.rows[1][0] == "x ");
	CHECK(t.rows[1][1] == " y");
	CHECK(t.rows[1][2] == " z");
}

TEST_CASE("csv: integer list helper (Tiled-style)")
{
	const char* src =
			"1, 2,3\n"
			"  4 ,5 ,6 \n"
			"\"7\" , \"8\" , 9";

	uint32_t* out = nullptr;
	size_t n = 0;
	auto o = defaults();
	leo_csv_result r = leo_csv_parse_uint32_alloc(src, std::strlen(src), &out, &n, &o);
	REQUIRE(r == LEO_CSV_OK);
	REQUIRE(out != nullptr);
	REQUIRE(n == 9);

	// Check some values
	CHECK(out[0] == 1u);
	CHECK(out[1] == 2u);
	CHECK(out[2] == 3u);
	CHECK(out[6] == 7u);
	CHECK(out[7] == 8u);
	CHECK(out[8] == 9u);

	free(out);
}

TEST_CASE("csv: count values heuristic")
{
	const char* src = "1,2,3\n4,5\n6";
	auto o = defaults();
	size_t cnt = leo_csv_count_values(src, std::strlen(src), &o);
	CHECK(cnt == 6);
}

TEST_CASE("csv: early abort from callback stops parsing")
{
	struct
	{
		Table t;
		size_t stop_after_rows = 1;
	} ctx;

	auto abort_after_first_row = [](void* user, const char* cell, size_t len, size_t row, size_t col) -> int
	{
		auto* c = static_cast<decltype(&ctx)>(user);
		// Collect first, then conditionally abort
		collect_cell(&c->t, cell, len, row, col);
		if (row + 1 >= c->stop_after_rows)
		{
			// Returning non-zero requests an early stop; parser should return OK.
			return 1;
		}
		return 0;
	};

	const char* src =
			"h1,h2\n"
			"a,b\n"
			"c,d\n";

	auto o = defaults();
	leo_csv_result r = leo_csv_parse(src, std::strlen(src), &o, abort_after_first_row, &ctx);
	REQUIRE(r == LEO_CSV_OK);

	// Only header row should be collected
	REQUIRE(ctx.t.rows.size() == 1);
	REQUIRE(ctx.t.rows[0].size() == 2);
	CHECK(ctx.t.rows[0][0] == "h1");
	CHECK(ctx.t.rows[0][1] == "h2");
}

TEST_CASE("csv: malformed quoted field yields format error")
{
	// Missing delimiter or EOL after a closed quote with trailing junk: `"abc"def`
	const char* bad = "\"abc\"def";
	Table t;
	auto o = defaults();
	leo_csv_result r = leo_csv_parse(bad, std::strlen(bad), &o, collect_cell, &t);
	CHECK(r == LEO_CSV_E_FORMAT);
}

TEST_CASE("csv: empty input yields zero rows and OK")
{
	const char* src = "";
	Table t;
	auto o = defaults();
	leo_csv_result r = leo_csv_parse(src, 0, &o, collect_cell, &t);
	REQUIRE(r == LEO_CSV_OK);
	CHECK(t.rows.empty());
}
