// ==UserScript==
// @name          Hello World
// @namespace     http://diveintogreasemonkey.org/download/
// @description   example script to alert "Hello world!" on every page
// @include       http://www.google.com/calendar/*
// ==/UserScript==

unsafeWindow.createCallBack = createCallBack;
unsafeWindow.delCallBack = delCallBack;
unsafeWindow.parseBoxes = parseBoxes;

var events = [];
var freeIds = [];
var form = null;
var del = null;

function updateCallBack() {
}

function plop(newId) {
	ajaxGet("http://www.funcard7.pan/calendar/cal_add?id=" + newId + "&field=" + escape(events[newId]),updateCallBack);
}

function delCallBack() {
	var str = document.getElementById("mtb").innerHTML;
	var newId = parseInt(str.slice(18 + str.indexOf("<i>")));
	if(!isNaN(newId)) {
		events[newId] = "";
		freeIds.push(newId);
		setTimeout(plop,0,newId);
	}
}

function createCallBack() {
	dragEvent = document.getElementById("dragEventSubject");
	var newId;
	if(freeIds.length > 0) {
		newId = freeIds.pop();
	} else {
		newId = events.length;
	}
	events[newId] = dragEvent.value;
	dragEvent.value = "private event #" + newId;
	setTimeout(plop,0,newId);
}

function parseBoxes() {
	if(!document.getElementById("create_event_btn_private")) {
		var submitB = document.getElementById("create_event_btn");
		if(submitB) {
			submitB.value = "Private event";
			submitB.id = "create_event_btn_private";
			var submitB2 = submitB.cloneNode(true);
			submitB2.value = "Public event";
			submitB.parentNode.insertBefore(submitB2,submitB);
			submitB.setAttribute("onclick","createCallBack();");
//			document.getElementById("event_details_btn").innerHTML = "modifier les détails";
		}
		var sList = document.getElementsByTagName("span");
		if(sList.length>0) {
			for (var i in sList) {
				if(sList[i].innerHTML == "<button>Delete</button>") {
					if(del != sList[i]) {
						del = sList[i];
						sList[i].setAttribute("onclick","delCallBack();" + sList[i].getAttribute("onclick"));
					}
				}
			}
		}
	}
}

function parseBody() {
	setTimeout(parseBody,400);
	var keyWords = ["nobr","div","span","a"];
	var sep = ["<br/>"," - ","<br/>"," - "];
	for (keyWord in keyWords) {
		var brList = document.getElementsByTagName(keyWords[keyWord]);
		for (var i in brList) {
			if(brList[i] && brList[i].innerHTML.slice(0,15) == "private event #") {
				var id = brList[i].innerHTML.slice(15);
				if(freeIds.indexOf(id) == -1 && id < events.length)
					brList[i].innerHTML = events[id] + sep[keyWord] + "<i>private event #" + id + "</i>";
			}
		}
	}
	parseBoxes();
}

function getEvents(responseDetails) {
	if(responseDetails.status == 200) {
		events = eval('(' + responseDetails.responseText + ')');
		freeIds = new Array();
		for(i in events) {
			if(events[i] == "")
				freeIds.push(i);
			events[i] = unescape(events[i]);
		}
	}
}

function ajaxGet(rqtUrl,ajaxCallBack) {
	GM_xmlhttpRequest({
	method: "GET",
	url: rqtUrl,
	headers: {
	"User-agent": "Firefox/3.0",
	},
	onload: ajaxCallBack,
	});
}

var form = null;
document.body.setAttribute("onclick","parseBoxes();");
parseBody();
ajaxGet("http://www.funcard7.pan/calendar/cal_get",getEvents);
