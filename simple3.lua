local l=require'lstage'

local s2=l.stage() --no environment yet

local chan=l.channel() --asynchronous channel (a la go)

local s=l.stage(function()
    print('hello',chan:get())
end):push()

s2:wrap(function(...) --sets the environment of s2
    chan:push(...) --upvalue
end)

s2:push('world')

l.event.sleep(1.0)