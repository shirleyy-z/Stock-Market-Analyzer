#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stock.h"

// helper to exit on fatal error
static void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// reads analyzed messages, flags spikes, determines trends
void run_detector(int in_fd, int out_fd) {
    AnalyzedMsg input;    // incoming analyzed message from analyzer
    DetectedMsg output;    // output message with trend and spike alerts 

    char ticker[TICKER_LEN] = "";   // current ticker
    int result;

    // read messages until EOF or MSG_DONE
    while ((result = read_full(in_fd, &input, sizeof(input))) > 0) {

        // handle done message
        if (input.kind == MSG_DONE) {
            output = (DetectedMsg){0};
            output.kind = MSG_DONE;
            if (write_full(out_fd, &output, sizeof(output)) == -1) {
                error_exit("detector write MSG_DONE");
            }
            break;
        }

        // check for unexpected message type
        if (input.kind != MSG_RECORD) {
            fprintf(stderr, "detector: unknown message kind %d\n", input.kind);
            exit(EXIT_FAILURE);
        }

        // detect new ticker
        if (strcmp(ticker, input.ticker) != 0) {
            strncpy(ticker, input.ticker, sizeof(ticker) - 1);
            ticker[sizeof(ticker) - 1] = '\0';
            printf(">> Detecting trends and spikes for %s\n", ticker);
            fflush(stdout);
        }

        // prepare output message
        output = (DetectedMsg){0};
        output.kind = MSG_RECORD;
        strncpy(output.ticker, input.ticker, sizeof(output.ticker) - 1);
        strncpy(output.date, input.date, sizeof(output.date) - 1);
        output.price = input.price;
        output.change_percent = input.change_percent;
        output.moving_avg = input.moving_avg;

        // flag spikes based on percent change
        if (input.change_percent > 5.0) {
            strncpy(output.alert, "SPIKE_UP", sizeof(output.alert) - 1);
        } else if (input.change_percent < -5.0) {
            strncpy(output.alert, "SPIKE_DOWN", sizeof(output.alert) - 1);
        } else {
            strncpy(output.alert, "NONE", sizeof(output.alert) - 1);
        }

        // determine price trend relative to moving average
        if (input.price > input.moving_avg) {
            strncpy(output.trend, "UP", sizeof(output.trend) - 1);
        } else if (input.price < input.moving_avg) {
            strncpy(output.trend, "DOWN", sizeof(output.trend) - 1);
        } else {
            strncpy(output.trend, "FLAT", sizeof(output.trend) - 1);
        }

        // write output safely to next process
        if (write_full(out_fd, &output, sizeof(output)) == -1) {
            error_exit("detector write");
        }
    }

    // check for read error
    if (result == -1) {
        error_exit("detector read");
    }

    // close file descriptors
    if (close_fd(in_fd) == -1) {
        error_exit("detector close in_fd");
    }
    if (close_fd(out_fd) == -1) {
        error_exit("detector close out_fd");
    }

    exit(EXIT_SUCCESS);
}