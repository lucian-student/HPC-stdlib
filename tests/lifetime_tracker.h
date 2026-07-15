struct LifetimeTracker
{
    static inline int active_instances = 0;
    static inline int total_constructions = 0;
    static inline int total_destructions = 0;

    int value = 0;

    static void reset()
    {
        active_instances = 0;
        total_constructions = 0;
        total_destructions = 0;
    }

    LifetimeTracker()
    {
        ++active_instances;
        ++total_constructions;
    }

    explicit LifetimeTracker(int val) : value(val)
    {
        ++active_instances;
        ++total_constructions;
    }

    LifetimeTracker(const LifetimeTracker &other) : value(other.value)
    {
        ++active_instances;
        ++total_constructions;
    }

    LifetimeTracker(LifetimeTracker &&other) noexcept : value(other.value)
    {
        ++active_instances;
        ++total_constructions;
    }

    ~LifetimeTracker()
    {
        --active_instances;
        ++total_destructions;
    }

    LifetimeTracker &operator=(const LifetimeTracker &) = default;
    LifetimeTracker &operator=(LifetimeTracker &&) = default;
};