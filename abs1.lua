--stages may be combined in interesting patterns
local lstage=require'lstage'

if lstage.pool:size() < lstage:cpus() then
  lstage.pool:add(lstage:cpus()-1>1 and lstage:cpus()-1 or 1)
end

local c=lstage.channel()

--transforms two stages into one function call
local function seq(s1,s2)
	local f1=lstage.event.decode(s1:getenv())
	local f2=lstage.event.decode(s2:getenv())

	return lstage.stage(function(...) 
		return f2(f1(...));
	end)
end

local s1=lstage.stage(function() return 's1' end)
local s2=lstage.stage(function(s) print(s,'s2') c:push() end)

getmetatable(lstage.stage()).__concat=seq

s3=s1..s2
s3:push()
c:get()