
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <limits>
#include <mutex>
#define _WIN32_WINNT 0x0A00
#include "httplib.h"

using namespace std;

// ======================================================
// SMART URBAN ROUTE PLANNING SYSTEM
// C++17 + CPP-HTTPLIB + HTML/CSS/JavaScript frontend
// ======================================================

const int N = 8;
const int M = 12;

struct City {
    string name;
    int x, y;
};

struct Road {
    int u, v;
    double km, time, traffic;
    bool open;
};

vector<City> cities = {
    {"Central Station", 70, 100},
    {"City Hospital", 210, 60},
    {"University", 350, 90},
    {"Shopping Mall", 490, 60},
    {"Airport", 570, 180},
    {"Tech Park", 400, 240},
    {"Bus Terminal", 220, 240},
    {"Old Town", 70, 260}
};

vector<Road> roads = {
    {0,1,4,8,1,true},
    {0,6,3,7,1,true},
    {0,7,2.5,6,1,true},
    {1,2,5,9,1,true},
    {1,6,4,8,1,true},
    {2,3,3.5,7,1,true},
    {2,5,4.5,9,1,true},
    {3,4,6,11,1,true},
    {3,5,4,8,1,true},
    {4,5,5.5,10,1,true},
    {5,6,5,9,1,true},
    {6,7,3,6,1,true}
};

vector<int> route;

int source = 0;
int destination = 4;
int scenario = 0;

double totalKm = 0;
double totalTime = 0;

bool found = false;

mutex dataMutex;

// ======================================================
// HELPER FUNCTIONS
// ======================================================

string number(double value) {
    ostringstream ss;
    ss << fixed << setprecision(1) << value;
    return ss.str();
}

int roadIndex(int a, int b) {
    for (int i = 0; i < M; i++) {
        if ((roads[i].u == a && roads[i].v == b) ||
            (roads[i].u == b && roads[i].v == a)) {
            return i;
        }
    }
    return -1;
}

bool isRouteRoad(int a, int b) {
    for (int i = 0; i + 1 < (int)route.size(); i++) {
        if ((route[i] == a && route[i+1] == b) ||
            (route[i] == b && route[i+1] == a)) {
            return true;
        }
    }
    return false;
}

// ======================================================
// TRAFFIC MANAGEMENT
// ======================================================

void resetTraffic() {
    for (int i = 0; i < M; i++) {
        roads[i].traffic = 1.0;
        roads[i].open = true;
    }

    scenario = 0;
}

void simulateTraffic() {
    resetTraffic();
    scenario++;

    if (scenario % 3 == 1) {
        roads[3].traffic = 2.2;
        roads[5].traffic = 1.8;
        roads[8].traffic = 1.7;
    }
    else if (scenario % 3 == 2) {
        roads[1].traffic = 2.0;
        roads[4].traffic = 1.8;
        roads[10].traffic = 2.3;
        roads[7].open = false;
    }
    else {
        roads[0].traffic = 1.6;
        roads[6].traffic = 2.1;
        roads[9].traffic = 1.8;
    }
}

// ======================================================
// DIJKSTRA'S SHORTEST TRAVEL-TIME ALGORITHM
// ======================================================

void calculateRoute() {

    vector<double> dist(N, numeric_limits<double>::max());
    vector<int> previous(N, -1);
    vector<int> visited(N, 0);

    dist[source] = 0;

    for (int step = 0; step < N; step++) {

        int u = -1;
        double best = numeric_limits<double>::max();

        for (int i = 0; i < N; i++) {
            if (!visited[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }

        if (u == -1)
            break;

        visited[u] = 1;

        if (u == destination)
            break;

        for (int r = 0; r < M; r++) {

            if (!roads[r].open)
                continue;

            int v = -1;

            if (roads[r].u == u)
                v = roads[r].v;
            else if (roads[r].v == u)
                v = roads[r].u;

            if (v == -1 || visited[v])
                continue;

            double weight = roads[r].time * roads[r].traffic;
            double newTime = dist[u] + weight;

            if (newTime < dist[v]) {
                dist[v] = newTime;
                previous[v] = u;
            }
        }
    }

    route.clear();
    found = false;
    totalKm = 0;
    totalTime = 0;

    if (source == destination) {
        route.push_back(source);
        found = true;
    }
    else if (dist[destination] != numeric_limits<double>::max()) {

        int current = destination;

        while (current != -1) {
            route.push_back(current);

            if (current == source)
                break;

            current = previous[current];
        }

        if (!route.empty() && route.back() == source) {
            reverse(route.begin(), route.end());
            found = true;
        }
        else {
            route.clear();
        }
    }

    if (found) {

        totalTime = dist[destination];

        for (int i = 0; i + 1 < (int)route.size(); i++) {
            int r = roadIndex(route[i], route[i+1]);

            if (r >= 0)
                totalKm += roads[r].km;
        }
    }
}

// ======================================================
// JSON RESPONSE
// ======================================================

string getStateJSON() {

    calculateRoute();

    ostringstream out;

    out << fixed << setprecision(2);

    out << "{";

    out << "\"found\":" << (found ? "true" : "false") << ",";
    out << "\"source\":" << source << ",";
    out << "\"destination\":" << destination << ",";
    out << "\"distance\":" << totalKm << ",";
    out << "\"time\":" << totalTime << ",";
    out << "\"scenario\":" << scenario << ",";

    out << "\"route\":[";

    for (int i = 0; i < (int)route.size(); i++) {
        if (i > 0) out << ",";
        out << route[i];
    }

    out << "],";

    out << "\"cities\":[";

    for (int i = 0; i < N; i++) {
        if (i > 0) out << ",";

        out << "{";
        out << "\"name\":\"" << cities[i].name << "\",";
        out << "\"x\":" << cities[i].x << ",";
        out << "\"y\":" << cities[i].y;
        out << "}";
    }

    out << "],";

    out << "\"roads\":[";

    for (int i = 0; i < M; i++) {

        if (i > 0) out << ",";

        out << "{";
        out << "\"u\":" << roads[i].u << ",";
        out << "\"v\":" << roads[i].v << ",";
        out << "\"km\":" << roads[i].km << ",";
        out << "\"time\":" << roads[i].time << ",";
        out << "\"traffic\":" << roads[i].traffic << ",";
        out << "\"open\":" << (roads[i].open ? "true" : "false");
        out << "}";
    }

    out << "]";

    out << "}";

    return out.str();
}

// ======================================================
// WEBSITE FRONTEND
// ======================================================

const char* website = R"HTML(

<!DOCTYPE html>
<html lang="en">

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>Smart Urban Route Planning</title>

<style>

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: Arial, sans-serif;
    background: #f0f4fa;
    color: #1e293b;
}

header {
    background: linear-gradient(120deg,#102b50,#1765a9);
    color: white;
    padding: 25px 35px;
    box-shadow: 0 4px 15px #0002;
}

header h1 {
    font-size: 27px;
    margin-bottom: 8px;
}

header p {
    color: #dbeafe;
    font-size: 14px;
}

.container {
    max-width: 1450px;
    margin: auto;
    padding: 25px;
}

.dashboard {
    display: grid;
    grid-template-columns: 1fr 320px;
    gap: 22px;
}

.card {
    background: white;
    padding: 22px;
    border-radius: 13px;
    box-shadow: 0 3px 14px #0000000c;
    margin-bottom: 20px;
}

.card h2 {
    font-size: 19px;
    margin-bottom: 18px;
    color: #183b65;
}

.map-area {
    width: 100%;
    overflow: auto;
    background: #f7fafc;
    border-radius: 10px;
    border: 1px solid #e1e9f2;
}

svg {
    width: 100%;
    min-width: 550px;
    height: auto;
    display: block;
}

select {
    width: 100%;
    padding: 12px;
    border: 1px solid #cbd5e1;
    border-radius: 7px;
    margin-top: 8px;
    margin-bottom: 17px;
    font-size: 14px;
    background: white;
}

label {
    font-weight: bold;
    font-size: 14px;
}

button {
    width: 100%;
    border: none;
    padding: 13px;
    border-radius: 7px;
    color: white;
    font-size: 14px;
    font-weight: bold;
    cursor: pointer;
    margin-bottom: 10px;
    transition: 0.2s;
}

button:hover {
    opacity: 0.85;
    transform: translateY(-1px);
}

.find-btn {
    background: #1769c2;
}

.traffic-btn {
    background: #e58a20;
}

.reset-btn {
    background: #16875a;
}

.stats {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-bottom: 20px;
}

.stat {
    background: #edf5ff;
    border-radius: 10px;
    padding: 18px;
}

.stat small {
    color: #64748b;
    font-size: 12px;
}

.stat h2 {
    font-size: 25px;
    margin: 8px 0 0;
    color: #1765b0;
}

.route-box {
    background: #f0f7ff;
    padding: 15px;
    border-radius: 8px;
    line-height: 1.9;
    font-size: 14px;
    overflow-wrap: anywhere;
}

.conditions {
    margin-top: 15px;
    font-size: 13px;
    line-height: 2;
}

.legend {
    display: flex;
    flex-wrap: wrap;
    gap: 15px;
    margin-top: 15px;
    font-size: 12px;
    color: #475569;
}

.dot {
    display: inline-block;
    width: 10px;
    height: 10px;
    border-radius: 50%;
    margin-right: 5px;
}

.status {
    background: #eaf7ef;
    color: #16804b;
    padding: 8px 12px;
    border-radius: 6px;
    display: inline-block;
    font-size: 12px;
    margin-bottom: 15px;
}

footer {
    text-align: center;
    color: #64748b;
    padding: 20px;
    font-size: 12px;
}

@media(max-width:900px) {
    .dashboard {
        grid-template-columns: 1fr;
    }
}

</style>

</head>

<body>

<header>

<h1>SMART URBAN ROUTE PLANNING SYSTEM</h1>

<p>Dynamic Path Optimization and Traffic Management Dashboard</p>

</header>

<div class="container">

<div class="dashboard">

<!-- LEFT SIDE -->

<div>

<div class="card">

<h2>City Road Network Map</h2>

<div class="map-area">

<svg id="cityMap" viewBox="0 0 700 350"></svg>

</div>

<div class="legend">

<span><i class="dot" style="background:#1677df"></i>Optimized Route</span>

<span><i class="dot" style="background:#ef9226"></i>Traffic Congestion</span>

<span><i class="dot" style="background:#dc3636"></i>Closed Road</span>

<span><i class="dot" style="background:#b4c3d2"></i>Normal Road</span>

</div>

</div>

<div class="card">

<h2>Optimized Route Details</h2>

<div id="routeOutput" class="route-box">

Calculating route...

</div>

</div>

<div class="card">

<h2>Road Conditions</h2>

<div id="conditions" class="conditions">

Loading road conditions...

</div>

</div>

</div>

<!-- RIGHT SIDE -->

<div>

<div class="card">

<h2>Route Configuration</h2>

<div class="status">C++ Route Optimization Engine</div>

<label>Starting Location</label>

<select id="start"></select>

<label>Destination</label>

<select id="end"></select>

<button class="find-btn" onclick="findRoute()">

Find Best Route

</button>

<button class="traffic-btn" onclick="simulateTraffic()">

Simulate Traffic

</button>

<button class="reset-btn" onclick="resetTraffic()">

Reset Traffic

</button>

</div>

<div class="card">

<h2>Journey Information</h2>

<div class="stats">

<div class="stat">

<small>Total Distance</small>

<h2 id="distance">-- km</h2>

</div>

<div class="stat">

<small>Estimated Time</small>

<h2 id="time">-- min</h2>

</div>

</div>

<p id="message" style="font-size:13px;color:#64748b;">

Ready to plan your journey.

</p>

</div>

<div class="card">

<h2>Traffic Simulation</h2>

<p style="font-size:13px;line-height:1.8;color:#64748b;">

Traffic simulation changes road congestion levels and can close selected roads. The C++ Dijkstra algorithm recalculates the best available route automatically.

</p>

<p style="margin-top:15px;font-weight:bold;">

Current Scenario: <span id="scenario">0</span>

</p>

</div>

</div>

</div>

</div>

<footer>

Smart Urban Route Planning System | Developed Using C++17

</footer>

<script>

// ==================================================
// FRONTEND DISPLAY AND SERVER COMMUNICATION
// Route calculations are performed by the C++ server.
// ==================================================

let state = null;

async function api(url) {

    const response = await fetch(url);

    if (!response.ok) {
        throw new Error("Server request failed");
    }

    return await response.json();
}

async function loadState() {

    try {

        state = await api("/api/state");

        updateDropdowns();
        drawMap();
        updateDetails();

    } catch(error) {

        document.getElementById("message").textContent =
            "Cannot connect to the C++ server. Please restart the application.";

    }
}

function updateDropdowns() {

    const start = document.getElementById("start");
    const end = document.getElementById("end");

    if (start.options.length === 0) {

        state.cities.forEach((city, i) => {

            const a = document.createElement("option");
            a.value = i;
            a.textContent = city.name;
            start.appendChild(a);

            const b = document.createElement("option");
            b.value = i;
            b.textContent = city.name;
            end.appendChild(b);

        });

    }

    start.value = state.source;
    end.value = state.destination;
}

async function findRoute() {

    const start = document.getElementById("start").value;
    const end = document.getElementById("end").value;

    try {

        state = await api(
            "/api/route?start=" + start + "&end=" + end
        );

        updateDropdowns();
        drawMap();
        updateDetails();

    } catch(error) {

        alert("Unable to calculate route.");

    }
}

async function simulateTraffic() {

    try {

        state = await api("/api/simulate");

        updateDropdowns();
        drawMap();
        updateDetails();

    } catch(error) {

        alert("Traffic simulation failed.");

    }
}

async function resetTraffic() {

    try {

        state = await api("/api/reset");

        updateDropdowns();
        drawMap();
        updateDetails();

    } catch(error) {

        alert("Unable to reset traffic.");

    }
}

// ==================================================
// DRAW INTERACTIVE CITY MAP
// ==================================================

function drawMap() {

    const svg = document.getElementById("cityMap");

    svg.innerHTML = "";

    const ns = "http://www.w3.org/2000/svg";

    function element(tag, attributes) {

        const el = document.createElementNS(ns, tag);

        Object.entries(attributes).forEach(([key,value]) => {
            el.setAttribute(key,value);
        });

        svg.appendChild(el);

        return el;
    }

    // Background

    element("rect", {
        x:0, y:0, width:700, height:350,
        fill:"#f7fafc"
    });

    // Grid

    for(let x=30; x<700; x+=35) {

        element("line", {
            x1:x, y1:0, x2:x, y2:350,
            stroke:"#eaf0f6",
            "stroke-width":1
        });

    }

    for(let y=20; y<350; y+=35) {

        element("line", {
            x1:0, y1:y, x2:700, y2:y,
            stroke:"#eaf0f6",
            "stroke-width":1
        });

    }

    // Roads

    state.roads.forEach(road => {

        const a = state.cities[road.u];
        const b = state.cities[road.v];

        let color = "#b4c3d2";
        let width = 3;
        let dash = "";

        const onRoute = state.route.some((id,i) =>
            i < state.route.length-1 &&
            ((id === road.u && state.route[i+1] === road.v) ||
             (id === road.v && state.route[i+1] === road.u))
        );

        if(!road.open) {

            color = "#dc3636";
            width = 4;
            dash = "8,5";

        }
        else if(onRoute) {

            color = "#1677df";
            width = 6;

        }
        else if(road.traffic >= 1.7) {

            color = "#ef9226";
            width = 4;

        }

        element("line", {

            x1:a.x,
            y1:a.y,
            x2:b.x,
            y2:b.y,
            stroke:color,
            "stroke-width":width,
            "stroke-dasharray":dash,
            "stroke-linecap":"round"

        });

        const mx = (a.x+b.x)/2;
        const my = (a.y+b.y)/2;

        const text = element("text", {
            x:mx,
            y:my-7,
            "text-anchor":"middle",
            fill:"#52657a",
            "font-size":"11",
            "font-weight":"bold"
        });

        text.textContent = road.km + " km";

    });

    // City nodes

    state.cities.forEach((city,i) => {

        let color = "#ffffff";

        if(i === state.source)
            color = "#6be39a";

        if(i === state.destination)
            color = "#c6a2f5";

        element("circle", {

            cx:city.x,
            cy:city.y,
            r:10,
            fill:color,
            stroke:"#344c66",
            "stroke-width":2

        });

        const text = element("text", {

            x:city.x,
            y:city.y+25,
            "text-anchor":"middle",
            fill:"#203650",
            "font-size":"12",
            "font-weight":"bold"

        });

        text.textContent = city.name;

    });

}

// ==================================================
// DISPLAY ROUTE AND ROAD CONDITIONS
// ==================================================

function updateDetails() {

    document.getElementById("distance").textContent =
        state.found ? state.distance.toFixed(1) + " km" : "-- km";

    document.getElementById("time").textContent =
        state.found ? state.time.toFixed(1) + " min" : "-- min";

    document.getElementById("scenario").textContent =
        state.scenario;

    const routeOutput = document.getElementById("routeOutput");

    if(state.found) {

        const names = state.route.map(i =>
            state.cities[i].name
        );

        routeOutput.innerHTML =
            "<b>BEST AVAILABLE ROUTE</b><br><br>" +
            names.map(escapeHTML).join(" &rarr; ") +
            "<br><br><b>Total Distance:</b> " +
            state.distance.toFixed(1) + " km" +
            "<br><b>Travel Time:</b> " +
            state.time.toFixed(1) + " minutes";

        document.getElementById("message").textContent =
            "Optimal travel-time route calculated successfully.";

    }
    else {

        routeOutput.textContent =
            "No route available. A road closure may have disconnected the locations.";

        document.getElementById("message").textContent =
            "Please select another route.";

    }

    const conditions = document.getElementById("conditions");

    let html = "";

    state.roads.forEach(road => {

        if(!road.open || road.traffic >= 1.7) {

            const name =
                state.cities[road.u].name + " - " +
                state.cities[road.v].name;

            if(!road.open) {

                html += "<div style='color:#dc3636'>CLOSED: " +
                    escapeHTML(name) + "</div>";

            }
            else {

                html += "<div style='color:#d97706'>CONGESTION: " +
                    escapeHTML(name) + " (" +
                    road.traffic.toFixed(1) + "x)</div>";

            }

        }

    });

    conditions.innerHTML = html ||
        "All roads are operating normally.";

}

function escapeHTML(value) {

    return String(value).replace(/[&<>"']/g, c => ({
        "&":"&amp;",
        "<":"&lt;",
        ">":"&gt;",
        '"':"&quot;",
        "'":"&#39;"
    })[c]);

}

// Initial website loading

loadState();

</script>

</body>
</html>

)HTML";

// ======================================================
// MAIN SERVER
// ======================================================

int main() {

    httplib::Server server;

    // Homepage

    server.Get("/", [](const httplib::Request& req,
                       httplib::Response& res) {

        res.set_content(website, "text/html");

    });

    // Get current route and traffic information

    server.Get("/api/state", [](const httplib::Request& req,
                                httplib::Response& res) {

        lock_guard<mutex> lock(dataMutex);

        res.set_content(getStateJSON(), "application/json");

    });

    // Calculate selected route

    server.Get("/api/route", [](const httplib::Request& req,
                                httplib::Response& res) {

        if (!req.has_param("start") || !req.has_param("end")) {

            res.status = 400;
            res.set_content("{\"error\":\"Select both locations\"}",
                            "application/json");
            return;

        }

        int s = -1;
        int d = -1;

        try {
            s = stoi(req.get_param_value("start"));
            d = stoi(req.get_param_value("end"));
        }
        catch (...) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid location\"}",
                            "application/json");
            return;
        }

        if (s < 0 || s >= N || d < 0 || d >= N) {

            res.status = 400;
            res.set_content("{\"error\":\"Location out of range\"}",
                            "application/json");
            return;

        }

        lock_guard<mutex> lock(dataMutex);

        source = s;
        destination = d;

        res.set_content(getStateJSON(), "application/json");

    });

    // Traffic simulation

    server.Get("/api/simulate", [](const httplib::Request& req,
                                   httplib::Response& res) {

        lock_guard<mutex> lock(dataMutex);

        simulateTraffic();

        res.set_content(getStateJSON(), "application/json");

    });

    // Reset traffic

    server.Get("/api/reset", [](const httplib::Request& req,
                                httplib::Response& res) {

        lock_guard<mutex> lock(dataMutex);

        resetTraffic();

        res.set_content(getStateJSON(), "application/json");

    });

    cout << "\n";
    cout << "============================================\n";
    cout << " SMART URBAN ROUTE PLANNING SYSTEM\n";
    cout << "============================================\n";
    cout << "C++ Web Server Started Successfully!\n";
    cout << "Open your browser at:\n";
    cout << "http://localhost:8080\n";
    cout << "============================================\n";

    if (!server.listen("127.0.0.1", 8080)) {

        cerr << "Error: Could not start server on port 8080.\n";
        cerr << "Check whether another application is using it.\n";

        return 1;

    }

    return 0;
}
