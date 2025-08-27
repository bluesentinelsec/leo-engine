// Minimal Tiled-map style parse test using leo_json_* (no VFS needed).
#include "leo/json.h"
#include <catch2/catch_all.hpp>
#include <cstring>

static const char* kTiledJSON = R"JSON(
{
  "height": 10,
  "width": 12,
  "tilewidth": 32,
  "tileheight": 32,
  "orientation": "orthogonal",
  "renderorder": "right-down",
  "layers": [
    {
      "type": "tilelayer",
      "name": "Ground",
      "width": 12,
      "height": 10,
      "data": [1,0,3,4,0,0,  2,2,2,0,0,1,  0,0,0,0,0,0,  5,5,5,5,5,5]
    },
    {
      "type": "objectgroup",
      "name": "Spawns",
      "objects": [
        { "name": "Player", "x": 64, "y": 128, "width": 32, "height": 32 },
        { "name": "Enemy",  "x": 160, "y": 128, "width": 32, "height": 32 }
      ]
    }
  ],
  "properties": [
    { "name": "music", "type": "string", "value": "intro_theme.ogg" },
    { "name": "difficulty", "type": "int", "value": 2 },
    { "name": "night", "type": "bool", "value": false }
  ]
}
)JSON";

TEST_CASE("json: parse minimal Tiled map header and layers")
{
	const char* err = nullptr;
	leo_JsonDoc* doc = leo_json_parse(kTiledJSON, std::strlen(kTiledJSON), &err);
	REQUIRE(doc != nullptr);

	leo_JsonNode root = leo_json_root(doc);

	int width = 0, height = 0, tw = 0, th = 0;
	REQUIRE(leo_json_get_int(root, "width", &width));
	REQUIRE(leo_json_get_int(root, "height", &height));
	REQUIRE(leo_json_get_int(root, "tilewidth", &tw));
	REQUIRE(leo_json_get_int(root, "tileheight", &th));

	CHECK(width == 12);
	CHECK(height == 10);
	CHECK(tw == 32);
	CHECK(th == 32);

	// orientation/renderorder sanity
	const char* orientation = nullptr;
	const char* renderorder = nullptr;
	REQUIRE(leo_json_get_string(root, "orientation", &orientation));
	REQUIRE(leo_json_get_string(root, "renderorder", &renderorder));
	CHECK(std::strcmp(orientation, "orthogonal") == 0);
	CHECK(std::strcmp(renderorder, "right-down") == 0);

	// layers[] array present
	leo_JsonNode layers = leo_json_obj_get(root, "layers");
	REQUIRE(leo_json_is_array(layers));
	REQUIRE(leo_json_arr_size(layers) >= 2);

	// Inspect first layer (tilelayer)
	{
		leo_JsonNode layer0 = leo_json_arr_get(layers, 0);
		REQUIRE_FALSE(leo_json_is_null(layer0));

		const char* type = leo_json_as_string(leo_json_obj_get(layer0, "type"));
		const char* name = leo_json_as_string(leo_json_obj_get(layer0, "name"));
		REQUIRE(type != nullptr);
		REQUIRE(name != nullptr);
		CHECK(std::strcmp(type, "tilelayer") == 0);
		CHECK(std::strcmp(name, "Ground") == 0);

		// tile data array
		leo_JsonNode data = leo_json_obj_get(layer0, "data");
		REQUIRE(leo_json_is_array(data));
		size_t n = leo_json_arr_size(data);
		REQUIRE(n > 0);

		// Spot-check a few values
		int t0 = leo_json_as_int(leo_json_arr_get(data, 0));
		int t1 = leo_json_as_int(leo_json_arr_get(data, 1));
		int t2 = leo_json_as_int(leo_json_arr_get(data, 2));
		CHECK(t0 == 1);
		CHECK(t1 == 0);
		CHECK(t2 == 3);
	}

	// Inspect second layer (objectgroup)
	{
		leo_JsonNode layer1 = leo_json_arr_get(layers, 1);
		REQUIRE_FALSE(leo_json_is_null(layer1));

		const char* type = leo_json_as_string(leo_json_obj_get(layer1, "type"));
		const char* name = leo_json_as_string(leo_json_obj_get(layer1, "name"));
		REQUIRE(type != nullptr);
		REQUIRE(name != nullptr);
		CHECK(std::strcmp(type, "objectgroup") == 0);
		CHECK(std::strcmp(name, "Spawns") == 0);

		leo_JsonNode objects = leo_json_obj_get(layer1, "objects");
		REQUIRE(leo_json_is_array(objects));
		REQUIRE(leo_json_arr_size(objects) == 2);

		// First object: Player
		{
			leo_JsonNode obj0 = leo_json_arr_get(objects, 0);
			const char* oname = leo_json_as_string(leo_json_obj_get(obj0, "name"));
			int x = leo_json_as_int(leo_json_obj_get(obj0, "x"));
			int y = leo_json_as_int(leo_json_obj_get(obj0, "y"));
			CHECK(oname != nullptr);
			CHECK(std::strcmp(oname, "Player") == 0);
			CHECK(x == 64);
			CHECK(y == 128);
		}

		// Second object: Enemy
		{
			leo_JsonNode obj1 = leo_json_arr_get(objects, 1);
			const char* oname = leo_json_as_string(leo_json_obj_get(obj1, "name"));
			int x = leo_json_as_int(leo_json_obj_get(obj1, "x"));
			int y = leo_json_as_int(leo_json_obj_get(obj1, "y"));
			CHECK(oname != nullptr);
			CHECK(std::strcmp(oname, "Enemy") == 0);
			CHECK(x == 160);
			CHECK(y == 128);
		}
	}

	// Tiled "properties" array exists and can be read
	{
		leo_JsonNode props = leo_json_obj_get(root, "properties");
		REQUIRE(leo_json_is_array(props));
		CHECK(leo_json_arr_size(props) == 3);

		// Check the typed values through generic access
		// music: string
		{
			leo_JsonNode p = leo_json_arr_get(props, 0);
			const char* name = leo_json_as_string(leo_json_obj_get(p, "name"));
			const char* type = leo_json_as_string(leo_json_obj_get(p, "type"));
			const char* value = leo_json_as_string(leo_json_obj_get(p, "value"));
			REQUIRE(name != nullptr);
			REQUIRE(type != nullptr);
			REQUIRE(value != nullptr);
			CHECK(std::strcmp(name, "music") == 0);
			CHECK(std::strcmp(type, "string") == 0);
			CHECK(std::strcmp(value, "intro_theme.ogg") == 0);
		}
		// difficulty: int
		{
			leo_JsonNode p = leo_json_arr_get(props, 1);
			const char* name = leo_json_as_string(leo_json_obj_get(p, "name"));
			const char* type = leo_json_as_string(leo_json_obj_get(p, "type"));
			int value = leo_json_as_int(leo_json_obj_get(p, "value"));
			REQUIRE(name != nullptr);
			REQUIRE(type != nullptr);
			CHECK(std::strcmp(name, "difficulty") == 0);
			CHECK(std::strcmp(type, "int") == 0);
			CHECK(value == 2);
		}
		// night: bool
		{
			leo_JsonNode p = leo_json_arr_get(props, 2);
			const char* name = leo_json_as_string(leo_json_obj_get(p, "name"));
			const char* type = leo_json_as_string(leo_json_obj_get(p, "type"));
			bool value = leo_json_as_bool(leo_json_obj_get(p, "value"));
			REQUIRE(name != nullptr);
			REQUIRE(type != nullptr);
			CHECK(std::strcmp(name, "night") == 0);
			CHECK(std::strcmp(type, "bool") == 0);
			CHECK(value == false);
		}
	}

	leo_json_free(doc);
}


static const char* kEdgeJSON = R"JSON(
{
  "str": "hello",
  "num": 42,
  "dbl": 3.5,
  "flag": true,
  "obj": { "inner": "x" },
  "arr": [1, "two", false]
}
)JSON";

TEST_CASE("json: basic typed getters succeed and fail appropriately")
{
	const char* err = nullptr;
	leo_JsonDoc* doc = leo_json_parse(kEdgeJSON, std::strlen(kEdgeJSON), &err);
	REQUIRE(doc != nullptr);

	leo_JsonNode root = leo_json_root(doc);

	// Success paths
	{
		const char* s = nullptr;
		int i = 0;
		double d = 0.0;
		bool b = false;

		REQUIRE(leo_json_get_string(root, "str", &s));
		REQUIRE(leo_json_get_int(root, "num", &i));
		REQUIRE(leo_json_get_double(root, "dbl", &d));
		REQUIRE(leo_json_get_bool(root, "flag", &b));

		CHECK(std::strcmp(s, "hello") == 0);
		CHECK(i == 42);
		CHECK(d == Catch::Approx(3.5));
		CHECK(b == true);
	}

	// Missing keys return false / null node
	{
		const char* s = "unchanged";
		int i = -1;
		double d = -1.0;
		bool b = true;

		CHECK_FALSE(leo_json_get_string(root, "nope", &s));
		CHECK_FALSE(leo_json_get_int(root, "nope", &i));
		CHECK_FALSE(leo_json_get_double(root, "nope", &d));
		CHECK_FALSE(leo_json_get_bool(root, "nope", &b));

		// as_* from missing/invalid node should return default-y values
		leo_JsonNode missing = leo_json_obj_get(root, "nope");
		CHECK(leo_json_is_null(missing));
		CHECK(leo_json_as_string(missing) == nullptr);
		CHECK(leo_json_as_int(missing) == 0);
		CHECK(leo_json_as_double(missing) == 0.0);
		CHECK(leo_json_as_bool(missing) == false);
	}

	// Array iteration + mixed types checks
	{
		leo_JsonNode arr = leo_json_obj_get(root, "arr");
		REQUIRE(leo_json_is_array(arr));
		REQUIRE(leo_json_arr_size(arr) == 3);

		CHECK(leo_json_as_int(leo_json_arr_get(arr, 0)) == 1);
		CHECK(std::strcmp(leo_json_as_string(leo_json_arr_get(arr, 1)), "two") == 0);
		CHECK(leo_json_as_bool(leo_json_arr_get(arr, 2)) == false);
	}

	// Object access and type predicates
	{
		leo_JsonNode obj = leo_json_obj_get(root, "obj");
		REQUIRE(leo_json_is_object(obj));
		const char* inner = leo_json_as_string(leo_json_obj_get(obj, "inner"));
		REQUIRE(inner != nullptr);
		CHECK(std::strcmp(inner, "x") == 0);
	}

	leo_json_free(doc);
}
