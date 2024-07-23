# LSO-23-24
Repository for the Operating Systems Laboratory course, Academic year 2023/24.

## Project Compilation Guide

This guide explains how to compile the project using the make tool. Follow the steps below to ensure successful compilation.

### Prerequisites

Before you begin, make sure you have the following installed on your system:
- GCC (GNU Compiler Collection)
- Make
- Bash
- Docker
- Docker compose

### Compilation Instructions

## To run the project on your machine

1. **Clone the Repository**

   First, clone the repository to your local machine. Open your terminal and run:

   ```sh
   git clone https://github.com/leokm02/LSO-23-24.git
   cd LSO-23-24
      ```

    If you already have a copy of the project you can skip this step

2.    **Compile the program**

The project includes a Makefile that handles the entire compilation process. To compile the project, simply run:
```sh
    make
```
## To run the project in a docker container

1. **Clone the Repository**

   First, clone the repository to your local machine. Open your terminal and run:

   ```sh
   git clone https://github.com/leokm02/LSO-23-24.git
   cd LSO-23-24
      ```

    If you already have a copy of the project you can skip this step


2.    **Run the app**

The project includes a 'launchApp.sh' script that handles the building and running phase. To do so run:
```sh
    ./launchApp.sh
```

3.    **Stop the app**

To stop the server from running press <kbd>Ctrl</kbd> + <kbd>C</kbd>

This will stop the server and also output a log.txt file containing the server log in the project folder.


      
### Run Instructions

To run the server run the following command from your terminal:

```sh
    ./server.out
```

If the server is running in a docker container already you can skip this step.

To run a bot client in automatic mode run the following command from your terminal:

```sh
    ./client.out
```
This will run a bot client that will take a random time to choose products and buy them.

To run an interactive client run the following command from your terminal:

```sh
    ./client.out -i
```
This will create a client controlled by you where you decide what to buy.

The project includes a launchMultipleClient.sh script that launches multiple bot clients at once. To do so, simply run:

```sh
    ./launchMultipleClient.sh CLIENTS_TO_LAUNCH
```

where CLIENTS_TO_LAUNCH is a number (e.g 10)

### Troubleshooting

•    **Permission Denied**: If you encounter a permission error when running launchApp.sh or launchMultipleClient.sh, you might need to make the scripts executable. Run:
```sh
    chmod +x launchApp.sh launchMultipleClient.sh
```
•    **Missing Dependencies**: Ensure all dependencies are installed. If you encounter any missing dependencies, install them using your package manager (e.g., apt, yum, brew). 

