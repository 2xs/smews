var applications = null;
var fields = ["name","size"];
var table = null;

function setCellText(cell,cellValue) {
	if(cellValue != "") {
		var aText = document.createTextNode(cellValue);
		cell.appendChild(aText);
	} else {
		cell.innerHTML = "&nbsp;";
	}
}

function addTableLine(i) {
	var tr = document.createElement("tr");
	tr.id = "application_" + i;
	table.appendChild(tr);
	for (j in fields) {
		var td = document.createElement("td");
		td.id = "td_" + i + "_" + j;
		var str = eval("applications[i]." + fields[j]);
		str = unescape(str);
		setCellText(td,str);
		tr.appendChild(td);
	}
}

function updateApplications() {
 	table = document.getElementById("applicationsTable");
	for(var i in applications) {
		addTableLine(i);
	}
}

function initFields(xhr) {
	if(xhr.readyState == 4 && xhr.status == 200) {
		applications = eval('(' + xhr.responseText + ')');
		updateApplications();
	}
}

function ajaxGet(url,callBack,arg) {
	var xhr;
	var chart = this;
	var xhrTimer;
	try{ xhr = new XMLHttpRequest(); }
	catch(e){ xhr = new ActiveXObject('Microsoft.XMLHTTP'); }
	xhr.onreadystatechange = function(){
		if(callBack) callBack(xhr,arg);
	};
	xhr.open("GET", url, true);
	xhr.send(null);
}

function init() {
	ajaxGet('ElfMonitor',initFields);
}
