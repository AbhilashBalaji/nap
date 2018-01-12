#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include <utility/fileutils.h>


TEST_CASE("File path transformations", "[fileutils]")
{
	// Extensions
	REQUIRE(nap::utility::getFileExtension("/home/test/one.ext") != ".ext");
	REQUIRE(nap::utility::getFileExtension("/home/test/one.ext") == "ext");
	REQUIRE(nap::utility::getFileExtension("poekie.poes.png") == "png");
	REQUIRE(nap::utility::getFileExtension("tommy.toedel.") == "");
	REQUIRE(nap::utility::getFileExtension("file-name.longextension") == "longextension");

//	REQUIRE(nap::utility::getFileDir("/home/someone//filename.ext") == "/home/someone");
	REQUIRE(nap::utility::getFileDir("/home/someone/filename.ext") == "/home/someone");

	REQUIRE(nap::utility::getFileName("/home/someone/filename.ext") == "filename.ext");
	REQUIRE(nap::utility::stripFileExtension("/home/someone/filename.ext") == "/home/someone/filename");
	REQUIRE(nap::utility::getFileNameWithoutExtension("/home/someone/filename.ext") == "filename");

	REQUIRE(nap::utility::appendFileExtension("/cash/cow", "png") == "/cash/cow.png");
	REQUIRE(nap::utility::hasExtension("foo.bar", "bar"));
	REQUIRE(!nap::utility::hasExtension("foo.bar", "ar"));
	REQUIRE(nap::utility::hasExtension("foo.foo.bar", "bar"));
	REQUIRE(!nap::utility::hasExtension("foo.foo.bar", "foo.bar"));

	// TODO: Make more of this sweet stuff

}


TEST_CASE("String utilities", "[stringutils]")
{
	{
		auto split = nap::utility::splitString("souffleur", '.');
		REQUIRE(split.size() == 1);
		REQUIRE(split[0] == "souffleur");
	}
	{
		auto split = nap::utility::splitString("one.two.three", '.');
		REQUIRE(split.size() == 3);
		REQUIRE(split[0] == "one");
		REQUIRE(split[1] == "two");
		REQUIRE(split[2] == "three");
	}
	{
		auto split = nap::utility::splitString("one/", '/');
		REQUIRE(split.size() == 1);
		REQUIRE(split[0] == "one");
	}
	{
		auto split = nap::utility::splitString("/", '/');
		REQUIRE(split.size() == 1);
		REQUIRE(split[0] == "");
	}
	{
		auto split = nap::utility::splitString("double//slash", '/');
		REQUIRE(split.size() == 3);
		REQUIRE(split[0] == "double");
		REQUIRE(split[1] == "");
		REQUIRE(split[2] == "slash");
	}
}