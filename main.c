#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "stock.h"

// helper to print an error message and exit
static void error_exit(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// print one formatted stock summary
static void print_stock_summary(const SummaryMsg *summary) {
    printf("========================================\n");
    printf("Ticker: %s\n", summary->ticker);
    printf("Records Processed: %d\n", summary->records_processed);
    printf("Start Price: %.4f\n", summary->first_price);
    printf("End Price: %.4f\n", summary->last_price);
    printf("Overall Change: %.2f%%\n", summary->overall_change_percent);
    printf("Average Closing Price: %.4f\n", summary->average_price);
    printf("Highest Price: %.4f\n", summary->highest_price);
    printf("Lowest Price: %.4f\n", summary->lowest_price);
    printf("Up Days: %d\n", summary->up_days);
    printf("Down Days: %d\n", summary->down_days);
    printf("Flat Days: %d\n", summary->flat_days);
    printf("Spike Ups: %d\n", summary->spike_ups);
    printf("Spike Downs: %d\n", summary->spike_downs);
    printf("General Trend: %s\n", summary->general_trend);
    printf("========================================\n\n");
    fflush(stdout);
}

// parse one input line into a raw stock record
static int parse_input_line(const char *line, RawRecordMsg *record) {
    char ticker[TICKER_LEN];
    char date[DATE_LEN];
    double price;

    if (sscanf(line, "%15[^,],%15[^,],%lf", ticker, date, &price) != 3) {
        return -1;
    }

    *record = (RawRecordMsg){0};
    record->kind = MSG_RECORD;
    strncpy(record->ticker, ticker, sizeof(record->ticker) - 1);
    strncpy(record->date, date, sizeof(record->date) - 1);
    record->price = price;
    return 0;
}

// wait for a child process and check that it exits normally
static void wait_for_child(pid_t pid, const char *name) {
    int status;
    pid_t wait_result = waitpid(pid, &status, 0);

    if (wait_result == -1) {
        error_exit("waitpid");
    }

    // stop if child crashed or exited with failure
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "%s exited abnormally\n", name);
        exit(EXIT_FAILURE);
    }
}

// set up pipes and child processes for the stock pipeline
int main(void) {
    int parent_to_analyzer[2];
    int analyzer_to_detector[2];
    int detector_to_reporter[2];
    int reporter_to_parent[2];

    pid_t analyzer_pid, detector_pid, reporter_pid;
    FILE *input_file = NULL;
    char line[256];
    int total_stocks = 0;

    printf("\nSTOCK MARKET ANALYSIS\n");
    fflush(stdout);

    if (pipe(parent_to_analyzer) == -1) {
        error_exit("pipe parent_to_analyzer");
    }

    if (pipe(analyzer_to_detector) == -1) {
        error_exit("pipe analyzer_to_detector");
    }

    if (pipe(detector_to_reporter) == -1) {
        error_exit("pipe detector_to_reporter");
    }

    if (pipe(reporter_to_parent) == -1) {
        error_exit("pipe reporter_to_parent");
    }

    // create analyzer process
    analyzer_pid = fork();
    if (analyzer_pid == -1) {
        error_exit("fork analyzer");
    }
    if (analyzer_pid == 0) {
        close_fd(parent_to_analyzer[1]);
        close_fd(analyzer_to_detector[0]);
        close_fd(detector_to_reporter[0]);
        close_fd(detector_to_reporter[1]);
        close_fd(reporter_to_parent[0]);
        close_fd(reporter_to_parent[1]);

        run_analyzer(parent_to_analyzer[0], analyzer_to_detector[1]);
    }

    // create detector process
    detector_pid = fork();
    if (detector_pid == -1) {
        error_exit("fork detector");
    }

    if (detector_pid == 0) {
        close_fd(parent_to_analyzer[0]);
        close_fd(parent_to_analyzer[1]);
        close_fd(analyzer_to_detector[1]);
        close_fd(detector_to_reporter[0]);
        close_fd(reporter_to_parent[0]);
        close_fd(reporter_to_parent[1]);

        run_detector(analyzer_to_detector[0], detector_to_reporter[1]);
    }

    // create reporter process
    reporter_pid = fork();
    if (reporter_pid == -1) {
        error_exit("fork reporter");
    }
    if (reporter_pid == 0) {
        close_fd(parent_to_analyzer[0]);
        close_fd(parent_to_analyzer[1]);
        close_fd(analyzer_to_detector[0]);
        close_fd(analyzer_to_detector[1]);
        close_fd(detector_to_reporter[1]);
        close_fd(reporter_to_parent[0]);

        run_reporter(detector_to_reporter[0], reporter_to_parent[1]);
    }

    // parent closes pipe ends it does not use
    close_fd(parent_to_analyzer[0]);
    close_fd(analyzer_to_detector[0]);
    close_fd(analyzer_to_detector[1]);
    close_fd(detector_to_reporter[0]);
    close_fd(detector_to_reporter[1]);
    close_fd(reporter_to_parent[1]);

    // open cleaned input file
    input_file = fopen("stock_data.csv", "r");
    if (!input_file) {
        error_exit("fopen stock_data.csv");
    }

    char prev_ticker[TICKER_LEN] = "";
    while (fgets(line, sizeof(line), input_file)) {
        RawRecordMsg record;

        // skip bad input lines
        if (parse_input_line(line, &record) == -1) {
            fprintf(stderr, "Skipping malformed line: %s", line);
            continue;
        }

        // print when a new ticker starts
        if (strcmp(prev_ticker, record.ticker) != 0) {
            printf(">> Reading stock data for %s\n", record.ticker);
            fflush(stdout);
            strncpy(prev_ticker, record.ticker, sizeof(prev_ticker) - 1);
        }

        // send record to analyzer
        if (write_full(parent_to_analyzer[1], &record, sizeof(record)) == -1) {
            error_exit("parent write record");
        }
    }

    if (ferror(input_file)) {
        fclose(input_file);
        error_exit("fgets");
    }
    fclose(input_file);

    // send done message to analyzer
    RawRecordMsg done = {0};
    done.kind = MSG_DONE;
    if (write_full(parent_to_analyzer[1], &done, sizeof(done)) == -1) {
        error_exit("parent write MSG_DONE");
    }
    close_fd(parent_to_analyzer[1]);

    // read summary messages from reporter
    SummaryMsg summary;
    while (1) {
        int result = read_full(reporter_to_parent[0], &summary, sizeof(summary));

        if (result == 0) {
            fprintf(stderr, "parent: unexpected EOF from reporter\n");
            exit(EXIT_FAILURE);
        }
        if (result == -1) {
            error_exit("parent read summary");
        }

        // stop when reporter sends done
        if (summary.kind == MSG_DONE) {
            break;
        }

        // reject unexpected message types
        if (summary.kind != MSG_SUMMARY) {
            fprintf(stderr, "parent: unknown summary message kind %d\n", summary.kind);
            exit(EXIT_FAILURE);
        }

        printf(">> Showing results for %s\n", summary.ticker);
        fflush(stdout);
        print_stock_summary(&summary);
        total_stocks++;
    }

    close_fd(reporter_to_parent[0]);

    // print final program summary
    printf("=========== FINAL SUMMARY ===========\n");
    printf("Total Stocks Analyzed: %d\n", total_stocks);
    printf("=====================================\n");
    fflush(stdout);

    wait_for_child(analyzer_pid, "Analyzer");
    wait_for_child(detector_pid, "Detector");
    wait_for_child(reporter_pid, "Reporter");

    return EXIT_SUCCESS;
}