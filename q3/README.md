# OSN Assignment 5 - Question 3
### ~ Mihir Bani, 2019113003
## How to Run
1. `client.cpp` and `server.cpp` are in `client` and `server` directories respectively.
2. **For Server Code**- Open a terminal in `server` directory, compile the code using ```g++ server.c -o server -lpthread``` and run the code by typing ```./server n``` on terminal.
Where *n* denotes the number of worker threads.
3. **For Client Code**- Open a separate terminal in `client` directory, compile the code using ```g++ client.c -o client -lpthread``` and run the code by typing ```./client``` on terminal.

4. In the client terminal session, give all the inputs together. Starting with a number *m*, which denotes the number of requests. Then the supported commands are:
    1. `insert <key> <value>`: inserts a new key-value if key doesnt exist already.
    1. `delete <key>`: to delete that key value pair from the server if it exists.  
    1. `update <key> <value>`: update a key-value pair, if key exists already.
    1. `concat <key1> <key2>`: concat the values of both keys to each other, if both values exist.
    1. `fetch <key>`: returns the value for mentioned key, if it exists.   

   Appropriate error messages and outcomes are printed on the Client session.

5. No command is needed to be entered in the server terminal session. Although for each request on client side, the server status is also updated.
6. Use `Ctrl+C` on client and server terminals to exit the code.

## Implementation
### Server
**Special points:**
- A Queue is created with stores the file descriptors returned from the `accept()` system call. This is basically the fds of the clients which want to send request to the server. Mutex lock is used whenever it is used in pushing or popping and in conditional wait.
- Dictionary is implementated as array of strings, and initialized as empty strings. Whenever a modificaion is required, mutex lock is done only on the particular `key` which is the index of the array. So it is thread-safe. Also in `concat` the key with smaller value is locked first, to maintain heirarchy of mutex, to avoid deadlocks in rare cases.  
  
**Code flow:**
1. A thread pool of size `n` (given as input) is initialized, which calls a function `handle_thread_pool()`. 
   1. This thread/function has an infinite loop and conditional waits on the Queue, whether it is empty or not.
   2. If Queue is not empty then it is popped and and this fd is used to read request from the client. 
   3. The request is handled according to the commands mentioned in client side, and output string is sent to the client. 
   4. If the process is successful, the loop starts again, and the thread can be reused.
2. The server creates a `listen_fd` socket to listen to connect requests. Then it uses `bind()` and `listen()` system calls.
3. It starts an infinite loop and uses `accept()` system call here to get client fd. It *locks* the mutex for Queue, push this fd in it and *signals* the conditional variable to stop waiting and then *unlocks* the mutex.
### Client
1. All input is take together and stored.
2. Each request is treated as a separate thread, which creates a socket by `socket()` and connects to the server by `connect()`.
3. Then it sends the command string to the server.
4. Then it receives the message from server, *lock mutex* for printing it to *stdout* and *unlocks the mutex*.
5. The thread then `return NULL` when its request is completed.