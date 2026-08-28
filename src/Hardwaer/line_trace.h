#ifndef __LINE_TRACE_H__
#define __LINE_TRACE_H__

#include "main.h"

#define TRACE_ERROR_MAX 300
#define TRACE_ERROR_MIN (-300)
extern uint8_t stop_request ;
typedef struct {
    int32_t error;
    uint8_t sensor_data;
} TraceData;

void line_trace_init(void);
int32_t line_trace_get_error(uint8_t sensor_data);
void line_trace_sensor_to_binary(uint8_t sensor_data, char *buffer);
uint8_t line_trace_stop_requested(void);

#endif
