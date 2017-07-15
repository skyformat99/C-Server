# C-Server

A web server written in C using the Socket API.

## Design
- Parse and validate command line arguments. 
- Creates and bind a socket to the user specified port. 
- If successful,  program provide user feedback and wait for requests or exit on "q" (then enter).
- On a request, validate it using `getResponse()`
- Read the associated file and log it server-side using `logResponse()`
- Serves the requested file to the client using `sendto()`.

## To Build and Run
- Use `make` to build
- You can then invoke the server by `./sws <port> <directory>`
