import QtQuick
import Quickshell
import Quickshell.Io
import qs.core

Scope {
    id: root

    property bool visible: false
    property bool busy: false
    property bool available: false
    property string icon: "󰖐"
    property string temp: "--"
    property string tempExact: "--"
    property string tempMin: "--"
    property string tempMax: "--"
    property string feels: "--"
    property string desc: "Loading weather..."
    property string detailedDesc: ""
    property string humidity: "--"
    property string city: "Locating..."
    property string country: ""
    property string wind: "--"
    property string windDir: ""
    property string windGust: ""
    property string pressure: "--"
    property string visibility: "--"
    property string clouds: "--"
    property string sunrise: "--"
    property string sunset: "--"

    readonly property string panelText: root.icon + " " + (root.temp !== "--" ? root.temp + "°C" : "")

    function toggle() {
        root.visible = !root.visible;
        if (root.visible && (!root.available || root.temp === "--")) {
            root.refresh();
        }
    }

    function close() {
        root.visible = false;
    }

    function open() {
        root.visible = true;
        if (!root.available || root.temp === "--") {
            root.refresh();
        }
    }

    function refresh() {
        if (weatherProcess.running) return;
        weatherProcess.running = true;
    }

    function parseWeather(raw) {
        try {
            const data = JSON.parse(raw);
            root.available = data.available !== false;
            root.icon = data.icon || "󰖐";
            root.temp = String(data.temp || "--");
            root.tempExact = String(data.temp_exact || data.temp || "--");
            root.tempMin = String(data.temp_min || "--");
            root.tempMax = String(data.temp_max || "--");
            root.feels = String(data.feels || "--");
            root.desc = data.desc || "Clear";
            root.detailedDesc = data.detailed_desc || data.desc || "";
            root.humidity = String(data.humidity || "--");
            root.city = data.city || "Local";
            root.country = data.country || "";
            root.wind = String(data.wind || "--");
            root.windDir = data.wind_dir || "";
            root.windGust = String(data.wind_gust || "");
            root.pressure = String(data.pressure || "--");
            root.visibility = String(data.visibility || "--");
            root.clouds = String(data.clouds || "--");
            root.sunrise = data.sunrise || "--";
            root.sunset = data.sunset || "--";
        } catch (e) {
            root.available = false;
            root.desc = "Weather unavailable";
        }
    }

    // Auto-refresh on startup and every 15 minutes (900,000 ms)
    Timer {
        interval: 900000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: root.refresh()
    }

    Process {
        id: weatherProcess

        command: Commands.weatherCommand("--json", [])
        running: false

        stdout: StdioCollector {
            onStreamFinished: root.parseWeather(this.text)
        }
    }
}
