#include <gtest/gtest.h>
#include "id.hpp"

TEST(digits10, BasicAssertions)
{
    ASSERT_EQ(digits10(1), 1);
    ASSERT_EQ(digits10(9), 1);
    ASSERT_EQ(digits10(10), 2);
    ASSERT_EQ(digits10(20), 2);
    ASSERT_EQ(digits10(90000090), 8);
    ASSERT_EQ(digits10(0), 1);
    ASSERT_EQ(digits10(1777777777777777777), 19);
    ASSERT_EQ(digits10(120399284982992928), 18);
    ASSERT_EQ(digits10(28387422838742), 14);
    ASSERT_EQ(digits10(748201803948400), 15);
    ASSERT_EQ(digits10(6709298447899222), 16);
    ASSERT_EQ(digits10(12003948900199992), 17);
}

TEST(assertID, BasicAssertions)
{
    ASSERT_EQ(SID::assertID(0), true);
    ASSERT_EQ(SID::assertID(9), false);
    ASSERT_EQ(SID::assertID(10), true);
    ASSERT_EQ(SID::assertID(20), false);
    ASSERT_EQ(SID::assertID(101), false);
    ASSERT_EQ(SID::assertID(102), false);
    ASSERT_EQ(SID::assertID(1021), false);
    ASSERT_EQ(SID::assertID(111111111111111), false);
    ASSERT_EQ(SID::assertID(11111111111111), true);
    ASSERT_EQ(SID::assertID(10211), true);
    ASSERT_EQ(SID::assertID(211), true);
    ASSERT_EQ(SID::assertID(201), false);
    ASSERT_EQ(SID::assertID(3111), true);
    ASSERT_EQ(SID::assertID(311141111), true);
    ASSERT_EQ(SID::assertID(31114111), false);
    ASSERT_EQ(SID::assertID(9999999999), true);
    ASSERT_EQ(SID::assertID(888888888), true);
    ASSERT_EQ(SID::assertID(77777777), true);
    ASSERT_EQ(SID::assertID(6666666), true);
    ASSERT_EQ(SID::assertID(555555), true);
    ASSERT_EQ(SID::assertID(44444), true);
    ASSERT_EQ(SID::assertID(3333), true);
    ASSERT_EQ(SID::assertID(222), true);
    ASSERT_EQ(SID::assertID(11), true);
    ASSERT_EQ(SID::assertID(99999999990), false);
    ASSERT_EQ(SID::assertID(9000000000), false);
    ASSERT_EQ(SID::assertID(8888888880), false);
    ASSERT_EQ(SID::assertID(777777770), false);
    ASSERT_EQ(SID::assertID(66666660), false);
    ASSERT_EQ(SID::assertID(5555550), false);
    ASSERT_EQ(SID::assertID(444440), false);
    ASSERT_EQ(SID::assertID(33330), false);
    ASSERT_EQ(SID::assertID(2220), false);
    ASSERT_EQ(SID::assertID(110), false);
}

// TEST(generateID, BasicAssertions)
// {
//     ASSERT_EQ(generateID(0, 112), 1);
//     ASSERT_EQ(generateID(12321, 112), 1);
//     ASSERT_EQ(generateID(101, 112), 1);
//     ASSERT_EQ(generateID(11110, 112), 1);
//     ASSERT_EQ(generateID(11, 3), 1113);
//     ASSERT_EQ(generateID(1111, 3), 111113);
//     ASSERT_EQ(generateID(10211, 3), 1021113);
//     ASSERT_EQ(generateID(10211, 13), 10211213);
//     ASSERT_EQ(generateID(1011, 23), 1011223);
//     ASSERT_EQ(generateID(1011, 299), 10113299);
//     ASSERT_EQ(generateID(1011233, 299), 10112333299);
//     ASSERT_EQ(generateID(1111111111111111111, 299), 1);
// }
// 

TEST(getParentID, BasicAssertions)
{
    ASSERT_EQ(SID::getParentID(0), -1);
    ASSERT_EQ(SID::getParentID(1), -1);
    ASSERT_EQ(SID::getParentID(9), -1);
    ASSERT_EQ(SID::getParentID(123), -1);
    ASSERT_EQ(SID::getParentID(1234), -1);
    ASSERT_EQ(SID::getParentID(1202301), -1);
    ASSERT_EQ(SID::getParentID(10), 0);
    ASSERT_EQ(SID::getParentID(11), 0);
    ASSERT_EQ(SID::getParentID(201), -1);
    ASSERT_EQ(SID::getParentID(211), 0);
    ASSERT_EQ(SID::getParentID(1111), 11);
    ASSERT_EQ(SID::getParentID(11211), 11);
    ASSERT_EQ(SID::getParentID(3001), -1);
    ASSERT_EQ(SID::getParentID(3101), 0);
    ASSERT_EQ(SID::getParentID(9000000000), -1);
    ASSERT_EQ(SID::getParentID(111111), 1111);
    ASSERT_EQ(SID::getParentID(122333444), 12233);
    ASSERT_EQ(SID::getParentID(1111111111111111211), 1111111111111111);
}

TEST(getNodeLevel, BasicAssertions)
{
    ASSERT_EQ(SID::getNodeLevel(0), 0);
    ASSERT_EQ(SID::getNodeLevel(10), 1);
    ASSERT_EQ(SID::getNodeLevel(11), 1);
    ASSERT_EQ(SID::getNodeLevel(201), -1);
    ASSERT_EQ(SID::getNodeLevel(211), 1);
    ASSERT_EQ(SID::getNodeLevel(1111), 2);
    ASSERT_EQ(SID::getNodeLevel(11211), 2);
    ASSERT_EQ(SID::getNodeLevel(3001), -1);
    ASSERT_EQ(SID::getNodeLevel(3101), 1);
    ASSERT_EQ(SID::getNodeLevel(9000000000), -1);
    ASSERT_EQ(SID::getNodeLevel(1111), 2);
    ASSERT_EQ(SID::getNodeLevel(122333444), 3);
    ASSERT_EQ(SID::getNodeLevel(1111111111111111211), 9);
    ASSERT_EQ(SID::getNodeLevel(9000000009), -1);
    ASSERT_EQ(SID::getNodeLevel(800000009), -1);
    ASSERT_EQ(SID::getNodeLevel(70000009), -1);
    ASSERT_EQ(SID::getNodeLevel(6100009), 1);
    ASSERT_EQ(SID::getNodeLevel(500009), -1);
    ASSERT_EQ(SID::getNodeLevel(41009), 1);
    ASSERT_EQ(SID::getNodeLevel(3009), -1);
    ASSERT_EQ(SID::getNodeLevel(219), 1);
    ASSERT_EQ(SID::getNodeLevel(19), 1);
}

TEST(getSubIndex, BasicAssertions)
{
    ASSERT_EQ(SID::getSubIndex(0), 0);
    ASSERT_EQ(SID::getSubIndex(10), 0);
    ASSERT_EQ(SID::getSubIndex(11), 1);
    ASSERT_EQ(SID::getSubIndex(201), -1);
    ASSERT_EQ(SID::getSubIndex(211), 11);
    ASSERT_EQ(SID::getSubIndex(1111), 1);
    ASSERT_EQ(SID::getSubIndex(11211), 11);
    ASSERT_EQ(SID::getSubIndex(3001), -1);
    ASSERT_EQ(SID::getSubIndex(3101), 101);
    ASSERT_EQ(SID::getSubIndex(9000000000), -1);
    ASSERT_EQ(SID::getSubIndex(1111), 1);
    ASSERT_EQ(SID::getSubIndex(122333444), 444);
    ASSERT_EQ(SID::getSubIndex(1111111111111111211), 11);
    ASSERT_EQ(SID::getSubIndex(9000000009), -1);
    ASSERT_EQ(SID::getSubIndex(800000009), -1);
    ASSERT_EQ(SID::getSubIndex(70000009), -1);
    ASSERT_EQ(SID::getSubIndex(6100009), 100009);
    ASSERT_EQ(SID::getSubIndex(500009), -1);
    ASSERT_EQ(SID::getSubIndex(41009), 1009);
    ASSERT_EQ(SID::getSubIndex(3009), -1);
    ASSERT_EQ(SID::getSubIndex(209), -1);
    ASSERT_EQ(SID::getSubIndex(19), 9);
}
// 
// TEST(getPath, BasicAssertions)
// {
//     std::vector<uint64_t> vID;
//     int ret = getPath(0, vID);
//     ASSERT_EQ(ret, false);
//     ret = getPath(1, vID);
//     ASSERT_EQ(ret, false);
//     ret = getPath(1111111111111111111, vID);
//     ASSERT_EQ(ret, false);
//     ret = getPath(99999999990, vID);
//     ASSERT_EQ(ret, false);
// 
//     ret = getPath(10, vID);
//     ASSERT_EQ(ret, 0);
//     ASSERT_EQ(vID.size(), 1);
//     ASSERT_EQ(vID[0], 10);
// 
//     ret = getPath(3111, vID);
//     ASSERT_EQ(ret, 0);
//     ASSERT_EQ(vID.size(), 1);
//     ASSERT_EQ(vID[0], 3111);
// 
//     ret = getPath(3111211, vID);
//     ASSERT_EQ(ret, 0);
//     ASSERT_EQ(vID.size(), 2);
//     ASSERT_EQ(vID[0], 3111);
//     ASSERT_EQ(vID[1], 3111211);
// 
//     ret = getPath(1111111111, vID);
//     ASSERT_EQ(ret, 0);
//     ASSERT_EQ(vID.size(), 5);
//     ASSERT_EQ(vID[0], 11);
//     ASSERT_EQ(vID[1], 1111);
//     ASSERT_EQ(vID[2], 111111);
//     ASSERT_EQ(vID[3], 11111111);
// 
//     ret = getPath(1111111111111111211, vID);
//     ASSERT_EQ(ret, 0);
//     ASSERT_EQ(vID.size(), 9);
//     ASSERT_EQ(vID[1], 1111);
//     ASSERT_EQ(vID[2], 111111);
//     ASSERT_EQ(vID[3], 11111111);
//     ASSERT_EQ(vID[7], 1111111111111111);
//     ASSERT_EQ(vID[8], 1111111111111111211);
// }
// 
