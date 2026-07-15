#include <gtest/gtest.h>
#include <hpc_stdlib/ring_buffer.h>
#include "lifetime_tracker.h"

// Basic test case
TEST(QueueTest, Empty)
{
    hpc_stdlib::RingBuffer<int, 32> q;
    EXPECT_TRUE(q.empty()); // Replace with your actual assertions
    EXPECT_EQ(0, q.size());
}

class RingBufferTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LifetimeTracker::reset();
    }
};

// 1. Verify initial empty state
TEST_F(RingBufferTest, DefaultConstructorState)
{
    hpc_stdlib::RingBuffer<int, 4> buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.capacity(), 4);
}

// 2. Verify basic push and pop flow
TEST_F(RingBufferTest, PushAndPopSuccessful)
{
    hpc_stdlib::RingBuffer<int, 4> buffer;

    auto push_res = buffer.push(42);
    EXPECT_TRUE(push_res.has_value());
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.front(), 42);

    auto pop_res = buffer.pop();
    EXPECT_TRUE(pop_res.has_value());
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
}

// 3. Verify emplace constructs in-place with multiple arguments
TEST_F(RingBufferTest, EmplaceConstructsCorrectly)
{
    struct ComplexType
    {
        std::string str;
        int val;
        ComplexType(std::string s, int v) : str(std::move(s)), val(v) {}
    };

    hpc_stdlib::RingBuffer<ComplexType, 4> buffer;
    auto res = buffer.emplace("hello", 1337);

    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(buffer.front().str, "hello");
    EXPECT_EQ(buffer.front().val, 1337);
}

// 4. Verify boundary errors (Full and Empty)
TEST_F(RingBufferTest, BufferBoundaryErrors)
{
    hpc_stdlib::RingBuffer<int, 2> buffer;

    // Pop on empty
    auto pop_empty = buffer.pop();
    ASSERT_FALSE(pop_empty.has_value());
    EXPECT_EQ(pop_empty.error(), hpc_stdlib::RingBufferError::Empty);

    // Push until full
    EXPECT_TRUE(buffer.push(1).has_value());
    EXPECT_TRUE(buffer.push(2).has_value());
    EXPECT_TRUE(buffer.full());

    // Push on full
    auto push_full = buffer.push(3);
    ASSERT_FALSE(push_full.has_value());
    EXPECT_EQ(push_full.error(), hpc_stdlib::RingBufferError::Full);
}

// 5. Verify the power-of-two wrap-around index logic works over many cycles
TEST_F(RingBufferTest, WrapAroundLogic)
{
    hpc_stdlib::RingBuffer<int, 4> buffer;

    // Fill the buffer
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(buffer.push(i).has_value());
    }
    EXPECT_TRUE(buffer.full());

    // Pop 2 items
    EXPECT_TRUE(buffer.pop().has_value()); // Pops 0
    EXPECT_TRUE(buffer.pop().has_value()); // Pops 1
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), 2);

    // Push 2 more items (this triggers wrap-around index logic)
    EXPECT_TRUE(buffer.push(4).has_value());
    EXPECT_TRUE(buffer.push(5).has_value());
    EXPECT_TRUE(buffer.full());

    // Pop remaining to verify FIFO ordering through wrap-around
    EXPECT_EQ(buffer.front(), 2);
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_EQ(buffer.front(), 3);
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_EQ(buffer.front(), 4);
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_EQ(buffer.front(), 5);
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_TRUE(buffer.empty());
}

// 6. Critical Lifetime Test: Ensure objects are constructed & destroyed correctly on pop
TEST_F(RingBufferTest, ExplicitLifetimeManagementOnPop)
{
    {
        hpc_stdlib::RingBuffer<LifetimeTracker, 4> buffer;

        EXPECT_EQ(LifetimeTracker::active_instances, 0);

        // Push constructs objects
        EXPECT_TRUE(buffer.emplace(10).has_value());
        EXPECT_TRUE(buffer.emplace(20).has_value());
        EXPECT_EQ(LifetimeTracker::active_instances, 2);

        // Pop destroys the object
        EXPECT_TRUE(buffer.pop().has_value());
        EXPECT_EQ(LifetimeTracker::active_instances, 1);
        EXPECT_EQ(LifetimeTracker::total_destructions, 1);
    } // Buffer goes out of scope here

    // Destructor of RingBuffer must clean up the remaining element
    EXPECT_EQ(LifetimeTracker::active_instances, 0);
    EXPECT_EQ(LifetimeTracker::total_destructions, 2);
}

// 7. Critical Lifetime Test: Ensure active elements are destroyed on buffer destruction
TEST_F(RingBufferTest, DestructorCleansUpActiveElements)
{
    {
        hpc_stdlib::RingBuffer<LifetimeTracker, 4> buffer;
        EXPECT_TRUE(buffer.emplace(1).has_value());
        EXPECT_TRUE(buffer.emplace(2).has_value());
        EXPECT_TRUE(buffer.emplace(3).has_value());

        EXPECT_EQ(LifetimeTracker::active_instances, 3);
        // Leaving scope: RingBuffer destructor should destroy all 3 remaining elements
    }

    EXPECT_EQ(LifetimeTracker::active_instances, 0);
    EXPECT_EQ(LifetimeTracker::total_destructions, 3);
}