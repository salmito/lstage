local lstage=require'lstage'
local event=require'lstage.event'
local channel=require'lstage.channel'
local pool=require'lstage.pool'

local arg={...}


--usefull functions and stages
local function empty()
end

--set up extra metatables
local stage_mt=getmetatable(lstage.stage(empty))

local stage=lstage.stage

local chain=function(s1,s2,reduce) 
	local c1=channel.new()
	local c2=channel.new()
	local f1=event.decode(s1:getenv())
	local f2=event.decode(s2:getenv())

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


local function seq2(s1,s2)
	local f1=event.decode(s1:getenv())
	local f2=event.decode(s2:getenv())

	return lstage.stage(function(...) 
		return f2(f1(...));
	end)
end

local function par2(s1,s2)
	return lstage.stage(function(...)
		s1:push(...)
		s2:push(...)
	end)
end


if(type(stage_mt)=='table') then
	stage_mt.__concat=seq2
	stage_mt.__add=par2
end

local printf=function(prefix) return function(...) print(prefix,...) return ... end end

lstage.pool:add(lstage:cpus()-1>1 and lstage:cpus()-1 or 1)

--if called from command line, perform some tests
if #arg==0 then
	print(stage_mt,lstage.pool:size())
	s1=lstage.stage(printf('s1'))
	s2=lstage.stage(printf('s2'))
	s3=lstage.stage(printf('s3'))
	s4=lstage.stage(printf('s4'))
	s5=lstage.stage(printf('s5'))
	s6=lstage.stage(printf('s6'))
	--s1:push('hello world from s1')
	s3=s1..s2..s3..s4..s5..s6
	s4=s1+s2+s3+s4+s5+s6
	print(s1,s2,s3)
	--:push('event1')
	--:push('event2')
	
	t1=lstage.stage(function(i) return i end)
	t2=lstage.stage(function(i) return i^2 end)
	t3=chain(t1,t2,function(i1,i2) return i1+i2 end)
	t4=t3..(lstage.stage(printf('t')))
	t4:push(4)	


	event.sleep(1)
end

