var ajaxTarget = 'photoSample';
var svgWin;
var samples;
var pollInterval;
var inFlight;
var lastTime = new Date().getTime();

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

function sampleTimeout(){
	inFlight = 0;
	document.getElementById("loader").style.visibility = "hidden";
}

function sampleCallBack(status,text){
	inFlight = 0;
	setTimeout(function() { if(inFlight == 0) document.getElementById("loader").style.visibility = "hidden"; }, 10);
	if(status == 200 && text != "")
		newSample(text/10.24);
}

function newSample(val){
	newTime = new Date().getTime();
	chartDuration = 0;
	maxDuration = svgWin.chartDuration * 1000;
	toRemove = 0;
	firstPlotIsTrunked = 0;
	samples.push([newTime-lastTime,val]);
	lastTime = newTime;
	for(i=samples.length-1;i>=1;i--) {
		if(chartDuration < maxDuration) {
			chartDuration += samples[i][0];
			if(chartDuration > maxDuration) {
				overLap = chartDuration - maxDuration;
				interval = samples[i][0];
				newInterval = interval - overLap;
				plotVal = samples[i][1];
				prevVal = samples[i-1][1];
				newVal = (prevVal * newInterval + plotVal * (overLap)) / interval;
				samples[i-1][1] = newVal;
				samples[i][0] = newInterval;
				chartDuration = maxDuration;
				firstPlotIsTrunked = 1;
			}
		}
		if (chartDuration == maxDuration) {
			toRemove = i - 1;
			break;
		}
	}
	for(i=0;i<toRemove;i++)
		samples.shift();
	svgWin.drawSamples(firstPlotIsTrunked,samples);
}

function setPollInterval(arg){
	argVal = Number(arg);
	if(isNaN(argVal)) {
		argVal = lastPollInterval;
	}
	pollInterval = argVal;
	document.getElementById("htmlInterval").value = pollInterval;
}

function onTimer(){
	setTimeout(onTimer,pollInterval);
	lastPollInterval = pollInterval;
	if(inFlight == 0) {
		inFlight = 1;
		document.getElementById("loader").style.visibility = "visible";
		doAjax(ajaxTarget,sampleCallBack,1000,sampleTimeout);
	}
}

function init(){
	svgObject = document.getElementById("svgDoc0");
	svgWin = svgObject.contentDocument.defaultView;
	svgWin.draw('Brigthness');
	samples = new Array();
	setPollInterval(1000);
	inFlight = 0;
	onTimer();
}
