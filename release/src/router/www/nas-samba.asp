﻿<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Samba Server - !!TB

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] NAS: Współdzielenie plików</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style tyle='text/css'>
#ss-grid {
	width: 99%;
}
#ss-grid .co1, #ss-grid .co2, #ss-grid .co3 {
	width: 25%;
}
#ss-grid .co4 {
	width: 16%;
}
#ss-grid .co5 {
	width: 9%;
}
</style>
<style type='text/css'>
textarea {
	width: 98%;
	height: 6em;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("smbd_enable,smbd_user,smbd_passwd,smbd_wgroup,smbd_cpage,smbd_custom,smbd_master,smbd_wins,smbd_shares,smbd_autoshare,wan_wins"); %>

var ssg = new TomatoGrid();

ssg.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

ssg.existName = function(name)
{
	return this.exist(0, name);
}

ssg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r = cmpText(da[col], db[col]);
	return this.sortAscending ? r : -r;
}

ssg.dataToView = function(data) {
	return [data[0], data[1], data[2], ['Tylko odczyt', 'Odczyt/Zapis'][data[3]], ['Nie', 'Tak'][data[4]]];
}

ssg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return [f[0].value, f[1].value, f[2].value, f[3].value, f[4].value];
}

ssg.verifyFields = function(row, quiet)
{
	var f, s;

	f = fields.getAll(row);

	s = f[0].value.trim().replace(/\s+/g, ' ');
	if (s.length > 0) {
		if (s.search(/^[ a-zA-Z0-9_\-\$]+$/) == -1) {
			ferror.set(f[0], 'Błędna nazwa udziału. Tylko znaki "$ A-Z 0-9 - _" i spacja są dozwolone.', quiet);
			return 0;
		}
		if (this.existName(s)) {
			ferror.set(f[0], 'Nazwa udziału już istnieje.', quiet);
			return 0;
		}
		f[0].value = s;
	}
	else {
		ferror.set(f[0], 'Pusta nazwa udziału nie jest dozwolona.', quiet);
		return 0;
	}

	if (!v_nodelim(f[1], quiet, 'Katalog', 1) || !v_path(f[1], quiet, 1)) return 0;
	if (!v_nodelim(f[2], quiet, 'Opis', 1)) return 0;

	return 1;
}

ssg.resetNewEditor = function()
{
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	f[0].value = '';
	f[1].value = '';
	f[2].value = '';
	f[3].selectedIndex = 0;
	f[4].selectedIndex = 0;
}

ssg.setup = function()
{
	this.init('ss-grid', 'sort', 50, [
		{ type: 'text', maxlen: 32 },
		{ type: 'text', maxlen: 256 },
		{ type: 'text', maxlen: 64 },
		{ type: 'select', options: [[0, 'Tylko odczyt'],[1, 'Odczyt / Zapis']] },
		{ type: 'select', options: [[0, 'Nie'],[1, 'Tak']] }
	]);
	this.headerSet(['Nazwa udziału', 'Katalog', 'Opis', 'Poziom dostępu', 'Ukryty']);

	var s = nvram.smbd_shares.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 5) {
			this.insertData(-1, t);
		}
	}

	this.sort(0);
	this.showNewEditor();
	this.resetNewEditor();
}

function verifyFields(focused, quiet)
{
	var a, b;

	a = E('_smbd_enable').value;

	elem.display(PR('_smbd_user'), PR('_smbd_passwd'), (a == 2));

	E('_smbd_wgroup').disabled = (a == 0);
	E('_smbd_cpage').disabled = (a == 0);
	E('_smbd_custom').disabled = (a == 0);
	E('_smbd_autoshare').disabled = (a == 0);
	E('_f_smbd_master').disabled = (a == 0);
	E('_f_smbd_wins').disabled = (a == 0 || (nvram.wan_wins != '' && nvram.wan_wins != '0.0.0.0'));

	if (a != 0 && !v_length('_smbd_custom', quiet, 0, 2048)) return 0;

	if (a == 2) {
		if (!v_length('_smbd_user', quiet, 1)) return 0;
		if (!v_length('_smbd_passwd', quiet, 1)) return 0;

		b = E('_smbd_user');
		if (b.value == 'root') {
			ferror.set(b, 'Nazwa użytkownika \"root\" nie jest dozwolona.', quiet);
			return 0;
		}
		ferror.clear(b);
	}

	return 1;
}

function save()
{
	if (ssg.isEditing()) return;
	if (!verifyFields(null, 0)) return;

	var fom = E('_fom');

	var data = ssg.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i) r.push(data[i].join('<'));
	fom.smbd_shares.value = r.join('>');
	fom.smbd_master.value = E('_f_smbd_master').checked ? 1 : 0;
	if (nvram.wan_wins == '' || nvram.wan_wins == '0.0.0.0')
		fom.smbd_wins.value = E('_f_smbd_wins').checked ? 1 : 0;
	else
		fom.smbd_wins.value = nvram.smbd_wins;

	form.submit(fom, 1);
}
</script>

</head>
<body>
<form id='_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Wersja <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='nas-samba.asp'>
<input type='hidden' name='_service' value='samba-restart'>

<input type='hidden' name='smbd_master'>
<input type='hidden' name='smbd_wins'>
<input type='hidden' name='smbd_shares'>

<div class='section-title'>Samba - Współdzielenie plików</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Włącz współdzielenie plików', name: 'smbd_enable', type: 'select',
		options: [['0', 'Nie'],['1', 'Tak, bez autoryzacji'],['2', 'Tak, autoryzacja wymagana']],
		value: nvram.smbd_enable },
	{ title: 'Nazwa użytkownika', indent: 2, name: 'smbd_user', type: 'text', maxlen: 50, size: 32,
		value: nvram.smbd_user },
	{ title: 'Hasło', indent: 2, name: 'smbd_passwd', type: 'password', maxlen: 50, size: 32, peekaboo: 1,
		value: nvram.smbd_passwd },
	null,
	{ title: 'Grupa robocza', name: 'smbd_wgroup', type: 'text', maxlen: 20, size: 32,
		value: nvram.smbd_wgroup },
	{ title: 'Strona kodowa klientów', name: 'smbd_cpage', type: 'select',
		options: [['', 'Nieokreślona'],['437', '437 (USA, Kanada)'],['850', '850 (Europa Zachodnia)'],['852', '852 (Europa Centralna/Wschodnia)'],['866', '866 (Cyrylica / Rosyjski)']
/* LINUX26-BEGIN */
		,['932', '932 (Japoński)'],['936', '936 (Chiński uproszczony)'],['949', '949 (Koreański)'],['950', '950 (Chiński tradycyjny / Big5)']
/* LINUX26-END */
		],
		suffix: ' <small> (dla Windows: uruchom cmd.exe i wpisz chcp żeby sprawdzić stronę kodową)</small>',
		value: nvram.smbd_cpage },
	{ title: 'Samba<br>Własna konfiguracja', name: 'smbd_custom', type: 'textarea', value: nvram.smbd_custom },
	{ title: 'Auto-udostępnianie wszystkich partycji', name: 'smbd_autoshare', type: 'select',
		options: [['0', 'Wyłączone'],['1', 'Tylko odczyt'],['2', 'Odczyt / Zapis'],['3', 'Ukryty Odczyt / Zapis']],
		value: nvram.smbd_autoshare },
	{ title: 'Opcje', multi: [
		{ suffix: '&nbsp; Master Browser &nbsp;&nbsp;&nbsp;', name: 'f_smbd_master', type: 'checkbox', value: nvram.smbd_master == 1 },
		{ suffix: '&nbsp; Serwer WINS &nbsp;',	name: 'f_smbd_wins', type: 'checkbox', value: (nvram.smbd_wins == 1) && (nvram.wan_wins == '' || nvram.wan_wins == '0.0.0.0') }
	] }
]);
</script>
</div>
<br>

<div class='section-title'>Lista udziałów sieciowych</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='ss-grid'></table>
	<script type='text/javascript'>ssg.setup();</script>
<br>
<small>Jeżeli nie zostaną zdefiniowane żadne udziały i wyłączone jest auto-udostępnianie, katalog <i>/mnt</i> zostanie udostępniony tylko do odczytu.</small>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Zapisz' id='save-button' onclick='save()'>
	<input type='button' value='Anuluj' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
