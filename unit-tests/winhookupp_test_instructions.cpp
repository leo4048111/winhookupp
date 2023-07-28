#include <gtest/gtest.h>

#include "instructions.h"

TEST(winhookupp_test, winhookupp_test_instructions)
{
    using namespace WINHOOKUPP_NM;

    EXPECT_EQ(sizeof(JmpRelShort), 2);
    EXPECT_EQ(sizeof(JmpRel), 5);
    EXPECT_EQ(sizeof(CallRel), 5);
    EXPECT_EQ(sizeof(JmpAbs), 14);
    EXPECT_EQ(sizeof(CallAbs), 16);
    EXPECT_EQ(sizeof(JccRel), 6);
    EXPECT_EQ(sizeof(JccAbs), 16);
}