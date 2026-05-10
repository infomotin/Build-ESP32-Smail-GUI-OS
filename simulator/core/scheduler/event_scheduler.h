#pragma once

#include <stdint.h>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <chrono>

// Event types
enum class EventType {
    TIMER_EVENT,
    INTERRUPT_EVENT,
    DMA_EVENT,
    PERIPHERAL_EVENT,
    CUSTOM_EVENT
};

// Event priority levels (lower number = higher priority)
enum class EventPriority {
    CRITICAL = 0,
    HIGH = 1,
    NORMAL = 2,
    LOW = 3
};

// Event structure
struct Event {
    uint64_t timestamp;        // Simulation time when event should fire
    uint32_t event_id;         // Unique event identifier
    EventType type;           // Type of event
    EventPriority priority;    // Event priority
    std::function<void()> callback;  // Event handler function
    bool recurring;            // Whether event repeats
    uint64_t interval;         // Recurrence interval (in simulation time)
    bool active;               // Whether event is currently active
    
    Event(uint64_t ts, uint32_t id, EventType t, EventPriority p, 
          std::function<void()> cb, bool rec = false, uint64_t inter = 0)
        : timestamp(ts), event_id(id), type(t), priority(p), callback(cb), 
          recurring(rec), interval(inter), active(true) {}
    
    // For priority queue ordering
    bool operator>(const Event& other) const {
        if (timestamp != other.timestamp) {
            return timestamp > other.timestamp;
        }
        return priority > other.priority;
    }
};

// Simulation time management
class SimulationClock {
public:
    SimulationClock() : current_time(0), time_scale(1.0), paused(false) {}
    
    uint64_t get_time() const { return current_time; }
    void set_time(uint64_t time) { current_time = time; }
    void advance_time(uint64_t delta) { current_time += delta; }
    
    void set_time_scale(double scale) { time_scale = scale; }
    double get_time_scale() const { return time_scale; }
    
    void pause() { paused = true; }
    void resume() { paused = false; }
    bool is_paused() const { return paused; }
    
    // Convert between wall-clock time and simulation time
    uint64_t wall_clock_to_sim_time(std::chrono::nanoseconds wall_time) const {
        return static_cast<uint64_t>(wall_time.count() * time_scale);
    }
    
    std::chrono::nanoseconds sim_time_to_wall_clock(uint64_t sim_time) const {
        return std::chrono::nanoseconds(static_cast<int64_t>(sim_time / time_scale));
    }

private:
    uint64_t current_time;
    double time_scale;
    bool paused;
};

// Event Scheduler class
class EventScheduler {
public:
    EventScheduler();
    ~EventScheduler();
    
    // Time management
    uint64_t get_current_time() const { return clock.get_time(); }
    void set_time_scale(double scale) { clock.set_time_scale(scale); }
    double get_time_scale() const { return clock.get_time_scale(); }
    void pause() { clock.pause(); }
    void resume() { clock.resume(); }
    bool is_paused() const { return clock.is_paused(); }
    
    // Event scheduling
    uint32_t schedule_event(uint64_t delay, EventType type, EventPriority priority,
                           std::function<void()> callback, bool recurring = false, 
                           uint64_t interval = 0);
    uint32_t schedule_absolute_event(uint64_t timestamp, EventType type, EventPriority priority,
                                   std::function<void()> callback, bool recurring = false, 
                                   uint64_t interval = 0);
    
    // Event management
    void cancel_event(uint32_t event_id);
    void pause_event(uint32_t event_id);
    void resume_event(uint32_t event_id);
    bool is_event_active(uint32_t event_id) const;
    
    // Processing
    void process_events();
    void process_events_until(uint64_t target_time);
    uint64_t get_next_event_time() const;
    bool has_pending_events() const;
    
    // Statistics
    struct Statistics {
        uint64_t total_events_scheduled;
        uint64_t total_events_processed;
        uint64_t events_cancelled;
        uint64_t events_missed;
        uint64_t processing_time_ns;
        uint64_t max_processing_time_ns;
        uint64_t min_processing_time_ns;
    };
    const Statistics& get_statistics() const { return stats; }
    void reset_statistics();

private:
    SimulationClock clock;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> event_queue;
    std::vector<std::unique_ptr<Event>> active_events;
    uint32_t next_event_id;
    Statistics stats;
    
    // Internal methods
    void process_single_event(const Event& event);
    uint32_t generate_event_id();
    void update_statistics(uint64_t processing_time);
    Event* find_event(uint32_t event_id);
    const Event* find_event(uint32_t event_id) const;
};

// Timer helper class for common timer operations
class Timer {
public:
    Timer(EventScheduler* scheduler, uint32_t interval_ms, std::function<void()> callback,
          bool recurring = true, EventPriority priority = EventPriority::NORMAL);
    ~Timer();
    
    void start();
    void stop();
    void reset();
    bool is_running() const { return running; }
    
    uint32_t get_interval() const { return interval; }
    void set_interval(uint32_t interval_ms);
    
private:
    EventScheduler* scheduler;
    uint32_t interval;
    std::function<void()> callback;
    bool recurring;
    EventPriority priority;
    uint32_t event_id;
    bool running;
    
    void schedule_next_event();
};

// Interrupt helper class for hardware interrupts
class InterruptController {
public:
    InterruptController(EventScheduler* scheduler);
    ~InterruptController();
    
    // Interrupt management
    void raise_interrupt(uint32_t level, uint32_t source, std::function<void()> handler);
    void clear_interrupt(uint32_t level, uint32_t source);
    void set_interrupt_mask(uint32_t mask);
    uint32_t get_interrupt_mask() const { return interrupt_mask; }
    uint32_t get_pending_interrupts() const;
    
    // Processing
    void process_pending_interrupts();
    bool has_pending_interrupts() const;
    
    // Configuration
    void set_interrupt_priority(uint32_t source, uint32_t priority);
    uint32_t get_interrupt_priority(uint32_t source) const;

private:
    struct InterruptSource {
        uint32_t level;
        uint32_t source;
        uint32_t priority;
        bool pending;
        std::function<void()> handler;
    };
    
    EventScheduler* scheduler;
    std::vector<InterruptSource> interrupt_sources;
    uint32_t interrupt_mask;
    uint32_t next_event_id;
    
    uint32_t find_highest_priority_interrupt() const;
    void deliver_interrupt(uint32_t source);
};

// DMA Controller helper
class DMAController {
public:
    DMAController(EventScheduler* scheduler);
    ~DMAController();
    
    // DMA operations
    uint32_t start_transfer(uint32_t src_addr, uint32_t dst_addr, uint32_t size,
                           std::function<void(uint32_t)> completion_callback,
                           EventPriority priority = EventPriority::NORMAL);
    void cancel_transfer(uint32_t transfer_id);
    bool is_transfer_active(uint32_t transfer_id) const;
    
    // Configuration
    void set_transfer_speed(uint32_t bytes_per_second);
    uint32_t get_transfer_speed() const { return transfer_speed; }

private:
    struct DMATransfer {
        uint32_t transfer_id;
        uint32_t src_addr;
        uint32_t dst_addr;
        uint32_t size;
        uint32_t bytes_transferred;
        std::function<void(uint32_t)> completion_callback;
        uint32_t event_id;
        bool active;
    };
    
    EventScheduler* scheduler;
    std::vector<std::unique_ptr<DMATransfer>> active_transfers;
    uint32_t transfer_speed;
    uint32_t next_transfer_id;
    
    void process_dma_transfer(DMATransfer* transfer);
    uint32_t generate_transfer_id();
};
