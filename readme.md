# Quick Suite Trading - Interview Assignment - C++ Developer

## Notes

 * Implemented two server variants:
   * threaded version: spawns a thread to handle each connection.
   * io socket multiplexing: handles all connections in a single thread
   by waiting for requests from each of them (and new connections) using select.
 * Concurrent & io socket multiplexing server variant not implemented due to additional complexity and lack of time.
 * For performance reasons aimed to write server code to inline as much as possible and without using heap allocation for message handling (the trade-off is to have stack allocation for maximum message size, but given spec this doesn't look prohibitive).
 * Spec states that initial_key requires a sum complement checksum of the username & password values, but the sample shows that just the plain sum of the characters was performed, so commented this out to align cipher tests with sample.
 * Sample client includes an interactive mode to log and then write messages to be echoed by server, and a benchmark mode that sets up multiple concurrent connections and then proceeds to send echo requests with lines read from a file.
 
See TODO for additional improvements & limitations.

## TODO

 * Error handling for malformed requests or malicious client/server.
   * The current version just uses assertions to validate data & format.
 * Use timeout in server read operations to avoid blocking on malformed requests.
 * Use an adapter/wrapper to handle socket fd lifetime automatically when the wrapper is destructed.
 * Threaded server
   * Add threadpool to limit the number of concurrent connections and avoid the possibility of DoS.
 * Implement concurrent & io socket multiplexing server
   * Add threadpool to handle client requests (instead of spinning a new thread for every request)
   * Queue handle request work item for threadpool to consume
   * Add queue for worker threads to notify main thread that a client
   request has been handled (so that the main thread doesn't check for
   events on the client socket until the current one is handled).
