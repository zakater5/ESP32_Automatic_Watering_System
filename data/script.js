//Funkcija za prikaz strani v stranskem meniju 
function showPage(pageId) {
	const pages = document.querySelectorAll('.page');
	pages.forEach(page => {
		if (page.id === pageId) { // Dodaj aktivni razred za sprožitev animacije
			page.classList.add('active');
		} else { // Odstrani aktivno, da skriješ stare strani
			page.classList.remove('active');
		}
	});
}

let isAuto = true;
let pinState = false; // false = OFF, true = ON
window.isAuto = true;

// Funkcija za nastavljanje načina upravljanja esp-ja (ročno / avtomatsko)
function toggleMode() {
    window.isAuto = !window.isAuto;
    const modeText = document.getElementById("mode-status");
    const modeBtn = document.getElementById("mode-btn");
    const onoffBtn = document.getElementById("onoff-btn");
    modeText.textContent = window.isAuto ? "Auto" : "Manual";
    modeBtn.textContent = window.isAuto ? "Switch to Manual" : "Switch to Auto";

    if (window.isAuto) {
        onoffBtn.disabled = true;
        onoffBtn.classList.add("disabled-btn");
    } else {
        onoffBtn.disabled = false;
        onoffBtn.classList.remove("disabled-btn");
    }

    fetch("/set_mode", { // povej ESP-ju
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ auto: window.isAuto })
    })
    .then(res => res.json())
    .then(data => console.log("ESP mode updated:", data))
    .catch(err => console.error("Error updating mode:", err));
}

// Funkcija za pošiljanje signala esp-ju za vklop pumpe / izklop
function toggleOnOff() {
	fetch("/signal")
		.then(response => response.text())
		.then(text => {
			pinState = !pinState;
			document.getElementById("onoff-btn").textContent = pinState ? "Turn OFF" : "Turn ON";
		})
		.catch(err => {
			console.error("Error toggling pin:", err);
		});
}

// Funkcija za risanje grafov (Pomagal ChatGPT)
function drawChartWithLabels(svgId, data, lineColor = "#2196f3") {
	/* 
		svgId = id html elementa v katerega rišemo
		data = podatki katere rišemo v graf
		lineColor = barva linij v grafu, privzeto = #2196f3
	*/
	const svg = document.getElementById(svgId);
	svg.innerHTML = "";

	const paddingLeft = 50; // increased left padding for labels
	const paddingTop = 30;
	const paddingBottom = 40; // a bit extra bottom padding for X labels
	const chartWidth = 300;
	const chartHeight = 150;

	const maxY = Math.max(...data);
	const minY = Math.min(...data);
	const yRange = maxY - minY || 1;

	const width = chartWidth + paddingLeft + 20; // some right padding
	const height = chartHeight + paddingTop + paddingBottom;

	svg.setAttribute("width", width);
	svg.setAttribute("height", height);
	svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
	svg.style.background = "transparent";
	svg.style.borderRadius = "5px";

	const stepX = chartWidth / (data.length - 1);

	// Horizontal grid lines + Y labels
	const numYLabels = 5;
	for (let i = 0; i <= numYLabels; i++) {
		const y = paddingTop + (chartHeight / numYLabels) * i;
		const value = (maxY - (yRange / numYLabels) * i).toFixed(1);

		const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
		line.setAttribute("x1", paddingLeft);
		line.setAttribute("y1", y);
		line.setAttribute("x2", paddingLeft + chartWidth);
		line.setAttribute("y2", y);
		line.setAttribute("stroke", "#333");
		line.setAttribute("stroke-width", "1");
		svg.appendChild(line);

		const label = document.createElementNS("http://www.w3.org/2000/svg", "text");
		label.setAttribute("x", paddingLeft - 10);
		label.setAttribute("y", y + 6);
		label.setAttribute("fill", "#ccc");
		label.setAttribute("font-size", "16"); // bigger font
		label.setAttribute("font-weight", "600");
		label.setAttribute("text-anchor", "end");
		label.textContent = value;
		svg.appendChild(label);
	}

	// Vertical grid lines + X labels
	for (let i = 0; i < data.length; i++) {
		const x = paddingLeft + stepX * i;

		if (i % 2 === 0 || data.length <= 6) {
			const label = document.createElementNS("http://www.w3.org/2000/svg", "text");
			label.setAttribute("x", x);
			label.setAttribute("y", paddingTop + chartHeight + 30);
			label.setAttribute("fill", "#ccc");
			label.setAttribute("font-size", "16"); // bigger font
			label.setAttribute("font-weight", "600");
			label.setAttribute("text-anchor", "middle");
			label.textContent = `t${i}`;
			svg.appendChild(label);
		}

		const vLine = document.createElementNS("http://www.w3.org/2000/svg", "line");
		vLine.setAttribute("x1", x);
		vLine.setAttribute("y1", paddingTop);
		vLine.setAttribute("x2", x);
		vLine.setAttribute("y2", paddingTop + chartHeight);
		vLine.setAttribute("stroke", "#333");
		vLine.setAttribute("stroke-width", "1");
		svg.appendChild(vLine);
	}

	// Axis lines
	const yAxis = document.createElementNS("http://www.w3.org/2000/svg", "line");
	yAxis.setAttribute("x1", paddingLeft);
	yAxis.setAttribute("y1", paddingTop);
	yAxis.setAttribute("x2", paddingLeft);
	yAxis.setAttribute("y2", paddingTop + chartHeight);
	yAxis.setAttribute("stroke", "#666");
	yAxis.setAttribute("stroke-width", "2");
	svg.appendChild(yAxis);

	const xAxis = document.createElementNS("http://www.w3.org/2000/svg", "line");
	xAxis.setAttribute("x1", paddingLeft);
	xAxis.setAttribute("y1", paddingTop + chartHeight);
	xAxis.setAttribute("x2", paddingLeft + chartWidth);
	xAxis.setAttribute("y2", paddingTop + chartHeight);
	xAxis.setAttribute("stroke", "#666");
	xAxis.setAttribute("stroke-width", "2");
	svg.appendChild(xAxis);

	// Polyline points
	const points = data.map((val, i) => {
		const x = paddingLeft + stepX * i;
		const y = paddingTop + ((maxY - val) / yRange) * chartHeight;
		return `${x},${y}`;
	}).join(" ");

	// Data polyline
	const polyline = document.createElementNS("http://www.w3.org/2000/svg", "polyline");
	polyline.setAttribute("points", points);
	polyline.setAttribute("fill", "none");
	polyline.setAttribute("stroke", lineColor);
	polyline.setAttribute("stroke-width", "2");
	svg.appendChild(polyline);

	// Interactive dots & tooltips
	data.forEach((val, i) => {
		const cx = paddingLeft + stepX * i;
		const cy = paddingTop + ((maxY - val) / yRange) * chartHeight;

		const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
		circle.setAttribute("cx", cx);
		circle.setAttribute("cy", cy);
		circle.setAttribute("r", 6); // slightly bigger dots
		circle.setAttribute("fill", lineColor);
		circle.style.cursor = "pointer";
		svg.appendChild(circle);

		const tooltipGroup = document.createElementNS("http://www.w3.org/2000/svg", "g");
		tooltipGroup.style.visibility = "hidden";

		const tooltipRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
		tooltipRect.setAttribute("x", cx - 30);
		tooltipRect.setAttribute("y", cy - 45);
		tooltipRect.setAttribute("width", 60);
		tooltipRect.setAttribute("height", 30);
		tooltipRect.setAttribute("rx", 6);
		tooltipRect.setAttribute("ry", 6);
		tooltipRect.setAttribute("fill", "#333");
		tooltipRect.setAttribute("opacity", "0.9");
		tooltipGroup.appendChild(tooltipRect);

		const tooltipText = document.createElementNS("http://www.w3.org/2000/svg", "text");
		tooltipText.setAttribute("x", cx);
		tooltipText.setAttribute("y", cy - 25);
		tooltipText.setAttribute("fill", "#fff");
		tooltipText.setAttribute("font-size", "16");
		tooltipText.setAttribute("font-weight", "600");
		tooltipText.setAttribute("text-anchor", "middle");
		tooltipText.textContent = `${val}`;
		tooltipGroup.appendChild(tooltipText);

		svg.appendChild(tooltipGroup);

		circle.addEventListener("mouseenter", () => {
			tooltipGroup.style.visibility = "visible";
		});
		circle.addEventListener("mouseleave", () => {
			tooltipGroup.style.visibility = "hidden";
		});
	});
}


// Posadabljanje grafa ob pridobitvi novega podatko o temperaturi
function chart_updateTemperatureData(newVal) {
	temperatureData.shift();
	temperatureData.push(newVal);
	drawChartWithLabels("tempChart", temperatureData, "#fc7703");
}

// Posadabljanje grafa ob pridobitvi novega podatko o vlagi
function chart_updateHumidityData(newVal) {
	humidityData.shift();
	humidityData.push(newVal);
	drawChartWithLabels("humidityChart", humidityData, "#035afc");
}

// Posadabljanje grafa ob pridobitvi novega podatko o svetlosti
function chart_updateLightData(newVal) {
	lightData.shift();
	lightData.push(newVal);
	drawChartWithLabels("lightChart", lightData, "#e8fc03");
}

// Posadabljanje grafa ob pridobitvi novega podatko o vlažnosti
function chart_updateMoistureData(newVal) {
	moistureData.shift();
	moistureData.push(newVal);
	drawChartWithLabels("moistureChart", moistureData, "#03fc28");
}

// Podatki za preizkus delovanje offline
const temperatureData = [22, 23, 22.5, 24, 23.5, 25, 26, 24.5, 23, 22.5, 23];
const humidityData = [45, 50, 47, 52, 48, 55, 53, 51, 50, 49, 48];
const lightData = [300, 320, 310, 330, 340, 350, 340, 335, 320, 310, 300];
const moistureData = [30, 32, 31, 29, 35, 33, 34, 32, 30, 31, 33];

drawChartWithLabels("tempChart", temperatureData, "#2196f3");     // blue
drawChartWithLabels("humidityChart", humidityData, "#4caf50");   // green
drawChartWithLabels("lightChart", lightData, "#ffc107");         // amber
drawChartWithLabels("moistureChart", moistureData, "#ff5722");   // deep orange


// error notif
let offlinePopup = document.getElementById('esp-offline-popup');
let offlineTimer = null;
let offlineState = false;

// Funkcija za prikaz obvestila ob nedosegljivosti esp-ja
function displayESP32offlineNotif() {
    if (offlineTimer) clearTimeout(offlineTimer);
    offlineTimer = setTimeout(() => {
        if (!offlineState) {
            offlinePopup.style.display = 'block';
            offlineState = true;
        }
    }, 2000); // pokaži samo, če 2 sekundi NI nobenega uspešnega fetch-a
}

// Funkcija za skrivanje obvestila ob nedosegljivosti esp-ja
function hideESP32offlineNotif() {
    if (offlineTimer) clearTimeout(offlineTimer);
    if (offlineState) {
        offlinePopup.style.display = 'none';
        offlineState = false;
    }
}


// funkcija za konverzijo podatka o svetlosti
function convertLight(input) {
	const clamped = Math.min(Math.max(input, 3000), 4000);
	const percent = 1 - (clamped - 3000) / 1000;
	return percent * 100;
}

// funkcija katera kliče esp vsakih x milisekund za nov podatek o svetlobi
function updateLightLevelData() {
	fetch("/light_sensor")
		.then(res => res.json())
		.then(data => {
			document.getElementById("light-value").textContent = "Current: " + data.light + " lx";
			chart_updateLightData(data.light);
			hideESP32offlineNotif();
		})
		.catch(err => displayESP32offlineNotif());
}

// funkcija katera kliče esp vsakih x milisekund za nov podatek o temperaturi in vlagi (ozračja :( )
function updateTempHumData() {
	fetch("/dht_sensor")
		.then(res => res.json())
		.then(data => {
			document.getElementById("temperature-value").textContent = "Current: " + data.temperature + " °C";
			document.getElementById("humidity-value").textContent = "Current: " + data.humidity + " %";
			chart_updateTemperatureData(data.temperature);
			chart_updateHumidityData(data.humidity);
			hideESP32offlineNotif();
		})
		.catch(err => displayESP32offlineNotif());
}

// funkcija katera kliče esp vsakih x milisekund za nov podatek o vlagi tal
function updateMoistureData() {
	fetch("/moisture_sensor")
		.then(res => res.json())
		.then(data => {
			document.getElementById("moisture1-value").textContent = "Moist_1: " + data.moisture1 + " %";
			document.getElementById("moisture2-value").textContent = "Moist_2: " + data.moisture2 + " %";
			document.getElementById("moisture3-value").textContent = "Moist_3: " + data.moisture3 + " %";
			document.getElementById("moisture4-value").textContent = "Moist_4: " + data.moisture4 + " %";
			const moisture_sum = (data.moisture1 + data.moisture2 + data.moisture3 + data.moisture4) / 4
			chart_updateMoistureData(moisture_sum);
			hideESP32offlineNotif();
		})
		.catch(err => displayESP32offlineNotif());
}

// Funkcija za prvotno nalaganje konfiguracije shranjene na esp-ju
function loadJsonDataFromEsp32() {
	// id-ji vseh HTML elementov kateri prikazujejo nastavitev konfiguracije:
	const wifi_ssid_config = document.getElementById('wifi-ssid-config');
	const wifi_password_config = document.getElementById('wifi-password-config');
	const temperature_unit_config = document.getElementById('temperature-unit-config');
	const sensor_update_interval_config = document.getElementById('sensor-update-interval-config');
	const display_enabled_config = document.getElementById('display-enabled-config');
	const display_brightness_config = document.getElementById('display-brightness-config');
	const display_timeout_config = document.getElementById('display-timeout-config');
	const language_select_config = document.getElementById('language-select-config');
	const logging_enabled_config = document.getElementById('logging-enabled-config');
	const logging_interval_config = document.getElementById('logging-interval-config');

	fetch('/get_json_data') // klic za pridobitev konfiguracije od esp-ja
		.then(res => res.json())
		.then(data => {
			console.log('Received JSON:', data);
			wifi_ssid_config.value = data["wifi_ssid_config"] || 'HomeNetwork';
			wifi_password_config.value = data["wifi_password_config"] || 'DRXJN525';
			temperature_unit_config.value = data["temperature_unit_config"] || 'celsius';
			sensor_update_interval_config.value = data["sensor_update_interval_config"] || 1;
			display_enabled_config.checked = data["display_enabled_config"] ?? true;
			display_brightness_config.value = data["display_brightness_config"] || 50;
			display_timeout_config.value = data["display_timeout_config"] || 50;
			language_select_config.value = data["language_select_config"] || 'en';
			logging_enabled_config.checked = data["logging_enabled_config"] ?? true;
			logging_interval_config.value = data["logging_interval_config"] || 50;
		})
		.catch(err => console.error('Error fetching JSON:', err));
}

// Funkcija za shranjevanje konfiguracija na esp
function saveJsonDataToEsp32() {
	// id-ji vseh HTML elementov kateri prikazujejo nastavitev konfiguracije:
	const wifi_ssid_config = document.getElementById('wifi-ssid-config');
	const wifi_password_config = document.getElementById('wifi-password-config');
	const temperature_unit_config = document.getElementById('temperature-unit-config');
	const sensor_update_interval_config = document.getElementById('sensor-update-interval-config');
	const display_enabled_config = document.getElementById('display-enabled-config');
	const display_brightness_config = document.getElementById('display-brightness-config');
	const display_timeout_config = document.getElementById('display-timeout-config');
	const language_select_config = document.getElementById('language-select-config');
	const logging_enabled_config = document.getElementById('logging-enabled-config');
	const logging_interval_config = document.getElementById('logging-interval-config');

	const new_json_config = { // zapakirat v JSON
		"wifi_ssid_config": wifi_ssid_config.value,
		"wifi_password_config": wifi_password_config.value,
		"temperature_unit_config": temperature_unit_config.value,
		"sensor_update_interval_config": sensor_update_interval_config.value,
		"display_enabled_config": display_enabled_config.value,
		"display_brightness_config": display_brightness_config.value,
		"display_timeout_config": display_timeout_config.value,
		"language_select_config": language_select_config.value,
		"logging_enabled_config": logging_enabled_config.value,
		"logging_interval_config": logging_interval_config.value,
	};

	fetch('/save_json_data', { // klic za shranjevanje konfiguracije od esp-ja
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify(new_json_config)
		})
		.then(response => {
			if (response.ok) {
				alert("Settings saved successfully!");
			} else {
				alert("Failed to save settings.");
			}
		})
		.catch(err => console.error("Save error:", err));
}

// Zagon kode ob začetku delovanja HTML-ja
document.addEventListener("DOMContentLoaded", () => {
	// Klic funkcij za osveževanje ob začetku
	updateLightLevelData();
	updateTempHumData();
	updateMoistureData();

	setInterval(() => { // Klic istih funkcij vsako sekundo (1000ms = 1s)
		updateLightLevelData();
		updateTempHumData();
		updateMoistureData();
	}, 1000);

	loadJsonDataFromEsp32(); // Klic funkcije za nalaganje konfiguracije esp-ja

	document.getElementById('configForm').addEventListener('submit', (e) => { // ob kliku "Shrani"
		e.preventDefault();
		saveJsonDataToEsp32(); // shranjevanje konfiguracije
	});
});



// Prikaz prvotnega "splash-screen" ob zagonu spletne strani
window.addEventListener("load", () => {
	const splash = document.getElementById("splash-screen");
	const terminal = document.getElementById("terminal-lines");

	const lines = [ // prikaz naklučnih podatkov
		"Initializing CRKN kernel v1.0...",
		"Loading sensor modules: [OK]",
		"Mounting filesystem: [OK]",
		"Reading configuration files: [OK]",
		"Establishing Wi-Fi connection...",
		"Wi-Fi connected: 10.0.0.9",
		"",
		"Launching services...",
		" - SensorService... [OK]",
		" - AutomationDaemon... [OK]",
		" - WebInterface... [OK]",
		"",
		"System time sync... [OK]",
		"System stable.",
		"",
		">> Welcome to 🌿 C.R.K.N.",
		">> Centralized Remote Knowledge Network",
		"",
		"Boot complete. Starting UI..."
	];

	let index = 0;
	function typeNextLine() { // Prikaz vsake linije
		if (index < lines.length) {
			terminal.textContent += lines[index] + "\n";
			index++;
			setTimeout(typeNextLine, 100); // 100ms na linijo
		} else {
			setTimeout(() => { // Skrij po končanem zagonskem zaporedju
				splash.style.opacity = "0";
				setTimeout(() => splash.style.display = "none", 500);
			}, 1000);
		}
	}
	typeNextLine();
});



document.addEventListener("DOMContentLoaded", () => {
	const intervalSlider = document.getElementById("sensor-update-interval-config");
	const intervalDisplay = document.getElementById("interval-display");

	intervalDisplay.textContent = intervalSlider.value + "s";
	intervalSlider.addEventListener("input", () => {
	    intervalDisplay.textContent = intervalSlider.value + "s";
	});

    const form = document.getElementById("configForm");
    fetch("/get_json_data") // Preberi config iz ESP
        .then(res => res.json())
        .then(config => {
			document.getElementById("wifi-ssid-config").value = config.wifi_ssid || "";
			document.getElementById("wifi-password-config").value = config.wifi_password || "";
			intervalSlider.value = config.sensor_update_interval || 2;
			intervalDisplay.textContent = intervalSlider.value + "s";
			document.getElementById("display-enabled-config").checked = config.display_enabled ?? true;
		})		
        .catch(err => console.error("Error loading config:", err));

	document.getElementById("configForm").addEventListener("submit", async (e) => {
		e.preventDefault(); // prepreči reload
	
		const config = {
			wifi_ssid: document.getElementById("wifi-ssid-config").value,
			wifi_password: document.getElementById("wifi-password-config").value,
			sensor_update_interval: parseInt(document.getElementById("sensor-update-interval-config").value, 10),
			display_enabled: document.getElementById("display-enabled-config").checked
		};
	
		try {
			const response = await fetch("/save_json_data", {
				method: "POST",
				headers: {
					"Content-Type": "application/json"
				},
				body: JSON.stringify(config)
			});
	
			if (response.ok) {
				alert("Configuration saved!");
			} else {
				alert("Failed to save configuration.");
			}
		} catch (err) {
			console.error(err);
			alert("Error sending configuration.");
		}
	});		
});
