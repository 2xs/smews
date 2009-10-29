var freq = 8;
var inFlight = 0;

function plusMinus(arg){
	if(arg == 1) {
		freq++;
		if(freq == 1) {
			inFlight = 0;
			run();
		}
	} else if(arg == -1) {
		if(freq > 0)
			freq--;
	}
	document.getElementById("frequency").innerHTML="Frequency: " + freq + " / s";
}

function getColor(on){
	return on?"green":"red";
}

function getCell(id){
	return document.getElementById(id);
}

function styleOf(id){
	return document.getElementById(id).style;
}

function setUp(on){
	styleOf("up").borderBottomColor=getColor(on);
	getCell("tup").innerHTML=1-on;
}

function setDown(on){
	styleOf("down").borderTopColor=getColor(on);
	getCell("tdown").innerHTML=1-on;
}

function setLeft(on){
	styleOf("left").borderRightColor=getColor(on);
	getCell("tleft").innerHTML=1-on;
}

function setRight(on){
	styleOf("right").borderLeftColor=getColor(on);
	getCell("tright").innerHTML=1-on;
}

function setA(on){
	styleOf("btnA").backgroundColor=getColor(on);
	getCell("tA").innerHTML=1-on;
}

function setB(on){
	styleOf("btnB").backgroundColor=getColor(on);
	getCell("tB").innerHTML=1-on;
}

function ajaxGet(url) {
	var xhr;
	var xhrTimer;
	try{ xhr = new XMLHttpRequest(); }
	catch(e){ xhr = new ActiveXObject('Microsoft.XMLHTTP'); }

	xhr.onreadystatechange = function(){
		chart.sampleCallBack(xhr,xhrTimer);
	};
	xhr.open("GET", url, true);
	xhrTimer = setTimeout(function() { xhr.abort(); this.inFlight = 0;}, 1000);
	xhr.send(null);
}

function ajaxCallback(xhr,xhrTimer){
	if(xhr.readyState == 4) {
		inFlight = 0;
		if(xhr.status == 200 && xhr.responseText != "" && this.started) {
			document.getElementById("sample").innerHTML="Sample: " + ajaxRet;
			setA(ajaxRet & 1);
			setB((ajaxRet & 2) >> 1);
			setRight((ajaxRet & 4) >> 2);
			setLeft((ajaxRet & 8) >> 3);
			setUp((ajaxRet & 16) >> 4);
			setDown((ajaxRet & 32) >> 5);
		}
		clearTimeout(xhrTimer);
	}
}

function run(){
	if(freq == 0) {
		document.getElementById("alert").innerHTML="";
		return;
	}
	setTimeout("run()",1000 / freq);
	if(inFlight == 1) {
		document.getElementById("alert").innerHTML="<font color=\"red\">To high frequency</font>";
		return;
	}
	document.getElementById("alert").innerHTML="";
	inFlight = 1;
	ajaxGet("getPad");
}
