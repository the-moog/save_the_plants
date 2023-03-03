#ifndef __EVENT_DETECT_H__
#define __EVENT_DETECT_H__

#include <Arduino.h>

typedef unsigned long long evt_time_t;
typedef short int evt_data_t;
typedef void (*event_cb_t)(evt_time_t ts, evt_data_t delta);
typedef unsigned char uint8_t;

class EventDetect {
  public:
    EventDetect(uint8_t * data_p, event_cb_t eventCallback_p=nullptr);
    void update();
    bool is_updated();

  private:
    event_cb_t _eventCallback_p = nullptr;
    evt_time_t last_event_time = 0;
    evt_time_t event_count = 0;
    uint8_t * curr_p = nullptr;
    uint8_t prev = 0;
    bool updated = false;
    uint8_t mask = 0x01;
};


#endif
