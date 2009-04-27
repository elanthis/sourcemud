<?xml version="1.0"?>
<html>
	<head>
		<title>Source MUD Portal</title>
		<link rel="stylesheet" href="/css/common.css" type="text/css" />
	</head>
	<body>
		<div id="title">Source MUD Portal</div>

		<div id="menu">
		{if $account}
			<b>{$account.name}</b>
			| <a href="/">Home</a>
			| <a href="/account">Account</a>
			| <a href="/logout">Logout</a>
		{else}
			<a href="/login">Login</a>
			| <a href="/">Home</a>
		{endif}
			| <a href="/stats">Stats</a>
		</div>

		<div id="body">
			{if $msg}<p id="msg">{$msg}</p>{endif}
