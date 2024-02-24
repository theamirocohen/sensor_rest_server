# Word-Count-Server

A very simple, fast, multithreaded, REST server to receive  comma-separated words and show occurrences statistics.

The API server used is the Simple-Web-Server [https://gitlab.com/eidheim/Simple-Web-Server] 

## Usage

See [server.cpp](https://github.com/theamirocohen/word_count_rest_server/blob/main/server.cpp) for example usage.
The following server resources are setup using regular expressions to match request paths:
* `POST /string` - parse the given string in to words.
* `GET /stats` - responds with statistic information regarding the words occurrences.

## Dependencies

* Boost.Asio or standalone Asio
* Boost is required to compile the examples

Installation instructions for the dependencies needed to compile the examples on a selection of platforms can be seen below.

### Debian based distributions

```sh
sudo apt-get install libboost-filesystem-dev libboost-thread-dev
```

### Arch Linux based distributions

```sh
sudo pacman -S boost
```

### MacOS

```sh
brew install boost
```

## Compile and run

Compile with a C++11 compliant compiler:
```sh
cmake -H. -Bbuild
cmake --build build
```

### RUN

Run the server: `./build/server`

To post string use:
```sh
curl -X POST http://localhost:8080/string -d "first_word,second_word"
```
To get statistic infornation:
```sh
curl -X GET http://localhost:8080/stats
```
OR
Direct your favorite browser to for instance http://localhost:8080/

### documentation 

For project documentation see [documentation.md](https://github.com/theamirocohen/word_count_rest_server/blob/main/documentation.md)
