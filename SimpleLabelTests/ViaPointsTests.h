#include <gtest/gtest.h>
#include "../SimpleLabel/Constants.h"

class ViaPointTests : public testing::Test
{
protected:
	virtual void SetUp()
	{
		p1 = ViaPoint(1, 30, QRect(0, 0, 10, 20));
		p2 = ViaPoint(1, 30, QRect(10, 10, 10, 20));
	}
	virtual void TearDown() 
	{
	}

	ViaPoint p1;
	ViaPoint p2;
	ViaPoint p3;
	ViaPoint p4;
};

TEST_F(ViaPointTests,  TheSamePointEquality)
{
	EXPECT_TRUE(p1 == p1);
}