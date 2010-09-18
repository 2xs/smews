// ==UserScript==
// @name           demo
// @namespace     http://diveintogreasemonkey.org/download/
// @include       http://www2.lifl.fr/POPS/Eng/People/*
// ==/UserScript==

var contactsHash = null;
var contactsBook = null;
var smewsContact;
var mouseX;
var mouseY;
var currentContact = null;
var timeout = null;

unsafeWindow.timeout = timeout;
unsafeWindow.moveMouse = moveMouse;
unsafeWindow.showInfos = showInfos;
unsafeWindow.hideInfos = hideInfos;

function moveMouse(e) {
	mouseX = e.pageX;
	mouseY = e.pageY;
}

function parseBody() {
	var bodyList = document.getElementsByTagName("body");
	if(bodyList.length > 0) {
		var body = bodyList[0];
		var regStr = "";
		for (i in contactsBook) {
			regStr += "|" + contactsBook[i].name + "";
		}
		var reg=new RegExp("([^\"])(" + regStr.slice(1) + ")([^\"])", "gi");
		if(regStr != "") {
			body.innerHTML = body.innerHTML.replace(reg,"$1$2<span style='font-size: 14pt;color:#F62' onmouseover='clearTimeout(timeout);timeout=null;showInfos(\"$2\")' onmouseout='if(!timeout) timeout = setTimeout(\"hideInfos()\",100);'>&nbsp;<img src=\"http://www.funcard7.pan/contactsBook/sc.png\" alt=\"*\"/></span>$3");
		}
		smewsContact = document.createElement("div");
		smewsContact.id = "SmewsContact";
		smewsContact.style.position = "absolute";
		smewsContact.style.backgroundColor = "#dde";
		smewsContact.style.padding = "6px";
		smewsContact.style.visibility = "hidden";
		smewsContact.style.border = "2px solid black";
		smewsContact.setAttribute("onMouseOut","if(!timeout) timeout = setTimeout('hideInfos()',200);");
		smewsContact.setAttribute("onMouseOver","clearTimeout(timeout);timeout=null;");
		body.appendChild(smewsContact);
		body.setAttribute("onMouseMove","moveMouse(event)");
	}
}

// function cometAlert(responseDetails) {
// 	if(responseDetails.status == 200) {
// 		window.location.reload();
// 	}
// }

function getContactBook(responseDetails) {
	if(responseDetails.status == 200) {
		contactsBook = eval('(' + responseDetails.responseText + ')');
		for(i in contactsBook) {
			contactsBook[i].name = unescape(contactsBook[i].name);
			contactsBook[i].email = unescape(contactsBook[i].email);
			contactsBook[i].phone = unescape(contactsBook[i].phone);
			contactsBook[i].company = unescape(contactsBook[i].company);
			contactsBook[i].address = unescape(contactsBook[i].address);
		}

		contactsHash = new Array();
		for(i in contactsBook) {
			contactsHash[contactsBook[i].name.toLowerCase()] = i;
		}
		parseBody();
// 		ajaxGet("http://192.168.0.2/contactsBook/cb_comet",cometAlert);
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

function showInfos(name) {
	if(currentContact != name) {
		currentContact = name;
		var i = contactsHash[name.toLowerCase()];
		infos = "<table onmouseover=\"clearTimeout(timeout);timeout=null;\">";
		infos += "<tr><td style=\"padding-right: 6px\" align=\"right\"><b>E-mail:</b></td><td><a href=\"mailto:" + contactsBook[i].email + "\">" + contactsBook[i].email + "</a><br/></td></tr>";
		infos += "<tr><td style=\"padding-right: 6px\" align=\"right\"><b>Tel.:</b></td><td>" + contactsBook[i].phone + "<br/></td></tr>";
		infos += "<tr><td style=\"padding-right: 6px\" align=\"right\"><b>Company:</b></td><td>" + contactsBook[i].company + "<br/></td></tr>";
		infos += "<tr><td style=\"padding-right: 6px\" align=\"right\"><b>Address:</b></td><td>" + contactsBook[i].address + "<br/></td></tr>";
		infos += "</table>";

		infos += '<iframe width="450" height="250" frameborder="0" scrolling="no" marginheight="0" marginwidth="0" src="http://www2.lifl.fr/~duquenno/gmap.php?address=' + contactsBook[i].address + '"></iframe>';
		infos += '<br/><small><a href="http://maps.google.com/?q='+ contactsBook[i].address + '" style="color:#0000FF;text-align:left">Agrandir le plan</a></small>'

		smewsContact.innerHTML = infos;

		var boxY;
		var relMouseY = mouseY - document.documentElement.scrollTop;
		var boxX;
		var relMouseX = mouseX - document.documentElement.scrollLeft;
		if(relMouseY + 180 > window.innerHeight)
			boxY = window.innerHeight - 380;
		else if(relMouseY < 208)
			boxY = 8;
		else
			boxY = relMouseY - 200;
		boxY += document.documentElement.scrollTop;

		if(relMouseX + 490 > window.innerWidth)
			boxX = mouseX - 475;
		else
			boxX = mouseX + 10;
		
		smewsContact.style.left = boxX + "px";
		smewsContact.style.top = boxY + "px";
		smewsContact.style.visibility = "visible";
	}
}

function hideInfos() {
	smewsContact.style.visibility = "hidden";
	currentContact = null;
}

ajaxGet("http://www.funcard7.pan/contactsBook/cb_get",getContactBook);
