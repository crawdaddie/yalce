#include "ylc_datatypes.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  double start;
  double stop;
} TidalTimeSpan;

// Value structure for event data
typedef struct {
  char s[64];   // Sound name (e.g., "bd", "sn")
  int n;        // Note number (optional)
  int has_n;    // Flag indicating if n is set
  double gain;  // Gain value (optional)
  int has_gain; // Flag indicating if gain is set

  // Add more fields as needed:
  // double pan;
  // int has_pan;
  // double speed;
  // int has_speed;
  // char vowel[8];
  // etc.
} TidalValue;

// Event structure
typedef struct {
  TidalTimeSpan part;  // The part of the cycle this event occupies
  TidalValue value;    // The actual event data
  TidalTimeSpan whole; // The whole duration this event represents
} TidalEvent;

// Pattern structure containing all events
typedef struct {
  double arc_len;     // Length of the arc/cycle
  TidalEvent *events; // Array of events
  int events_count;   // Number of events
} TidalPattern;

// Function declarations
void *td_json_str_to_obj(char *json_str);
void td_free_pattern(TidalPattern *pattern);
void td_print_pattern(TidalPattern *pattern);

// Helper function to parse a time span (part/whole)
static TidalTimeSpan parse_time_span(json_object *span_obj) {
  TidalTimeSpan span = {0.0, 0.0};

  if (!span_obj)
    return span;

  json_object *start_obj, *stop_obj;

  if (json_object_object_get_ex(span_obj, "start", &start_obj)) {
    span.start = json_object_get_double(start_obj);
  }

  if (json_object_object_get_ex(span_obj, "stop", &stop_obj)) {
    span.stop = json_object_get_double(stop_obj);
  }

  return span;
}

// Helper function to parse event value
static TidalValue parse_value(json_object *value_obj) {
  TidalValue value = {0};

  if (!value_obj)
    return value;

  // Parse 's' field (sound name)
  json_object *s_obj;
  if (json_object_object_get_ex(value_obj, "s", &s_obj)) {
    const char *s_str = json_object_get_string(s_obj);
    if (s_str) {
      // Make a copy of the string
      size_t len = strlen(s_str);
      if (len < sizeof(value.s) - 1) {
        strcpy(value.s, s_str);
      } else {
        // Truncate if too long
        strncpy(value.s, s_str, sizeof(value.s) - 1);
        value.s[sizeof(value.s) - 1] = '\0';
      }
    }
  }

  // You can add more fields here as needed (n, gain, etc.)
  // Example for numeric values:
  json_object *n_obj;
  if (json_object_object_get_ex(value_obj, "n", &n_obj)) {
    value.n = json_object_get_int(n_obj);
    value.has_n = 1;
  }

  json_object *gain_obj;
  if (json_object_object_get_ex(value_obj, "gain", &gain_obj)) {
    value.gain = json_object_get_double(gain_obj);
    value.has_gain = 1;
  }

  return value;
}

// Helper function to parse a single event
static TidalEvent parse_event(json_object *event_obj) {
  TidalEvent event = {0};

  if (!event_obj)
    return event;

  json_object *part_obj, *value_obj, *whole_obj;

  // Parse part
  if (json_object_object_get_ex(event_obj, "part", &part_obj)) {
    event.part = parse_time_span(part_obj);
  }

  // Parse value
  if (json_object_object_get_ex(event_obj, "value", &value_obj)) {
    event.value = parse_value(value_obj);
  }

  // Parse whole
  if (json_object_object_get_ex(event_obj, "whole", &whole_obj)) {
    event.whole = parse_time_span(whole_obj);
  }

  return event;
}

void *td_json_str_to_obj(char *json_str) {
  if (!json_str) {
    fprintf(stderr, "Error: NULL json_str\n");
    return NULL;
  }
  printf("json_str:\n%s\n", json_str);

  // Parse the JSON string
  json_object *root_obj = json_tokener_parse(json_str);
  if (!root_obj) {
    fprintf(stderr, "Error: Failed to parse JSON\n");
    return NULL;
  }

  // Allocate the pattern structure
  TidalPattern *pattern = malloc(sizeof(TidalPattern));
  if (!pattern) {
    fprintf(stderr, "Error: Failed to allocate memory for pattern\n");
    json_object_put(root_obj);
    return NULL;
  }

  // Initialize pattern
  memset(pattern, 0, sizeof(TidalPattern));

  // Parse arcLen
  json_object *arc_len_obj;
  if (json_object_object_get_ex(root_obj, "arcLen", &arc_len_obj)) {
    pattern->arc_len = json_object_get_double(arc_len_obj);
  }

  // Parse events array
  json_object *events_obj;
  if (json_object_object_get_ex(root_obj, "events", &events_obj)) {
    if (json_object_is_type(events_obj, json_type_array)) {
      int events_count = json_object_array_length(events_obj);

      if (events_count > 0) {
        // Allocate events array
        pattern->events = malloc(sizeof(TidalEvent) * events_count);
        if (!pattern->events) {
          fprintf(stderr, "Error: Failed to allocate memory for events\n");
          free(pattern);
          json_object_put(root_obj);
          return NULL;
        }

        pattern->events_count = events_count;

        // Parse each event
        for (int i = 0; i < events_count; i++) {
          json_object *event_obj = json_object_array_get_idx(events_obj, i);
          pattern->events[i] = parse_event(event_obj);
        }

        printf("Parsed %d events successfully\n", events_count);
      }
    } else {
      fprintf(stderr, "Error: 'events' is not an array\n");
    }
  } else {
    fprintf(stderr, "Warning: No 'events' field found\n");
  }

  // Clean up JSON object
  json_object_put(root_obj);

  return pattern;
}

// Helper function to free the pattern
void td_free_pattern(TidalPattern *pattern) {
  if (pattern) {
    if (pattern->events) {
      free(pattern->events);
    }
    free(pattern);
  }
}

// Helper function to print pattern (for debugging)
void td_print_pattern(TidalPattern *pattern) {
  if (!pattern) {
    printf("Pattern is NULL\n");
    return;
  }

  printf("Pattern: arcLen=%.2f, events_count=%d\n", pattern->arc_len,
         pattern->events_count);

  for (int i = 0; i < pattern->events_count; i++) { // Print first 5 events
    TidalEvent *event = &pattern->events[i];
    printf("  Event %d: part(%.2f-%.2f) whole(%.2f-%.2f) s='%s'", i,
           event->part.start, event->part.stop, event->whole.start,
           event->whole.stop, event->value.s);

    if (event->value.has_n) {
      printf(" n=%d", event->value.n);
    }
    if (event->value.has_gain) {
      printf(" gain=%.2f", event->value.gain);
    }
    printf("\n");
  }

  // if (pattern->events_count > 5) {
  //   printf("  ... and %d more events\n", pattern->events_count - 5);
  // }
}
