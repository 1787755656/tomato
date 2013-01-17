<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2009 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2010-2011 Jean-Yves Avenard, jean-yves@avenard.org

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] VPN: Klient PPTP</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='/tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='/debug.js'></script>

<script type='text/javascript'>


//	<% nvram("pptp_client_enable,pptp_client_peerdns,pptp_client_mtuenable,pptp_client_mtu,pptp_client_mruenable,pptp_client_mru,pptp_client_nat,pptp_client_srvip,pptp_client_srvsub,pptp_client_srvsubmsk,pptp_client_username,pptp_client_passwd,pptp_client_mppeopt,pptp_client_crypt,pptp_client_custom,pptp_client_dfltroute,pptp_client_stateless"); %>

pptpup = parseInt('<% psup("pptpclient"); %>');

var changed = 0;

function toggle(service, isup)
{
	if (changed) {
		if (!confirm("Niezapisane zmiany zostaną utracone. Kontynuować mimo to?")) return;
	}
	E('_' + service + '_button').disabled = true;
	form.submitHidden('/service.cgi', {
		_redirect: 'vpn-pptp.asp',
		_service: service + (isup ? '-stop' : '-start')
	});
}

function verifyFields(focused, quiet)
{
    var ret = 1;

	elem.display(PR('_pptp_client_srvsub'), PR('_pptp_client_srvsubmsk'), !E('_f_pptp_client_dfltroute').checked);

	var f = E('_pptp_client_mtuenable').value == '0';
	if (f) {
		E('_pptp_client_mtu').value = '1450';
	}
	E('_pptp_client_mtu').disabled = f;
	f = E('_pptp_client_mruenable').value == '0';
	if (f) {
		E('_pptp_client_mru').value = '1450';
	}
	E('_pptp_client_mru').disabled = f;

	if (!v_range('_pptp_client_mtu', quiet, 576, 1500)) ret = 0;
	if (!v_range('_pptp_client_mru', quiet, 576, 1500)) ret = 0;
	if (!v_ip('_pptp_client_srvip', true) && !v_domain('_pptp_client_srvip', true)) { ferror.set(E('_pptp_client_srvip'), "Błędny adres serwera.", quiet); ret = 0; }
	if (!E('_f_pptp_client_dfltroute').checked && !v_ip('_pptp_client_srvsub', true)) { ferror.set(E('_pptp_client_srvsub'), "Błędny adres podsieci.", quiet); ret = 0; }
	if (!E('_f_pptp_client_dfltroute').checked && !v_ip('_pptp_client_srvsubmsk', true)) { ferror.set(E('_pptp_client_srvsubmsk'), "Błędny adres podsieci.", quiet); ret = 0; }
	
    changed |= ret;
	return ret;
}

function save()
{
	if (!verifyFields(null, false)) return;

	var fom = E('_fom');

    E('pptp_client_enable').value = E('_f_pptp_client_enable').checked ? 1 : 0;
    E('pptp_client_nat').value = E('_f_pptp_client_nat').checked ? 1 : 0;
    E('pptp_client_dfltroute').value = E('_f_pptp_client_dfltroute').checked ? 1 : 0;
    E('pptp_client_stateless').value = E('_f_pptp_client_stateless').checked ? 1 : 0;

	form.submit(fom, 1);

	changed = 0;
}
</script>

<style type='text/css'>
textarea {
	width: 98%;
	height: 10em;
}
</style>

</head>
<body>
<form id='_fom' method='post' action='/tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Wersja <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='vpn-pptp.asp'>
<input type='hidden' name='_service' value=''>
<input type='hidden' name='_nextwait' value='5'>

<input type='hidden' id='pptp_client_enable' name='pptp_client_enable'>
<input type='hidden' id='pptp_client_peerdns' name='pptp_client_peerdns'>
<input type='hidden' id='pptp_client_nat' name='pptp_client_nat'>
<input type='hidden' id='pptp_client_dfltroute' name='pptp_client_dfltroute'>
<input type='hidden' id='pptp_client_stateless' name='pptp_client_stateless'>

<div class='section-title'>Konfiguracja Klienta PPTP</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Startuj z WAN', name: 'f_pptp_client_enable', type: 'checkbox', value: nvram.pptp_client_enable != 0 },
    { title: 'Adres serwera', name: 'pptp_client_srvip', type: 'text', size: 17, value: nvram.pptp_client_srvip },
    { title: 'Użytkownik: ', name: 'pptp_client_username', type: 'text', maxlen: 50, size: 54, value: nvram.pptp_client_username },
    { title: 'Hasło: ', name: 'pptp_client_passwd', type: 'password', maxlen: 50, size: 54, value: nvram.pptp_client_passwd },
	{ title: 'Szyfrowanie', name: 'pptp_client_crypt', type: 'select', value: nvram.pptp_client_crypt,
        options: [['0', 'Automatycznie'],['1', 'Brak'],['2','Maksymalne (tylko 128 bitów)'],['3','Wymagane (40 lub 128 bitów)']] },
	{ title: 'Stateless MPPE connection', name: 'f_pptp_client_stateless', type: 'checkbox', value: nvram.pptp_client_stateless != 0 },
	{ title: 'Akceptuj konfigurację DNS', name: 'pptp_client_peerdns', type: 'select', options: [[0, 'Wyłączone'],[1, 'Tak'],[2, 'Wyjątkowo']], value: nvram.pptp_client_peerdns },
	{ title: 'Przekieruj ruch internetowy', name: 'f_pptp_client_dfltroute', type: 'checkbox', value: nvram.pptp_client_dfltroute != 0 },
    { title: 'Zdalna podsieć / maska podsieci', multi: [
        { name: 'pptp_client_srvsub', type: 'text', maxlen: 15, size: 17, value: nvram.pptp_client_srvsub },
        { name: 'pptp_client_srvsubmsk', type: 'text', maxlen: 15, size: 17, prefix: ' /&nbsp', value: nvram.pptp_client_srvsubmsk } ] },
	{ title: 'Utwórz NAT na tunel', name: 'f_pptp_client_nat', type: 'checkbox', value: nvram.pptp_client_nat != 0 },
	{ title: 'MTU', multi: [
		{ name: 'pptp_client_mtuenable', type: 'select', options: [['0', 'Domyślne'],['1','Własne']], value: nvram.pptp_client_mtuenable },
		{ name: 'pptp_client_mtu', type: 'text', maxlen: 4, size: 6, value: nvram.pptp_client_mtu } ] },
	{ title: 'MRU', multi: [
		{ name: 'pptp_client_mruenable', type: 'select', options: [['0', 'Domyślne'],['1','Własne']], value: nvram.pptp_client_mruenable },
		{ name: 'pptp_client_mru', type: 'text', maxlen: 4, size: 6, value: nvram.pptp_client_mru } ] },
    { title: 'Własna konfiguracja', name: 'pptp_client_custom', type: 'textarea', value: nvram.pptp_client_custom }
]);
    W('<input type="button" value="' + (pptpup ? 'Stop' : 'Start') + ' teraz" onclick="toggle(\'pptpclient\', pptpup)" id="_pptpclient_button">');
</script>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Zapisz' id='save-button' onclick='save()'>
	<input type='button' value='Anuluj' id='cancel-button' onclick='reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
