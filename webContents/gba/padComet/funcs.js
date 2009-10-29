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

function ajaxCallback(ajaxRet){
	document.getElementById("sample").innerHTML="Sample: " + ajaxRet;
	if(ajaxRet != "") {
		setA(ajaxRet & 1);
		setB((ajaxRet & 2) >> 1);
		setRight((ajaxRet & 4) >> 2);
		setLeft((ajaxRet & 8) >> 3);
		setUp((ajaxRet & 16) >> 4);
		setDown((ajaxRet & 32) >> 5);
	}
	run()
}

function run(){
	ajaxGet("pushPad");
}

function ajaxGet(url) {
	var xhr;
	try{ xhr = new XMLHttpRequest(); }
	catch(e){ xhr = new ActiveXObject('Microsoft.XMLHTTP'); }

	xhr.onreadystatechange = function(){
		if(xhr.readyState  == 4){
			if(ajaxRet=xhr.status==200)
				ajaxCallback(xhr.responseText);
		}
	};
	xhr.open("GET", url, true);
	xhr.send(null);
}
