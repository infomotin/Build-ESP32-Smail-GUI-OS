#include "event_scheduler.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

EventScheduler::EventScheduler() : next_event_id(1) {
    stats = {};
}

EventScheduler::~EventScheduler() {
    // Cleanup handled by smart pointers
}

uint32_t EventScheduler::schedule_event(uint64_t delay, EventType type, EventPriority priority,
                                       std::function<void()> callback, bool recurring, 
                                       uint64_t interval) {
    uint64_t timestamp = clock.get_time() + delay;
    return schedule_absolute_event(timestamp, type, priority, callback, recurring, interval);
}

uint32_t EventScheduler::schedule_absolute_event(uint64_t timestamp, EventType type, EventPriority priority,
                                               std::function<void()> callback, bool recurring, 
                                               uint64_t interval) {
    auto event = std::make_unique<Event>(timestamp, next_event_id, type, priority, callback, recurring, interval);
    event_queue.push(*event);
    active_events.push_back(std::move(event));
    
    stats.total_events_scheduled++;
    return next_event_id++;
}

void EventScheduler::cancel_event(uint32_t event_id) {
    Event* event = find_event(event_id);
    if (event) {
        event->active = false;
        stats.events_cancelled++;
    }
}

void EventScheduler::pause_event(uint32_t event_id) {
    Event* event = find_event(event_id);
    if (event) {
        event->active = false;
    }
}

void EventScheduler::resume_event(uint32_t event_id) {
    Event* event = find_event(event_id);
    if (event && !event->active) {
        event->active = true;
        // Reschedule if it's a recurring event
        if (event->recurring && event->timestamp <= clock.get_time()) {
            event->timestamp = clock.get_time() + event->interval;
        }
    }
}

bool EventScheduler::is_event_active(uint32_t event_id) const {
    const Event* event = find_event(event_id);
    return event ? event->active : false;
}

void EventScheduler::process_events() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    while (!event_queue.empty() && !clock.is_paused()) {
        const Event& event = event_queue.top();
        
        if (event.timestamp > clock.get_time()) {
            break; // No more events to process at this time
        }
        
        if (event.active) {
            process_single_event(event);
        }
        
        event_queue.pop();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    update_statistics(processing_time);
}

void EventScheduler::process_events_until(uint64_t target_time) {
    while (!event_queue.empty() && !clock.is_paused()) {
        const Event& event = event_queue.top();
        
        if (event.timestamp > target_time) {
            break;
        }
        
        if (event.active) {
            process_single_event(event);
        }
        
        event_queue.pop();
    }
}

uint64_t EventScheduler::get_next_event_time() const {
    if (event_queue.empty()) {
        return UINT64_MAX;
    }
    return event_queue.top().timestamp;
}

bool EventScheduler::has_pending_events() const {
    return !event_queue.empty();
}

void EventScheduler::process_single_event(const Event& event) {
    if (event.callback) {
        try {
            event.callback();
        } catch (const std::exception& e) {
            std::cerr << "Event callback exception: " << e.what() << std::endl;
        }
    }
    
    stats.total_events_processed++;
    
    // Handle recurring events
    if (event.recurring && event.active) {
        // Find the event and reschedule it
        for (auto& active_event : active_events) {
            if (active_event->event_id == event.event_id) {
                active_event->timestamp = clock.get_time() + active_event->interval;
                event_queue.push(*active_event);
                break;
            }
        }
    }
}

void EventScheduler::update_statistics(uint64_t processing_time) {
    stats.processing_time_ns += processing_time;
    
    if (stats.max_processing_time_ns == 0) {
        stats.max_processing_time_ns = processing_time;
        stats.min_processing_time_ns = processing_time;
    } else {
        stats.max_processing_time_ns = std::max(stats.max_processing_time_ns, processing_time);
        stats.min_processing_time_ns = std::min(stats.min_processing_time_ns, processing_time);
    }
}

void EventScheduler::reset_statistics() {
    stats = {};
}

uint32_t EventScheduler::generate_event_id() {
    return next_event_id++;
}

Event* EventScheduler::find_event(uint32_t event_id) {
    for (auto& event : active_events) {
        if (event->event_id == event_id) {
            return event.get();
        }
    }
    return nullptr;
}

const Event* EventScheduler::find_event(uint32_t event_id) const {
    for (const auto& event : active_events) {
        if (event->event_id == event_id) {
            return event.get();
        }
    }
    return nullptr;
}

// Timer implementation
Timer::Timer(EventScheduler* scheduler, uint32_t interval_ms, std::function<void()> callback,
             bool recurring, EventPriority priority)
    : scheduler(scheduler), interval(interval_ms), callback(callback), recurring(recurring),
      priority(priority), event_id(0), running(false) {
}

Timer::~Timer() {
    stop();
}

void Timer::start() {
    if (!running) {
        event_id = scheduler->schedule_event(interval, EventType::TIMER_EVENT, priority, callback, recurring, interval);
        running = true;
    }
}

void Timer::stop() {
    if (running && event_id != 0) {
        scheduler->cancel_event(event_id);
        running = false;
        event_id = 0;
    }
}

void Timer::reset() {
    if (running) {
        stop();
        start();
    }
}

void Timer::set_interval(uint32_t interval_ms) {
    interval = interval_ms;
    if (running) {
        reset();
    }
}

void Timer::schedule_next_event() {
    if (recurring && running) {
        event_id = scheduler->schedule_event(interval, EventType::TIMER_EVENT, priority, callback, true, interval);
    }
}

// Interrupt Controller implementation
InterruptController::InterruptController(EventScheduler* scheduler)
    : scheduler(scheduler), interrupt_mask(0xFFFFFFFF), next_event_id(1) {
}

InterruptController::~InterruptController() {
    // Cleanup handled by smart pointers
}

void InterruptController::raise_interrupt(uint32_t level, uint32_t source, std::function<void()> handler) {
    // Find existing interrupt source or create new one
    for (auto& int_src : interrupt_sources) {
        if (int_src.source == source) {
            int_src.level = level;
            int_src.handler = handler;
            int_src.pending = true;
            return;
        }
    }
    
    // Create new interrupt source
    InterruptSource int_src;
    int_src.level = level;
    int_src.source = source;
    int_src.priority = level; // Default priority to level
    int_src.pending = true;
    int_src.handler = handler;
    interrupt_sources.push_back(int_src);
    
    // Schedule interrupt processing
    scheduler->schedule_event(0, EventType::INTERRUPT_EVENT, EventPriority::CRITICAL, 
                             [this]() { process_pending_interrupts(); });
}

void InterruptController::clear_interrupt(uint32_t level, uint32_t source) {
    for (auto& int_src : interrupt_sources) {
        if (int_src.source == source && int_src.level == level) {
            int_src.pending = false;
            break;
        }
    }
}

void InterruptController::set_interrupt_mask(uint32_t mask) {
    interrupt_mask = mask;
}

uint32_t InterruptController::get_pending_interrupts() const {
    uint32_t pending = 0;
    for (const auto& int_src : interrupt_sources) {
        if (int_src.pending && (interrupt_mask & (1 << int_src.level))) {
            pending |= (1 << int_src.level);
        }
    }
    return pending;
}

void InterruptController::process_pending_interrupts() {
    uint32_t highest_priority = find_highest_priority_interrupt();
    if (highest_priority != UINT32_MAX) {
        deliver_interrupt(highest_priority);
    }
}

bool InterruptController::has_pending_interrupts() const {
    return get_pending_interrupts() != 0;
}

void InterruptController::set_interrupt_priority(uint32_t source, uint32_t priority) {
    for (auto& int_src : interrupt_sources) {
        if (int_src.source == source) {
            int_src.priority = priority;
            break;
        }
    }
}

uint32_t InterruptController::get_interrupt_priority(uint32_t source) const {
    for (const auto& int_src : interrupt_sources) {
        if (int_src.source == source) {
            return int_src.priority;
        }
    }
    return 0;
}

uint32_t InterruptController::find_highest_priority_interrupt() const {
    uint32_t highest_priority = UINT32_MAX;
    uint32_t highest_level = 0;
    
    for (const auto& int_src : interrupt_sources) {
        if (int_src.pending && (interrupt_mask & (1 << int_src.level))) {
            if (int_src.priority < highest_priority || highest_priority == UINT32_MAX) {
                highest_priority = int_src.priority;
                highest_level = int_src.level;
            }
        }
    }
    
    return highest_priority == UINT32_MAX ? UINT32_MAX : highest_level;
}

void InterruptController::deliver_interrupt(uint32_t source) {
    for (auto& int_src : interrupt_sources) {
        if (int_src.source == source && int_src.pending) {
            if (int_src.handler) {
                int_src.handler();
            }
            int_src.pending = false;
            break;
        }
    }
}

// DMA Controller implementation
DMAController::DMAController(EventScheduler* scheduler)
    : scheduler(scheduler), transfer_speed(1000000), next_transfer_id(1) { // 1MB/s default
}

DMAController::~DMAController() {
    // Cleanup handled by smart pointers
}

uint32_t DMAController::start_transfer(uint32_t src_addr, uint32_t dst_addr, uint32_t size,
                                       std::function<void(uint32_t)> completion_callback,
                                       EventPriority priority) {
    auto transfer = std::make_unique<DMATransfer>();
    transfer->transfer_id = next_transfer_id++;
    transfer->src_addr = src_addr;
    transfer->dst_addr = dst_addr;
    transfer->size = size;
    transfer->bytes_transferred = 0;
    transfer->completion_callback = completion_callback;
    transfer->active = true;
    
    // Calculate transfer time
    uint64_t transfer_time_ns = (size * 1000000000ULL) / transfer_speed;
    uint64_t transfer_time_ms = transfer_time_ns / 1000000;
    
    // Schedule completion event
    transfer->event_id = scheduler->schedule_event(transfer_time_ms, EventType::DMA_EVENT, priority,
        [this, transfer_id = transfer->transfer_id]() {
            // Find and complete the transfer
            for (auto& active_transfer : active_transfers) {
                if (active_transfer->transfer_id == transfer_id) {
                    process_dma_transfer(active_transfer.get());
                    break;
                }
            }
        });
    
    uint32_t transfer_id = transfer->transfer_id;
    active_transfers.push_back(std::move(transfer));
    return transfer_id;
}

void DMAController::cancel_transfer(uint32_t transfer_id) {
    for (auto& transfer : active_transfers) {
        if (transfer->transfer_id == transfer_id && transfer->active) {
            scheduler->cancel_event(transfer->event_id);
            transfer->active = false;
            break;
        }
    }
}

bool DMAController::is_transfer_active(uint32_t transfer_id) const {
    for (const auto& transfer : active_transfers) {
        if (transfer->transfer_id == transfer_id) {
            return transfer->active;
        }
    }
    return false;
}

void DMAController::set_transfer_speed(uint32_t bytes_per_second) {
    transfer_speed = bytes_per_second;
}

void DMAController::process_dma_transfer(DMATransfer* transfer) {
    if (!transfer || !transfer->active) {
        return;
    }
    
    // Mark transfer as complete
    transfer->active = false;
    transfer->bytes_transferred = transfer->size;
    
    // Call completion callback
    if (transfer->completion_callback) {
        transfer->completion_callback(transfer->transfer_id);
    }
}

uint32_t DMAController::generate_transfer_id() {
    return next_transfer_id++;
}

void EventScheduler::reset() {
    // Clear all pending events
    while (!event_queue.empty()) {
        event_queue.pop();
    }
    active_events.clear();
    next_event_id = 1;
    stats = {};
    clock = SimulationClock(); // reset clock to default
}

bool EventScheduler::initialize() {
    return true;
}
