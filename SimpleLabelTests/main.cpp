#include <gtest/gtest.h>
#include "ViaPointsTests.h"

int doubleIt(int a)
{
	return 2*a;
}

TEST(t, First) 
{
	EXPECT_EQ(4, doubleIt(2));
}


TEST(ViaPointTests1, EqualityTest)
{
	ViaPoint p1(1, 30, QRect(0, 0, 10, 20));
	ViaPoint p2(1, 30, QRect(0, 0, 10, 20));

	EXPECT_TRUE(p1 == p2);
}

int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

