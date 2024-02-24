## Intuition
I wanted the project to be as efficient as possible, thus allowing concurrency and trying to minimize critical section as much as possible.

Not all input checks were done, thus the input should be as exact as possible, stats will be printed only for valid input, else nothing will be printed.

The main thread will run the server and use threads to retrieve input strings, each thread will parse the string containing sensor id, temperature and date and store them in a hash table where the key is the sensor id and the value is another hash table, where the key for it is the day of the week (derived from the date) and the value is the stats of the sensor id regarding the specific day.
I chose std::unordered_map to use as data base, for they are utilizing a hash map which is much faster then a regular map.
a critical section is used for entering data to the hash tables to prevent tempering of the data during writing.

When calling the get stats method I retrive the data depending on the stats required, and will print the desired data.
If a specific sensor and day are selected we will print directly the corresponding struct that hold the stats if exist, for weekly stats (sensor specific or all sensors) the server will calculate the relevant stats and return the value.

I chose to trade off between slow excution time on the stats method (all sensors weekly stats) in order to read and set data from sensors as fast as possible following the requirements of the exercise, mainly because the critical section reside in the insertion section.

The server I chose to work with is a git repository called simple-web-server (https://gitlab.com/eidheim/Simple-Web-Server) which provided a very simple, fast, multithreaded, platform independent HTTP server and client library, adding the desired POST and GET methods was fast and intuitive.

The functionality code is entirely in [server.cpp](https://github.com/theamirocohen/sensor_rest_server/blob/main/server.cpp). 

## algorithm explained

In order to save only the past week stats (following the exercise requirements), I chose to create a hash map that contains at most 7 daily buckets, meaning, the last 7 days where each day corresponds to its number in the week.
Each bucket contain min temperature, max temperature, sum of all temperatures, num of samples and a date, the date functions as a validation that the current day stats are not "old" (not from the past week), if, inserting new data to an "old" day stats is needed, new data will overwrite it, and when printing stats "old" data will be ignored.

For now there is no "memory cleanup" (garbage collection) to the server, several methods could be implemented to do so, one of them is during stats printing, while encountering "old" data that can be freed, but it will requiere critical section addition.

## complexity

### Time complexity:
Adding a sensor data base is O(1)

Printing specific sensor and day stats requires to fetch an already stats struct in O(1).

Printing weekly stats for specific sensor requires to fetch all weekly struct stats (max of 7 structs) in O(1).

Printing weekly stats for all sensor requires to fetch all data struct stats in O(N).


### Space complexity:
Holding the sensor data to the global data base is O(N)



