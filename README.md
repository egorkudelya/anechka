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
  estimate you are bound to notice significant perfomance degradation due to collisions.
- The engine relies on file mmaping, so it is recommended that you use a 64-bit machine as a host.
  
## Populating the Engine

To populate the search engine with data, call RequestRecursiveDirIndexing and pass the path to the directory containing the dataset.
Note that the directory tree is traversed recursively and the engine will index every txt file in the provided directory and its children.
You can also call RequestTxtFileIndexing to index an individual txt file.
The engine will index the data and make it searchable using RequestTokenSearch or RequestTokenSearchWithContext.

## Demo

Let's query some data and test searching capabilities of the engine. For example, executing RequestTokenSearchWithContext and passing it some token,
in our example "conscientious", returns the following tuple:

![Example Image](https://drive.google.com/uc?id=1HLaM91TeJT5SEHFjjzqb3R2h5JZJC4V3)

As you can see, the message conforms to the format specified in the proto file -- we got back the time it took to execute the query in ms and
a number of json documents, where each document contains a path to the file and a context array with the searched token.

