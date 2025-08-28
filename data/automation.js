const automationRules = [];

const ruleForm = document.getElementById("ruleForm");
const rulesList = document.getElementById("rulesList");

function renderRules() {
	rulesList.innerHTML = "";
	automationRules.forEach(rule => {
		const li = document.createElement("li");

		// Display rule description
		const timeDesc = rule.timeRange ?
			` between ${rule.timeRange.start} and ${rule.timeRange.end}` :
			"";

		const desc = document.createElement("span");
		desc.textContent = `If ${rule.sensor} ${rule.operator} ${rule.value}${timeDesc}`;
		if (!rule.enabled) desc.style.opacity = "0.5";
		li.appendChild(desc);

		// Enable/disable toggle
		const toggle = document.createElement("input");
		toggle.type = "checkbox";
		toggle.checked = rule.enabled;
		toggle.addEventListener("change", () => {
			rule.enabled = toggle.checked;
			renderRules();
		});
		li.appendChild(toggle);

		// Delete button
		const delBtn = document.createElement("button");
		delBtn.textContent = "Delete";
		delBtn.addEventListener("click", () => {
			const index = automationRules.findIndex(r => r.id === rule.id);
			if (index !== -1) {
				automationRules.splice(index, 1);
				renderRules();
			}
		});
		li.appendChild(delBtn);

		rulesList.appendChild(li);
	});
}

ruleForm.addEventListener("submit", e => {
	e.preventDefault();

	const sensor = document.getElementById("sensorSelect").value;
	const operator = document.getElementById("operatorSelect").value;
	const value = parseFloat(document.getElementById("valueInput").value);
	const timeStart = document.getElementById("timeStart").value;
	const timeEnd = document.getElementById("timeEnd").value;

	const timeRange = timeStart && timeEnd ? {
		start: timeStart,
		end: timeEnd
	} : null;

	automationRules.push({
		id: Date.now(),
		sensor,
		operator,
		value,
		timeRange,
		action: () => {
			console.log(`Action triggered for rule on ${sensor}`);
			// Add actual action logic here
		},
		enabled: true,
	});

	renderRules();
	ruleForm.reset();
});

// Initial empty render
renderRules();

// Example evaluation function - integrate with your sensor data updates:
function evaluateAutomation(sensorData) {
	automationRules.forEach(rule => {
		if (!rule.enabled) return;

		const sensorValue = sensorData[rule.sensor];
		if (sensorValue === undefined) return;

		let conditionMet = false;
		switch (rule.operator) {
			case ">":
				conditionMet = sensorValue > rule.value;
				break;
			case "<":
				conditionMet = sensorValue < rule.value;
				break;
			case "=":
				conditionMet = sensorValue === rule.value;
				break;
		}

		const timeOk = rule.timeRange ? isWithinTimeRange(rule.timeRange.start, rule.timeRange.end) : true;

		if (conditionMet && timeOk) {
			rule.action();
		}
	});
}

// Time check helper (same as earlier)
function isWithinTimeRange(start, end) {
	const now = new Date();
	const startParts = start.split(":").map(Number);
	const endParts = end.split(":").map(Number);

	const startDate = new Date(now);
	startDate.setHours(startParts[0], startParts[1], 0, 0);

	const endDate = new Date(now);
	endDate.setHours(endParts[0], endParts[1], 0, 0);

	return now >= startDate && now <= endDate;
}