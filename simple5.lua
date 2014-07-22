--Add threads to the pool for true paralelism
local l=require'lstage'

l.pool:add(1) --without this they will execute sequentially

local chan=l.channel()

local s1=l.stage(function()
    print('very expensive operation')
    for i=1,100000000 do 
      z=(i^2+2*i+2^2)
    end
    chan:push('s1 ended')
end):push()

local s2=l.stage(function()
    print('very expensive operation')
    for i=1,100000000 do 
      z=(i^2+2*i+2^2)
    end
    chan:push('s2 ended')
end):push()

print(chan:get())
print(chan:get())