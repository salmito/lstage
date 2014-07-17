--stages may be combined in interesting patterns
local lstage=require'lstage'

if lstage.pool:size() < lstage:cpus() then
  lstage.pool:add(lstage:cpus()-1)
end

local function par(s1,s2)
	return lstage.stage(function(...)
		s1:push(...)
		s2:push(...)
	end)
end

local c=lstage.channel()

local s1=lstage.stage(function(i) print('s1',i) c:push() end)
local s2=lstage.stage(function(i) print('s2',i) c:push() end)

getmetatable(lstage.stage()).__add=par

s3=s1+s2

s3:push(9)

c:get()
c:get() --2 events