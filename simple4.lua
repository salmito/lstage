local l=require'lstage'

local s2=l.stage() --no environment yet

local chan=l.channel() --asynchronous channel (a la go)

local s=l.stage(function()
    local par=chan:get()
    for i=1,par do 
      print('very expensive operation',i)
      l.event.sleep(1.42)
      print('end',i)
    end
end):push()

s2:wrap(function(...) --sets the environment of s2
    chan:push(...) --upvalue
end)

s2:push(10)

l.event.sleep(10.0)