{if eq {$account} ''}
<form action="/login" method="post">
	<table align="center">
		<tr><th colspan="2">Login</th></tr>
		<tr>
			<td><label for="username">Account:</label></td>
			<td><input type="text" size="25" name="username" id="username" /></td>
		</tr>
		<tr>
			<td><label for="password">Passphrase:</label></td>
			<td><input type="password" size="25" name="password" id="password" /></td>
		</tr>
		<tr>
			<td colspan="2" class="options"><input type="submit" name="action" value="Login" /></td>
		</tr>
	</table>
</form>
{endif}
