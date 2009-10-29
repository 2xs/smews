var ajaxTarget = 'comet_cpu';

function init(){
	document.getElementById("loader").style.visibility = "hidden";
}

function ajaxCallback(status,text){
	document.getElementById("loader").style.visibility = "hidden";
	document.getElementById("launch").style.visibility = "visible";
	if(status == 200 && text != "") {
		text = parseInt(text/10.24);
		document.getElementById("alert").innerHTML="CPU Usage: " + text + " %";
	}
}

function run(){
	document.getElementById("loader").style.visibility = "visible";
	document.getElementById("launch").style.visibility = "hidden";
	document.getElementById("alert").innerHTML = "";
	threshold = Number(document.getElementById("htmlThres").value);
	if(isNaN(threshold))
		threshold = 50;
	threshold = parseInt(10.24 * threshold);
	
	doAjax(ajaxTarget+"?thres="+threshold,ajaxCallback,0,0);
}

function doAjax(ajaxTarget,ajaxCallBack,timeout,timeoutCallBack){
	var xhr;
	var xhrTimer;
	try{ xhr = new XMLHttpRequest(); }
	catch(e){ xhr = new ActiveXObject('Microsoft.XMLHTTP'); }
	xhr.onreadystatechange = function(){
		if(xhr.readyState == 4) {
			ajaxCallBack(xhr.status,xhr.responseText);
			if(timeout > 0)
				clearTimeout(xhrTimer);
		}
	};
	xhr.open("GET", ajaxTarget, true);
	if(timeout > 0)
		xhrTimer = setTimeout(function() { xhr.abort(); timeoutCallBack();}, timeout);
	xhr.send(null);
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
