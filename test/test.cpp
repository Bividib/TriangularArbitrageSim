#include <gtest/gtest.h>
#include <websocket/websocket.h>

TEST(WebApplicationTest, AddFunctionCorrectlyAddsIntegers) {
    ASSERT_EQ(add(5, 3), 8) << "add(5, 3) should be 8";
}