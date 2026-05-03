# Multi-Process Stock Analysis Pipeline (C, UNIX Pipes)

This project implements a multi-process stock analysis pipeline in C using UNIX pipes and process control. It demonstrates inter-process communication (IPC) and concurrent data processing through a structured pipeline architecture.

## Overview

The application processes cleaned historical stock data for three companies:

- AAPL (Apple) – long-term growth  
- AMZN (Amazon) – volatility  
- XOM (Exxon Mobil) – cyclical trends  

The system is designed to simulate a real-world data pipeline where each stage performs a specific transformation on streaming data.

## Architecture

The system consists of a parent process and three worker processes connected through pipes:

Parent → Analyzer → Detector → Reporter → Parent

### Pipeline Stages

- Analyzer  
  Computes derived metrics such as percentage change  

- Detector  
  Classifies trends and detects price spikes  

- Reporter  
  Aggregates processed data and produces final summaries  

## Input Format

The program reads a CSV file with the following format:

ticker,date,open,high,low,close,volume

Each row represents a single stock record.

The parent process parses each line, converts it into a structured message, and sends it through the pipeline.

## Output

The program produces one summary per stock, including:

- Average price  
- Overall percentage change  
- Count of upward movements  
- Count of downward movements  
- Spike detection results  
- Overall trend classification  

## Key Concepts

- UNIX pipes for inter-process communication  
- Process creation using fork  
- Streaming structured data between processes  
- Concurrent pipeline execution  

## Build and Run

Compile:
make

Run:
./stock_pipeline stock_data.csv

Clean:
make clean
