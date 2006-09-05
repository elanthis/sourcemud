<p>Welcome to the AweMUD portal!</p>

{if $account}
<p>Welcome, {$account.name}!</p>
{else}
<p><a href="/login">Please Login</a></p>
{endif}
