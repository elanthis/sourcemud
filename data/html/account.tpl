<form action="/account" method="post">
	<table align="center">
		<tr><th colspan="2">Account Details</th></tr>
		<tr><td><label>Account ID:</label></td><td>{html {$account.id}}</td></tr>
		<tr><td><label for="acct_name">Your Name:</label></td><td><input type="text" name="acct_name" id="acct_name" size="30" value="{html {$account.name}}" /></td></tr>
		<tr><td><label for="acct_email">Email Address:</label></td><td><input type="text" name="acct_email" id="acct_email" size="30" value="{html {$account.email}}" /></td></tr>
		<tr><td colspan="2"><hr /></td></tr>
		<tr><td><label for="new_pass1">New Passphrase:</label></td><td><input type="password" name="new_pass1" id="new_pass1" size="20" /> (optional)</td></tr>
		<tr><td><label for="new_pass2">Retype Passphrase:</label></td><td><input type="password" name="new_pass2" id="new_pass2" size="20" /> (must match)</td></tr>
		<tr><td class="options" colspan="2"><input type="submit" name="action" value="Save Changes" /></td></tr>
	</table>
</form>
