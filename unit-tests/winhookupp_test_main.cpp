#include <gtest/gtest.h>

int main(int argc, char** argv)
{
	using namespace testing;
	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}