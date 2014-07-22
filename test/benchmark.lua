local lstage=require "lstage"

--lstage.pool:add(lstage.cpus())

local pool=lstage.pool
pool:add(th or 0)

local n=n or 100
local it=it or 100000
local thn=lstage.now()
local thn_clk=os.clock()
local now=lstage.now

local finish=lstage.channel()
local test=lstage.channel()

local s2=lstage.stage(function()
	i=(i or 0) +1
	if i==n then
		finish:push('end')
	end 
end)

local channel_test=lstage.stage(function()
	for i=1,n do
		local g=0
		for y=1,it do
	    	   g=(g+y)
		end
		i=(i or 0) +1
		test:get()
		--print(i)
		if i==n then
			finish:push('end')
			return
		end 
	end
end):push()


local function f(data)
  local n=0
  for i=1,it do
    n=(n+i)
  end
  s2=s2 and s2:push(data)
end

local function f(data)
  local n=0
  for i=1,it do
    n=(n+i)
  end
  s2=s2 and s2:push(data)
end


local s={}

--local out=lstage.channel()
local s1=lstage.stage(f,n)
for i=1,n do
	s1:push(1)
end

print("initiated stage queue test",now()-thn)
thn=now()
finish:get()
print("end stage",pool:size(),n,it,now()-thn,os.clock()-thn_clk)

print("initiated channel queue test",now()-thn)
thn=lstage.now()
thn_clk=os.clock()
for i=0,n do
  test:push(1)
end
finish:get()
print("end channel",pool:size(),n,it,now()-thn,os.clock()-thn_clk)
print("initiated serial test",now()-thn)
thn=lstage.now()
thn_clk=os.clock()
for i=0,n do
  f(1)
end
print("end serial",now()-thn,os.clock()-thn_clk)

