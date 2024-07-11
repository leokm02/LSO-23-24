# LSO-23-24
Repository per il progetto di Laboratorio di Sistemi Operativi, anno 2023/24.

## Project Compilation Guide

This guide explains how to compile the project using the provided `compile.sh` script. Follow the steps below to ensure successful compilation.

### Prerequisites

Before you begin, make sure you have the following installed on your system:
- GCC (GNU Compiler Collection)
- Make
- Bash

### Compilation Instructions

1. **Clone the Repository**

   First, clone the repository to your local machine. Open your terminal and run:

   ```sh
   git clone https://github.com/leokm02/LSO-23-24.git
   cd LSO-23-24
      ```
2.    **Execute the Compile Script**

   The project includes a compile.sh script that handles the entire compilation process. To compile the project, simply run:
    
   ```sh
      ./compile.sh
   ```
      
### Run Instructions

To run the server run the following command from your terminal:

```sh
    ./server
```

To run a client run the following command from your terminal:

```sh
    ./client
```

The project includes a launchMultipleClient.sh script that launches multiple client at once. To do so, simply run:

```sh
    ./launchMultipleClient.sh
```

### Troubleshooting
•    **Missing Separator Error**: Ensure that all commands in the Makefile are prefixed with a tab character and not spaces.
•    **Permission Denied**: If you encounter a permission error when running compile.sh, you might need to make the script executable. Run:
```sh
    chmod +x compile.sh
```
•    **Dependencies**: Ensure all dependencies are installed. If you encounter any missing dependencies, install them using your package manager (e.g., apt, yum, brew). 

