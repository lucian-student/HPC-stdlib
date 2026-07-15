#include <gtest/gtest.h>
#include <hpc_stdlib/lock_free_ring_buffer.h>
#include "lifetime_tracker.h"

class SPSCRingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        LifetimeTracker::reset();
    }
};

// 1. Test Initial State
TEST_F(SPSCRingBufferTest, InitialStateIsEmptyAndNotFull) {
    hpc_stdlib::SPSCRingBuffer<int, 4> buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
}

// 2. Test Basic Push and Pop
TEST_F(SPSCRingBufferTest, PushAndPopSuccessful) {
    hpc_stdlib::SPSCRingBuffer<int, 4> buffer;

    auto push_res = buffer.push(42);
    EXPECT_TRUE(push_res.has_value());
    EXPECT_FALSE(buffer.empty());

    EXPECT_EQ(buffer.front(), 42);

    auto pop_res = buffer.pop();
    EXPECT_TRUE(pop_res.has_value());
    EXPECT_TRUE(buffer.empty());
}

// 3. Test Emplace Functionality
TEST_F(SPSCRingBufferTest, EmplaceConstructsInPlace) {
    struct ComplexType {
        int a;
        double b;
        ComplexType(int x, double y) : a(x), b(y) {}
    };

    hpc_stdlib::SPSCRingBuffer<ComplexType, 4> buffer;
    auto emplace_res = buffer.emplace(10, 20.5);
    EXPECT_TRUE(emplace_res.has_value());
    
    EXPECT_EQ(buffer.front().a, 10);
    EXPECT_DOUBLE_EQ(buffer.front().b, 20.5);
}

// 4. Test Boundary Conditions (Full / Empty Errors)
TEST_F(SPSCRingBufferTest, BufferFullAndEmptyBoundaries) {
    hpc_stdlib::SPSCRingBuffer<int, 2> buffer;

    // Fill the buffer
    EXPECT_TRUE(buffer.push(1).has_value());
    EXPECT_TRUE(buffer.push(2).has_value());
    EXPECT_TRUE(buffer.full());

    // Pushing to a full buffer should return RingBufferError::Full
    auto failed_push = buffer.push(3);
    ASSERT_FALSE(failed_push.has_value());
    EXPECT_EQ(failed_push.error(), hpc_stdlib::RingBufferError::Full);

    // Empty the buffer
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_TRUE(buffer.pop().has_value());
    EXPECT_TRUE(buffer.empty());

    // Popping an empty buffer should return RingBufferError::Empty
    auto failed_pop = buffer.pop();
    ASSERT_FALSE(failed_pop.has_value());
    EXPECT_EQ(failed_pop.error(), hpc_stdlib::RingBufferError::Empty);
}

// 5. Test Object Lifecycle (Proper Construction & Destruction)
TEST_F(SPSCRingBufferTest, ProperlyDestroysObjectsOnPopAndClear) {
    {
        hpc_stdlib::SPSCRingBuffer<LifetimeTracker, 4> buffer;

        buffer.push(LifetimeTracker(100));
        buffer.push(LifetimeTracker(200));

        // 2 pushed + local temporaries in push argument list = construction count will be > 2
        int initial_constructions = LifetimeTracker::total_constructions;

        // Pop one element. It must be explicitly destroyed via std::destroy_at.
        int destructions_before_pop = LifetimeTracker::total_destructions;
        EXPECT_TRUE(buffer.pop().has_value());
        EXPECT_EQ(LifetimeTracker::total_destructions, destructions_before_pop + 1);
    } 
    // Out of scope: The buffer's destructor runs and should clear remaining elements.
    // Ensure everything constructed is fully accounted for by a matching destruction.
    EXPECT_EQ(LifetimeTracker::total_constructions, LifetimeTracker::total_destructions);
}

// 6. Test Ring-Wrapping Behavior (Index Masking)
TEST_F(SPSCRingBufferTest, WrapAroundIndicesSucceeds) {
    hpc_stdlib::SPSCRingBuffer<int, 2> buffer;

    // Push 2, Pop 2 (Wraps indices internally once)
    buffer.push(10);
    buffer.push(20);
    buffer.pop();
    buffer.pop();

    // Push 2 more elements into wrapped indices
    EXPECT_TRUE(buffer.push(30).has_value());
    EXPECT_TRUE(buffer.push(40).has_value());
    EXPECT_TRUE(buffer.full());

    EXPECT_EQ(buffer.front(), 30);
    buffer.pop();
    EXPECT_EQ(buffer.front(), 40);
}

// 7. Multi-threaded SPSC Concurrency Stress Test
TEST_F(SPSCRingBufferTest, ConcurrentSingleProducerSingleConsumer) {
    constexpr std::size_t Capacity = 64;
    constexpr int NumElements = 100'000;
    hpc_stdlib::SPSCRingBuffer<int, Capacity> buffer;

    std::atomic<bool> start_signal{false};

    // Producer Thread
    std::thread producer([&]() {
        while (!start_signal.load()) { std::this_thread::yield(); }
        
        for (int i = 0; i < NumElements; ) {
            if (buffer.push(i).has_value()) {
                i++;
            } else {
                std::this_thread::yield(); // Backoff if full
            }
        }
    });

    // Consumer Thread
    std::vector<int> consumed_results;
    consumed_results.reserve(NumElements);

    std::thread consumer([&]() {
        while (!start_signal.load()) { std::this_thread::yield(); }

        while (consumed_results.size() < NumElements) {
            if (!buffer.empty()) {
                consumed_results.push_back(buffer.front());
                auto res = buffer.pop();
                EXPECT_TRUE(res.has_value());
            } else {
                std::this_thread::yield(); // Backoff if empty
            }
        }
    });

    // Fire threads concurrently
    start_signal.store(true);

    producer.join();
    consumer.join();

    // Validate that all elements were transferred sequentially and intact
    ASSERT_EQ(consumed_results.size(), NumElements);
    for (int i = 0; i < NumElements; ++i) {
        ASSERT_EQ(consumed_results[i], i);
    }
}