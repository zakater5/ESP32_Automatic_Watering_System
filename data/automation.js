const automationRules = [];
const ruleForm = document.getElementById("ruleForm");
const rulesList = document.getElementById("rulesList");

function renderRules() {
    rulesList.innerHTML = "";
    automationRules.forEach(rule => {
        const li = document.createElement("li");
        const timeDesc = rule.timeRange ? ` between ${rule.timeRange.start} and ${rule.timeRange.end}` : "";
        const desc = document.createElement("span");
        desc.textContent = `If ${rule.sensor} ${rule.operator} ${rule.value}${timeDesc}, then ${rule.actionSelect}`;
        if (!rule.enabled) desc.style.opacity = "0.5";
        li.appendChild(desc);

        const toggle = document.createElement("input");
        toggle.type = "checkbox";
        toggle.checked = rule.enabled;
        toggle.addEventListener("change", () => {
            rule.enabled = toggle.checked;
            renderRules();
            saveRulesToESP();
        });
        li.appendChild(toggle);

        const delBtn = document.createElement("button");
        delBtn.textContent = "Delete";
        delBtn.addEventListener("click", () => {
            const index = automationRules.findIndex(r => r.id === rule.id);
            if (index !== -1) {
                automationRules.splice(index, 1);
                renderRules();
                saveRulesToESP();
            }
        });
        li.appendChild(delBtn);

        rulesList.appendChild(li);
    });
}

ruleForm.addEventListener("submit", (e) => {
	e.preventDefault();

	const sensor = document.getElementById("sensorSelect").value;
	const operator = document.getElementById("operatorSelect").value;
	const value = parseFloat(document.getElementById("valueInput").value);
	const timeStart = document.getElementById("timeStart").value;
	const timeEnd = document.getElementById("timeEnd").value;
	const actionSelect = document.getElementById("actionSelect").value;

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
		actionSelect,
		enabled: true,
	});

	renderRules();
	ruleForm.reset();
	saveRulesToESP();
});

// Initial empty render
renderRules();

function saveRulesToESP() {
    fetch("/save_rules", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify(automationRules)
    })
    .then(res => {
        if (!res.ok) throw new Error("Server error: " + res.status);
        return res.json();
    })    
    .then(data => {
        console.log("Rules saved to ESP:", data);
    })
    .catch(err => {
        console.error("Error saving rules:", err);
    });
}


fetch("/get_rules")
  .then(res => res.json())
  .then(data => {
    automationRules.length = 0;   // reset array
    automationRules.push(...data);
    renderRules();
});