#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stock.h"

typedef struct {
    int initialized;
    char ticker[TICKER_LEN];
    int records_processed;
    double first_price;
    double last_price;
    double sum_price;
    double highest_price;
    double lowest_price;
    int up_days;
    int down_days;
    int flat_days;
    int spike_ups;
    int spike_downs;
} RunningSummary;

// print an error message and exit
static void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// reset all running summary values
static void reset_running_summary(RunningSummary *running) {
    *running = (RunningSummary){0};
}

// build and send one summary message
static void send_summary(int out_fd, const RunningSummary *running) {
    SummaryMsg output = {0};

    output.kind = MSG_SUMMARY;
    strncpy(output.ticker, running->ticker, sizeof(output.ticker) - 1);
    output.records_processed = running->records_processed;
    output.first_price = running->first_price;
    output.last_price = running->last_price;
    output.average_price = running->sum_price / running->records_processed;
    output.highest_price = running->highest_price;
    output.lowest_price = running->lowest_price;
    output.up_days = running->up_days;
    output.down_days = running->down_days;
    output.flat_days = running->flat_days;
    output.spike_ups = running->spike_ups;
    output.spike_downs = running->spike_downs;

    // calculate overall percent change for the ticker
    output.overall_change_percent =
        ((running->last_price - running->first_price) / running->first_price) * 100.0;

    // set overall trend label
    if (output.overall_change_percent > 0.0) {
        strncpy(output.general_trend, "UPWARD", sizeof(output.general_trend) - 1);
    } 
    else if (output.overall_change_percent < 0.0) {
        strncpy(output.general_trend, "DOWNWARD", sizeof(output.general_trend) - 1);
    } 
    else {
        strncpy(output.general_trend, "FLAT", sizeof(output.general_trend) - 1);
    }

    // send summary to parent
    if (write_full(out_fd, &output, sizeof(output)) == -1) {
        error_exit("reporter write summary");
    }
}

// read detected messages and build summary stats for each ticker
void run_reporter(int in_fd, int out_fd) {
    DetectedMsg input;  // detected message from detector
    SummaryMsg done;    // done message sent at the end
    RunningSummary running;    // running stats for current ticker

    reset_running_summary(&running);

    int result;
    while ((result = read_full(in_fd, &input, sizeof(input))) > 0) {

        // handle done message from detector
        if (input.kind == MSG_DONE) {
            if (running.initialized) {
                send_summary(out_fd, &running);
            }
            done = (SummaryMsg){0};
            done.kind = MSG_DONE;
            if (write_full(out_fd, &done, sizeof(done)) == -1) {
                error_exit("reporter write MSG_DONE");
            }
            break;
        }

        // stop if message type is invalid
        if (input.kind != MSG_RECORD) {
            fprintf(stderr, "reporter: unknown message kind %d\n", input.kind);
            exit(EXIT_FAILURE);
        }

        // start tracking the first ticker
        if (!running.initialized) {
            running.initialized = 1;
            strncpy(running.ticker, input.ticker, sizeof(running.ticker) - 1);
            running.first_price = input.price;
            running.highest_price = input.price;
            running.lowest_price = input.price;

        // send previous summary and reset for a new ticker
        } else if (strcmp(running.ticker, input.ticker) != 0) {
            send_summary(out_fd, &running);
            reset_running_summary(&running);

            running.initialized = 1;
            strncpy(running.ticker, input.ticker, sizeof(running.ticker) - 1);
            running.first_price = input.price;
            running.highest_price = input.price;
            running.lowest_price = input.price;
        }

        // update running totals
        running.records_processed++;
        running.last_price = input.price;
        running.sum_price += input.price;

        // update highest and lowest prices
        if (input.price > running.highest_price) {
            running.highest_price = input.price;
        }
        if (input.price < running.lowest_price) {
            running.lowest_price = input.price;
        }

        // count trend days
        if (strcmp(input.trend, "UP") == 0) {
            running.up_days++;
        } 
        else if (strcmp(input.trend, "DOWN") == 0) {
            running.down_days++;
        } 
        else {
            running.flat_days++;
        }

        // count spike alerts
        if (strcmp(input.alert, "SPIKE_UP") == 0) {
            running.spike_ups++;
        } 
        else if (strcmp(input.alert, "SPIKE_DOWN") == 0) {
            running.spike_downs++;
        }
    }

    // check for read error
    if (result == -1) {
        error_exit("reporter read");
    }

    // close file descriptors
    if (close_fd(in_fd) == -1) {
        error_exit("reporter close in_fd");
    }
    if (close_fd(out_fd) == -1) {
        error_exit("reporter close out_fd");
    }

    exit(EXIT_SUCCESS);
}