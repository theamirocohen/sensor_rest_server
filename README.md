# Sensor rest server

A very simple, fast, multithreaded, REST server to receive  comma-separated sensors statistics and show them.

The API server used is the Simple-Web-Server [https://gitlab.com/eidheim/Simple-Web-Server] 

## Usage

See [server.cpp](https://github.com/theamirocohen/sensor_rest_server/blob/main/server.cpp) for example usage.
The following server resources are setup using regular expressions to match request paths:
* `POST /string` - parse the given sensor samples.
* `GET /stats` - responds with statistic information.

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
curl -X POST http://localhost:8080/string -d "sensor_id,temparture,date"
```
For example:
```sh
curl -X POST http://localhost:8080/string -d "2,30,20/2/2024"
```
To get statistic infornation:
```sh
curl -X GET http://localhost:8080/stats?<sensor_id/ALL>,SUN/MON/TUE/WED/THU/FRI/SAT/ALL>
```
For example:
```sh
curl -X GET http://localhost:8080/stats?2,MON
```
OR
Direct your favorite browser to for instance http://localhost:8080/stats?<sensor_id/ALL>,SUN/MON/TUE/WED/THU/FRI/SAT/ALL>

### documentation 

For project documentation see [documentation.md](https://github.com/theamirocohen/sensor_rest_server/blob/main/documentation.md)
