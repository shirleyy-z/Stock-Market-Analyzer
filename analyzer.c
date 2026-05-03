#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stock.h"

// print an error message and exit program
static void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// process stock records, compute percent change and moving average, and send processed messages
void run_analyzer(int in_fd, int out_fd) {
    RawRecordMsg input;    // raw stock record from input
    AnalyzedMsg output;    // sprocessed record to send to output

    char ticker[TICKER_LEN] = "";   // current ticker 
    double last_price = 0.0;    // previous price for change calculation

    double prices[MOVING_AVG_WINDOW] = {0.0};   // buffer for moving average
    int count = 0;  // number of valid prices 
    int position = 0;   // next index in buffer 

    int result;
    while ((result = read_full(in_fd, &input, sizeof(input))) > 0) {

        // handle end-of-stream message
        if (input.kind == MSG_DONE) {
            output = (AnalyzedMsg){0};
            output.kind = MSG_DONE;
            if (write_full(out_fd, &output, sizeof(output)) == -1) {
                error_exit("analyzer write MSG_DONE");
            }
            break;
        }

        // check for unexpected message kind
        if (input.kind != MSG_RECORD) {
            fprintf(stderr, "analyzer: unknown message %d\n", input.kind);
            exit(EXIT_FAILURE);
        }

        // detect new ticker and reset moving average state
        if (strcmp(ticker, input.ticker) != 0) {
            strncpy(ticker, input.ticker, sizeof(ticker) - 1);
            ticker[sizeof(ticker) - 1] = '\0';

            last_price = input.price;
            count = 0;
            position = 0;
            for (int i = 0; i < MOVING_AVG_WINDOW; i++) {
                prices[i] = 0.0;
            }

            printf(">> computing changes and averages for %s\n", ticker);
            fflush(stdout);
        }

        // prepare output message
        output = (AnalyzedMsg){0};
        output.kind = MSG_RECORD;
        strncpy(output.ticker, input.ticker, sizeof(output.ticker) - 1);
        strncpy(output.date, input.date, sizeof(output.date) - 1);
        output.price = input.price;

        // calculate daily percent change
        if (count == 0) {
            output.change_percent = 0.0;  
        } else {
            output.change_percent = ((input.price - last_price) / last_price) * 100.0;
}

        // update circular buffer for moving average
        prices[position] = input.price;
        position = (position + 1) % MOVING_AVG_WINDOW;
        if (count < MOVING_AVG_WINDOW) {
            count++;
        }

        // compute moving average
        double sum = 0.0;
        for (int i = 0; i < count; i++) {
            sum += prices[i];
        }
        output.moving_avg = sum / count;

        // write processed message safely
        if (write_full(out_fd, &output, sizeof(output)) == -1) {
            error_exit("analyzer write");
        }

        last_price = input.price;  // update last_price for next percent change
    }

    // check for read error
    if (result == -1) {
        error_exit("analyzer read");
    }

    // close file descriptors safely
    if (close_fd(in_fd) == -1) {
        error_exit("analyzer close in_fd");
    }
    if (close_fd(out_fd) == -1) {
        error_exit("analyzer close out_fd");
    }

    exit(EXIT_SUCCESS);
}