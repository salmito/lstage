local l=require'lstage'

local s2=l.stage() --no environment yet

local chan=l.channel() --asynchronous channel (a la go)
local ret=l.channel() --return channel

local s=l.stage(function(i)
      print('very expensive operation',i)
      l.event.sleep(1)
      print('end',i)
      ret:push(i)
end)

s2:wrap(function(par) --sets the environment of s2
    for i=1,par do 
      s:push(i) --upvalue
    end
end)

s2:push(10)
for i=1,10 do
  print('ended',ret:get())
end