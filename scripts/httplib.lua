-- Source MUD, HTTP Library for Lua
--
-- Includes routines for Lua scripts that must work with HTTP requests,
-- include formatting out output, template language expansion, and more.
--
-- Copyright (C) 2008 Sean Middleditch
-- See COPYING for license details

module(..., package.seeall)

-- HTML escape
function htmlescape(str)
	return str:gsub('<', '&lt;'):gsub('>', '&gt;'):gsub('"', '&quot;'):gsub('&', '&amp;')
end

-- Load a template file
function template(path)
	-- load source
	local f = assert(io.open(path))
	local src = f:read('*a')
	f:close()

	-- create output string
	function mkprint(text, s, e)
			text = string.sub(text, s, e or -1)
			text = text:gsub('([\\\n\'])', '\\%1')
			return " print('"..text.."'); "
	end

	-- parse source
	local rs = {}
	local s = 1
	while true do
		-- find next chunk of code
		local ip, fp, target, expr, text = src:find('<%[ \t]*(=?)(.-)%>', s)
		if not ip then break end

		-- append non-code chunk to output
		if start < ip - 1 then
			table.insert(rs, mkprint(src, s, ip - 1))
		end

		-- append code chunk to output
		if expr == '=' then
			table.insert(rs, ' print('..text..'); ')
		else
			table.insert(rs, ' '..text..' ')
		end
	end
	table.insert(rs, mkprint(src, s))

	-- compile source
	local chunk, err = loadstring(table.concat(rs))
	if not chunk then error(err, 1) end

	-- bind to custom environment
	local env = getfenv(chunk)
	local mt = {}
	local vars = {}
	mt.__index = env
	setmetatable(vars, mt)
	setfenv(chunk, vars)

	-- create template object
	local tpl = {}
	tpl.vars = vars
	tpl.chunk = chunk

	-- exec() method
	tpl.exec = function(self)
		self.chunk()
	end

	-- set() method
	tpl.set = function(self, name, value)
		self.vars[name] = value
	end

	-- all done
	return tpl
end
