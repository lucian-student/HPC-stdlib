#include <gtest/gtest.h>
#include <hpc_stdlib/ring_buffer.h>

// Basic test case
TEST(QueueTest, Empty)
{
    hpc_stdlib::RingBuffer<int, 32> q;
    EXPECT_TRUE(q.empty()); // Replace with your actual assertions
    EXPECT_EQ(0, q.size());
}