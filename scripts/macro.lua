-- Source MUD
-- Copyright (C) 2009  Sean Middleditch
-- See the file COPYING for license details
-- http://www.sourcemud.org

-- support code for the macro facility
macro = {
	-- uppercase the first letter of text, for the ^ operator
	raise = function(str)
		return string.upper(string.sub(str, 1, 1)) .. string.sub(str, 2, -1)
	end,

	-- return a variable value by name
	lookup = function(name)
		return 'LOOKUP[' .. name .. ']'
	end,

	-- invoke a built-in function/value
	invoke = function(name, ...)
		-- look up the built-in
		local what = macro.builtins[name]
		-- evaluate function
		if type(what) == 'function' then
			return what(...)
		-- return static value
		elseif what ~= nil then
			return what
		-- error case
		else
			return '[[unknown function \'' .. name .. '\']]'
		end
	end,

	-- list of built-ins
	builtins = {
		bold = function(str) return "\027!C14!", str, "\027!C0!" end,
		version = VERSION,
		build = BUILD
	}
}
