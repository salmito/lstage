--Chaining two parallel stages
local lstage=require'lstage'

if lstage.pool:size() < lstage:cpus() then
  lstage.pool:add(lstage:cpus()-1>1 and lstage:cpus()-1 or 1)
end

local chain=function(p1,p2,reduce) 
	local c1=lstage.channel()
	local c2=lstage.channel()
	local f1=lstage.event.decode(p1:getenv())
	local f2=lstage.event.decode(p2:getenv())

	local s1=lstage.stage(function(...)
		c1:push(f1(...))
	end)

	local s2=lstage.stage(function(...)
		c2:push(f2(...))
	end)

	return lstage.stage(function(...)	
		s1:push(...)
		s2:push(...)
		return reduce(c1:get(),c2:get()) 
	end)
end

--transforms two stages into one function call
local function seq(s1,s2)
	local f1=lstage.event.decode(s1:getenv())
	local f2=lstage.event.decode(s2:getenv())

	return lstage.stage(function(...) 
		return f2(f1(...));
	end)
end

getmetatable(lstage.stage()).__mul=function(s1,s2)
    return chain(s1,s2,function(a,b) return a*b end)
end

getmetatable(lstage.stage()).__concat=seq

local c=lstage.channel()

local s1=lstage.stage(function() return 2 end)
local s2=lstage.stage(function() return 3 end)
local s3=lstage.stage(function() return 4 end)
local s4=lstage.stage(function(r) print('fatorial of 4 is',r) c:push() end)

s5=(s1*s2*s3)..s4
s5:push()
c:get()