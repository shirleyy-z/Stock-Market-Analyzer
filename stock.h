#ifndef STOCK_H
#define STOCK_H

#include <stddef.h>
#include <stdint.h>

#define TICKER_LEN 16
#define DATE_LEN 16
#define TREND_LEN 16
#define ALERT_LEN 16
#define MOVING_AVG_WINDOW 3

#define MSG_RECORD   1
#define MSG_DONE     2
#define MSG_SUMMARY  3

// raw stock record sent from parent to analyzer
typedef struct {
    int32_t kind;   // MSG_RECORD or MSG_DONE
    char ticker[TICKER_LEN];
    char date[DATE_LEN];
    double price;
} RawRecordMsg;

// analyzed stock record sent from analyzer to detector
typedef struct {
    int32_t kind;   // MSG_RECORD or MSG_DONE
    char ticker[TICKER_LEN];
    char date[DATE_LEN];
    double price;
    double change_percent;
    double moving_avg;
} AnalyzedMsg;

// detected stock record sent from detector to reporter
typedef struct {
    int32_t kind;   // MSG_RECORD or MSG_DONE
    char ticker[TICKER_LEN];
    char date[DATE_LEN];
    double price;
    double change_percent;
    double moving_avg;
    char trend[TREND_LEN];
    char alert[ALERT_LEN];
} DetectedMsg;

// summary record sent from reporter to parent
typedef struct {
    int32_t kind;   // MSG_SUMMARY or MSG_DONE
    char ticker[TICKER_LEN];
    int32_t records_processed;
    double first_price;
    double last_price;
    double average_price;
    double highest_price;
    double lowest_price;
    int32_t up_days;
    int32_t down_days;
    int32_t flat_days;
    int32_t spike_ups;
    int32_t spike_downs;
    double overall_change_percent;
    char general_trend[TREND_LEN];
} SummaryMsg;

// read a full message from a file descriptor
int read_full(int fd, void *buf, size_t count);

// write a full message to a file descriptor
int write_full(int fd, const void *buf, size_t count);

// close a file descriptor safely
int close_fd(int fd);

// analyzer worker entry point
void run_analyzer(int in_fd, int out_fd);

// detector worker entry point
void run_detector(int in_fd, int out_fd);

// reporter worker entry point
void run_reporter(int in_fd, int out_fd);

#endif