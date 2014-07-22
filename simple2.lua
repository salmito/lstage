-- for circular references and recursion, stages may be 
-- initialized later
local l=require'lstage'

--no environment yet
local s1=l.stage() 

local s2=l.stage(function(i)
    print('world',i)
    s1:push(i-1)
end)

s1:wrap(function(i) --sets the environment of s1
    if i==0 then return end
    print('hello',i)
    s2:push(i) --can reference s2
end)

s1:push(10)

l.event.sleep(1.0)