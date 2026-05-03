This project implements a multi-process stock analysis pipeline using UNIX pipes and process control in C.
 
The application processes cleaned historical stock data for three companies: AAPL (Apple), AMZN
(Amazon), and XOM (Exxon Mobil). These stocks were selected to represent different types of market
behavior, including long-term growth, volatility, and cyclical trends.
The system is structured as a pipeline consisting of three worker processes:
• Analyzer: computes derived metrics such as percentage change
• Detector: classifies trends and detects price spikes
• Reporter: aggregates processed data and produces final summaries
 
The input to the program is a CSV file with the format:
ticker,date,open,high,low,close,volume
Each row represents a single stock record. The parent parses each line and sends structured messages
through the pipeline.
The output consists of one summary per stock, including average price, overall percentage change, counts
of upward and downward movements, spike detection results, and an overall trend class
