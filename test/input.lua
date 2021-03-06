local lstage=require'lstage'

local it=tonumber((...)) or 10000

local bench={
	start=function(self)
		self.t=lstage.now()
		self.c=os.clock()
	end,
	stop=function(self)
		return lstage.now()-self.t,os.clock()-self.c
	end,
	clock=function(self,str)
		local t,c=self:stop()
		print(string.format('%s %.3fs real time %.3fs cpu time',str,t,c))
		self:start()
	end
}

bench:start()

local function f(i)
	local j=0
	if i%1000 == 0 then print(i) end
	for i=1,10000 do j=(j+i^1/2)/i^1/2 end
	return 'end', i
end



for i=1,it do
	f(i)
end

bench:clock('serial lua')


local s1=lstage.stage(f)

for i=1,it do
	s1(i)
end

for i=1,it do
	s1:output():get()
end
print(s1:output():size())

bench:clock('serial stage')

lstage.pool:add(lstage.cpus()-1)
s1:add(lstage.cpus()-1)
for i=1,it do
	s1(i)
end

for i=1,it do
	s1:output():get()
end
bench:clock('parallel stage')
