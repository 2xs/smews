var contactsBook = null;
var currSel = null;
var fields = ["name","email","phone","company","address"];
var table = null;

function add(){
	contactsBook.push({name: "",email: "",phone: "",company: "",address: ""});
	addTableLine(contactsBook.length-1);
	ajaxGet("cb_add?id=" + (contactsBook.length-1) + "&num=0&field=",updateField,{i:contactsBook.length-1,j:0});
}

function del(){
	contactsBook.pop();
	var tr = document.getElementById("contact_" + contactsBook.length);
	table.removeChild(tr);
	ajaxGet("cb_add",null,null);
}

function updateField(xhr,cell) {
	if(xhr.readyState == 4 && xhr.status == 200) {
		var cell = document.getElementById("td_" + cell.i + "_" + cell.j);
		cell.className = "idleCell";
		if(xhr.responseText != "")
			cell.innerHTML = unescape(xhr.responseText);
		else
			cell.innerHTML = "&nbsp;";
		addC = false;
	}
}

function setCellText(cell,cellValue) {
	if(cellValue != "") {
		var aText = document.createTextNode(cellValue);
		cell.appendChild(aText);
	} else {
		cell.innerHTML = "&nbsp;";
	}
}

function deSelectCell() {
	if(currSel) {
		var cell = document.getElementById("td_" + currSel.i + "_" + currSel.j);
		var cellValue = cell.childNodes[0].value;
		cell.removeChild((cell.childNodes[0]));
		setCellText(cell,cellValue);
		if(cellValue!=currSel.lastVal) {
			cell.className = "busyCell";
			ajaxGet("cb_add?id=" + currSel.i + "&num=" + currSel.j + "&field=" + escape(cellValue),updateField,currSel);
		}
		currSel = null;
	}
}

function selectCell(i,j) {
	if(!currSel || currSel.i != i|| currSel.j != j) {
		deSelectCell();
		var cell = document.getElementById("td_" + i + "_" + j);
		var anInput = document.createElement("input");
// 		anInput.className = "fieldInput";
		anInput.style.width = "97%";
		anInput.type = "text";
		anInput.name = fields[j];
		if(cell.innerHTML != "&nbsp;")
			anInput.value = cell.innerHTML;
		else
			anInput.value = "";
		if(cell.hasChildNodes())
			cell.removeChild((cell.childNodes[0]));
		cell.appendChild(anInput);
		anInput.select();
		currSel = {i: i, j: j, lastVal: anInput.value};
	}
}

function keyPressed(event) {
	var key = event.keyCode;
	if(key==13) {
		deSelectCell();
		event.preventDefault();
	} else if(key==9) {
		var i = currSel.i;
		var j = currSel.j;
		if(++j>=fields.length) {
			j = 0;
			if(++i>=contactsBook.length)
				i = 0;
		}
		selectCell(i,j);
		event.preventDefault();
	}
}

function addTableLine(i) {
	var tr = document.createElement("tr");
	tr.id = "contact_" + i;
	table.appendChild(tr);
	for (j in fields) {
		var td = document.createElement("td");
		td.id = "td_" + i + "_" + j;
		td.setAttribute("onClick","selectCell(" + i + "," + j + ");");
		td.setAttribute("onKeyDown","keyPressed(event);");
		var str = eval("contactsBook[i]." + fields[j]);
		str = unescape(str);
		setCellText(td,str);
		tr.appendChild(td);
	}
}

function updateContacts() {
 	table = document.getElementById("contactsTable");
	for(var i in contactsBook) {
		addTableLine(i);
	}
}

function initFields(xhr) {
	if(xhr.readyState == 4 && xhr.status == 200) {
		contactsBook = eval('(' + xhr.responseText + ')');
		updateContacts();
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
	ajaxGet('cb_get',initFields);
}
