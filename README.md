## Overview

Welcome to Anechka, a simple search engine designed to be lightweight and easy to set up.

## Prerequisites

To use the engine, you only need the following:

- C++17-compatible compiler
- CMake
- Python

Everything required for the project is included in the "vendor" directory, eliminating the need for additional external dependencies.

## Configuration

To configure the search engine, navigate to the "config" directory and edit the JSON config file. Customize the settings according to your preferences and requirements.

## Running the Server

To run the server, execute the server executable and pass the path to the configuration file as an argument. For example, the following
command starts the server with the specified configuration.

./server config/config.json

## Running the Client

Execute the following command to run the client:

./client

The client connects to the server and provides an interface for interacting with the search engine.

## System Requirements

- The search engine is memory-intensive, and it is recommended to have at least 8 gigabytes of RAM for optimal performance. Note that without an appropriate token count
  estimate you are bound to notice significant performance degradation due to collisions.
- The engine relies on file mmaping, so it is recommended that you use a 64-bit machine as a host.
  
## Populating the Engine

To populate the search engine with data, call RequestRecursiveDirIndexing and pass the path to the directory containing the dataset.
Note that the directory tree is traversed recursively and the engine will index every txt file in the provided directory and its children.
You can also call RequestTxtFileIndexing to index an individual txt file.
The engine will index the data and make it searchable using RequestTokenSearch or RequestTokenSearchWithContext.

## Demo

Let's query some data and test searching capabilities of the engine. For example, executing RequestTokenSearchWithContext and passing it some token,
in our case "conscientious", returns the following tuple:

![Example Image](https://drive.google.com/uc?id=1pkI0eLNgG8YZtJsYMyKFzJKV7hLVgCri)

As you can see, the message conforms to the format specified in the proto file. We received the time it took to execute the query in milliseconds and a number of JSON documents. Each document contains the path to the file, along with the context array containing the sentences where the searched token appears.

