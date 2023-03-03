
#include "event_detect.h"

EventDetect::EventDetect(uint8_t * data_p, event_cb_t eventCallback_p) {
    _eventCallback_p = eventCallback_p;
    curr_p = data_p;
}

void EventDetect::update() {
    evt_data_t delta = 0;
    unsigned long long now = millis();
    uint8_t curr;
    
    if (! curr_p) return;

    curr = (* curr_p) & mask;
    // Find a change or capture the first value
    if (curr == prev && updated) return;
    
    updated == true;
    // Handle wraps (every 50 days)
    if (now < last_event_time)now += ((1LL<<32) - 1LL);
    last_event_time = now;
    if (curr==1 && prev == 0)delta = 1;
    if (curr==0 && prev == 1)delta = -1;
    prev = curr;
    event_count++;
    _eventCallback_p(now, delta);
}


bool EventDetect::is_updated() {
    return updated;
}