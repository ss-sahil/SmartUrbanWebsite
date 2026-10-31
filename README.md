# Smart Urban Route Planning System

A C++17 web application that finds the fastest route between locations in a small simulated city. It runs Dijkstra's algorithm on a road network whose travel times respond to traffic congestion and road closures. The route, road conditions and journey statistics are shown on an interactive map in the browser.

The back end is a single C++ file using [cpp-httplib](https://github.com/yhirose/cpp-httplib) (header-only). The front end (HTML, CSS and JavaScript) is embedded in the executable, so there is nothing else to deploy.

## Features

- **Shortest travel-time routing** using Dijkstra's algorithm, weighted by `base time × traffic multiplier`
- **Interactive SVG city map** with the optimized route highlighted
- **Traffic simulation** with three rotating scenarios that add congestion and close roads
- **Automatic rerouting** around congested or closed roads
- **Journey summary** showing total distance (km), estimated time (min) and the stop-by-stop route
- **Road conditions panel** listing closed roads and heavy congestion
- **Input validation** on API parameters, and HTML escaping of names in the UI
- **Thread-safe state** guarded by a mutex

## City Network

8 locations and 12 roads:

| ID | Location        |
|----|-----------------|
| 0  | Central Station |
| 1  | City Hospital   |
| 2  | University      |
| 3  | Shopping Mall   |
| 4  | Airport         |
| 5  | Tech Park       |
| 6  | Bus Terminal    |
| 7  | Old Town        |

Each road has a distance in km, a base travel time in minutes, a traffic multiplier (1.0 = normal) and an open/closed flag.

## Traffic Scenarios

Each press of **Simulate Traffic** resets conditions and applies the next scenario in a cycle of three:

1. Heavy congestion on University–Hospital, University–Mall and Mall–Tech Park roads
2. Congestion on several roads and **closure of the Mall–Airport road**
3. Congestion on Central Station–Hospital, University–Tech Park and Airport–Tech Park roads

**Reset Traffic** restores all roads to normal and sets the scenario back to 0.

## Requirements

- A C++17 compiler (g++, clang++ or MSVC)
- `httplib.h` (cpp-httplib) in the same folder as `main.cpp`
- Threading support (`-pthread` on Linux/macOS)

Tested against cpp-httplib 0.64.0. HTTPS is not used, so OpenSSL is not needed.

## Build and Run

### Linux / macOS

```bash
g++ -std=c++17 -O2 main.cpp -o route_planner -pthread
./route_planner
```

### Windows (MinGW)

```bash
g++ -std=c++17 -O2 main.cpp -o route_planner.exe -lws2_32
route_planner.exe
```

### Windows (MSVC, Developer Command Prompt)

```bash
cl /std:c++17 /EHsc /O2 main.cpp
main.exe
```

Then open **http://localhost:8080** in your browser.

> `main.cpp` defines `_WIN32_WINNT 0x0A00` before including `httplib.h`, as cpp-httplib requires Windows 10 or later. On Windows, the redefinition warning can be ignored if your compiler already defines it.

## Usage

1. Choose a **Starting Location** and **Destination**.
2. Click **Find Best Route** to compute the fastest path.
3. Click **Simulate Traffic** to apply the next scenario; the route is recalculated automatically.
4. Click **Reset Traffic** to return to normal conditions.

Map legend: blue = optimized route, orange = congestion, red dashed = closed road, grey = normal road.

If a closure disconnects the two locations, the UI reports that no route is available.

## API

All endpoints are `GET` and return the full state as JSON.

| Endpoint                       | Description                                        |
|--------------------------------|----------------------------------------------------|
| `/`                            | The web interface                                  |
| `/api/state`                   | Current route, cities and road conditions          |
| `/api/route?start=<id>&end=<id>` | Set source and destination, return the new route |
| `/api/simulate`                | Advance to the next traffic scenario               |
| `/api/reset`                   | Clear all congestion and closures                  |

`/api/route` returns HTTP 400 for missing, non-numeric or out-of-range IDs.

### Example response

```json
{
  "found": true,
  "source": 0,
  "destination": 4,
  "distance": 14.50,
  "time": 28.00,
  "scenario": 0,
  "route": [0, 1, 2, 3, 4],
  "cities": [{"name": "Central Station", "x": 70, "y": 100}],
  "roads": [{"u": 0, "v": 1, "km": 4.00, "time": 8.00, "traffic": 1.00, "open": true}]
}
```

(The values above are illustrative; `cities` and `roads` are abbreviated.)

## Project Structure

```
.
├── main.cpp      # Server, routing algorithm, traffic logic, embedded front end
├── httplib.h     # cpp-httplib (third-party, MIT licensed)
└── README.md
```

## How It Works

1. `calculateRoute()` runs Dijkstra's algorithm over the graph, skipping closed roads and using `time × traffic` as the edge weight.
2. The path is rebuilt from the `previous` array, and the total distance is summed along the chosen roads.
3. Each API handler locks `dataMutex`, updates the shared state and returns it as JSON.
4. The browser redraws the SVG map and panels from that JSON.

## Customization

- **Change the map**: edit the `cities` and `roads` vectors, and update `N` and `M` to match the counts.
- **Change traffic scenarios**: edit `simulateTraffic()`.
- **Change the port or bind address**: edit the `server.listen("127.0.0.1", 8080)` call. Use `"0.0.0.0"` to allow access from other machines.

## Troubleshooting

- **"Could not start server on port 8080"**: another program is using the port. Close it or change the port.
- **"Cannot connect to the C++ server"** in the browser: the executable has stopped. Restart it.
- **Compile error about `httplib.h`**: make sure the header is in the same directory as `main.cpp`.
- **Linker errors on Windows**: link against `ws2_32` (`-lws2_32`).

## Licenses

cpp-httplib is © Yuji Hirose and distributed under the MIT License.
