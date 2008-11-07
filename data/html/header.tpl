<?xml version="1.0"?>
<html>
	<head>
		<title>Source MUD Portal</title>
		<style type="text/css">
			td \{
				text-align: left;
				vertical-align: top;
			}
			th \{
				text-align: left;
				vertical-align: top;
				background: #666;
				font-weight: bold;
				color: #fff;
				padding: 2px;
			}
			td.options \{
				text-align: right;
			}
			a \{
				color: #00C;
			}
			#title \{
				background: #eee;
				text-align: center;
				padding: 8px;
				-moz-border-radius: 6px;
				font-size: 150%;
				font-weight: bold;
			}
			#menu \{
				padding: 4px;
				border-bottom: 1px solid #eee;
			}
			#footer \{
				padding: 4px;
				text-align: center;
				border-top: 1px solid #eee;
				color: #ccc;
				font-size: small;
			}
			#body \{
				margin: 12px;
			}
			#msg \{
				text-align: center;
				font-weight: bold;
				color: #c00;
			}
		</style>
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
